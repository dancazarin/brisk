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

#include <bit>
#include <array>
#include <stdint.h>
#include <stdlib.h>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include "Math.hpp"
#include "BasicTypes.hpp"

#if 0
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__wasm)
#define SIMD_USE_X86
#define SIMD_USE_SIMD
#include <x86intrin.h>
#elif defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(__aarch64__)
#define SIMD_USE_ARM
#define SIMD_USE_SIMD
#include <arm_neon.h>
#endif
#endif

namespace Brisk {

namespace Internal {
/**
 * @brief Function to remove move semantics from an object.
 */
template <typename T>
constexpr T& unmove(T&& t) {
    return static_cast<T&>(t);
}
} // namespace Internal

namespace SimdInternal {} // namespace SimdInternal

/**
 * @brief Concept to define the types compatible with SIMD operations.
 *
 * The SIMDCompatible concept ensures that only floating-point or integral types
 * (excluding `bool`) are accepted as SIMD data types.
 *
 * @tparam T Type to be checked.
 */
template <typename T>
concept SIMDCompatible = (std::is_floating_point_v<T> || std::is_integral_v<T>)&&!std::is_same_v<T, bool>;

/**
 * @brief A template class that represents a SIMD (Single Instruction Multiple Data) type.
 *
 * The SIMD class provides various utility functions and operations for working with
 * arrays of SIMD-compatible types. It supports element-wise arithmetic, comparisons,
 * shuffling, and more.
 *
 * @tparam T The data type contained in the SIMD array (must satisfy SIMDCompatible).
 * @tparam N The number of elements in the SIMD array.
 */
template <SIMDCompatible T, size_t N>
struct alignas(std::bit_ceil(N) * sizeof(T)) SIMD {
    static_assert(N >= 1 && N <= 16);

    /// Default constructor that initializes the SIMD array to zeros.
    constexpr SIMD() noexcept : m_data{} {}

    /// Copy constructor.
    constexpr SIMD(const SIMD&) noexcept = default;

    /**
     * @brief Constructs a SIMD from another SIMD of possibly different type.
     *
     * @tparam U The type of the other SIMD.
     * @param value The other SIMD to copy values from.
     */
    template <typename U>
    explicit constexpr SIMD(const SIMD<U, N>& value) noexcept {
        for (size_t i = 0; i < N; i++) {
            m_data[i] = value.m_data[i];
        }
    }

    /**
     * @brief Variadic constructor that initializes the SIMD with a list of values.
     *
     * @tparam Args The types of the values.
     * @param values The values to initialize the SIMD.
     */
    template <std::convertible_to<T>... Args>
    constexpr SIMD(Args... values) noexcept
        requires(sizeof...(Args) == N)
        : m_data{ static_cast<T>(values)... } {}

    /**
     * @brief Broadcast constructor that initializes all elements with the same value.
     *
     * @param value The value to broadcast.
     */
    constexpr SIMD(T value) noexcept
        requires(N > 1)
    {
        for (size_t i = 0; i < N; i++) {
            m_data[i] = value;
        }
    }

    /// Returns the size of the SIMD array.
    static constexpr size_t size() noexcept {
        return N;
    }

    /**
     * @brief Access operator for reading elements.
     *
     * @param i Index of the element.
     * @return T The element at index `i`.
     */
    constexpr T operator[](size_t i) const noexcept {
        return m_data[i];
    }

    /**
     * @brief Access operator for modifying elements.
     *
     * @param i Index of the element.
     * @return T& Reference to the element at index `i`.
     */
    constexpr T& operator[](size_t i) noexcept {
        return m_data[i];
    }

    /**
     * @brief Static function to read data from an array into a SIMD.
     *
     * @param data Pointer to the data array.
     * @return SIMD<T, N> A SIMD object populated with the array data.
     */
    static SIMD<T, N> read(const T* data) noexcept {
        SIMD<T, N> result;
        memcpy(result.data(), data, sizeof(T) * N);
        return result;
    }

