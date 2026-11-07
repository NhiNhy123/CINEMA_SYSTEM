#ifndef BUTTON_H
#define BUTTON_H

#include "UITheme.h"
#include <functional>
#include <string>

namespace UI {

enum class ButtonState {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

class Button {
protected:
    sf::Vector2f position;
    sf::Vector2f size;
    std::string text;
    unsigned int fontSize;
    const sf::Font* font;
    ButtonState state;
    bool enabled;
    bool visible;
    std::function<void()> onClick;

public:
    Button(
        const std::string& label = "",
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(120.f, 40.f),
        unsigned int fSize = Theme::SizeBody
    ) : position(pos), size(sz), text(label), fontSize(fSize),
        font(&Theme::FontPrimary), state(ButtonState::Normal),
        enabled(true), visible(true), onClick(nullptr) {}

    virtual ~Button() = default;

    virtual void setPosition(sf::Vector2f pos) { position = pos; }
    virtual void setSize(sf::Vector2f sz) { size = sz; }
    virtual void setText(const std::string& label) { text = label; }
    virtual void setFont(const sf::Font& f) { font = &f; }
    virtual void setFontSize(unsigned int sz) { fontSize = sz; }
    virtual void setEnabled(bool en) {
        enabled = en;
        state = en ? ButtonState::Normal : ButtonState::Disabled;
    }
    virtual void setVisible(bool vis) { visible = vis; }
    virtual void setOnClick(std::function<void()> cb) { onClick = cb; }
    virtual void triggerClick() {
        if (enabled && visible && onClick) onClick();
    }

    virtual sf::Vector2f getPosition() const { return position; }
    virtual sf::Vector2f getSize() const { return size; }
    virtual const std::string& getText() const { return text; }
    virtual bool isEnabled() const { return enabled; }
    virtual bool isVisible() const { return visible; }
    virtual ButtonState getState() const { return state; }

    virtual bool contains(sf::Vector2f pt) const {
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y && pt.y <= position.y + size.y;
    }

    virtual bool handleMouseMove(sf::Vector2f mousePos) {
        if (!visible || !enabled) return false;
        bool inside = contains(mousePos);
        if (inside) {
            if (state != ButtonState::Pressed) state = ButtonState::Hovered;
            return true;
        } else {
            if (state != ButtonState::Pressed) state = ButtonState::Normal;
            return false;
        }
    }

    virtual bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!visible || !enabled) return false;
        if (button == sf::Mouse::Left && contains(mousePos)) {
            state = ButtonState::Pressed;
            return true;
        }
        return false;
    }

    virtual bool handleMouseButtonReleased(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!visible || !enabled) return false;
        if (button == sf::Mouse::Left) {
            if (state == ButtonState::Pressed) {
                if (contains(mousePos)) {
                    state = ButtonState::Hovered;
                    if (onClick) onClick();
                    return true;
                } else {
                    state = ButtonState::Normal;
                }
            }
        }
        return false;
    }

    virtual void update(float dt) {}
    virtual void draw(sf::RenderTarget& target) = 0;
};

} // namespace UI

#endif // BUTTON_H

