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
#pragma once

#include <memory>

namespace Brisk {

/** @brief Alias for a shared pointer type using std::shared_ptr.
 *  @tparam T The type of object managed by the shared pointer.
 */
template <typename T>
using RC = std::shared_ptr<T>;

/** @brief Alias for a weak pointer type using std::weak_ptr.
 *  @tparam T The type of object managed by the weak pointer.
 */
template <typename T>
using WeakRC = std::weak_ptr<T>;

namespace Internal {

/** @brief Utility for creating a shared pointer (RC) from a raw pointer.
 *  The operator `*` is overloaded to construct an `RC` object from a raw pointer.
 *  @details This is used with the `rcnew` macro to allocate objects
 *  and automatically wrap them in an `RC` (std::shared_ptr).
 */
struct RCNew {
    template <typename T>
    RC<T> operator*(T* rawPtr) const {
        return RC<T>(rawPtr);
    }
};

/** @brief Instance of RCNew used for creating reference-counted pointers.
 *  @see RCNew
 */
constexpr inline RCNew rcNew{};

} // namespace Internal

/** @brief Macro to simplify the creation of reference-counted (shared) objects.
 *  @details This macro allows the use of `rcnew` in place of `new`
 *  to directly allocate objects wrapped in `RC` (std::shared_ptr).
 */
#define rcnew ::Brisk::Internal::rcNew* new

/** @brief Wraps a raw pointer in a shared pointer (`RC`) without taking ownership.
 *  @tparam T The type of the object being pointed to.
 *  @param pointer A raw pointer to the object. The pointer is not managed or deleted.
 *  @return A shared pointer (`RC`) that does not manage the lifetime of the object.
 */
template <typename T>
std::shared_ptr<T> notManaged(T* pointer) {
    return std::shared_ptr<T>(pointer, [](T*) {
        /* do nothing */
    });
}

/** @brief A smart pointer-like structure that holds an object in-place.
 *  @details This class provides pointer-like semantics while storing the object
 *  directly within the structure. It can be implicitly converted to a shared pointer
 *  (`RC`) without managing the memory of the contained object.
 *  @tparam T The type of object stored in the structure.
 */
template <typename T>
struct InplacePtr {
    /** @brief The object stored in-place. */
    T value;

    /** @brief Constructs the object in-place with forwarded arguments.
     *  @tparam Args The types of arguments used for constructing the object.
     *  @param args The arguments to construct the object.
     */
    template <typename... Args>
    explicit InplacePtr(Args&&... args) : value{ std::forward<Args>(args)... } {}

    /** @brief Const dereference operator to access the stored object.
     *  @return A const pointer to the stored object.
     */
    constexpr const T* operator->() const noexcept {
        return &value;
    }

    /** @brief Const dereference operator to access the stored object.
     *  @return A const reference to the stored object.
     */
    constexpr const T& operator*() const noexcept {
        return value;
    }

    /** @brief Dereference operator to access the stored object.
     *  @return A pointer to the stored object.
     */
    constexpr T* operator->() noexcept {
        return &value;
    }

    /** @brief Dereference operator to access the stored object.
     *  @return A reference to the stored object.
     */
    constexpr T& operator*() noexcept {
        return value;
    }

    /** @brief Implicit conversion to a const shared pointer (`RC<const T>`).
     *  @return A shared pointer (`RC<const T>`) that does not manage the object.
     */
    operator RC<const T>() const noexcept {
        return notManaged(&value);
    }

    /** @brief Implicit conversion to a const shared pointer of a convertible type.
     *  @tparam U The target type to convert to (must be convertible from T).
     *  @return A shared pointer (`RC<const U>`) that does not manage the object.
     */
    template <typename U>
    operator RC<const U>() const noexcept
        requires std::is_convertible_v<const T*, const U*>
    {
        return notManaged(&value);
    }

    /** @brief Implicit conversion to a shared pointer (`RC<T>`).
     *  @return A shared pointer (`RC<T>`) that does not manage the object.
     */
    operator RC<T>() noexcept {
        return notManaged(&value);
    }

    /** @brief Implicit conversion to a shared pointer of a convertible type.
     *  @tparam U The target type to convert to (must be convertible from T).
     *  @return A shared pointer (`RC<U>`) that does not manage the object.
     */
    template <typename U>
    operator RC<U>() noexcept
        requires std::is_convertible_v<T*, U*>
    {
        return notManaged(&value);
    }
};

} // namespace Brisk