    /**
     * @brief Writes the contents of the SIMD to an array.
     *
     * @param data Pointer to the destination array.
     */
    void write(T* data) noexcept {
        memcpy(data, this->data(), sizeof(T) * N);
    }

    /**
     * @brief Shuffles the elements of the SIMD according to the provided indices.
     *
     * @tparam indices The indices to shuffle the elements by.
     * @return SIMD<T, sizeof...(indices)> A new SIMD with the shuffled elements.
     */
    template <size_t... indices>
    constexpr SIMD<T, sizeof...(indices)> shuffle(size_constants<indices...>) const noexcept {
        return SIMD<T, sizeof...(indices)>{ m_data[indices]... };
    }

    /**
     * @brief Returns the first Nout elements of the SIMD.
     *
     * @tparam Nout The number of elements to return.
     * @return SIMD<T, Nout> A new SIMD containing the first Nout elements.
     */
    template <size_t Nout>
    constexpr SIMD<T, Nout> firstn(size_constant<Nout> = {}) const noexcept
        requires((Nout <= N))
    {
        return shuffle(size_sequence<Nout>{});
    }

    /**
     * @brief Returns the last Nout elements of the SIMD.
     *
     * @tparam Nout The number of elements to return.
     * @return SIMD<T, Nout> A new SIMD containing the last Nout elements.
     */
    template <size_t Nout>
    constexpr SIMD<T, Nout> lastn(size_constant<Nout> = {}) const noexcept
        requires((Nout <= N))
    {
        return shuffle(size_sequence<Nout>{} + size_constant<N - Nout>{});
    }

    /**
     * @brief Splits the SIMD in half, returning the lower half.
     *
     * @return SIMD<T, N/2> A SIMD containing the lower half of elements.
     */
    SIMD<T, N / 2> low() const noexcept
        requires(N % 2 == 0)
    {
        return shuffle(size_sequence<N / 2>{});
    }

    /**
     * @brief Splits the SIMD in half, returning the upper half.
     *
     * @return SIMD<T, N/2> A SIMD containing the upper half of elements.
     */
    SIMD<T, N / 2> high() const noexcept
        requires(N % 2 == 0)
    {
        return shuffle(size_sequence<N / 2>{} + size_constant<N / 2>{});
    }

    /// Returns the first element in the SIMD.
    T front() const noexcept {
        return m_data[0];
    }

    /// Returns a reference to the first element in the SIMD.
    T& front() noexcept {
        return m_data[0];
    }

    /// Returns the last element in the SIMD.
    T back() const noexcept {
        return m_data[N - 1];
    }

    /// Returns a reference to the last element in the SIMD.
    T& back() noexcept {
        return m_data[N - 1];
    }

    /// Returns a pointer to the underlying data.
    const T* data() const noexcept {
        return m_data;
    }

    /// Returns a mutable pointer to the underlying data.
    T* data() noexcept {
        return m_data;
    }

