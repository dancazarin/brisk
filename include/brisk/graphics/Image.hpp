#pragma once

#include "Pixel.hpp"
#include "Geometry.hpp"
#include "Color.hpp"
#include <memory>
#include <brisk/core/RC.hpp>
#include <brisk/core/internal/Optional.hpp>
#include <fmt/format.h>
#include <brisk/core/Exceptions.hpp>
#include <brisk/core/Memory.hpp>

namespace Brisk {

template <typename T>
struct StridedData {
    T* data;
    int32_t byteStride;

    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }
};

void convertPixels(PixelFormat dstFmt, StridedData<uint8_t> dst, PixelFormat srcFmt,
                   StridedData<const uint8_t> src, Size size);

enum class AccessMode {
    R,
    W,
    RW,
};

template <typename U, AccessMode Mode>
using ConstIfR = std::conditional_t<Mode == AccessMode::R, const U, U>;

template <AccessMode Mode>
using CAccessMode = constant<AccessMode, Mode>;

using CAccessR    = CAccessMode<AccessMode::R>;
using CAccessW    = CAccessMode<AccessMode::W>;
using CAccessRW   = CAccessMode<AccessMode::RW>;

enum class ImageMapFlags : uint32_t {
    Default = 0,
};

BRISK_FLAGS(ImageMapFlags)

template <typename T>
struct ImageData {
    T* data;
    Size size;
    int32_t byteStride;
    int32_t components;

    ImageData() noexcept                 = default;
    ImageData(const ImageData&) noexcept = default;

    operator StridedData<T>() const {
        return StridedData<T>{
            data,
            byteStride,
        };
    }

    ImageData(T* data, Size size, int32_t byteStride, int32_t components)
        : data(data), size(size), byteStride(byteStride), components(components) {}

    // ImageData<T> -> ImageData<const T> conversion
    template <typename U = T, std::enable_if_t<std::is_const_v<U>>* = nullptr>
    ImageData(const ImageData<std::remove_const_t<T>>& other)
        : data(other.data), size(other.size), byteStride(other.byteStride), components(other.components) {}

    template <typename U>
    ImageData<U> to() const {
        if (components * sizeof(T) % sizeof(U) != 0) {
            throwException(EArgument("ImageData: invalid conversion"));
        }
        return { reinterpret_cast<U*>(data), size, byteStride,
                 static_cast<int32_t>(components * sizeof(T) / sizeof(U)) };
    }

    void copyFrom(const ImageData<const T>& src) const {
        auto srcLine = src.lineIterator();
        auto dstLine = lineIterator();
        int32_t w    = memoryWidth();
        for (int32_t y = 0; y < size.height; ++y, ++srcLine, ++dstLine) {
            std::copy_n(srcLine.data, w, dstLine.data);
        }
    }

    ImageData subrect(Rectangle rect) const {
        if (rect.intersection(Rectangle{ Point{ 0, 0 }, size }) != rect) {
            throwException(EArgument("ImageData: invalid rectangle passed to subrect"));
        }
        return ImageData{
            pixel(rect.x1, rect.y1),
            rect.size(),
            byteStride,
            components,
        };
    }

    int32_t memoryWidth() const {
        return size.width * components;
    }

    size_t memorySize() const {
        return area() * components;
    }

    size_t area() const {
        return static_cast<int64_t>(size.width) * static_cast<int64_t>(size.height);
    }

    size_t byteSize() const {
        return sizeof(T) * memorySize();
    }

    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }

    template <typename U = T>
    U* pixel(int32_t x, int32_t y) const {
        return line(y) + x * components;
    }

    struct LineIterator {
        T* data;
        int32_t byteStride;

        LineIterator& operator++() {
            reinterpret_cast<U8*&>(data) += byteStride;
            return *this;
        }

        LineIterator operator++(int) {
            LineIterator copy = *this;
            ++copy;
            return copy;
        }
    };

    LineIterator lineIterator() const {
        return { data, byteStride };
    }

    LineIterator lineReverseIterator() const {
        return { line(size.height - 1), -byteStride };
    }
};

