#ifndef TEXT_BUTTON_H
#define TEXT_BUTTON_H

#include "Button.h"

namespace UI {

class TextButton : public Button {
private:
    sf::Color normalColor;
    sf::Color hoverColor;
    sf::Color pressedColor;
    bool underlineOnHover;

public:
    TextButton(
        const std::string& label = "",
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        unsigned int fSize = Theme::SizeBody,
        sf::Color normal = Theme::Primary,
        sf::Color hover = Theme::Cyan
    ) : Button(label, pos, sf::Vector2f(0.f, 0.f), fSize),
        normalColor(normal),
        hoverColor(hover),
        pressedColor(Theme::PrimaryActive),
        underlineOnHover(true)
    {
        updateSize();
    }

    void updateSize() {
        if (font && !text.empty()) {
            sf::Text sfText;
            sfText.setFont(*font);
            sfText.setString(toSfString(text));
            sfText.setCharacterSize(fontSize);
            sf::FloatRect b = sfText.getLocalBounds();
            size = sf::Vector2f(b.width + 10.f, b.height + 8.f);
        }
    }

    void setText(const std::string& label) override {
        Button::setText(label);
        updateSize();
    }

    void setColors(sf::Color normal, sf::Color hover) {
        normalColor = normal;
        hoverColor = hover;
    }

    void draw(sf::RenderTarget& target) override {
        if (!visible || !font || text.empty()) return;

        sf::Color curColor = normalColor;
        if (!enabled) curColor = Theme::TextMuted;
        else if (state == ButtonState::Hovered) curColor = hoverColor;
        else if (state == ButtonState::Pressed) curColor = pressedColor;

        sf::Text sfText;
        sfText.setFont(*font);
        sfText.setString(toSfString(text));
        sfText.setCharacterSize(fontSize);
        sfText.setFillColor(curColor);
        sfText.setPosition(position.x, position.y);
        target.draw(sfText);

        if (underlineOnHover && state == ButtonState::Hovered && enabled) {
            sf::FloatRect b = sfText.getLocalBounds();
            sf::RectangleShape line(sf::Vector2f(b.width, 1.5f));
            line.setFillColor(curColor);
            line.setPosition(position.x, position.y + b.height + 4.f);
            target.draw(line);
        }
    }
};

} // namespace UI

#endif // TEXT_BUTTON_H