    /// Underlying storage for the SIMD elements.
    T m_data[(N)];
};

/**
 * @brief Template deduction guide for SIMD.
 *
 * This deduction guide allows for automatic template deduction based on constructor arguments.
 */
template <typename... Args>
SIMD(Args&&...) -> SIMD<std::common_type_t<Args...>, sizeof...(Args)>;

/**
 * @typedef SIMDMask
 * @brief Alias for a boolean mask used with SIMD operations.
 *
 * @tparam N The size of the mask.
 */
template <size_t N>
using SIMDMask = std::array<bool, N>;

/**
 * @typedef SIMDIndices
 * @brief Alias for an index array used in SIMD shuffling operations.
 *
 * @tparam N The number of indices.
 */
template <size_t N>
using SIMDIndices = std::array<size_t, N>;

/**
 * @brief Performs element-wise addition between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N>& A reference to the updated SIMD object after addition.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator+=(SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] += rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Adds two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator+(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    return Internal::unmove(SIMD<T, N>(lhs)) += rhs;
}

/**
 * @brief Adds a scalar value to each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N>& A reference to the updated SIMD object after addition.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator+=(SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs += SIMD<T, N>(rhs);
}

/**
 * @brief Adds a scalar value to each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator+(const SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs + SIMD<T, N>(rhs);
}

/**
 * @brief Adds a SIMD object to a scalar value, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the addition.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator+(T lhs, const SIMD<T, N>& rhs) noexcept {
    return SIMD<T, N>(lhs) + rhs;
}

/**
 * @brief Performs element-wise subtraction between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N>& A reference to the updated SIMD object after subtraction.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator-=(SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] -= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Subtracts two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator-(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    return Internal::unmove(SIMD<T, N>(lhs)) -= rhs;
}

/**
 * @brief Subtracts a scalar value from each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N>& A reference to the updated SIMD object after subtraction.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator-=(SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs -= SIMD<T, N>(rhs);
}

/**
 * @brief Subtracts a scalar value from each element of a SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator-(const SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs - SIMD<T, N>(rhs);
}

/**
 * @brief Subtracts each element of a SIMD object from a scalar value, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the subtraction.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator-(T lhs, const SIMD<T, N>& rhs) noexcept {
    return SIMD<T, N>(lhs) - rhs;
}

/**
 * @brief Performs element-wise multiplication between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N>& A reference to the updated SIMD object after multiplication.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator*=(SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] *= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Multiplies two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator*(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    return Internal::unmove(SIMD<T, N>(lhs)) *= rhs;
}

/**
 * @brief Multiplies each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N>& A reference to the updated SIMD object after multiplication.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator*=(SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs *= SIMD<T, N>(rhs);
}

/**
 * @brief Multiplies each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator*(const SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs * SIMD<T, N>(rhs);
}

/**
 * @brief Multiplies a scalar value by each element of a SIMD object, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the multiplication.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator*(T lhs, const SIMD<T, N>& rhs) noexcept {
    return SIMD<T, N>(lhs) * rhs;
}

/**
 * @brief Performs element-wise division between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N>& A reference to the updated SIMD object after division.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator/=(SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] /= rhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Divides two SIMD objects element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator/(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    return Internal::unmove(SIMD<T, N>(lhs)) /= rhs;
}

/**
 * @brief Divides each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N>& A reference to the updated SIMD object after division.
 */
template <typename T, size_t N>
constexpr SIMD<T, N>& operator/=(SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs /= SIMD<T, N>(rhs);
}

/**
 * @brief Divides each element of a SIMD object by a scalar value.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @param rhs The scalar value.
 * @return SIMD<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator/(const SIMD<T, N>& lhs, T rhs) noexcept {
    return lhs / SIMD<T, N>(rhs);
}

/**
 * @brief Divides a scalar value by each element of a SIMD object, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The scalar value.
 * @param rhs The SIMD object.
 * @return SIMD<T, N> A new SIMD object containing the result of the division.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator/(T lhs, const SIMD<T, N>& rhs) noexcept {
    return SIMD<T, N>(lhs) / rhs;
}

/**
 * @brief Unary plus operator for SIMD objects (returns the object unchanged).
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return SIMD<T, N> The input SIMD object, unchanged.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator+(const SIMD<T, N>& lhs) noexcept {
    return lhs;
}

/**
 * @brief Unary negation operator for SIMD objects (negates each element).
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return SIMD<T, N> A new SIMD object with negated elements.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> operator-(SIMD<T, N> lhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        lhs.m_data[i] = -lhs.m_data[i];
    }
    return lhs;
}

/**
 * @brief Equality comparison between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return true if all elements are equal, false otherwise.
 */
template <typename T, size_t N>
constexpr bool operator==(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    for (size_t i = 0; i < N; i++) {
        if (lhs.m_data[i] != rhs.m_data[i])
            return false;
    }
    return true;
}

/**
 * @brief Inequality comparison between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return true if any element differs, false otherwise.
 */
template <typename T, size_t N>
constexpr bool operator!=(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    return !operator==(lhs, rhs);
}

/**
 * @brief Compares two SIMD objects for equality, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements are equal.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> eq(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] == rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects for inequality, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements are not equal.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> ne(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] != rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is less than the right-hand side,
 * element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements in the left-hand side are less than the corresponding
 * elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> lt(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] < rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is greater than the right-hand side,
 * element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements in the left-hand side are greater than the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> gt(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] > rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is less than or equal to the right-hand
 * side, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements in the left-hand side are less than or equal to the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> le(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] <= rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Compares two SIMD objects to determine if the left-hand side is greater than or equal to the
 * right-hand side, element-wise.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMDMask<N> A mask indicating which elements in the left-hand side are greater than or equal to the
 * corresponding elements in the right-hand side.
 */
template <typename T, size_t N>
constexpr SIMDMask<N> ge(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs.m_data[i] >= rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Performs a bitwise OR between two SIMD masks.
 *
 * @tparam N The number of elements in the mask.
 * @param lhs The left-hand side SIMD mask.
 * @param rhs The right-hand side SIMD mask.
 * @return SIMDMask<N> A mask with the result of the OR operation applied element-wise.
 */
template <size_t N>
constexpr SIMDMask<N> maskOr(const SIMDMask<N>& lhs, const SIMDMask<N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs[i] || rhs[i];
    }
    return result;
}

/**
 * @brief Performs a bitwise AND between two SIMD masks.
 *
 * @tparam N The number of elements in the mask.
 * @param lhs The left-hand side SIMD mask.
 * @param rhs The right-hand side SIMD mask.
 * @return SIMDMask<N> A mask with the result of the AND operation applied element-wise.
 */
template <size_t N>
constexpr SIMDMask<N> maskAnd(const SIMDMask<N>& lhs, const SIMDMask<N>& rhs) {
    SIMDMask<N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = lhs[i] && rhs[i];
    }
    return result;
}

