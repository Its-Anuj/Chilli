#pragma once

namespace Chilli
{
#pragma region Vec 2
	struct Vec2
	{
		float x, y;

		Vec2(float px = 0.0f, float py = 0.0f) : x(px), y(py) {}

		Vec2& operator=(const Vec2& other)
		{
			this->x = other.x;
			this->y = other.y;
			return *this;
		}

		// Arithmetic assignment operators
		Vec2& operator+=(const Vec2& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		Vec2& operator-=(const Vec2& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		Vec2& operator*=(const Vec2& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		Vec2& operator/=(const Vec2& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}

		// Comparison operators
		bool operator==(const Vec2& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}
		bool operator!=(const Vec2& rhs) const
		{
			return !(*this == rhs);
		}
		bool operator<(const Vec2& rhs) const
		{
			return (x < rhs.x) || (x == rhs.x && y < rhs.y);
		}
		bool operator>(const Vec2& rhs) const
		{
			return (x > rhs.x) || (x == rhs.x && y > rhs.y);
		}

		// Vec2 operators
		inline Vec2 operator+(const Vec2& other) { return Vec2(x + other.x, y + other.y); }
		inline Vec2 operator-(const Vec2& other) { return Vec2(x - other.x, y - other.y); }
		inline Vec2 operator/(const Vec2& other) { return Vec2(x / other.x, y / other.y); }
		inline Vec2 operator*(const Vec2& other) { return Vec2(x * other.x, y * other.y); }
	};

	struct DVec2
	{
		double x, y;

		DVec2(double px = 0.0, double py = 0.0) : x(px), y(py) {}

		DVec2& operator=(const DVec2& other)
		{
			this->x = other.x;
			this->y = other.y;
			return *this;
		}

		// Arithmetic assignment operators
		DVec2& operator+=(const DVec2& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		DVec2& operator-=(const DVec2& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		DVec2& operator*=(const DVec2& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		DVec2& operator/=(const DVec2& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}

		// Comparison operators
		bool operator==(const DVec2& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}
		bool operator!=(const DVec2& rhs) const
		{
			return !(*this == rhs);
		}
		bool operator<(const DVec2& rhs) const
		{
			return (x < rhs.x) || (x == rhs.x && y < rhs.y);
		}
		bool operator>(const DVec2& rhs) const
		{
			return (x > rhs.x) || (x == rhs.x && y > rhs.y);
		}
	};

	struct IVec2
	{
		int x, y;

		IVec2(int px = 0, int py = 0) : x(px), y(py) {}

		IVec2& operator=(const IVec2& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}

		IVec2& operator+=(const IVec2& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		IVec2& operator-=(const IVec2& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		IVec2& operator*=(const IVec2& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		IVec2& operator/=(const IVec2& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}

		bool operator==(const IVec2& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}
		bool operator!=(const IVec2& rhs) const
		{
			return !(*this == rhs);
		}
		bool operator<(const IVec2& rhs) const
		{
			return (x < rhs.x) || (x == rhs.x && y < rhs.y);
		}
		bool operator>(const IVec2& rhs) const
		{
			return (x > rhs.x) || (x == rhs.x && y > rhs.y);
		}
	};
#pragma endregion  
#pragma region Vec3
	struct Vec3
	{
		float x, y, z;

		Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
		Vec3(float p = 0.0f) : x(p), y(p), z(p) {}

		inline Vec3& operator=(const Vec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		inline Vec3& operator+=(const Vec3& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
		inline Vec3& operator-=(const Vec3& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
		inline Vec3& operator*=(const Vec3& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}
		inline Vec3& operator/=(const Vec3& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			return *this;
		}

		inline bool operator==(const Vec3& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
		inline bool operator!=(const Vec3& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const Vec3& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			return z < rhs.z;
		}
		inline bool operator>(const Vec3& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			return z > rhs.z;
		}

		inline Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
		inline Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
		inline Vec3 operator/(const Vec3& other) const { return Vec3(x / other.x, y / other.y, z / other.z); }
		inline Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }

		inline Vec3 operator+(float other) const { return Vec3(x + other, y + other, z + other); }
		inline Vec3 operator-(float other) const { return Vec3(x - other, y - other, z - other); }
		inline Vec3 operator/(float other) const { return Vec3(x / other, y / other, z / other); }
		inline Vec3 operator*(float other) const { return Vec3(x * other, y * other, z * other); }
	};

	struct DVec3
	{
		double x, y, z;

		DVec3(double px = 0.0, double py = 0.0, double pz = 0.0) : x(px), y(py), z(pz) {}
		DVec3(double p) : x(p), y(p), z(p) {}

		inline DVec3& operator=(const DVec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		inline DVec3& operator+=(const DVec3& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
		inline DVec3& operator-=(const DVec3& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
		inline DVec3& operator*=(const DVec3& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}
		inline DVec3& operator/=(const DVec3& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			return *this;
		}

		inline bool operator==(const DVec3& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
		inline bool operator!=(const DVec3& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const DVec3& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			return z < rhs.z;
		}
		inline bool operator>(const DVec3& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			return z > rhs.z;
		}

		inline DVec3 operator+(const DVec3& other) const { return DVec3(x + other.x, y + other.y, z + other.z); }
		inline DVec3 operator-(const DVec3& other) const { return DVec3(x - other.x, y - other.y, z - other.z); }
		inline DVec3 operator/(const DVec3& other) const { return DVec3(x / other.x, y / other.y, z / other.z); }
		inline DVec3 operator*(const DVec3& other) const { return DVec3(x * other.x, y * other.y, z * other.z); }

		inline DVec3 operator+(double other) const { return DVec3(x + other, y + other, z + other); }
		inline DVec3 operator-(double other) const { return DVec3(x - other, y - other, z - other); }
		inline DVec3 operator/(double other) const { return DVec3(x / other, y / other, z / other); }
		inline DVec3 operator*(double other) const { return DVec3(x * other, y * other, z * other); }
	};

	struct IVec3
	{
		int x, y, z;

		IVec3(int px = 0, int py = 0, int pz = 0) : x(px), y(py), z(pz) {}

		IVec3& operator=(const IVec3& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

		inline IVec3& operator+=(const IVec3& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
		inline IVec3& operator-=(const IVec3& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
		inline IVec3& operator*=(const IVec3& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}
		inline IVec3& operator/=(const IVec3& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			return *this;
		}

		inline bool operator==(const IVec3& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
		inline bool operator!=(const IVec3& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const IVec3& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			return z < rhs.z;
		}
		inline bool operator>(const IVec3& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			return z > rhs.z;
		}

		inline IVec3 operator+(const IVec3& other) const { return IVec3(x + other.x, y + other.y, z + other.z); }
		inline IVec3 operator-(const IVec3& other) const { return IVec3(x - other.x, y - other.y, z - other.z); }
		inline IVec3 operator/(const IVec3& other) const { return IVec3(x / other.x, y / other.y, z / other.z); }
		inline IVec3 operator*(const IVec3& other) const { return IVec3(x * other.x, y * other.y, z * other.z); }

		inline IVec3 operator+(int other) const { return IVec3(x + other, y + other, z + other); }
		inline IVec3 operator-(int other) const { return IVec3(x - other, y - other, z - other); }
		inline IVec3 operator/(int other) const { return IVec3(x / other, y / other, z / other); }
		inline IVec3 operator*(int other) const { return IVec3(x * other, y * other, z * other); }
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

		inline Vec4& operator=(const Vec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		inline Vec4& operator+=(const Vec4& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}
		inline Vec4& operator-=(const Vec4& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}
		inline Vec4& operator*=(const Vec4& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			w *= rhs.w;
			return *this;
		}
		inline Vec4& operator/=(const Vec4& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			w /= rhs.w;
			return *this;
		}

		inline bool operator==(const Vec4& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
		}
		inline bool operator!=(const Vec4& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const Vec4& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			if (z < rhs.z)
				return true;
			if (z > rhs.z)
				return false;
			return w < rhs.w;
		}
		inline bool operator>(const Vec4& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			if (z > rhs.z)
				return true;
			if (z < rhs.z)
				return false;
			return w > rhs.w;
		}

		inline Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		inline Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		inline Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		inline Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		inline Vec4 operator+(float other) const { return Vec4(x + other, y + other, z + other, w + other); }
		inline Vec4 operator-(float other) const { return Vec4(x - other, y - other, z - other, w - other); }
		inline Vec4 operator/(float other) const { return Vec4(x / other, y / other, z / other, w / other); }
		inline Vec4 operator*(float other) const { return Vec4(x * other, y * other, z * other, w * other); }
	};;

	struct IVec4
	{
		int x, y, z, w;

		IVec4(int px = 0, int py = 0, int pz = 0, int pw = 0)
			: x(px), y(py), z(pz), w(pw) {
		}
		IVec4(float p) : x(p), y(p), z(p), w(p) {}

		inline IVec4& operator=(const IVec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		inline IVec4& operator+=(const IVec4& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}
		inline IVec4& operator-=(const IVec4& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}
		inline IVec4& operator*=(const IVec4& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			w *= rhs.w;
			return *this;
		}
		inline IVec4& operator/=(const IVec4& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			w /= rhs.w;
			return *this;
		}

		inline bool operator==(const IVec4& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
		}
		inline bool operator!=(const IVec4& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const IVec4& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			if (z < rhs.z)
				return true;
			if (z > rhs.z)
				return false;
			return w < rhs.w;
		}
		inline bool operator>(const IVec4& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			if (z > rhs.z)
				return true;
			if (z < rhs.z)
				return false;
			return w > rhs.w;
		}

		inline IVec4 operator+(const IVec4& other) const { return IVec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		inline IVec4 operator-(const IVec4& other) const { return IVec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		inline IVec4 operator/(const IVec4& other) const { return IVec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		inline IVec4 operator*(const IVec4& other) const { return IVec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		inline IVec4 operator+(int other) const { return IVec4(x + other, y + other, z + other, w + other); }
		inline IVec4 operator-(int other) const { return IVec4(x - other, y - other, z - other, w - other); }
		inline IVec4 operator/(int other) const { return IVec4(x / other, y / other, z / other, w / other); }
		inline IVec4 operator*(int other) const { return IVec4(x * other, y * other, z * other, w * other); }
	};

	struct DVec4
	{
		int x, y, z, w;

		DVec4(double px = 0, double py = 0, double pz = 0, double pw = 0)
			: x(px), y(py), z(pz), w(pw) {
		}
		DVec4(double p) : x(p), y(p), z(p), w(p) {}

		inline DVec4& operator=(const DVec4& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		inline DVec4& operator+=(const DVec4& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}
		inline DVec4& operator-=(const DVec4& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}
		inline DVec4& operator*=(const DVec4& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			w *= rhs.w;
			return *this;
		}
		inline DVec4& operator/=(const DVec4& rhs)
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			w /= rhs.w;
			return *this;
		}

		inline bool operator==(const DVec4& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
		}
		inline bool operator!=(const DVec4& rhs) const
		{
			return !(*this == rhs);
		}
		inline bool operator<(const DVec4& rhs) const
		{
			if (x < rhs.x)
				return true;
			if (x > rhs.x)
				return false;
			if (y < rhs.y)
				return true;
			if (y > rhs.y)
				return false;
			if (z < rhs.z)
				return true;
			if (z > rhs.z)
				return false;
			return w < rhs.w;
		}
		inline bool operator>(const DVec4& rhs) const
		{
			if (x > rhs.x)
				return true;
			if (x < rhs.x)
				return false;
			if (y > rhs.y)
				return true;
			if (y < rhs.y)
				return false;
			if (z > rhs.z)
				return true;
			if (z < rhs.z)
				return false;
			return w > rhs.w;
		}

		inline DVec4 operator+(const DVec4& other) const { return DVec4(x + other.x, y + other.y, z + other.z, w + other.w); }
		inline DVec4 operator-(const DVec4& other) const { return DVec4(x - other.x, y - other.y, z - other.z, w - other.w); }
		inline DVec4 operator/(const DVec4& other) const { return DVec4(x / other.x, y / other.y, z / other.z, w / other.w); }
		inline DVec4 operator*(const DVec4& other) const { return DVec4(x * other.x, y * other.y, z * other.z, w * other.w); }

		inline DVec4 operator+(double other) const { return DVec4(x + other, y + other, z + other, w + other); }
		inline DVec4 operator-(double other) const { return DVec4(x - other, y - other, z - other, w - other); }
		inline DVec4 operator/(double other) const { return DVec4(x / other, y / other, z / other, w / other); }
		inline DVec4 operator*(double other) const { return DVec4(x * other, y * other, z * other, w * other); }
	};
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
}
