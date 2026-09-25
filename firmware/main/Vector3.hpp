#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <cmath>

/**
 * A three-dimensional vector.
 */
class Vector3
{
public:
	float x, y, z;

	/**
	 * Constructs a zero vector.
	 */
	constexpr Vector3() noexcept
		: x(0.0F), y(0.0F), z(0.0F) {}

	/**
	 * Constructs a vector with the given values.
	 *
	 * @param x The value of the x element.
	 * @param y The value of the y element.
	 * @param z The value of the z element.
	 */
	constexpr Vector3(const float x, const float y, const float z) noexcept
		: x(x), y(y), z(z) {}

	/**
	 * Constructs a vector with the elements of another vector.
	 *
	 * @param vector The vector from which to copy the elements.
	 */
	constexpr Vector3(const Vector3 &vector) noexcept
		: x(vector.x), y(vector.y), z(vector.z) {}

	/**
	 * Copies the elements of another vector into this vector.
	 *
	 * @param vector The vector from which to copy the elements.
	 * @return A reference to this vector.
	 */
	constexpr Vector3 &operator=(const Vector3 &vector) noexcept {
		this->x = vector.x;
		this->y = vector.y;
		this->z = vector.z;
		return *this;
	}

	/**
	 * Adds another vector to this vector elementwise.
	 *
	 * @param vector The vector to add to this vector.
	 * @return The resulting vector from adding another vector to this vector.
	 */
	constexpr Vector3 operator+(const Vector3 &vector) const noexcept {
		return Vector3(
			this->x + vector.x,
			this->y + vector.y,
			this->z + vector.z
		);
	}

	/**
	 * Adds another vector to this vector elementwise, the result is stored in this vector.
	 *
	 * @param vector The vector to add to this vector.
	 * @return A reference to this vector.
	 */
	constexpr Vector3 &operator+=(const Vector3 &vector) noexcept {
		this->x += vector.x;
		this->y += vector.y;
		this->z += vector.z;
		return *this;
	}

	/**
	 * Subtracts another vector from this vector elementwise.
	 *
	 * @param vector The vector to subtract from this vector.
	 * @return The resulting vector from subtracting another vector from this vector.
	 */
	constexpr Vector3 operator-(const Vector3 &vector) const noexcept {
		return Vector3(
			this->x - vector.x,
			this->y - vector.y,
			this->z - vector.z
		);
	}

	/**
	 * Subtracts another vector from this vector elementwise, the result is stored in this vector.
	 *
	 * @param vector The vector to subtract from this vector.
	 * @return A reference to this vector.
	 */
	constexpr Vector3 &operator-=(const Vector3 &vector) noexcept {
		this->x -= vector.x;
		this->y -= vector.y;
		this->z -= vector.z;
		return *this;
	}

	/**
	 * Multiplies a scalar with all the elements of this vector.
	 *
	 * @param scalar The scalar to multiply with all the elements.
	 * @return The resulting vector from multiplying a scalar with all elements of this vector.
	 */
	constexpr Vector3 operator*(const float scalar) const noexcept {
		return Vector3(
			this->x * scalar,
			this->y * scalar,
			this->z * scalar
		);
	}

	/**
	 * Multiplies a scalar with all the elements of this vector, the result is stored in this vector.
	 *
	 * @param scalar The scalar to multiply with all the elements.
	 * @return A reference to this vector.
	 */
	constexpr Vector3 &operator*=(const float scalar) noexcept {
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		return *this;
	}

	/**
	 * Compares whether another vector is equal to this vector.
	 *
	 * @param vector The vector to compare with this vector.
	 * @return true, if the other vector is equal to this vector. false, otherwise.
	 */
	constexpr bool operator==(const Vector3 &vector) const noexcept {
		return (this->x == vector.x) && (this->y == vector.y) && (this->z == vector.z);
	}

	/**
	 * Compares whether another vector is not equal to this vector.
	 *
	 * @param vector The vector to compare with this vector.
	 * @return true, if the other vector is not equal to this vector. false, otherwise.
	 */
	constexpr bool operator!=(const Vector3 &vector) const noexcept {
		return (this->x != vector.x) || (this->y != vector.y) || (this->z != vector.z);
	}

	/**
	 * Calculates the dot product between this vector and another vector.
	 *
	 * @param vector The vector to calculate the dot product with.
	 * @return The dot product between this vector and another vector.
	 */
	constexpr float dotProduct(const Vector3 &vector) const noexcept {
		return this->x * vector.x + this->y * vector.y + this->z * vector.z;
	}

	/**
	 * Calculates the cross product between this vector and another vector.
	 *
	 * @param vector The vector to calculate the cross product with.
	 * @return The cross product between this vector and another vector.
	 */
	constexpr Vector3 crossProduct(const Vector3 &vector) const noexcept {
		return Vector3(
			this->y * vector.z - this->z * vector.y,
			this->z * vector.x - this->x * vector.z,
			this->x * vector.y - this->y * vector.x
		);
	}

	/**
	 * Calculates the magnitude/length of this vector.
	 *
	 * @return The magnitude/length of this vector.
	 */
	inline float magnitude() const noexcept {
		return sqrtf(this->dotProduct(*this));
	}

	/**
	 * Normalizes this vector.
	 *
	 * @return This vector normalized.
	 */
	inline Vector3 normalize() const noexcept {
		const float scalar = 1.0F / this->magnitude();
		return (*this * scalar);
	}

	/**
	 * Negates this vector.
	 *
	 * @return This vector negated.
	 */
	constexpr Vector3 negate() const noexcept {
		return Vector3(
			-this->x,
			-this->y,
			-this->z
		);
	}
};

#endif // VECTOR3_HPP