struct MappedRegion {
    Point origin;
    ImageMapFlags flags = ImageMapFlags::Default;
};

namespace Internal {
template <PixelFormat Format, typename ComponentType>
struct PixelOfFormat {
    using Type = Pixel<ComponentType, Format>;
};

template <typename ComponentType>
struct PixelOfFormat<PixelFormat::Unknown, ComponentType> {
    using Type = ComponentType;
};
} // namespace Internal

template <PixelType Type, PixelFormat Format, AccessMode Mode>
struct ImageAccess {
    ImageAccess()                              = delete;
    ImageAccess(const ImageAccess&)            = delete;
    ImageAccess& operator=(const ImageAccess&) = delete;
    ImageAccess& operator=(ImageAccess&&)      = delete;

    ImageAccess(ImageAccess&& other) : m_data{}, m_mapped{}, m_commit{}, m_format{} {
        swap(other);
    }

    using ComponentType = PixelTypeOf<Type>;
    using StorageType   = typename Internal::PixelOfFormat<Format, ComponentType>::Type;

    using value_type    = ConstIfR<StorageType, Mode>;
    using reference     = value_type&;
    using pointer       = value_type*;

    struct UnmapFn {
        using Fn = void (*)(void*, ImageData<value_type>&, MappedRegion&);
        Fn fn;
        void* self;

        void operator()(ImageData<value_type>& data, MappedRegion& mapped) {
            fn(self, data, mapped);
        }
    };

    ImageAccess(const std::tuple<ImageData<value_type>, MappedRegion>& dataMapped, UnmapFn commit,
                PixelFormat format)
        requires(Format == PixelFormat::Unknown)
        : ImageAccess(std::get<0>(dataMapped), std::get<1>(dataMapped), commit, format) {}

    ImageAccess(const ImageData<value_type>& data, const MappedRegion& mapped, UnmapFn commit,
                PixelFormat format)
        requires(Format == PixelFormat::Unknown)
        : m_data(data), m_mapped(mapped), m_commit(std::move(commit)), m_format(format) {}

    ImageAccess(const std::tuple<ImageData<value_type>, MappedRegion>& dataMapped, UnmapFn commit)
        requires(Format != PixelFormat::Unknown)
        : ImageAccess(std::get<0>(dataMapped), std::get<1>(dataMapped), commit, Format) {}

    ImageAccess(const ImageData<value_type>& data, const MappedRegion& mapped, UnmapFn commit)
        requires(Format != PixelFormat::Unknown)
        : m_data(data), m_mapped(mapped), m_commit(std::move(commit)), m_format(Format) {}

    void swap(ImageAccess& other) {
        std::swap(m_data, other.m_data);
        std::swap(m_mapped, other.m_mapped);
        std::swap(m_commit, other.m_commit);
        std::swap(m_format, other.m_format);
    }

    using LineIterator = typename ImageData<value_type>::LineIterator;

    [[noreturn]] static void throwRangeError(const std::string& str) {
        throwException(ERange(str.c_str()));
    }

    reference operator()(int32_t x, int32_t y) const {
#ifndef NDEBUG
        if (x < 0 || x >= width())
            throwRangeError(
                fmt::format("operator(): invalid coordinate {}x{} (size={}x{})", x, y, width(), height()));
#endif
        return line(y)[x];
    }

    Size size() const {
        return m_data.size;
    }

    int32_t width() const {
        return m_data.size.width;
    }

    int32_t height() const {
        return m_data.size.height;
    }

    int32_t memoryWidth() const {
        return m_data.memoryWidth();
    }

    void copyFrom(const ImageAccess<Type, Format, AccessMode::R>& src)
        requires(Mode != AccessMode::R)
    {
        return copyFrom(src.m_data);
    }

    void copyFrom(const ImageAccess<Type, Format, AccessMode::RW>& src)
        requires(Mode != AccessMode::R)
    {
        return copyFrom(src.m_data);
    }

    ~ImageAccess() {
        m_commit(m_data, m_mapped);
    }

    pointer data() const {
        return m_data.data;
    }