/**
 * @brief Determines if all elements of a SIMD mask are true.
 *
 * @tparam N The number of elements in the mask.
 * @param value The SIMD mask to check.
 * @return true if all elements are true, false otherwise.
 */
template <size_t N>
constexpr bool horizontalAll(const SIMDMask<N>& value) {
    return [&]<size_t... indices>(size_constants<indices...>) {
        return (value[indices] && ...);
    }(size_sequence<N>{});
}

/**
 * @brief Determines if any element of a SIMD mask is true.
 *
 * @tparam N The number of elements in the mask.
 * @param value The SIMD mask to check.
 * @return true if any element is true, false otherwise.
 */
template <size_t N>
constexpr bool horizontalAny(const SIMDMask<N>& value) {
    return [&]<size_t... indices>(size_constants<indices...>) {
        return (value[indices] || ...);
    }(size_sequence<N>{});
}

/**
 * @brief Selects elements from two SIMD objects based on a mask.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param mask The SIMD mask used for selection.
 * @param trueval The SIMD object whose elements are selected where the mask is true.
 * @param falseval The SIMD object whose elements are selected where the mask is false.
 * @return SIMD<T, N> The resulting SIMD object with selected elements.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> select(const SIMDMask<N>& mask, const SIMD<T, N>& trueval, const SIMD<T, N>& falseval) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = mask[i] ? trueval.m_data[i] : falseval.m_data[i];
    }
    return result;
}

/**
 * @brief Shuffles elements in the SIMD object based on specified indices.
 *
 * @tparam indices The indices to shuffle the elements.
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param source The SIMD object to shuffle.
 * @return SIMD<T, sizeof...(indices)> The shuffled SIMD object.
 */
