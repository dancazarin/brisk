#pragma once

#include "Geometry.hpp"

namespace Brisk {

/**
 * @brief Represents a 2D matrix of floating point values.
 *
 * This template class provides a 2D transformation matrix implementation with
 * support for translation, scaling, rotation, reflection, and skewing. It
 * works on any floating-point type (e.g., float, double).
 *
 * @tparam T The type of floating-point values (e.g., float or double).
 */
template <typename T>
struct Matrix2DOf {
    static_assert(std::is_floating_point_v<T>, "Matrix2DOf requires a floating-point type.");
    using vec_subtype = SIMD<T, 2>;
    using vec_type    = std::array<vec_subtype, 3>;

    union {
        vec_type v; ///< Array of SIMD vectors for efficient storage.

        struct {
            T a, b, c, d, e, f; ///< Individual matrix coefficients.
        };
    };

    /**
     * @brief Constructs an identity matrix.
     */
    constexpr Matrix2DOf() : Matrix2DOf(1, 0, 0, 1, 0, 0) {}

    /**
     * @brief Constructs a matrix with specified coefficients.
     *
     * @param a Matrix coefficient at position (0,0).
     * @param b Matrix coefficient at position (0,1).
     * @param c Matrix coefficient at position (1,0).
     * @param d Matrix coefficient at position (1,1).
     * @param e Matrix translation component along the x-axis.
     * @param f Matrix translation component along the y-axis.
     */
    constexpr Matrix2DOf(T a, T b, T c, T d, T e, T f) : v{ SIMD{ a, b }, SIMD{ c, d }, SIMD{ e, f } } {}

    /**
     * @brief Returns the matrix coefficients as an array.
     *
     * @return std::array<T, 6> The array of coefficients {a, b, c, d, e, f}.
     */
    constexpr std::array<T, 6> coefficients() const {
        return { { a, b, c, d, e, f } };
    }

    /**
     * @brief Constructs a matrix from a given vector type.
     *
     * @param v The vector type representing the matrix.
     */
    constexpr explicit Matrix2DOf(const vec_type& v) : v(v) {}

