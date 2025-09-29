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
} // namespace VEngine