template <size_t... indices, typename T, size_t N>
constexpr SIMD<T, sizeof...(indices)> shuffle(const SIMD<T, N>& source) noexcept {
    return SIMD<T, sizeof...(indices)>{ { source.m_data[indices]... } };
}

/**
 * @brief Concatenates multiple SIMD objects into a single SIMD object.
 *
 * @tparam T The data type contained in the SIMD arrays.
 * @tparam Nin The sizes of the SIMD arrays to concatenate.
 * @param source The SIMD objects to concatenate.
 * @return SIMD<T, (Nin + ...)> The concatenated SIMD object.
 */
template <typename T, size_t... Nin>
constexpr SIMD<T, (Nin + ...)> concat(const SIMD<T, Nin>&... source) noexcept {
    SIMD<T, (Nin + ...)> result;
    size_t j = 0;
    (([&](auto x) {
         for (size_t i = 0; i < x.size(); ++i) {
             result.m_data[j++] = x.m_data[i];
         }
     })(source),
     ...);
    return result;
}

/**
 * @brief Returns the element-wise minimum between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A SIMD object containing the element-wise minimum values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> min(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    SIMD<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::min(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Returns the element-wise maximum between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> A SIMD object containing the element-wise maximum values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> max(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    SIMD<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::max(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Clamps each element of the SIMD object between a lower and upper bound.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param x The SIMD object to clamp.
 * @param low The lower bound.
 * @param high The upper bound.
 * @return SIMD<T, N> A SIMD object with elements clamped between the low and high values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> clamp(const SIMD<T, N>& x, const SIMD<T, N>& low, const SIMD<T, N>& high) noexcept {
    SIMD<T, N> result;
    for (size_t i = 0; i < N; i++) {
        result.m_data[i] = std::clamp(x.m_data[i], low.m_data[i], high.m_data[i]);
    }
    return result;
}

/**
 * @brief Blends two SIMD objects based on a mask.
 *
 * @tparam mask The mask for blending, where 1 means selecting from val1, 0 means selecting from val0.
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param val0 The first SIMD object.
 * @param val1 The second SIMD object.
 * @return SIMD<T, N> The blended SIMD object.
 */
template <int... mask, typename T, size_t N>
constexpr SIMD<T, N> blend(const SIMD<T, N>& val0, const SIMD<T, N>& val1) noexcept
    requires(sizeof...(mask) == N)
{
    return [&]<size_t... indices>(std::index_sequence<indices...>) {
        return SIMD<T, N>{ (mask ? val1.m_data[indices] : val0.m_data[indices])... };
    }(std::make_index_sequence<N>{});
}

namespace Internal {
/**
 * @brief Computes the absolute value of a number, optimized for constant evaluation.
 *
 * @tparam T The type of the input value.
 * @param x The input value.
 * @return T The absolute value of the input.
 */
template <typename T>
constexpr T constexprAbs(T x) {
    if (std::is_constant_evaluated())
        return x < T(0) ? -x : x;
    else
        return std::abs(x);
}

/**
 * @brief Copies the sign from one value to another, optimized for constant evaluation.
 *
 * @tparam T The type of the input values.
 * @param x The value whose magnitude is used.
 * @param s The value whose sign is copied.
 * @return T The value of x with the sign of s.
 */
template <typename T>
constexpr T constexprCopysign(T x, T s) {
    if (std::is_constant_evaluated())
        return s < 0 ? -constexprAbs(x) : constexprAbs(x);
    else
        return std::copysign(x, s);
}
} // namespace Internal

