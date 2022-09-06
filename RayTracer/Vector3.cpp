#include <cmath>
#include <stdio.h>

struct Vector3 {
	double x;
	double y;
	double z;

	Vector3() = default;

	Vector3(double x, double y, double z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Vector3(const Vector3& copy) {
		this->x = copy.x;
		this->y = copy.y;
		this->z = copy.z;
	}

	Vector3& operator +=(const Vector3& r) {
		this->x += r.x;
		this->y += r.y;
		this->z += r.z;

		return *this;
	}

	Vector3& operator -=(const Vector3& r) {
		this->x -= r.x;
		this->y -= r.y;
		this->z -= r.z;

		return *this;
	}

	Vector3& operator *=(double r) {
		this->x *= r;
		this->y *= r;
		this->z *= r;

		return *this;
	}

	Vector3& operator /=(double r) {
		this->x /= r;
		this->y /= r;
		this->z /= r;

		return *this;
	}

	double getIndex(int index) const {
		switch (index) {
		case 0:
			return this->x;
		case 1:
			return this->y;
		case 2:
			return this->z;
		default:
			printf("getIndex ERROR! Attempted out-of-bounds Vector3 access with index: %d\n", index);
			return 0.0;
		}
	}

	Vector3& setIndex(int index, double value) {
		switch (index) {
		case 0:
			this->x = value;
			break;
		case 1:
			this->y = value;
			break;
		case 2:
			this->z = value;
			break;
		default:
			printf("setIndex ERROR! Attempted out-of-bounds Vector3 access with index: %d\n", index);
		}
		return *this;
	}

	Vector3 MatrixMultiply(double m[3][3]) const {
		Vector3 result = Vector3(0, 0, 0);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				double newVal = result.getIndex(i) + this->getIndex(j) * m[i][j];
				result.setIndex(i, newVal);
			}
		}

		return result;
	}

	double Dot(const Vector3& r) const {
		return this->x * r.x + this->y * r.y + this->z * r.z;
	}

	double Magnitude() const {
		return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	}

	// Could be improved by using reciprocal sqrt with scalar multiplication
	Vector3 Normalized() {
		double m = this->Magnitude();
		return Vector3(this->x / m, this->y / m, this->z / m);
	}
};


// Negation
inline Vector3 operator -(const Vector3& r) {
	return Vector3(-r.x, -r.y, -r.z);
}

inline Vector3 operator +(const Vector3& l, const Vector3& r) {
	return Vector3(l.x + r.x, l.y + r.y, l.z + r.z);
}

inline Vector3 operator -(const Vector3& l, const Vector3& r) {
	return Vector3(l.x - r.x, l.y - r.y, l.z - r.z);
}

inline Vector3 operator *(double l, const Vector3& r) {
	return Vector3(l * r.x, l * r.y, l * r.z);
}

inline Vector3 operator *(const Vector3& l, double r) {
	return Vector3(l.x * r, l.y * r, l.z * r);
}

inline Vector3 operator /(const Vector3& l, double r)  {
	return Vector3(l.x / r, l.y / r, l.z / r);
}