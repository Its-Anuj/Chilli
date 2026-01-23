#pragma once

#include <cstddef> // for size_t

namespace Chilli
{

	// Forward declarations for all vector types
	struct IVec2;
	struct DVec2;
	struct Vec2;

	struct IVec3;
	struct DVec3;
	struct Vec3;

	struct IVec4;
	struct DVec4;
	struct Vec4;

#pragma region Vec2
	struct Vec2
	{
		float x, y;

		Vec2(float px = 0.0f, float py = 0.0f) : x(px), y(py) {}
		Vec2(float v) : x(v), y(v) {}

		// Casting constructors
		explicit Vec2(const IVec2& v);
		explicit Vec2(const DVec2& v);

		// Unary operators
		Vec2 operator-() const { return Vec2(-x, -y); }
		Vec2 operator+() const { return *this; }

		Vec2& operator=(const Vec2& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}

		// Arithmetic assignment operators
		Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
		Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
		Vec2& operator*=(const Vec2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
		Vec2& operator/=(const Vec2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }

		// Comparison operators
		bool operator==(const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }
		bool operator!=(const Vec2& rhs) const { return !(*this == rhs); }
		bool operator<(const Vec2& rhs) const { return (x < rhs.x) || (x == rhs.x && y < rhs.y); }
		bool operator>(const Vec2& rhs) const { return (x > rhs.x) || (x == rhs.x && y > rhs.y); }

		// Arithmetic operators
		Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
		Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
		Vec2 operator/(const Vec2& other) const { return Vec2(x / other.x, y / other.y); }
		Vec2 operator*(const Vec2& other) const { return Vec2(x * other.x, y * other.y); }

		// Conversion operators
		explicit operator IVec2() const;
		explicit operator DVec2() const;
	};

	struct DVec2
	{
		double x, y;

		DVec2(double px = 0.0, double py = 0.0) : x(px), y(py) {}

		// Casting constructors
		explicit DVec2(const Vec2& v);
		explicit DVec2(const IVec2& v);

		// Unary operators
		DVec2 operator-() const { return DVec2(-x, -y); }
		DVec2 operator+() const { return *this; }

		DVec2& operator=(const DVec2& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}