/**
 * @brief Computes the minimum element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The minimum value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalMin(const SIMD<T, N>& lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result = std::min(result, lhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the maximum element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The maximum value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalMax(const SIMD<T, N>& lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result = std::max(result, lhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the maximum absolute element of the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The maximum absolute value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalAbsMax(const SIMD<T, N>& lhs) noexcept {
    T result = Internal::constexprAbs(lhs[0]);
    for (size_t i = 1; i < N; i++) {
        result = std::max(result, Internal::constexprAbs(lhs.m_data[i]));
    }
    return result;
}

/**
 * @brief Computes the sum of all elements in the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The sum of all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalSum(const SIMD<T, N>& lhs) noexcept {
    T result = lhs[0];
    for (size_t i = 1; i < N; i++) {
        result += lhs.m_data[i];
    }
    return result;
}

/**
 * @brief Computes the root mean square (RMS) of the elements in the SIMD object.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The SIMD object.
 * @return T The RMS value across all elements in the SIMD object.
 */
template <typename T, size_t N>
constexpr T horizontalRMS(const SIMD<T, N>& lhs) noexcept {
    T result = sqr(lhs[0]);
    for (size_t i = 1; i < N; i++) {
        result += sqr(lhs.m_data[i]);
    }
    return std::sqrt(result);
}

/**
 * @brief Computes the dot product of two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return T The dot product of the two SIMD objects.
 */
template <typename T, size_t N>
constexpr T dot(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) noexcept {
    T result = lhs[0] * rhs[0];
    for (size_t i = 1; i < N; i++) {
        result += lhs.m_data[i] * rhs.m_data[i];
    }
    return result;
}

/**
 * @brief Linearly interpolates between two SIMD objects.
 *
 * @tparam T The data type contained in the SIMD array.
 * @tparam N The number of elements in the SIMD array.
 * @param t The interpolation factor, typically between 0 and 1.
 * @param lhs The left-hand side SIMD object.
 * @param rhs The right-hand side SIMD object.
 * @return SIMD<T, N> The interpolated SIMD object.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> mix(float t, const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    return lhs * (1 - t) + rhs * t;
}

/**
 * @brief Raises each element of the SIMD object to the power of the corresponding element in another SIMD
 * object.
 *
 * @tparam T1 The data type of the base SIMD array.
 * @tparam T2 The data type of the exponent SIMD array.
 * @tparam N The number of elements in the SIMD arrays.
 * @tparam U The resulting type after the power operation.
 * @param lhs The base SIMD object.
 * @param rhs The exponent SIMD object.
 * @return SIMD<U, N> A SIMD object containing the element-wise power results.
 */
template <typename T1, typename T2, size_t N,
          typename U = decltype(std::pow(std::declval<T1>(), std::declval<T2>()))>
