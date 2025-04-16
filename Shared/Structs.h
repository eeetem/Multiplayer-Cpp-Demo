#pragma once
namespace World {
    struct Vector2 {
        float x;
        float y;
        bool operator==(const Vector2& other) const {
            return x == other.x && y == other.y;
        }

        bool operator!=(const Vector2& other) const {
            return !(*this == other);
        }
        Vector2 operator+(const Vector2& other) const {
            return { x + other.x, y + other.y };
        }
        Vector2 operator*(float scalar) const {
            return { x * scalar, y * scalar };
        }
        Vector2& operator+=(const Vector2& other) {
            x += other.x;
            y += other.y;
            return *this;
		}
        Vector2& operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            return *this;
        }

    };
    struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
}