    int32_t byteStride() const {
        return m_data.byteStride;
    }

    size_t memorySize() const {
        return m_data.memorySize();
    }

    size_t byteSize() const {
        return m_data.byteSize();
    }

    PixelFormat format() const {
        return m_format;
    }

    int32_t components() const {
        return m_data.components;
    }

    LineIterator lineIterator() const {
        return m_data.lineIterator();
    }

    LineIterator lineReverseIterator() const {
        return m_data.lineReverseIterator();
    }

    pointer line(int32_t y) const {
#ifndef NDEBUG
        if (y < 0 || y >= height())
            throwRangeError(fmt::format("line(): invalid line index {} (height={})", y, height()));
#endif
        return m_data.line(y);
    }

    void writeTo(bytes_mutable_view data) const {
#ifndef NDEBUG
        if (data.size() != sizeof(StorageType) * m_data.memorySize())
            throwRangeError(fmt::format("writeTo(): invalid size {} (required={})", data.size(),
                                        sizeof(StorageType) * m_data.memorySize()));
#endif
        LineIterator l   = lineIterator();
        StorageType* dst = reinterpret_cast<StorageType*>(data.data());
        int32_t w        = m_data.memoryWidth();
        for (int32_t y = 0; y < height(); ++y, ++l, dst += w)
            memcpy(dst, l.data, w * sizeof(StorageType));
    }

    void readFrom(bytes_view data) const
        requires(Mode != AccessMode::R)
    {
#ifndef NDEBUG
        if (data.size() != sizeof(StorageType) * m_data.memorySize())
            throwRangeError(fmt::format("readFrom(): invalid size {} (required={})", data.size(),
                                        sizeof(StorageType) * m_data.memorySize()));
#endif
        LineIterator l         = lineIterator();
        const StorageType* src = reinterpret_cast<const StorageType*>(data.data());
        int32_t w              = m_data.memoryWidth();
        for (int32_t y = 0; y < height(); ++y, ++l, src += w)
            memcpy(l.data, src, w * sizeof(StorageType));
    }

    bool isContiguous() const {
        return m_data.stride == m_data.size.width;
    }

    bool isTopDown() const {
        return m_data.stride > 0;
    }

    void clear(StorageType value) const
        requires(Mode != AccessMode::R)
    {
        LineIterator l = lineIterator();
        int32_t w      = m_data.memoryWidth();
        for (int32_t y = 0; y < m_data.size.height; ++y, ++l) {
            std::fill_n(l.data, w, value);
        }
    }

    void flip(FlipAxis axis) const
        requires(Mode != AccessMode::R)
    {
        switch (axis) {
        case FlipAxis::X:
            for (int32_t y = 0; y < m_data.size.height; ++y) {
                value_type* l = line(y);
                for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                    swapItem(l, x1, l, x2);
                }
            }
            break;
        case FlipAxis::Y:
            for (int32_t y1 = 0, y2 = m_data.size.height - 1; y1 < y2; ++y1, --y2) {
                value_type* l1 = line(y1);
                value_type* l2 = line(y2);
                for (int32_t x = 0; x < m_data.size.width; ++x) {
                    swapItem(l1, x, l2, x);
                }
            }
            break;
        case FlipAxis::Both:
            for (int32_t y1 = 0, y2 = m_data.size.height - 1; y1 <= y2; ++y1, --y2) {
                if (y1 != y2) {
                    value_type* l1 = line(y1);
                    value_type* l2 = line(y2);
                    for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                        swapItem(l1, x1, l2, x2);
                        swapItem(l1, x2, l2, x1);
                    }
                } else {
                    value_type* l = line(y1);
                    for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                        swapItem(l, x1, l, x2);
                    }
                }
            }
            break;
        }
    }

    template <PixelFormat PFormat = Format, typename Fn>
    void forPixels(Fn&& fn) {
        if constexpr (PFormat != PixelFormat::Unknown) {
            LineIterator l = lineIterator();
            int32_t w      = m_data.size.width;
            for (int32_t y = 0; y < m_data.size.height; ++y, ++l) {
                for (int32_t x = 0; x < w; ++x) {
                    fn(x, y, l.data[x]);
                }
            }
        } else {
            DO_PIX_FMT(format(), return forPixels<FMT>(std::forward<Fn>(fn)););
        }
    }

    void premultiplyAlpha()
        requires(Mode != AccessMode::R && Type != PixelType::Unknown)
    {
        if (pixelAlpha(format()) != PixelFlagAlpha::None && format() != PixelFormat::Alpha) {
            forPixels([](int32_t, int32_t, auto& pix) {
                pix = colorToPixel<Type, std::remove_cvref_t<decltype(pix)>::format>(
                    pixelToColor<Type>(pix).premultiply());
            });
        }
    }

    const ImageData<value_type>& imageData() const {
        return m_data;
    }

    template <PixelType, PixelFormat, AccessMode>
    friend struct ImageAccess;

    void copyFrom(const ImageData<const StorageType>& src)
        requires(Mode != AccessMode::R)
    {
#ifndef NDEBUG
        if (src.size != m_data.size) {
            throwRangeError(fmt::format("copyFrom: source size = {}x{}, target size = {}x{}", src.size.x,
                                        src.size.y, m_data.size.x, m_data.size.y));
        }
        if (src.components != m_data.components) {
            throwRangeError(fmt::format("copyFrom: source components = {}, target components = {}",
                                        src.components, m_data.components));
        }
#endif
        m_data.copyFrom(src);
    }