constexpr SIMD<U, N> pow(const SIMD<T1, N>& lhs, const SIMD<T2, N>& rhs) {
    SIMD<U, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::pow(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise absolute value of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with absolute values of each element.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> abs(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = Internal::constexprAbs(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise square root of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with square roots of each element.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> sqrt(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::sqrt(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the sine and cosine for each element of a SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with alternating sine and cosine values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> sincos(const SIMD<T, N>& val)
    requires(N % 2 == 0)
{
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i += 2) {
        result[i]     = std::sin(val.m_data[i]);
        result[i + 1] = std::cos(val.m_data[i + 1]);
    }
    return result;
}

/**
 * @brief Computes the cosine and sine for each element of a SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with alternating cosine and sine values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> cossin(const SIMD<T, N>& val)
    requires(N % 2 == 0)
{
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i += 2) {
        result[i]     = std::cos(val.m_data[i]);
        result[i + 1] = std::sin(val.m_data[i + 1]);
    }
    return result;
}

/**
 * @brief Copies the sign from one SIMD object to another element-wise.
 *
 * @tparam T The type of the elements in the SIMD objects.
 * @tparam N The number of elements in the SIMD objects.
 * @param lhs The SIMD object providing the magnitude.
 * @param rhs The SIMD object providing the sign.
 * @return SIMD<T, N> A new SIMD object where each element has the magnitude of lhs and the sign of rhs.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> copysign(const SIMD<T, N>& lhs, const SIMD<T, N>& rhs) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = Internal::constexprCopysign(lhs.m_data[i], rhs.m_data[i]);
    }
    return result;
}

/**
 * @brief Rounds each element in the SIMD object to the nearest integer.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with each element rounded.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> round(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::round(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise floor (round down) of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with floored values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> floor(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::floor(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Computes the element-wise ceiling (round up) of a SIMD object.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with ceiling values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> ceil(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::ceil(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Truncates each element in the SIMD object to its integer part.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with truncated values.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> trunc(const SIMD<T, N>& val) {
    SIMD<T, N> result{};
    for (size_t i = 0; i < N; i++) {
        result[i] = std::trunc(val.m_data[i]);
    }
    return result;
}

/**
 * @brief Swaps adjacent elements in the SIMD object.
 *
 * @note The number of elements (N) must be even.
 *
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, N> A new SIMD object with adjacent elements swapped.
 */
template <typename T, size_t N>
constexpr SIMD<T, N> swapAdjacent(const SIMD<T, N>& val)
    requires(N % 2 == 0)
{
    return val.shuffle(size_sequence<N>{} ^ size_constant<1>{});
}

/**
 * @brief Repeats the elements of a SIMD object multiple times.
 *
 * @tparam Ncount The number of times to repeat the elements.
 * @tparam T The type of the elements in the SIMD object.
 * @tparam N The number of elements in the SIMD object.
 * @param val The SIMD object.
 * @return SIMD<T, Ncount * N> A new SIMD object with repeated elements.
 */
template <size_t Ncount, typename T, size_t N>
constexpr SIMD<T, Ncount * N> repeat(const SIMD<T, N>& val) {
    return val.shuffle(size_sequence<Ncount * N>{} % size_constant<N>{});
}

/**
 * @brief Rescales the elements of a SIMD object, either between different ranges or floating-point types.
 *
 * @note This overload is for rescaling with floating-point values.
 *
 * @tparam Tout The output type of the elements.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the elements.
 * @tparam N The number of elements in the SIMD object.
 * @param value The SIMD object to rescale.
 * @return SIMD<Tout, N> A rescaled SIMD object.
 */
template <SIMDCompatible Tout, int Mout, int Min, SIMDCompatible Tin, size_t N>
constexpr SIMD<Tout, N> rescale(const SIMD<Tin, N>& value)
    requires(Mout != Min && (std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))
{
    using Tcommon      = std::common_type_t<Tin, Tout>;
    SIMD<Tcommon, N> x = static_cast<SIMD<Tcommon, N>>(value) * Tcommon(Mout) / Tcommon(Min);
    if constexpr (!std::is_floating_point_v<Tout>)
        x += Tcommon(0.5);
    return static_cast<SIMD<Tout, N>>(clamp(x, SIMD<Tcommon, N>(0), SIMD<Tcommon, N>(Mout)));
}

/**
 * @brief Rescales the elements of a SIMD object, either between different ranges or integral types.
 *
 * @note This overload is for rescaling with integral values.
 *
 * @tparam Tout The output type of the elements.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the elements.
 * @tparam N The number of elements in the SIMD object.
 * @param value The SIMD object to rescale.
 * @return SIMD<Tout, N> A rescaled SIMD object.
 */
template <SIMDCompatible Tout, int Mout, int Min, SIMDCompatible Tin, size_t N>
constexpr SIMD<Tout, N> rescale(const SIMD<Tin, N>& value)
    requires(Mout != Min && !(std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))
{
    using Tcommon =
        find_integral_type<std::numeric_limits<Tin>::min() * Mout, std::numeric_limits<Tin>::max() * Mout>;
    return static_cast<SIMD<Tout, N>>(static_cast<SIMD<Tcommon, N>>(value) * Tcommon(Mout) / Tcommon(Min));
}

/**
 * @brief Rescales a single value or a SIMD object when the input and output ranges are equal.
 *
 * @tparam Tout The output type of the value.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the value.
 * @return SIMD<Tout, N> The rescaled value or SIMD object.
 */
template <SIMDCompatible Tout, int Mout, int Min, SIMDCompatible Tin, size_t N>
constexpr SIMD<Tout, N> rescale(const SIMD<Tin, N>& value)
    requires(Mout == Min)
{
    if constexpr (!std::is_floating_point_v<Tout>) {
        return static_cast<SIMD<Tout, N>>(round(value));
    } else {
        return static_cast<SIMD<Tout, N>>(value);
    }
}

/**
 * @brief Rescales a single value between two ranges.
 *
 * @tparam Tout The output type of the value.
 * @tparam Mout The maximum value for the output range.
 * @tparam Min The minimum value for the input range.
 * @tparam Tin The input type of the value.
 * @param value The value to rescale.
 * @return Tout The rescaled value.
 */
template <SIMDCompatible Tout, int Mout, int Min, SIMDCompatible Tin>
constexpr Tout rescale(Tin value) {
    return rescale<Tout, Mout, Min, Tin>(SIMD<Tin, 1>(value)).front();
}

namespace Internal {
/**
 * @brief Byte-swaps a 16-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 16-bit unsigned integer.
 * It can be used at compile time when the value is known at that time, but
 * switches to a platform-optimized version at runtime when needed.
 *
 * @param x The 16-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 16-bit unsigned integer.
 */
constexpr uint16_t byteswap(uint16_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap16(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    return std::rotr(x, 8); ///< MSVC implementation using rotate-right by 8 bits.
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint16_t>(0x1122)) == static_cast<uint16_t>(0x2211));

/**
 * @brief Byte-swaps a 32-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 32-bit unsigned integer.
 * It can be used at compile time if the value is known, otherwise it switches
 * to a platform-optimized implementation at runtime.
 *
 * @param x The 32-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 32-bit unsigned integer.
 */
constexpr uint32_t byteswap(uint32_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap32(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    if (__builtin_is_constant_evaluated()) {
        // Compile-time byte-swapping using recursive bit-casting and swapping.
        auto a = std::bit_cast<std::array<uint16_t, 2>>(x);
        a      = { byteswap(a[1]), byteswap(a[0]) };
        return std::bit_cast<uint32_t>(a);
    } else {
        // Optimized runtime byte-swap using MSVC intrinsic.
        return _byteswap_ulong(x);
    }
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint32_t>(0x11223344)) == static_cast<uint32_t>(0x44332211));

/**
 * @brief Byte-swaps a 64-bit unsigned integer.
 *
 * This constexpr function swaps the byte order of a 64-bit unsigned integer.
 * It can be used at compile time if the value is known, otherwise it switches
 * to a platform-optimized implementation at runtime.
 *
 * @param x The 64-bit unsigned integer to be byte-swapped.
 * @return The byte-swapped 64-bit unsigned integer.
 */
constexpr uint64_t byteswap(uint64_t x) {
#ifdef BRISK_GNU_ATTR
    return __builtin_bswap64(x); ///< Optimized GCC/Clang intrinsic for byte-swapping.
#elif defined _MSC_VER
    if (__builtin_is_constant_evaluated()) {
        // Compile-time byte-swapping using recursive bit-casting and swapping.
        auto a = std::bit_cast<std::array<uint32_t, 2>>(x);
        a      = { byteswap(a[1]), byteswap(a[0]) };
        return std::bit_cast<uint64_t>(a);
    } else {
        // Optimized runtime byte-swap using MSVC intrinsic.
        return _byteswap_uint64(x);
    }
#endif
}

// Static assertion to ensure that the byte-swap works as expected at compile time.
static_assert(byteswap(static_cast<uint64_t>(0x1122334455667788)) ==
              static_cast<uint64_t>(0x8877665544332211));

} // namespace Internal

} // namespace Brisk