    /**
     * @brief Translates the matrix by a given point offset.
     *
     * @param offset The point by which to translate.
     * @return Matrix2DOf The translated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf translate(PointOf<T> offset) const {
        vec_type m = v;
        m[2] += offset.v;
        return Matrix2DOf(m);
    }

    /**
     * @brief Translates the matrix by given x and y offsets.
     *
     * @param x The x-axis translation.
     * @param y The y-axis translation.
     * @return Matrix2DOf The translated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf translate(T x, T y) const {
        return translate({ x, y });
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @return Matrix2DOf The scaled matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf scale(T x, T y) const {
        vec_type m = v;
        m[0] *= SIMD{ x, y };
        m[1] *= SIMD{ x, y };
        m[2] *= SIMD{ x, y };
        return Matrix2DOf(m);
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors with respect to an origin point.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @param origin The origin point.
     * @return Matrix2DOf The scaled matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf scale(T x, T y, PointOf<T> origin) const {
        return translate(-origin).scale(x, y).translate(origin);
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors with respect to a specified origin.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return Matrix2DOf The scaled matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf scale(T x, T y, T originx, T originy) const {
        return scale(x, y, { originx, originy });
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @return Matrix2DOf The skewed matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf skew(T x, T y) const {
        vec_type m;
        m[0] = SIMD{ v[0][0] + v[0][1] * x, v[0][0] * y + v[0][1] };
        m[1] = SIMD{ v[1][0] + v[1][1] * x, v[1][0] * y + v[1][1] };
        m[2] = SIMD{ v[2][0] + v[2][1] * x, v[2][0] * y + v[2][1] };
        return Matrix2DOf(m);
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients with respect to an origin point.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @param origin The origin point.
     * @return Matrix2DOf The skewed matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf skew(T x, T y, PointOf<T> origin) const {
        return translate(-origin).skew(x, y).translate(origin);
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients with respect to a specified origin.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return Matrix2DOf The skewed matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf skew(T x, T y, T originx, T originy) const {
        return skew(x, y, { originx, originy });
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees).
     *
     * @param angle The angle in degrees.
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate(T angle) const {
        vec_type m     = v;
        vec_subtype sc = sincos(vec_subtype(deg2rad<T> * angle));
        vec_subtype cs = swapAdjacent(sc) * SIMD{ T(1), T(-1) };
        m[0]           = SIMD{ dot(m[0], cs), dot(m[0], sc) };
        m[1]           = SIMD{ dot(m[1], cs), dot(m[1], sc) };
        m[2]           = SIMD{ dot(m[2], cs), dot(m[2], sc) };
        return Matrix2DOf(m);
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees) with respect to an origin point.
     *
     * @param angle The angle in degrees.
     * @param origin The origin point for rotation.
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate(T angle, PointOf<T> origin) const {
        return translate(-origin).rotate(angle).translate(origin);
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees) with respect to a specified origin.
     *
     * @param angle The angle in degrees.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate(T angle, T originx, T originy) const {
        return rotate(angle, { originx, originy });
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate90(int angle) const {
        Matrix2DOf result = *this;
        switch (unsigned(angle) % 4) {
        case 0:
        default:
            break;
        case 1:
            result = { -b, a, -d, c, -f, e };
            break;
        case 2:
            result = { -a, -b, -c, -d, -e, -f };
            break;
        case 3:
            result = { b, -a, d, -c, f, -e };
            break;
        }
        return result;
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees with respect to a point.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @param origin The origin point for rotation.
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate90(int angle, PointOf<T> origin) const {
        return translate(-origin).rotate90(angle).translate(origin);
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees with respect to an origin.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return Matrix2DOf The rotated matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf rotate90(int angle, T originx, T originy) const {
        return rotate90(angle, { originx, originy });
    }

    /**
     * @brief Reflects the matrix over the specified axis.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @return Matrix2DOf The reflected matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf reflect(FlipAxis axis) const {
        switch (axis) {
        case FlipAxis::X:
            return scale(-1, 1);
        case FlipAxis::Y:
            return scale(1, -1);
        case FlipAxis::Both:
            return scale(-1, -1);
        }
    }

    /**
     * @brief Reflects the matrix over the specified axis with respect to a point.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @param origin The origin point for the reflection.
     * @return Matrix2DOf The reflected matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf reflect(FlipAxis axis, PointOf<T> origin) const {
        return translate(-origin).reflect(axis).translate(origin);
    }

    /**
     * @brief Reflects the matrix over the specified axis with respect to an origin.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return Matrix2DOf The reflected matrix.
     */
    [[nodiscard]] constexpr Matrix2DOf reflect(FlipAxis axis, T originx, T originy) const {
        return reflect(axis, { originx, originy });
    }

    /**
     * @brief Creates a translation matrix.
     *
     * @param x Translation along the x-axis.
     * @param y Translation along the y-axis.
     * @return Matrix2DOf The translation matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf translation(T x, T y) {
        return Matrix2DOf{ 1, 0, 0, 1, x, y };
    }

    /**
     * @brief Creates a scaling matrix.
     *
     * @param x Scaling factor along the x-axis.
     * @param y Scaling factor along the y-axis.
     * @return Matrix2DOf The scaling matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf scaling(T x, T y) {
        return Matrix2DOf{ x, 0, 0, y, 0, 0 };
    }

    /**
     * @brief Creates a rotation matrix.
     *
     * @param angle The rotation angle in degrees.
     * @return Matrix2DOf The rotation matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf rotation(T angle) {
        vec_subtype sc = sincos(vec_subtype(deg2rad<T> * angle));
        return Matrix2DOf{ sc[1], sc[0], -sc[0], sc[1], 0, 0 };
    }

    /**
     * @brief Creates a 90-degree rotation matrix.
     *
     * @param angle The multiple of 90 degrees (0, 90, 180, or 270).
     * @return Matrix2DOf The 90-degree rotation matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf rotation90(int angle) {
        constexpr Matrix2DOf<T> m[4] = {
            { 1, 0, 0, 1, 0, 0 },
            { 0, 1, -1, 0, 0, 0 },
            { -1, 0, 0, -1, 0, 0 },
            { 0, -1, 1, 0, 0, 0 },
        };
        return m[angle % 4];
    }

    /**
     * @brief Creates a reflection matrix over the specified axis.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @return Matrix2DOf The reflection matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf reflection(FlipAxis axis) {
        switch (axis) {
        case FlipAxis::X:
            return scaling(-1, 1);
        case FlipAxis::Y:
            return scaling(1, -1);
        case FlipAxis::Both:
            return scaling(-1, -1);
        }
    }

    /**
     * @brief Creates a skewness matrix.
     *
     * @param x The x-axis skew factor.
     * @param y The y-axis skew factor.
     * @return Matrix2DOf The skewness matrix.
     */
    [[nodiscard]] static constexpr Matrix2DOf skewness(T x, T y) {
        return Matrix2DOf{ 1, y, x, 1, 0, 0 };
    }