private:
    ImageData<value_type> m_data;
    MappedRegion m_mapped;
    UnmapFn m_commit;
    PixelFormat m_format;

    void swapItem(value_type* a, int32_t ax, value_type* b, int32_t bx) const {
        if constexpr (Mode != AccessMode::R) {
            a += ax * m_data.components;
            b += bx * m_data.components;
            for (int32_t i = 0; i < m_data.components; ++i) {
                std::swap(a[i], b[i]);
            }
        }
    }
};

template <PixelType Type = PixelType::Unknown, PixelFormat Format = PixelFormat::Unknown>
class ImageTyped;

using ImageAny = ImageTyped<>;

namespace Internal {
struct ImageBackend {
    virtual ~ImageBackend()                             = 0;
    /// Can transfer image data from backend if the image is changed on GPU since previous call to update
    virtual void begin(AccessMode mode, Rectangle rect) = 0;
    /// Can transfer image data to backend
    virtual void end(AccessMode mode, Rectangle rect)   = 0;
};

ImageBackend* getBackend(RC<ImageAny> image);
void setBackend(RC<ImageAny> image, ImageBackend* backend);
} // namespace Internal

template <typename T>
inline ImageData<T> allocateImageData(Size size, int32_t components = 1, int32_t strideAlignment = 1) {
    if (std::max(size.width, size.height) >= 65536) {
        throwException(EArgument("Invalid size for image data: {}x{}", size.x, size.y));
    }
    size_t byteStride = alignUp(size.width * sizeof(T) * components, strideAlignment);
    return ImageData<T>{
        alignedAlloc<T>(size.height * byteStride),
        size,
        int32_t(byteStride),
        components,
    };
}

template <typename T>
inline void deallocateImageData(const ImageData<T>& data) {
    alignedFree(data.data);
}

