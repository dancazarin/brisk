/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/core/Cryptography.hpp>
#include <tomcrypt.h>

namespace Brisk {

size_t cryptoRandomInplaceSafe(bytes_mutable_view data) {
    size_t result = rng_get_bytes(data.data(), data.size(), nullptr);
    return result;
}

void cryptoRandomInplace(bytes_mutable_view data) {
    size_t result = cryptoRandomInplaceSafe(data);
    if (result != data.size()) {
        throwException(ECrypto("Not enough randomness for cryptoRandomInplace"));
    }
}

bytes cryptoRandom(size_t size) {
    bytes result(size);
    cryptoRandomInplace(result);
    return result;
}

class RandomReader : public SequentialReader {
public:
    RandomReader() = default;

    Transferred read(uint8_t* data, size_t size) final {
        return cryptoRandomInplaceSafe(bytes_mutable_view{ data, size });
    }
};

RC<Stream> cryptoRandomReader() {
    return RC<Stream>(new RandomReader());
}

static int id_aes;
static int id_md5;
static int id_sha1;
static int id_sha256;
static int id_sha512;
static int id_sha3_256;
static int id_sha3_512;

namespace Internal {
void registerAlgorithms() {
    id_aes      = register_cipher(&aes_desc);
    id_md5      = register_hash(&md5_desc);
    id_sha1     = register_hash(&sha1_desc);
    id_sha256   = register_hash(&sha256_desc);
    id_sha512   = register_hash(&sha512_desc);
    id_sha3_256 = register_hash(&sha3_256_desc);
    id_sha3_512 = register_hash(&sha3_512_desc);
}
} // namespace Internal

static bool cfbDecode(int algo, bytes_mutable_view plaintext, bytes_view ciphertext, bytes_view key,
                      bytes_view iv) {
    Internal::registerAlgorithms();
    if (ciphertext.size() != plaintext.size())
        return false;

    symmetric_CFB cfb;
    int e = cfb_start(algo, iv.data(), key.data(), key.size(), 0, &cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    e = cfb_decrypt(ciphertext.data(), plaintext.data(), plaintext.size(), &cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    e = cfb_done(&cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    return true;
}

static bool cfbEncode(int algo, bytes_mutable_view ciphertext, bytes_view plaintext, bytes_view key,
                      bytes_view iv) {
    Internal::registerAlgorithms();
    if (ciphertext.size() != plaintext.size())
        return false;

    symmetric_CFB cfb;
    int e = cfb_start(algo, iv.data(), key.data(), key.size(), 0, &cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    e = cfb_encrypt(plaintext.data(), ciphertext.data(), ciphertext.size(), &cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    e = cfb_done(&cfb);
    if (e != CRYPT_OK) {
        return false;
    }

    return true;
}

bytes aesCFBDecode(bytes_view ciphertext, const AESKey& key, const AESIV& iv) {
    bytes plaintext(ciphertext.size());
    cfbDecode(id_aes, plaintext, ciphertext, key, iv);
    return plaintext;
}

bytes aesCFBEncode(bytes_view plaintext, const AESKey& key, const AESIV& iv) {
    bytes ciphertext(plaintext.size());
    cfbEncode(id_aes, ciphertext, plaintext, key, iv);
    return ciphertext;
}

void aesCFBEncodeInplace(bytes_mutable_view data, const AESKey& key, const AESIV& iv) {
    cfbEncode(id_aes, data, data, key, iv);
}

void aesCFBDecodeInplace(bytes_mutable_view data, const AESKey& key, const AESIV& iv) {
    cfbDecode(id_aes, data, data, key, iv);
}

class CFBDecoder final : public Reader {
public:
    explicit CFBDecoder(int algo, RC<Stream> reader, bytes_view key, bytes_view iv)
        : algo(algo), reader(std::move(reader)) {
        Internal::registerAlgorithms();
        int e = cfb_start(algo, iv.data(), key.data(), key.size(), 0, &cfb);
        if (e != CRYPT_OK) {
            failed_init = true;
            return;
        }
    }

    Transferred read(uint8_t* data, size_t size) final {
        if (failed_init)
            return 0;

        Transferred sz = reader->read(data, size);
        if (!sz)
            return sz;

        int e = cfb_decrypt(data, data, sz.bytes(), &cfb);
        if (e != CRYPT_OK) {
            return Transferred::Error;
        }

        return sz;
    }

    ~CFBDecoder() final {
        if (failed_init)
            return;

        int e = cfb_done(&cfb);
        if (e != CRYPT_OK) {
            return;
        }
    }

    int algo;
    RC<Stream> reader;
    symmetric_CFB cfb;
    bool failed_init = false;
};

constexpr static size_t batchSize = 16384;

class CFBEncoder final : public Writer {
public:
    explicit CFBEncoder(int algo, RC<Stream> writer, bytes_view key, bytes_view iv)
        : algo(algo), writer(std::move(writer)) {
        Internal::registerAlgorithms();
        int e = cfb_start(algo, iv.data(), key.data(), key.size(), 0, &cfb);
        if (e != CRYPT_OK) {
            failed_init = true;
            return;
        }

        buffer.reset(new uint8_t[batchSize]);
    }

    Transferred write(const uint8_t* data, size_t size) final {
        if (failed_init || failed)
            return Transferred::Error;

        size_t elapsed_size = size;
        while (elapsed_size > 0) {
            size_t sz = std::min(elapsed_size, batchSize);
            int e     = cfb_encrypt(data, buffer.get(), sz, &cfb);
            if (e != CRYPT_OK) {
                failed = true;
                return Transferred::Error;
            }

            Transferred wsz = writer->write(buffer.get(), sz);
            if (wsz.bytes() != sz) {
                failed = true;
                return wsz;
            }

            elapsed_size -= sz;
            data += sz;
        }
        BRISK_ASSERT(elapsed_size == 0);

        return size;
    }

    bool flush() final {
        return writer->flush();
    }

    ~CFBEncoder() final {
        if (failed_init)
            return;

        int e = cfb_done(&cfb);
        if (e != CRYPT_OK) {
            return;
        }
    }

    int algo;
    RC<Stream> writer;
    symmetric_CFB cfb;
    std::unique_ptr<uint8_t[]> buffer;
    bool failed_init = false;
    bool failed      = false;
};

RC<Stream> aesCFBDecoder(RC<Stream> reader, const AESKey& key, const AESIV& iv) {
    return RC<Stream>(new CFBDecoder(id_aes, std::move(reader), key, iv));
}

RC<Stream> aesCFBEncoder(RC<Stream> writer, const AESKey& key, const AESIV& iv) {
    return RC<Stream>(new CFBEncoder(id_aes, std::move(writer), key, iv));
}

static constexpr size_t hashBitSize(HashMethod method) {
    switch (method) {
    case HashMethod::MD5:
        return 128;
    case HashMethod::SHA1:
        return 160;
    case HashMethod::SHA256:
    case HashMethod::SHA3_256:
        return 256;
    case HashMethod::SHA512:
    case HashMethod::SHA3_512:
        return 512;
    default:
        return 0;
    }
}

static const ltc_hash_descriptor* hashDescriptor[enumSize<HashMethod>]{
    &md5_desc, &sha1_desc, &sha256_desc, &sha512_desc, &sha3_256_desc, &sha3_512_desc,
};

static void hashTo(HashMethod method, bytes_view data, bytes_mutable_view hash) {
    if (auto desc = lookupByEnum(hashDescriptor, method)) {
        hash_state state;
        desc->init(&state);
        desc->process(&state, data.data(), data.size());
        desc->done(&state, hash.data());
    } else {
        throwException(EArgument("Invalid hash method"));
    }
}

template <HashMethod method>
static FixedBits<hashBitSize(method)> hash(bytes_view data) {
    FixedBits<hashBitSize(method)> result;
    hashTo(method, data, result);
    return result;
}

MD5Hash md5(bytes_view data) {
    return hash<HashMethod::MD5>(data);
}

SHA1Hash sha1(bytes_view data) {
    return hash<HashMethod::SHA1>(data);
}

SHA256Hash sha256(bytes_view data) {
    return hash<HashMethod::SHA256>(data);
}

SHA512Hash sha512(bytes_view data) {
    return hash<HashMethod::SHA512>(data);
}

SHA3_256Hash sha3_256(bytes_view data) {
    return hash<HashMethod::SHA3_256>(data);
}

SHA3_512Hash sha3_512(bytes_view data) {
    return hash<HashMethod::SHA3_512>(data);
}

Bytes hash(HashMethod method, bytes_view data) {
    Bytes result(hashBitSize(method));
    hashTo(method, data, result);
    return result;
}

MD5Hash md5(string_view data) {
    return hash<HashMethod::MD5>(toBytesView(data));
}

SHA1Hash sha1(string_view data) {
    return hash<HashMethod::SHA1>(toBytesView(data));
}

SHA256Hash sha256(string_view data) {
    return hash<HashMethod::SHA256>(toBytesView(data));
}

SHA512Hash sha512(string_view data) {
    return hash<HashMethod::SHA512>(toBytesView(data));
}

SHA3_256Hash sha3_256(string_view data) {
    return hash<HashMethod::SHA3_256>(toBytesView(data));
}

SHA3_512Hash sha3_512(string_view data) {
    return hash<HashMethod::SHA3_512>(toBytesView(data));
}

Bytes hash(HashMethod method, string_view data) {
    Bytes result(hashBitSize(method));
    hashTo(method, toBytesView(data), result);
    return result;
}

class HashStream final : public SequentialWriter {
public:
    HashStream(const ltc_hash_descriptor* desc, bytes_mutable_view hash) : desc(desc), hash(hash) {
        if (desc->init(&state) != CRYPT_OK) {
            failed = true;
        }
    }

    ~HashStream() final {
        if (!flushed) {
            std::ignore = flush();
        }
    }

    Transferred write(const uint8_t* data, size_t size) final {
        if (failed || flushed)
            return Transferred::Error;

        if (desc->process(&state, data, size) != CRYPT_OK) {
            failed = true;
            return Transferred::Error;
        }

        return size;
    }

    bool flush() final {
        return getHash(hash);
    }

    bool getHash(bytes_mutable_view hash) {
        if (flushed)
            return false;

        if (desc->hashsize != hash.size()) {
            return false;
        }

        if (failed) {
            std::fill(hash.begin(), hash.end(), 0);
        } else {
            desc->done(&state, hash.data());
        }

        flushed = true;
        return true;
    }

private:
    const ltc_hash_descriptor* desc;
    bytes_mutable_view hash;
    hash_state state;
    bool failed  = false;
    bool flushed = false;
};

RC<Stream> hashStream(HashMethod method, bytes_mutable_view hash) {
    if (auto desc = lookupByEnum(hashDescriptor, method)) {
        return RC<Stream>(new HashStream(desc, hash));
    } else {
        throwException(EArgument("Invalid hash method"));
    }
}

RC<Stream> md5HashStream(MD5Hash& hash) {
    return hashStream(HashMethod::MD5, toBytesMutableView(hash));
}

RC<Stream> sha1HashStream(SHA1Hash& hash) {
    return hashStream(HashMethod::SHA1, toBytesMutableView(hash));
}

RC<Stream> sha256HashStream(SHA256Hash& hash) {
    return hashStream(HashMethod::SHA256, toBytesMutableView(hash));
}

RC<Stream> sha512HashStream(SHA512Hash& hash) {
    return hashStream(HashMethod::SHA512, toBytesMutableView(hash));
}

RC<Stream> sha3_256HashStream(SHA3_256Hash& hash) {
    return hashStream(HashMethod::SHA3_256, toBytesMutableView(hash));
}

RC<Stream> sha3_512HashStream(SHA3_512Hash& hash) {
    return hashStream(HashMethod::SHA3_512, toBytesMutableView(hash));
}

static_assert(sizeof(decltype(Hasher::state)) == sizeof(hash_state));

Hasher::Hasher() noexcept : method(static_cast<HashMethod>(-1)) {}

Hasher::Hasher(HashMethod method) noexcept : method(method) {
    if (auto desc = lookupByEnum(hashDescriptor, method)) {
        desc->init(reinterpret_cast<hash_state*>(state.data()));
    } else {
        throwException(EArgument("Invalid hash method"));
    }
}

bool Hasher::finish(bytes_mutable_view bytes) {
    if (auto desc = lookupByEnum(hashDescriptor, method)) {
        BRISK_ASSERT(bytes.size() == desc->hashsize);
        return desc->done(reinterpret_cast<hash_state*>(state.data()), bytes.data()) == CRYPT_OK;
    } else {
        throwException(EArgument("Invalid hash method"));
    }
}

bool Hasher::write(bytes_view data) {
    if (auto desc = lookupByEnum(hashDescriptor, method)) {
        return desc->process(reinterpret_cast<hash_state*>(state.data()), data.data(), data.size()) ==
               CRYPT_OK;
    } else {
        throwException(EArgument("Invalid hash method"));
    }
}
} // namespace Brisk
