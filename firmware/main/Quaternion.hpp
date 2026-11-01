#ifndef QUATERNION_HPP
#define QUATERNION_HPP

#include <cmath>
#include "Vector3.hpp"

/**
 * A quaternion.
 */
class Quaternion
{
public:
	float a, b, c, d;

	/**
	 * Constructs a zero quaternion.
	 */
	constexpr Quaternion() noexcept
		: a(0.0F), b(0.0F), c(0.0F), d(0.0F) {}

	/**
	 * Constructs a quaternion with only the scalar part.
	 *
	 * @param scalar The scalar (a) element of the quaternion.
	 */
	constexpr Quaternion(const float scalar) noexcept
		: a(scalar), b(0.0F), c(0.0F), d(0.0F) {}

	/**
	 * Constructs a quaternion with only the vector part.
	 *
	 * @param vector The vector (b, c, d) elements of the quaternion.
	 */
	constexpr Quaternion(const Vector3 &vector) noexcept
		: a(0.0F), b(vector.x), c(vector.y), d(vector.z) {}

	/**
	 * Constructs a quaternion from the scalar and vector parts.
	 *
	 * @param scalar The scalar (a) element of the quaternion.
	 * @param vector The vector (b, c, d) elements of the quaternion.
	 */
	constexpr Quaternion(const float scalar, const Vector3 &vector) noexcept
		: a(scalar), b(vector.x), c(vector.y), d(vector.z) {}

	/**
	 * Constructs a quaternion with the given values.
	 *
	 * @param a The value of the a element.
	 * @param b The value of the b element.
	 * @param c The value of the c element.
	 * @param d The value of the d element.
	 */
	constexpr Quaternion(const float a, const float b, const float c, const float d) noexcept
		: a(a), b(b), c(c), d(d) {}

	/**
	 * Constructs a quaternion with the elements from another quaternion.
	 *
	 * @param quaternion The quaternion from which to copy the elements.
	 */
	constexpr Quaternion(const Quaternion &quaternion) noexcept
		: a(quaternion.a), b(quaternion.b), c(quaternion.c), d(quaternion.d) {}

	/**
	 * Copies the elements of another quaternion into this quaternion.
	 *
	 * @param quaternion The quaternion from which to copy the elements.
	 * @return A reference to this quaternion.
	 */
	constexpr Quaternion &operator=(const Quaternion &quaternion) noexcept {
		this->a = quaternion.a;
		this->b = quaternion.b;
		this->c = quaternion.c;
		this->d = quaternion.d;
		return *this;
	}

	/**
	 * Adds another quaternion to this quaternion elementwise.
	 *
	 * @param quaternion The quaternion to add to this quaternion.
	 * @return The resulting quaternion from adding another quaternion to this quaternion.
	 */
	[[nodiscard]] constexpr Quaternion operator+(const Quaternion &quaternion) const noexcept {
		return Quaternion(
			this->a + quaternion.a,
			this->b + quaternion.b,
			this->c + quaternion.c,
			this->d + quaternion.d
		);
	}

	/**
	 * Adds another quaternion to this quaternion elementwise, the result is stored in this quaternion.
	 *
	 * @param quaternion The quaternion to add to this quaternion.
	 * @return A reference to this quaternion.
	 */
	constexpr Quaternion &operator+=(const Quaternion &quaternion) noexcept {
		this->a += quaternion.a;
		this->b += quaternion.b;
		this->c += quaternion.c;
		this->d += quaternion.d;
		return *this;
	}

	/**
	 * Subtracts another quaternion from this quaternion elementwise.
	 *
	 * @param quaternion The quaternion to subtract from this quaternion.
	 * @return The resulting quaternion from subtracting another quaternion from this quaternion.
	 */
	[[nodiscard]] constexpr Quaternion operator-(const Quaternion &quaternion) const noexcept {
		return Quaternion(
			this->a - quaternion.a,
			this->b - quaternion.b,
			this->c - quaternion.c,
			this->d - quaternion.d
		);
	}

	/**
	 * Subtracts another quaternion from this quaternion elementwise, the result is stored in this quaternion.
	 *
	 * @param quaternion The quaternion to subtract from this quaternion.
	 * @return A reference to this quaternion.
	 */
	constexpr Quaternion &operator-=(const Quaternion &quaternion) noexcept {
		this->a -= quaternion.a;
		this->b -= quaternion.b;
		this->c -= quaternion.c;
		this->d -= quaternion.d;
		return *this;
	}