template <>
class ImageTyped<PixelType::Unknown, PixelFormat::Unknown>
    : public std::enable_shared_from_this<ImageTyped<PixelType::Unknown, PixelFormat::Unknown>> {
public:
    ImageTyped()                             = delete;
    ImageTyped(const ImageTyped&)            = delete;
    ImageTyped(ImageTyped&&)                 = delete;
    ImageTyped& operator=(const ImageTyped&) = delete;
    ImageTyped& operator=(ImageTyped&&)      = delete;

    using value_type                         = UntypedPixel;

    int32_t width() const noexcept {
        return m_data.size.width;
    }

    int32_t height() const noexcept {
        return m_data.size.height;
    }

    Size size() const noexcept {
        return m_data.size;
    }

    Rectangle bounds() const noexcept {
        return Rectangle{ Point{ 0, 0 }, size() };
    }

    size_t byteSize() const noexcept {
        return m_data.byteSize();
    }

    PixelType type() const noexcept {
        return m_type;
    }

    PixelFormat format() const noexcept {
        return m_format;
    }

    int32_t componentsPerPixel() const noexcept {
        return pixelComponents(format());
    }

    int32_t bytesPerPixel() const noexcept {
        return pixelSize(type(), format());
    }

    bool isGreyscale() const noexcept {
        return pixelColor(format()) == PixelFlagColor::Greyscale;
    }

    bool isColor() const noexcept {
        return pixelColor(format()) == PixelFlagColor::RGB;
    }

    bool hasAlpha() const noexcept {
        return pixelAlpha(format()) != PixelFlagAlpha::None;
    }

    bool isAlphaOnly() const noexcept {
        return pixelColor(format()) == PixelFlagColor::None;
    }

    bool isLinear() const noexcept {
        return type() != PixelType::U8Gamma;
    }

    template <AccessMode Mode>
    using ImageAccessT = ImageAccess<PixelType::Unknown, PixelFormat::Unknown, Mode>;

    using AccessR      = ImageAccessT<AccessMode::R>;
    using AccessW      = ImageAccessT<AccessMode::W>;
    using AccessRW     = ImageAccessT<AccessMode::RW>;

    AccessR mapRead() const {
        return map<AccessMode::R>(data(), this->bounds(), m_backend.get(), m_format);
    }

    AccessW mapWrite() {
        return map<AccessMode::W>(data(), this->bounds(), m_backend.get(), m_format);
    }

    AccessRW mapReadWrite() {
        return map<AccessMode::RW>(data(), this->bounds(), m_backend.get(), m_format);
    }

    AccessR mapRead(Rectangle rect) const {
        return map<AccessMode::R>(data(), rect, m_backend.get(), m_format);
    }

    AccessW mapWrite(Rectangle rect) {
        return map<AccessMode::W>(data(), rect, m_backend.get(), m_format);
    }

    AccessRW mapReadWrite(Rectangle rect) {
        return map<AccessMode::RW>(data(), rect, m_backend.get(), m_format);
    }

    void clear(value_type value) {
        AccessW w = mapWrite();
        w.clear(value);
    }

    void copyFrom(const RC<ImageTyped>& source, Rectangle sourceRect, Rectangle destRect) {
        AccessR r = source->mapRead(sourceRect);
        AccessW w = mapWrite(destRect);
        w.copyFrom(r);
    }

    void copyFrom(const RC<ImageTyped>& source) {
        return copyFrom(source, source->bounds(), this->bounds());
    }

    ImageData<value_type> data() const noexcept {
        return m_data;
    }

    template <PixelType Type, PixelFormat Format = PixelFormat::Unknown>
    RC<const ImageTyped<Type, Format>> as() const {
        if (Type != this->type() || Format != this->format()) {
            throwException(
                EArgument("Image: cannot cast to an image of type {} and format {}", Type, Format));
        }
        // Casting to a derived type.
        // In case this class was not created as ImageTyped<Type, Format>
        // this is technically an Undefined Behavior.
        // But as long as the derived types have no extra fields
        // the memory layout matches exactly, so
        // this is ok with all modern compilers.
        return adopt(static_cast<const ImageTyped<Type, Format>*>(this));
    }

    template <PixelType Type, PixelFormat Format = PixelFormat::Unknown>
    RC<ImageTyped<Type, Format>> as() {
        if (Type != this->type() || Format != this->format()) {
            throwException(
                EArgument("Image: cannot cast to an image of type {} and format {}", Type, Format));
        }
        // Casting to a derived type.
        // In case this class was not created as ImageTyped<Type, Format>
        // this is technically an Undefined Behavior.
        // But as long as the derived types have no extra fields
        // the memory layout matches exactly, so
        // this is ok with all modern compilers.
        return std::static_pointer_cast<ImageTyped<Type, Format>>(this->shared_from_this());
    }

    /// Create image with given size, pixel type and pixel format and allocates memory for it.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(Size size, PixelType type = PixelType::U8Gamma,
                        PixelFormat format = PixelFormat::RGBA)
        : ImageTyped(allocateImageData<UntypedPixel>(size, pixelSize(type, format)), type, format,
                     &deallocateImageData<UntypedPixel>) {}

    /// Create image with given size, pixel type and pixel format and reference existing data.
    /// Data is not copied. Caller is responsible for data lifetime.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(void* data, Size size, int32_t byteStride, PixelType type = PixelType::U8Gamma,
                        PixelFormat format = PixelFormat::RGBA)
        : ImageTyped(ImageData<value_type>{ reinterpret_cast<UntypedPixel*>(data), size, byteStride,
                                            pixelComponents(format) },
                     type, format, nullptr) {}

    ~ImageTyped() {
        if (m_deleter)
            m_deleter(m_data);
    }

