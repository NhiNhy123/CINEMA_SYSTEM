#ifndef ROUND_RECT_BUTTON_H
#define ROUND_RECT_BUTTON_H

#include "Button.h"
#include "RoundedRectRenderer.h"

namespace UI {

enum class ButtonStyle {
    Primary,
    Secondary,
    Success,
    Danger,
    Ghost,
    Dark
};

class RoundRectButton : public Button {
private:
    float cornerRadius;
    sf::Color normalColor;
    sf::Color hoverColor;
    sf::Color pressedColor;
    sf::Color disabledColor;

    sf::Color textColorNormal;
    sf::Color textColorDisabled;

    float outlineThickness;
    sf::Color outlineColor;

public:
    RoundRectButton(
        const std::string& label = "",
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(120.f, 40.f),
        ButtonStyle style = ButtonStyle::Primary,
        float radius = Theme::RadiusMedium
    ) : Button(label, pos, sz, Theme::SizeBody),
        cornerRadius(radius),
        outlineThickness(0.f),
        outlineColor(sf::Color::Transparent)
    {
        setStyle(style);
    }

    void setStyle(ButtonStyle style) {
        outlineThickness = 0.f;
        outlineColor = sf::Color::Transparent;
        textColorNormal = Theme::TextPrimary;
        textColorDisabled = Theme::TextMuted;

        switch (style) {
            case ButtonStyle::Primary:
                normalColor   = Theme::Primary;
                hoverColor    = Theme::PrimaryHover;
                pressedColor  = Theme::PrimaryActive;
                disabledColor = Theme::CardBorder;
                break;
            case ButtonStyle::Secondary:
                normalColor   = Theme::Cyan;
                hoverColor    = Theme::CyanHover;
                pressedColor  = sf::Color(14, 116, 144);
                disabledColor = Theme::CardBorder;
                break;
            case ButtonStyle::Success:
                normalColor   = Theme::Success;
                hoverColor    = sf::Color(5, 150, 105);
                pressedColor  = sf::Color(4, 120, 87);
                disabledColor = Theme::CardBorder;
                break;
            case ButtonStyle::Danger:
                normalColor   = Theme::Danger;
                hoverColor    = Theme::DangerHover;
                pressedColor  = sf::Color(185, 28, 28);
                disabledColor = Theme::CardBorder;
                break;
            case ButtonStyle::Ghost:
                normalColor   = sf::Color::Transparent;
                hoverColor    = Theme::CardBgHover;
                pressedColor  = Theme::CardBorder;
                disabledColor = sf::Color::Transparent;
                outlineThickness = 1.5f;
                outlineColor     = Theme::CardBorder;
                break;
            case ButtonStyle::Dark:
                normalColor   = Theme::CardBg;
                hoverColor    = Theme::CardBgHover;
                pressedColor  = Theme::CardBorder;
                disabledColor = sf::Color(20, 26, 38);
                outlineThickness = 1.0f;
                outlineColor     = Theme::CardBorder;
                break;
        }
    }

    void setCustomColors(sf::Color normal, sf::Color hover, sf::Color pressed, sf::Color disabled) {
        normalColor   = normal;
        hoverColor    = hover;
        pressedColor  = pressed;
        disabledColor = disabled;
    }

    void setCornerRadius(float r) { cornerRadius = r; }
    void setTextColor(sf::Color normal, sf::Color disabled = Theme::TextMuted) {
        textColorNormal = normal;
        textColorDisabled = disabled;
    }
    void setOutline(float thickness, sf::Color color) {
        outlineThickness = thickness;
        outlineColor = color;
    }

    void draw(sf::RenderTarget& target) override {
        if (!visible) return;

        sf::Color bg;
        sf::Color txtColor = textColorNormal;

        switch (state) {
            case ButtonState::Normal:
                bg = normalColor;
                break;
            case ButtonState::Hovered:
                bg = hoverColor;
                break;
            case ButtonState::Pressed:
                bg = pressedColor;
                break;
            case ButtonState::Disabled:
                bg = disabledColor;
                txtColor = textColorDisabled;
                break;
        }

        // Render rounded rect background
        RoundedRectRenderer::render(
            target,
            position,
            size,
            cornerRadius,
            bg,
            outlineThickness,
            outlineColor
        );

        // Render centered text
        if (!text.empty() && font) {
            sf::Text sfText;
            sfText.setFont(*font);
            sfText.setString(toSfString(text));
            sfText.setCharacterSize(fontSize);
            sfText.setFillColor(txtColor);

            sf::FloatRect bounds = sfText.getLocalBounds();
            float tx = position.x + (size.x - bounds.width) / 2.0f - bounds.left;
            float ty = position.y + (size.y - bounds.height) / 2.0f - bounds.top;
            sfText.setPosition(tx, ty);

            target.draw(sfText);
        }
    }
};

} // namespace UI

#endif // ROUND_RECT_BUTTON_H

