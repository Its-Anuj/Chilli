#pragma once

namespace Chilli
{
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

    struct Vec3
    {
        float x, y, z;

        Vec3(float px = 0.0f, float py = 0.0f, float pz = 0.0f) : x(px), y(py), z(pz) {}

        Vec3& operator=(const Vec3& other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
            return *this;
        }

        Vec3& operator+=(const Vec3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }
        Vec3& operator-=(const Vec3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }
        Vec3& operator*=(const Vec3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }
        Vec3& operator/=(const Vec3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        bool operator==(const Vec3& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }
        bool operator!=(const Vec3& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const Vec3& rhs) const
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
        bool operator>(const Vec3& rhs) const
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
    };

    struct Vec4
    {
        float x, y, z, w;

        Vec4(float px = 0.0f, float py = 0.0f, float pz = 0.0f, float pw = 0.0f)
            : x(px), y(py), z(pz), w(pw) {
        }

        Vec4& operator=(const Vec4& other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
            return *this;
        }

        Vec4& operator+=(const Vec4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }
        Vec4& operator-=(const Vec4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }
        Vec4& operator*=(const Vec4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }
        Vec4& operator/=(const Vec4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        bool operator==(const Vec4& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
        }
        bool operator!=(const Vec4& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const Vec4& rhs) const
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
        bool operator>(const Vec4& rhs) const
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
    };

    struct IVec4
    {
        int x, y, z, w;

        IVec4(int px = 0, int py = 0, int pz = 0, int pw = 0)
            : x(px), y(py), z(pz), w(pw) {
        }

        IVec4& operator=(const IVec4& other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
            return *this;
        }

        IVec4& operator+=(const IVec4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }
        IVec4& operator-=(const IVec4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }
        IVec4& operator*=(const IVec4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }
        IVec4& operator/=(const IVec4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        bool operator==(const IVec4& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
        }
        bool operator!=(const IVec4& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const IVec4& rhs) const
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
        bool operator>(const IVec4& rhs) const
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
    };

    struct DVec3
    {
        double x, y, z;

        DVec3(double px = 0.0, double py = 0.0, double pz = 0.0) : x(px), y(py), z(pz) {}

        DVec3& operator=(const DVec3& other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
            return *this;
        }

        DVec3& operator+=(const DVec3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }
        DVec3& operator-=(const DVec3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }
        DVec3& operator*=(const DVec3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }
        DVec3& operator/=(const DVec3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        bool operator==(const DVec3& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }
        bool operator!=(const DVec3& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const DVec3& rhs) const
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
        bool operator>(const DVec3& rhs) const
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
    };

    struct DVec4
    {
        double x, y, z, w;

        DVec4(double px = 0.0, double py = 0.0, double pz = 0.0, double pw = 0.0)
            : x(px), y(py), z(pz), w(pw) {}

        DVec4& operator=(const DVec4& other)
        {
            x = other.x;
            y = other.y;
            z = other.z;
            w = other.w;
            return *this;
        }

        DVec4& operator+=(const DVec4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }
        DVec4& operator-=(const DVec4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }
        DVec4& operator*=(const DVec4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }
        DVec4& operator/=(const DVec4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        bool operator==(const DVec4& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
        }
        bool operator!=(const DVec4& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const DVec4& rhs) const
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
        bool operator>(const DVec4& rhs) const
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

        IVec3& operator+=(const IVec3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }
        IVec3& operator-=(const IVec3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }
        IVec3& operator*=(const IVec3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }
        IVec3& operator/=(const IVec3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        bool operator==(const IVec3& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }
        bool operator!=(const IVec3& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const IVec3& rhs) const
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
        bool operator>(const IVec3& rhs) const
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
    };
} // namespace VEngine