protected:
    using ImageDataDeleter = void (*)(const ImageData<UntypedPixel>& data);

    ImageTyped(ImageData<value_type> data, PixelType type, PixelFormat format, ImageDataDeleter deleter,
               std::unique_ptr<Internal::ImageBackend> backend = nullptr)
        : m_data(std::move(data)), m_type(type), m_format(format), m_deleter(deleter),
          m_backend(std::move(backend)) {}

    template <AccessMode Mode>
    static ImageAccessT<Mode> map(const ImageData<value_type>& data, Rectangle rect,
                                  Internal::ImageBackend* backend, PixelFormat format) {
        if (backend)
            backend->begin(Mode, rect);
        return ImageAccessT<Mode>(data.subrect(rect), MappedRegion{ rect.p1 },
                                  typename ImageAccessT<Mode>::UnmapFn{ &unmap<Mode>, backend }, format);
    }

    template <AccessMode M>
    static void unmap(void* backend_, ImageData<ConstIfR<value_type, M>>& data, MappedRegion& mapped) {
        Internal::ImageBackend* backend = static_cast<Internal::ImageBackend*>(backend_);
        if (backend)
            backend->end(M, Rectangle{ mapped.origin, data.size });
    }

    ImageData<value_type> m_data;
    PixelType m_type;
    PixelFormat m_format;
    ImageDataDeleter m_deleter;

    friend Internal::ImageBackend* Internal::getBackend(RC<ImageTyped> image);
    friend void Internal::setBackend(RC<ImageTyped> image, Internal::ImageBackend* backend);
    mutable std::unique_ptr<Internal::ImageBackend> m_backend;
};

template <PixelType Type>
class ImageTyped<Type, PixelFormat::Unknown> : public ImageTyped<> {
public:
    using value_subtype = PixelTypeOf<Type>;
    using value_type    = value_subtype;

    template <AccessMode Mode>
    using ImageAccessT = ImageAccess<Type, PixelFormat::Unknown, Mode>;

    using AccessR      = ImageAccessT<AccessMode::R>;
    using AccessW      = ImageAccessT<AccessMode::W>;
    using AccessRW     = ImageAccessT<AccessMode::RW>;

    AccessR mapRead() const {
        return map<AccessMode::R>(data(), this->bounds(), this->m_backend.get(), m_format);
    }

    AccessW mapWrite() {
        return map<AccessMode::W>(data(), this->bounds(), this->m_backend.get(), m_format);
    }

    AccessRW mapReadWrite() {
        return map<AccessMode::RW>(data(), this->bounds(), this->m_backend.get(), m_format);
    }

    AccessR mapRead(Rectangle rect) const {
        return map<AccessMode::R>(data(), rect, this->m_backend.get(), m_format);
    }

    AccessW mapWrite(Rectangle rect) {
        return map<AccessMode::W>(data(), rect, this->m_backend.get(), m_format);
    }

    AccessRW mapReadWrite(Rectangle rect) {
        return map<AccessMode::RW>(data(), rect, this->m_backend.get(), m_format);
    }

    void clear(value_type value) {
        AccessW w = mapWrite();
        w.clear(value);
    }

    ImageData<value_type> data() const {
        return this->m_data.template to<value_type>();
    }