		DVec2& operator+=(const DVec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
		DVec2& operator-=(const DVec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
		DVec2& operator*=(const DVec2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
		DVec2& operator/=(const DVec2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }

		bool operator==(const DVec2& rhs) const { return x == rhs.x && y == rhs.y; }
		bool operator!=(const DVec2& rhs) const { return !(*this == rhs); }
		bool operator<(const DVec2& rhs) const { return (x < rhs.x) || (x == rhs.x && y < rhs.y); }
		bool operator>(const DVec2& rhs) const { return (x > rhs.x) || (x == rhs.x && y > rhs.y); }

		// Conversion operators
		explicit operator Vec2() const;
		explicit operator IVec2() const;
	};

	struct IVec2
	{
		int x, y;

		IVec2(int px = 0, int py = 0) : x(px), y(py) {}

		// Casting constructors
		explicit IVec2(const Vec2& v);
		explicit IVec2(const DVec2& v);

		// Unary operators
		IVec2 operator-() const { return IVec2(-x, -y); }
		IVec2 operator+() const { return *this; }

		IVec2& operator=(const IVec2& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}

		IVec2& operator+=(const IVec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
		IVec2& operator-=(const IVec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
		IVec2& operator*=(const IVec2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
		IVec2& operator/=(const IVec2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }

		bool operator==(const IVec2& rhs) const { return x == rhs.x && y == rhs.y; }
		bool operator!=(const IVec2& rhs) const { return !(*this == rhs); }
		bool operator<(const IVec2& rhs) const { return (x < rhs.x) || (x == rhs.x && y < rhs.y); }
		bool operator>(const IVec2& rhs) const { return (x > rhs.x) || (x == rhs.x && y > rhs.y); }

		// Conversion operators
		explicit operator Vec2() const;
		explicit operator DVec2() const;
	};
#pragma endregion

#pragma region Vec3
	struct Vec3
	{
		float x, y, z;

		Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
		Vec3(float p = 0.0f) : x(p), y(p), z(p) {}

		// Casting constructors
		explicit Vec3(const IVec3& v);
		explicit Vec3(const DVec3& v);

		// Unary operators
		Vec3 operator-() const { return Vec3(-x, -y, -z); }
		Vec3 operator+() const { return *this; }

		Vec3& operator=(const Vec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
		Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
		Vec3& operator*=(const Vec3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
		Vec3& operator/=(const Vec3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

		bool operator==(const Vec3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
		bool operator!=(const Vec3& rhs) const { return !(*this == rhs); }
		bool operator<(const Vec3& rhs) const { return (x < rhs.x) || (x == rhs.x && (y < rhs.y || (y == rhs.y && z < rhs.z))); }
		bool operator>(const Vec3& rhs) const { return (x > rhs.x) || (x == rhs.x && (y > rhs.y || (y == rhs.y && z > rhs.z))); }

		Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
		Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
		Vec3 operator/(const Vec3& other) const { return Vec3(x / other.x, y / other.y, z / other.z); }
		Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }

		Vec3 operator+(float other) const { return Vec3(x + other, y + other, z + other); }
		Vec3 operator-(float other) const { return Vec3(x - other, y - other, z - other); }
		Vec3 operator/(float other) const { return Vec3(x / other, y / other, z / other); }
		Vec3 operator*(float other) const { return Vec3(x * other, y * other, z * other); }

		// Conversion operators
		explicit operator IVec3() const;
		explicit operator DVec3() const;
	};

	struct DVec3
	{
		double x, y, z;

		DVec3(double px = 0.0, double py = 0.0, double pz = 0.0) : x(px), y(py), z(pz) {}
		DVec3(double p) : x(p), y(p), z(p) {}

		// Casting constructors
		explicit DVec3(const Vec3& v);
		explicit DVec3(const IVec3& v);

		// Unary operators
		DVec3 operator-() const { return DVec3(-x, -y, -z); }
		DVec3 operator+() const { return *this; }

		DVec3& operator=(const DVec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		DVec3& operator+=(const DVec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
		DVec3& operator-=(const DVec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
		DVec3& operator*=(const DVec3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
		DVec3& operator/=(const DVec3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

		bool operator==(const DVec3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
		bool operator!=(const DVec3& rhs) const { return !(*this == rhs); }
		bool operator<(const DVec3& rhs) const { return (x < rhs.x) || (x == rhs.x && (y < rhs.y || (y == rhs.y && z < rhs.z))); }
		bool operator>(const DVec3& rhs) const { return (x > rhs.x) || (x == rhs.x && (y > rhs.y || (y == rhs.y && z > rhs.z))); }

		// Conversion operators
		explicit operator Vec3() const;
		explicit operator IVec3() const;
	};

	struct IVec3
	{
		int x, y, z;

		IVec3(int px = 0, int py = 0, int pz = 0) : x(px), y(py), z(pz) {}

		// Casting constructors
		explicit IVec3(const Vec3& v);
		explicit IVec3(const DVec3& v);

		// Unary operators
		IVec3 operator-() const { return IVec3(-x, -y, -z); }
		IVec3 operator+() const { return *this; }

		IVec3& operator=(const IVec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		IVec3& operator+=(const IVec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
		IVec3& operator-=(const IVec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
		IVec3& operator*=(const IVec3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
		IVec3& operator/=(const IVec3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

		bool operator==(const IVec3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
		bool operator!=(const IVec3& rhs) const { return !(*this == rhs); }
		bool operator<(const IVec3& rhs) const { return (x < rhs.x) || (x == rhs.x && (y < rhs.y || (y == rhs.y && z < rhs.z))); }
		bool operator>(const IVec3& rhs) const { return (x > rhs.x) || (x == rhs.x && (y > rhs.y || (y == rhs.y && z > rhs.z))); }

		// Conversion operators
		explicit operator Vec3() const;
		explicit operator DVec3() const;
	};
#pragma endregion

#pragma region Vec4
	struct Vec4
	{
		float x, y, z, w;

		Vec4(float px = 0.0f, float py = 0.0f, float pz = 0.0f, float pw = 0.0f)
			: x(px), y(py), z(pz), w(pw) {
		}
		Vec4(float p) : x(p), y(p), z(p), w(p) {}

		// Casting constructors
		explicit Vec4(const IVec4& v);
		explicit Vec4(const DVec4& v);

		// Unary operators
		Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
		Vec4 operator+() const { return *this; }

		Vec4& operator=(const Vec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		Vec4& operator+=(const Vec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
		Vec4& operator-=(const Vec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
		Vec4& operator*=(const Vec4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
		Vec4& operator/=(const Vec4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

		bool operator==(const Vec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
		bool operator!=(const Vec4& rhs) const { return !(*this == rhs); }
		bool operator<(const Vec4& rhs) const
		{
			if (x < rhs.x) return true;
			if (x > rhs.x) return false;
			if (y < rhs.y) return true;
			if (y > rhs.y) return false;
			if (z < rhs.z) return true;
			if (z > rhs.z) return false;
			return w < rhs.w;
		}
		bool operator>(const Vec4& rhs) const
		{
			if (x > rhs.x) return true;
			if (x < rhs.x) return false;
			if (y > rhs.y) return true;
			if (y < rhs.y) return false;
			if (z > rhs.z) return true;
			if (z < rhs.z) return false;
			return w > rhs.w;
		}

		Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		Vec4 operator+(float other) const { return Vec4(x + other, y + other, z + other, w + other); }
		Vec4 operator-(float other) const { return Vec4(x - other, y - other, z - other, w - other); }
		Vec4 operator/(float other) const { return Vec4(x / other, y / other, z / other, w / other); }
		Vec4 operator*(float other) const { return Vec4(x * other, y * other, z * other, w * other); }

		// Conversion operators
		explicit operator IVec4() const;
		explicit operator DVec4() const;
	};
	// ... (existing code above remains unchanged)

	struct DVec4
	{
		double x, y, z, w;

		DVec4(double px = 0.0, double py = 0.0, double pz = 0.0, double pw = 0.0)
			: x(px), y(py), z(pz), w(pw) {
		}
		DVec4(double p) : x(p), y(p), z(p), w(p) {}

		// Casting constructors
		explicit DVec4(const Vec4& v);
		explicit DVec4(const IVec4& v);

		// Unary operators
		DVec4 operator-() const { return DVec4(-x, -y, -z, -w); }
		DVec4 operator+() const { return *this; }

		DVec4& operator=(const DVec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		DVec4& operator+=(const DVec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
		DVec4& operator-=(const DVec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
		DVec4& operator*=(const DVec4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
		DVec4& operator/=(const DVec4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

		bool operator==(const DVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
		bool operator!=(const DVec4& rhs) const { return !(*this == rhs); }
		bool operator<(const DVec4& rhs) const
		{
			if (x < rhs.x) return true;
			if (x > rhs.x) return false;
			if (y < rhs.y) return true;
			if (y > rhs.y) return false;
			if (z < rhs.z) return true;
			if (z > rhs.z) return false;
			return w < rhs.w;
		}
		bool operator>(const DVec4& rhs) const
		{
			if (x > rhs.x) return true;
			if (x < rhs.x) return false;
			if (y > rhs.y) return true;
			if (y < rhs.y) return false;
			if (z > rhs.z) return true;
			if (z < rhs.z) return false;
			return w > rhs.w;
		}

		DVec4 operator+(const DVec4& other) const { return DVec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		DVec4 operator-(const DVec4& other) const { return DVec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		DVec4 operator/(const DVec4& other) const { return DVec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		DVec4 operator*(const DVec4& other) const { return DVec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		DVec4 operator+(double other) const { return DVec4(x + other, y + other, z + other, w + other); }
		DVec4 operator-(double other) const { return DVec4(x - other, y - other, z - other, w - other); }
		DVec4 operator/(double other) const { return DVec4(x / other, y / other, z / other, w / other); }
		DVec4 operator*(double other) const { return DVec4(x * other, y * other, z * other, w * other); }

		// Conversion operators
		explicit operator Vec4() const;
		explicit operator IVec4() const;
	};

	// ... (rest of the file unchanged)
	struct IVec4
	{
		int x, y, z, w;

		IVec4(int px = 0, int py = 0, int pz = 0, int pw = 0)
			: x(px), y(py), z(pz), w(pw) {
		}
		IVec4(float p) : x(p), y(p), z(p), w(p) {}

		// Casting constructors
		explicit IVec4(const Vec4& v);
		explicit IVec4(const DVec4& v);

		// Unary operators
		IVec4 operator-() const { return IVec4(-x, -y, -z, -w); }
		IVec4 operator+() const { return *this; }

		IVec4& operator=(const IVec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		IVec4& operator+=(const IVec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
		IVec4& operator-=(const IVec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
		IVec4& operator*=(const IVec4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
		IVec4& operator/=(const IVec4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }

		bool operator==(const IVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
		bool operator!=(const IVec4& rhs) const { return !(*this == rhs); }
		bool operator<(const IVec4& rhs) const
		{
			if (x < rhs.x) return true;
			if (x > rhs.x) return false;
			if (y < rhs.y) return true;
			if (y > rhs.y) return false;
			if (z < rhs.z) return true;
			if (z > rhs.z) return false;
			return w < rhs.w;
		}
		bool operator>(const IVec4& rhs) const
		{
			if (x > rhs.x) return true;
			if (x < rhs.x) return false;
			if (y > rhs.y) return true;
			if (y < rhs.y) return false;
			if (z > rhs.z) return true;
			if (z < rhs.z) return false;
			return w > rhs.w;
		}

		IVec4 operator+(const IVec4& other) const { return IVec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		IVec4 operator-(const IVec4& other) const { return IVec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		IVec4 operator/(const IVec4& other) const { return IVec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		IVec4 operator*(const IVec4& other) const { return IVec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		IVec4 operator+(int other) const { return IVec4(x + other, y + other, z + other, w + other); }
		IVec4 operator-(int other) const { return IVec4(x - other, y - other, z - other, w - other); }
		IVec4 operator/(int other) const { return IVec4(x / other, y / other, z / other, w / other); }
		IVec4 operator*(int other) const { return IVec4(x * other, y * other, z * other, w * other); }

		// Conversion operators
		explicit operator Vec4() const;
		explicit operator DVec4() const;
	};
#pragma endregion

	// --- Implementations for casting constructors and conversion operators ---

	// Vec2
	inline Vec2::Vec2(const IVec2& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}
	inline Vec2::Vec2(const DVec2& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}
	inline Vec2::operator IVec2() const { return IVec2(static_cast<int>(x), static_cast<int>(y)); }
	inline Vec2::operator DVec2() const { return DVec2(static_cast<double>(x), static_cast<double>(y)); }

	// DVec2
	inline DVec2::DVec2(const Vec2& v) : x(static_cast<double>(v.x)), y(static_cast<double>(v.y)) {}
	inline DVec2::DVec2(const IVec2& v) : x(static_cast<double>(v.x)), y(static_cast<double>(v.y)) {}
	inline DVec2::operator Vec2() const { return Vec2(static_cast<float>(x), static_cast<float>(y)); }
	inline DVec2::operator IVec2() const { return IVec2(static_cast<int>(x), static_cast<int>(y)); }

	// IVec2
	inline IVec2::IVec2(const Vec2& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)) {}
	inline IVec2::IVec2(const DVec2& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)) {}
	inline IVec2::operator Vec2() const { return Vec2(static_cast<float>(x), static_cast<float>(y)); }
	inline IVec2::operator DVec2() const { return DVec2(static_cast<double>(x), static_cast<double>(y)); }

	// Vec3
	inline Vec3::Vec3(const IVec3& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)) {}
	inline Vec3::Vec3(const DVec3& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)) {}
	inline Vec3::operator IVec3() const { return IVec3(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)); }
	inline Vec3::operator DVec3() const { return DVec3(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)); }

	// DVec3
	inline DVec3::DVec3(const Vec3& v) : x(static_cast<double>(v.x)), y(static_cast<double>(v.y)), z(static_cast<double>(v.z)) {}
	inline DVec3::DVec3(const IVec3& v) : x(static_cast<double>(v.x)), y(static_cast<double>(v.y)), z(static_cast<double>(v.z)) {}
	inline DVec3::operator Vec3() const { return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }
	inline DVec3::operator IVec3() const { return IVec3(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)); }

	// IVec3
	inline IVec3::IVec3(const Vec3& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)), z(static_cast<int>(v.z)) {}
	inline IVec3::IVec3(const DVec3& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)), z(static_cast<int>(v.z)) {}
	inline IVec3::operator Vec3() const { return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)); }
	inline IVec3::operator DVec3() const { return DVec3(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)); }

	// Vec4
	inline Vec4::Vec4(const IVec4& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)), w(static_cast<float>(v.w)) {}
	inline Vec4::Vec4(const DVec4& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)), w(static_cast<float>(v.w)) {}
	inline Vec4::operator IVec4() const { return IVec4(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z), static_cast<int>(w)); }
	inline Vec4::operator DVec4() const { return DVec4(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)); }

	// IVec4
	inline IVec4::IVec4(const Vec4& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)), z(static_cast<int>(v.z)), w(static_cast<int>(v.w)) {}
	inline IVec4::IVec4(const DVec4& v) : x(static_cast<int>(v.x)), y(static_cast<int>(v.y)), z(static_cast<int>(v.z)), w(static_cast<int>(v.w)) {}
	inline IVec4::operator Vec4() const { return Vec4(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w)); }
	inline IVec4::operator DVec4() const { return DVec4(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)); }

#pragma endregion

	struct AABB
	{
		Vec3 Min, Max;

		AABB() = default;
		AABB(const Vec3& min, const Vec3& max) : Min(min), Max(max) {}

		bool IsColliding(const AABB& Other) const
		{
			return (Min.x < Other.Max.x && Max.x > Other.Min.x
				&& (Min.y < Other.Max.y && Max.y > Other.Min.y)
				&& (Min.z < Other.Max.z && Max.z > Other.Min.z));
		}

		bool Contains(const Vec3& point) const
		{
			return (point.x >= Min.x && point.x <= Max.x) &&
				(point.y >= Min.y && point.y <= Max.y) &&
				(point.z >= Min.z && point.z <= Max.z);
		}

		const Vec3& GetCenter() const { return (Max + Min) / 2.0f; }
	};

	// Binary search function
	// T: type of array elements (must be comparable with < and ==)
	template <typename T>
	int BinarySearch(const T* arr, size_t length, const T& value)
	{
		if (!arr || length == 0)
			return -1; // invalid state

		size_t left = 0;
		size_t right = length - 1;

		while (left <= right) {
			size_t mid = left + (right - left) / 2;

			if (arr[mid] == value) {
				return static_cast<int>(mid); // found
			}
			else if (arr[mid] < value) {
				left = mid + 1;
			}
			else {
				if (mid == 0) break; // prevent underflow
				right = mid - 1;
			}
		}

		return -1; // not found
	}

}