	/**
	 * Caculates the Hamilton product of this quaternion and another quaternion.
	 *
	 * @param quaternion The quaternion to calculate the Hamilton product with.
	 * @return The resulting quaternion from the Hamilton product of this quaternion and another quaternion.
	 */
	[[nodiscard]] constexpr Quaternion operator*(const Quaternion &quaternion) const noexcept {
		return Quaternion(
			(this->a * quaternion.a) - (this->b * quaternion.b) - (this->c * quaternion.c) - (this->d * quaternion.d),
			(this->a * quaternion.b) + (this->b * quaternion.a) + (this->c * quaternion.d) - (this->d * quaternion.c),
			(this->a * quaternion.c) - (this->b * quaternion.d) + (this->c * quaternion.a) + (this->d * quaternion.b),
			(this->a * quaternion.d) + (this->b * quaternion.c) - (this->c * quaternion.b) + (this->d * quaternion.a)
		);
	}

	/**
	 * Calculates the Hamilton product of this quaternion and another quaternion, the result is stored in this quaternion.
	 *
	 * @param quaternion The quaternion to calculate the Hamilton product with.
	 * @return A reference to this quaternion.
	 */
	constexpr Quaternion &operator*=(const Quaternion &quaternion) noexcept {
		return (*this = (*this * quaternion));
	}

	/**
	 * Multiplies a scalar with all the elements in this quaternion.
	 *
	 * @param scalar The scalar to multiply with all the elements.
	 * @return The resulting quaternion from multiplying a scalar with all the elements of this quaternion.
	 */
	[[nodiscard]] constexpr Quaternion operator*(const float scalar) const noexcept {
		return Quaternion(
			this->a * scalar,
			this->b * scalar,
			this->c * scalar,
			this->d * scalar
		);
	}

	/**
	 * Multiplies a scalar with all the elements in this quaternion, the result is stored in this quaternion.
	 *
	 * @param scalar The scalar to multiply with all the elements.
	 * @return A reference to this quaternion.
	 */
	constexpr Quaternion &operator*=(const float scalar) noexcept {
		this->a *= scalar;
		this->b *= scalar;
		this->c *= scalar;
		this->d *= scalar;
		return *this;
	}

	/**
	 * Compares whether another quaternion is equal to this quaternion.
	 *
	 * @param quaternion The quaternion to compare with this quaternion.
	 * @return true, if the other quaternion is equal to this quaternion. false, otherwise.
	 */
	[[nodiscard]] constexpr bool operator==(const Quaternion &quaternion) const noexcept {
		return (this->a == quaternion.a) && (this->b == quaternion.b) && (this->c == quaternion.c) && (this->d == quaternion.d);
	}

	/**
	 * Compares whether another quaternion is not equal to this quaternion.
	 *
	 * @param quaternion The quaternion to compare with this quaternion.
	 * @return true, if the other quaternion is not equal to this quaternion. false, otherwise.
	 */
	[[nodiscard]] constexpr bool operator!=(const Quaternion &quaternion) const noexcept {
		return (this->a != quaternion.a) || (this->b != quaternion.b) || (this->c != quaternion.c) || (this->d != quaternion.d);
	}

	/**
	 * Returns the scalar part of the quaternion.
	 *
	 * @return The scalar part of the quaternion.
	 */
	[[nodiscard]] constexpr float scalar() const noexcept {
		return this->a;
	}

	/**
	 * Returns the vector part of the quaternion.
	 *
	 * @return The vector part of the quaternion.
	 */
	[[nodiscard]] constexpr Vector3 vector() const noexcept {
		return Vector3(this->b, this->c, this->d);
	}

	/**
	 * Returns the conjugate of this quaternion.
	 *
	 * @return The conjugate of this quaternion.
	 */
	[[nodiscard]] constexpr Quaternion conjugate() const noexcept {
		return Quaternion(
			this->a,
			-this->b,
			-this->c,
			-this->d
		);
	}

	/**
	 * Calculates the magnitude/length of this quaternion.
	 *
	 * @return The magnitude/length of this quaternion.
	 */
	[[nodiscard]] inline float magnitude() const noexcept {
		return sqrtf(this->a * this->a + this->b * this->b + this->c * this->c + this->d * this->d);
	}

	/**
	 * Normalizes this quaternion.
	 *
	 * @return This quaternion normalized.
	 */
	[[nodiscard]] inline Quaternion normalize() const noexcept {
		const float scalar = 1.0F / this->magnitude();
		return (*this * scalar);
	}

	/**
	 * Rotates a vector.
	 *
	 * @param vector the vector to rotate.
	 * @return The rotated vector.
	 */
	[[nodiscard]] constexpr Vector3 rotateVector(const Vector3 &vector) const noexcept {
		const Quaternion vectorQuaternion = Quaternion(vector);
		return (*this * vectorQuaternion * this->conjugate()).vector();
	}
};

#endif // QUATERNION_HPP