    /// Create image with given size, pixel type and pixel format and allocates memory for it.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(Size size, PixelFormat format = PixelFormat::RGBA)
        : ImageTyped<>(size, Type, format) {}

    /// Create image with given size, pixel type and pixel format and reference existing data.
    /// Data is not copied. Caller is responsible for data lifetime.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(void* data, Size size, int32_t byteStride, PixelFormat format = PixelFormat::RGBA)
        : ImageTyped<>(data, size, byteStride, Type, format) {}

protected:
    using ImageTyped<>::ImageTyped;

    template <AccessMode Mode>
    static ImageAccessT<Mode> map(const ImageData<value_type>& data, Rectangle rect,
                                  Internal::ImageBackend* backend, PixelFormat format) {
        if (backend)
            backend->begin(Mode, rect);
        return ImageAccessT<Mode>(data.subrect(rect), MappedRegion{ rect.p1 },
                                  typename ImageAccessT<Mode>::UnmapFn{ &unmap<Mode>, backend }, format);
    }

    template <AccessMode M>
    static void unmap(void* backend_, ImageData<ConstIfR<value_type, M>>& data, MappedRegion& mapped) {
        Internal::ImageBackend* backend = static_cast<Internal::ImageBackend*>(backend_);
        if (backend)
            backend->end(M, Rectangle{ mapped.origin, data.size });
    }
};

template <PixelType Type, PixelFormat Format>
class ImageTyped : public ImageTyped<Type, PixelFormat::Unknown> {
public:
    using value_subtype = PixelTypeOf<Type>;
    using value_type    = Pixel<value_subtype, Format>;

    template <AccessMode Mode>
    using ImageAccessT = ImageAccess<Type, Format, Mode>;

    using AccessR      = ImageAccessT<AccessMode::R>;
    using AccessW      = ImageAccessT<AccessMode::W>;
    using AccessRW     = ImageAccessT<AccessMode::RW>;

    AccessR mapRead() const {
        return map<AccessMode::R>(data(), this->bounds(), this->m_backend.get());
    }

    AccessW mapWrite() {
        return map<AccessMode::W>(data(), this->bounds(), this->m_backend.get());
    }

    AccessRW mapReadWrite() {
        return map<AccessMode::RW>(data(), this->bounds(), this->m_backend.get());
    }

    AccessR mapRead(Rectangle rect) const {
        return map<AccessMode::R>(data(), rect, this->m_backend.get());
    }

    AccessW mapWrite(Rectangle rect) {
        return map<AccessMode::W>(data(), rect, this->m_backend.get());
    }

    AccessRW mapReadWrite(Rectangle rect) {
        return map<AccessMode::RW>(data(), rect, this->m_backend.get());
    }

    void clear(value_type value) {
        AccessW w = mapWrite();
        w.clear(value);
    }

    ImageData<value_type> data() const {
        return this->m_data.template to<value_type>();
    }

    /// Create image with given size, pixel type and pixel format and allocates memory for it.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(Size size, optional<value_type> clearValue = nullopt)
        : ImageTyped<Type, PixelFormat::Unknown>(size, Type, Format) {
        if (clearValue) {
            clear(*clearValue);
        }
    }

    /// Create image with given size, pixel type and pixel format and reference existing data.
    /// Data is not copied. Caller is responsible for data lifetime.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit ImageTyped(void* data, Size size, int32_t byteStride)
        : ImageTyped<Type, PixelFormat::Unknown>(data, size, byteStride, Type, Format) {}

protected:
    template <AccessMode Mode>
    static ImageAccessT<Mode> map(const ImageData<value_type>& data, Rectangle rect,
                                  Internal::ImageBackend* backend) {
        if (backend)
            backend->begin(Mode, rect);
        return ImageAccessT<Mode>(data.subrect(rect), MappedRegion{ rect.p1 },
                                  typename ImageAccessT<Mode>::UnmapFn{ &unmap<Mode>, backend });
    }