    /**
     * @brief Multiplies two matrices together.
     *
     * @param m The first matrix.
     * @param n The second matrix.
     * @return Matrix2DOf The resulting matrix.
     */
    constexpr friend Matrix2DOf<T> operator*(const Matrix2DOf<T>& m, const Matrix2DOf<T>& n) {
        using v2             = SIMD<T, 2>;
        using v3             = SIMD<T, 3>;
        using v4             = SIMD<T, 4>;
        using v8             = SIMD<T, 8>;

        std::array<v2, 3> mm = m.v;
        std::array<v3, 2> nn{ v3{ n.a, n.c, n.e }, v3{ n.b, n.d, n.f } };
        v4 n01   = concat(nn[0].firstn(size_constant<2>{}), nn[1].firstn(size_constant<2>{}));
        v4 m0n01 = mm[0].shuffle(size_constants<0, 1, 0, 1>{}) * n01;
        v4 m1n01 = mm[1].shuffle(size_constants<0, 1, 0, 1>{}) * n01;
        v4 m2n01 = mm[2].shuffle(size_constants<0, 1, 0, 1>{}) * n01;

        v8 m01   = concat(m0n01, m1n01).shuffle(size_constants<0, 2, 4, 6, 1, 3, 5, 7>{});
        v8 m23   = concat(m2n01, m2n01).shuffle(size_constants<0, 2, 4, 6, 1, 3, 5, 7>{});
        v4 t0    = m01.low() + m01.high();
        v4 t1    = m23.low() + m23.high();

        return {
            t0[0],            //
            t0[1],            //
            t0[2],            //
            t0[3],            //
            t1[0] + nn[0][2], //
            t1[1] + nn[1][2], //
        };
    }

    /**
     * @brief Transforms a point using the matrix.
     *
     * @param pt The point to transform.
     * @param m The transformation matrix.
     * @return PointOf<T> The transformed point.
     */
    constexpr friend PointOf<T> operator*(const PointOf<T>& pt, const Matrix2DOf<T>& m) {
        return m.transform(pt);
    }

    /**
     * @brief Flattens the matrix coefficients into a SIMD array.
     *
     * @return SIMD<T, 6> The flattened matrix.
     */
    constexpr SIMD<T, 6> flatten() const noexcept {
        SIMD<T, 6> result;
        std::memcpy(&result, v.data(), sizeof(*this));
        return result;
    }

    /**
     * @brief Checks if two matrices are equal.
     *
     * @param m The matrix to compare with.
     * @return true if the matrices are equal, false otherwise.
     */
    constexpr bool operator==(const Matrix2DOf<T>& m) const {
        return horizontalRMS(flatten() - m.flatten()) < 0.0001f;
    }

    /**
     * @brief Checks if two matrices are not equal.
     *
     * @param m The matrix to compare with.
     * @return true if the matrices are not equal, false otherwise.
     */
    constexpr bool operator!=(const Matrix2DOf<T>& m) const {
        return !operator==(m);
    }

