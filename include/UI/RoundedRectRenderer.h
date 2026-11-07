#ifndef ROUNDED_RECT_RENDERER_H
#define ROUNDED_RECT_RENDERER_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>

namespace UI {

class RoundedRectRenderer {
public:
    static sf::ConvexShape create(
        sf::Vector2f size,
        float radius,
        sf::Color fillColor,
        float outlineThickness = 0.f,
        sf::Color outlineColor = sf::Color::Transparent,
        unsigned int pointsPerCorner = 8
    ) {
        // Clamp radius to half of width/height
        float maxR = std::min(size.x, size.y) / 2.0f;
        if (radius > maxR) radius = maxR;
        if (radius < 0.f) radius = 0.f;

        if (radius <= 0.001f || pointsPerCorner < 1) {
            sf::ConvexShape shape(4);
            shape.setPoint(0, sf::Vector2f(0.f, 0.f));
            shape.setPoint(1, sf::Vector2f(size.x, 0.f));
            shape.setPoint(2, sf::Vector2f(size.x, size.y));
            shape.setPoint(3, sf::Vector2f(0.f, size.y));
            shape.setFillColor(fillColor);
            shape.setOutlineThickness(outlineThickness);
            shape.setOutlineColor(outlineColor);
            return shape;
        }

        const float PI = 3.14159265358979323846f;
        unsigned int totalPoints = pointsPerCorner * 4;
        sf::ConvexShape shape(totalPoints);

        unsigned int idx = 0;
        // Centers of 4 corner circles
        sf::Vector2f centers[4] = {
            sf::Vector2f(size.x - radius, radius),          // Top-right
            sf::Vector2f(size.x - radius, size.y - radius), // Bottom-right
            sf::Vector2f(radius, size.y - radius),          // Bottom-left
            sf::Vector2f(radius, radius)                    // Top-left
        };

        float startAngles[4] = {
            -PI / 2.0f, // Top-right arc: -90 deg to 0 deg
            0.0f,       // Bottom-right arc: 0 deg to 90 deg
            PI / 2.0f,  // Bottom-left arc: 90 deg to 180 deg
            PI          // Top-left arc: 180 deg to 270 deg
        };

        for (int c = 0; c < 4; ++c) {
            float startAngle = startAngles[c];
            float step = (PI / 2.0f) / static_cast<float>(pointsPerCorner - 1);
            for (unsigned int i = 0; i < pointsPerCorner; ++i) {
                float angle = startAngle + step * static_cast<float>(i);
                float px = centers[c].x + radius * std::cos(angle);
                float py = centers[c].y + radius * std::sin(angle);
                shape.setPoint(idx++, sf::Vector2f(px, py));
            }
        }

        shape.setFillColor(fillColor);
        shape.setOutlineThickness(outlineThickness);
        shape.setOutlineColor(outlineColor);
        return shape;
    }

    static void render(
        sf::RenderTarget& target,
        sf::Vector2f position,
        sf::Vector2f size,
        float radius,
        sf::Color fillColor,
        float outlineThickness = 0.f,
        sf::Color outlineColor = sf::Color::Transparent,
        unsigned int pointsPerCorner = 8
    ) {
        sf::ConvexShape shape = create(size, radius, fillColor, outlineThickness, outlineColor, pointsPerCorner);
        shape.setPosition(position);
        target.draw(shape);
    }
};

} // namespace UI

#endif // ROUNDED_RECT_RENDERER_H