    template <AccessMode M>
    static void unmap(void* backend_, ImageData<ConstIfR<value_type, M>>& data, MappedRegion& mapped) {
        Internal::ImageBackend* backend = static_cast<Internal::ImageBackend*>(backend_);
        if (backend)
            backend->end(M, Rectangle{ mapped.origin, data.size });
    }
};

template <PixelType Type>
using ImageRGBTyped = ImageTyped<Type, PixelFormat::RGB>;
template <PixelType Type>
using ImageRGBATyped = ImageTyped<Type, PixelFormat::RGBA>;
template <PixelType Type>
using ImageGreyscaleTyped = ImageTyped<Type, PixelFormat::Greyscale>;
template <PixelType Type>
using ImageATyped        = ImageTyped<Type, PixelFormat::Alpha>;

using Image              = ImageTyped<PixelType::U8Gamma>;
using ImageRGB           = ImageRGBTyped<PixelType::U8Gamma>;
using ImageRGBA          = ImageRGBATyped<PixelType::U8Gamma>;
using ImageGreyscale     = ImageGreyscaleTyped<PixelType::U8Gamma>;
using ImageA             = ImageATyped<PixelType::U8Gamma>;

using Image_U8           = ImageTyped<PixelType::U16>;
using ImageRGB_U8        = ImageRGBTyped<PixelType::U8>;
using ImageRGBA_U8       = ImageRGBATyped<PixelType::U8>;
using ImageGreyscale_U8  = ImageGreyscaleTyped<PixelType::U8>;
using ImageA_U8          = ImageATyped<PixelType::U8>;

using Image_U16          = ImageTyped<PixelType::U16>;
using ImageRGB_U16       = ImageRGBTyped<PixelType::U16>;
using ImageRGBA_U16      = ImageRGBATyped<PixelType::U16>;
using ImageGreyscale_U16 = ImageGreyscaleTyped<PixelType::U16>;
using ImageA_U16         = ImageATyped<PixelType::U16>;

using Image_F32          = ImageTyped<PixelType::F32>;
using ImageRGB_F32       = ImageRGBTyped<PixelType::F32>;
using ImageRGBA_F32      = ImageRGBATyped<PixelType::F32>;
using ImageGreyscale_F32 = ImageGreyscaleTyped<PixelType::F32>;
using ImageA_F32         = ImageATyped<PixelType::F32>;

inline RC<ImageTyped<PixelType::U8Gamma>> createImage(Size size, PixelFormat fmt = PixelFormat::RGBA) {
    return rcnew ImageTyped<PixelType::U8Gamma>(size, fmt);
}

inline RC<ImageAny> createImage(Size size, PixelType type, PixelFormat fmt) {
    return rcnew ImageAny(size, type, fmt);
}

template <PixelType Type, PixelFormat Format>
inline RC<ImageTyped<Type, Format>> createImage(
    Size size, optional<Pixel<PixelTypeOf<Type>, Format>> clearValue = nullopt) {
    static_assert(Format != PixelFormat::Unknown);
    return rcnew ImageTyped<Type, Format>(size, std::move(clearValue));
}

template <PixelFormat Format>
inline RC<ImageTyped<PixelType::U8Gamma, Format>> createImage(
    Size size, optional<Pixel<uint8_t, Format>> clearValue = nullopt) {
    return createImage<PixelType::U8Gamma, Format>(size, std::move(clearValue));
}

template <PixelType Type, PixelFormat Format>
inline RC<ImageTyped<Type, Format>> createImageLike(RC<ImageTyped<Type, Format>> reference) {
    return createImage(reference->size(), reference->type(), reference->format())
        ->template as<Type, Format>();
}

namespace Internal {
inline ImageBackend* getBackend(RC<ImageAny> image) {
    return image->m_backend.get();
}

inline void setBackend(RC<ImageAny> image, ImageBackend* backend) {
    image->m_backend.reset(backend);
}

} // namespace Internal

/**
 * @brief Custom exception class for image-related errors.
 *
 * This class derives from the standard runtime_error to provide specific error handling for image processing.
 */
class EImageError : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception; ///< Inherit constructors from the base exception class
};

} // namespace Brisk
