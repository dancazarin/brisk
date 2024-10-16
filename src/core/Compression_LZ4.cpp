#include <brisk/core/Compression.hpp>
#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>
#include <memory>

namespace Brisk {

constexpr static size_t batchSize = 65536;

class LZ4Decoder : public SequentialReader {
public:
    explicit LZ4Decoder(RC<Stream> reader) : reader(std::move(reader)) {
        buffer.reset(new uint8_t[batchSize]);
        bufferUsed              = 0;
        LZ4F_errorCode_t result = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
        if (LZ4F_isError(result)) {
            dctx = nullptr;
        }
    }

    [[nodiscard]] Transferred read(uint8_t* data, size_t size) final {
        if (!dctx)
            return Transferred::Error;
        if (finished)
            return Transferred::Eof;

        size_t available_out = size;
        uint8_t* next_out    = data;
        size_t available_in;
        const uint8_t* next_in;

        do {
            next_in = buffer.get();
            if (bufferUsed < batchSize) {
                Transferred sz = reader->read(buffer.get() + bufferUsed, batchSize - bufferUsed);
                if (sz.isError()) {
                    return sz;
                }
                available_in = bufferUsed + sz.bytes();
            } else {
                available_in = bufferUsed;
            }

            size_t next_out_size = available_out;
            size_t result = LZ4F_decompress(dctx, next_out, &next_out_size, next_in, &available_in, nullptr);
            if (LZ4F_isError(result)) {
                return Transferred::Error;
            }

            if (next_out_size == 0) {
                finished = true;
                break;
            }

            available_out -= next_out_size;
            next_out += next_out_size;

            bufferUsed = 0;
        } while (available_out > 0);

        if (available_in > 0) {
            memcpy(buffer.get(), next_in, available_in);
            bufferUsed = available_in;
        }

        if (size == available_out && finished) {
            return Transferred::Eof;
        } else {
            return size - available_out;
        }
    }

    ~LZ4Decoder() {
        if (dctx) {
            LZ4F_freeDecompressionContext(dctx);
        }
    }

private:
    RC<Stream> reader;
    LZ4F_dctx* dctx = nullptr;
    std::unique_ptr<uint8_t[]> buffer;
    size_t bufferUsed = 0;
    bool finished     = false;
};

class LZ4Encoder final : public SequentialWriter {
public:
    explicit LZ4Encoder(RC<Stream> writer, CompressionLevel level) : writer(std::move(writer)) {

        LZ4F_errorCode_t result = LZ4F_createCompressionContext(&cctx, LZ4F_VERSION);
        if (LZ4F_isError(result)) {
            cctx = nullptr;
        }
        preferences.compressionLevel =
            LZ4HC_CLEVEL_MIN + (LZ4HC_CLEVEL_MAX - LZ4HC_CLEVEL_MIN) * static_cast<int>(level) / 9;

        bufferSize = LZ4F_compressBound(batchSize, &preferences);
        buffer.reset(new uint8_t[bufferSize]);
    }

    bool writeHeader() {
        uint8_t headerData[LZ4F_HEADER_SIZE_MAX];
        size_t headerSize = LZ4F_compressBegin(cctx, headerData, batchSize, &preferences);
        return this->writer->write(headerData, headerSize) == headerSize;
    }

    [[nodiscard]] Transferred write(const uint8_t* data, size_t size) final {
        if (!headerWritten) {
            if (!writeHeader())
                return Transferred::Error;
            headerWritten = true;
        }

        if (!cctx || size == 0) {
            return Transferred::Error;
        }

        const uint8_t* next_in = data;
        size_t available_in    = size;

        while (available_in > 0) {
            size_t available_out = bufferSize;
            uint8_t* next_out    = buffer.get();

            size_t result        = LZ4F_compressUpdate(cctx, next_out, available_out, next_in,
                                                       std::min(available_in, batchSize), nullptr);
            if (LZ4F_isError(result)) {
                return Transferred::Error;
            }

            size_t flushSize = result;
            if (flushSize > 0) {
                Transferred wr = writer->write(buffer.get(), flushSize);
                if (wr.bytes() != flushSize) {
                    return wr;
                }
            }
            available_in -= std::min(available_in, batchSize);
        }
        return size;
    }

    [[nodiscard]] bool flush() final {
        if (!cctx) {
            return false;
        }
        if (!headerWritten) {
            if (!writeHeader())
                return Transferred::Error;
            headerWritten = true;
        }

        size_t available_out = batchSize;
        uint8_t* next_out    = buffer.get();

        size_t result        = LZ4F_compressEnd(cctx, next_out, available_out, nullptr);
        if (LZ4F_isError(result)) {
            return false;
        }

        size_t flushSize = result;
        if (flushSize > 0) {
            Transferred wr = writer->write(buffer.get(), flushSize);
            if (wr.bytes() != flushSize) {
                return false;
            }
        }

        return writer->flush();
    }

    ~LZ4Encoder() {
        if (cctx) {
            LZ4F_freeCompressionContext(cctx);
        }
    }

private:
    RC<Stream> writer;
    bool headerWritten = false;
    LZ4F_cctx* cctx    = nullptr;
    LZ4F_preferences_t preferences{};
    std::unique_ptr<uint8_t[]> buffer;
    size_t bufferSize{};
};

RC<Stream> lz4Decoder(RC<Stream> reader) {
    return RC<Stream>(new LZ4Decoder(std::move(reader)));
}

RC<Stream> lz4Encoder(RC<Stream> writer, CompressionLevel level) {
    return RC<Stream>(new LZ4Encoder(std::move(writer), level));
}

bytes lz4Encode(bytes_view data, CompressionLevel level) {
    bytes result;
    size_t max_compressed_size = LZ4F_compressFrameBound(data.size(), nullptr);
    result.resize(max_compressed_size);

    size_t compressed_size =
        LZ4F_compressFrame(result.data(), max_compressed_size, data.data(), data.size(), nullptr);
    if (LZ4F_isError(compressed_size)) {
        return {};
    }

    result.resize(compressed_size);
    return result;
}

bytes lz4Decode(bytes_view data) {
    bytes result;
    result.resize(data.size() * 3);

    LZ4F_dctx* dctx;
    LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);

    size_t decoded_size  = result.size();
    size_t consumed_size = data.size();

    size_t result_size =
        LZ4F_decompress(dctx, result.data(), &decoded_size, data.data(), &consumed_size, nullptr);
    if (LZ4F_isError(result_size)) {
        LZ4F_freeDecompressionContext(dctx);
        return {};
    }

    result.resize(decoded_size);
    LZ4F_freeDecompressionContext(dctx);
    return result;
}

} // namespace Brisk