    /**
     * @brief Transforms a rectangle using the matrix.
     *
     * @param pt The rectangle to transform.
     * @return RectangleOf<T> The transformed rectangle.
     */
    constexpr RectangleOf<T> transform(RectangleOf<T> pt) const {
        PointOf<T> points[4] = { pt.p1, pt.p2, { pt.x1, pt.y2 }, { pt.x2, pt.y1 } };
        transform(points);
        RectangleOf<T> result;
        result.x1 = std::min(std::min(points[0].x, points[1].x), std::min(points[2].x, points[3].x));
        result.y1 = std::min(std::min(points[0].y, points[1].y), std::min(points[2].y, points[3].y));
        result.x2 = std::max(std::max(points[0].x, points[1].x), std::max(points[2].x, points[3].x));
        result.y2 = std::max(std::max(points[0].y, points[1].y), std::max(points[2].y, points[3].y));
        return result;
    }

    /**
     * @brief Estimates the average scaling factor of the matrix.
     *
     * This method calculates an average scale by computing the hypotenuse of
     * the matrix's first two columns and averaging them.
     *
     * @return T The estimated scaling factor.
     */
    constexpr T estimateScale() const {
        T x = std::hypot(v[0][0], v[1][0]);
        T y = std::hypot(v[0][1], v[1][1]);
        return T(0.5) * (x + y);
    }

    /**
     * @brief Transforms a point using the matrix.
     *
     * Applies a 2D transformation to a given point using the current matrix.
     * The transformation follows the formula:
     * \f$ x' = x \cdot a + y \cdot c + e \f$
     * \f$ y' = x \cdot b + y \cdot d + f \f$
     *
     * @param pt The point to transform.
     * @return PointOf<T> The transformed point.
     */
    constexpr PointOf<T> transform(PointOf<T> pt) const {
        // Formula: pt.x * a + pt.y * c + e, pt.x * b + pt.y * d + f;
        SIMD<T, 4> tmp = pt.v.shuffle(size_constants<0, 0, 1, 1>{}) * flatten().template firstn<4>();
        PointOf<T> result(tmp.low() + tmp.high() + flatten().template lastn<2>());
        return result;
    }

    /**
     * @brief Transforms a collection of points using the matrix.
     *
     * Applies a 2D transformation to a span of points in an optimized SIMD
     * approach, processing multiple points in parallel when possible.
     *
     * @param points The span of points to transform.
     */
    constexpr void transform(std::span<PointOf<T>> points) const {
        constexpr size_t N  = 8;
        constexpr size_t N2 = N * 2;
        size_t i            = 0;
        SIMD<T, N2> ad      = repeat<N>(SIMD{ a, d });
        SIMD<T, N2> cb      = repeat<N>(SIMD{ c, b });
        SIMD<T, N2> ef      = repeat<N>(SIMD{ e, f });
        for (; i + N - 1 < points.size(); i += N) {
            SIMD<T, N2> xy = *reinterpret_cast<const SIMD<T, N2>*>(points.data() + i);
            xy             = xy * ad + swapAdjacent(xy) * cb + ef;
            *reinterpret_cast<SIMD<T, N2>*>(points.data() + i) = xy;
        }
        for (; i < points.size(); ++i) {
            points[i] = transform(points[i]);
        }
    }

    /**
     * @brief Inverts the matrix, if possible.
     *
     * @return std::optional<Matrix2DOf> The inverse of the matrix if it is invertible, or `std::nullopt` if
     * the matrix is singular (non-invertible).
     */
    std::optional<Matrix2DOf> invert() const {
        T det = a * d - b * c; // determinant

        if (det < std::numeric_limits<T>::epsilon()) {
            return std::nullopt; // Matrix is not invertible
        }

        Matrix2DOf result;
        // Calculate the inverse of the matrix
        result.a = d / det;
        result.b = -b / det;
        result.c = -c / det;
        result.d = a / det;
        result.e = (c * f - d * e) / det;
        result.f = (b * e - a * f) / det;
        return result;
    }
};

using Matrix2D = Matrix2DOf<float>;
} // namespace Brisk
