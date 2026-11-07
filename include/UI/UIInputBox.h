#ifndef UI_INPUT_BOX_H
#define UI_INPUT_BOX_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <functional>
#include <string>

namespace UI {

class UIInputBox {
protected:
    sf::Vector2f position;
    sf::Vector2f size;
    sf::String unicodeText;
    std::string placeholder;
    unsigned int fontSize;
    const sf::Font* font;

    bool isFocused;
    bool isVisible;
    bool isEnabled;

    sf::Clock blinkClock;
    bool showCursor;

    float cornerRadius;
    float textInset;
    sf::Color backgroundColor;
    sf::Color borderColor;
    sf::Color focusColor;
    sf::Color textColor;
    sf::Color placeholderColor;
    std::function<void(const std::string&)> onTextChanged;
    std::function<void(const std::string&)> onEnter;

    // Helper: convert sf::String to UTF-8 std::string
    static std::string sfStringToUtf8(const sf::String& s) {
        std::basic_string<sf::Uint8> utf8 = s.toUtf8();
        return std::string(utf8.begin(), utf8.end());
    }

public:
    UIInputBox(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(280.f, 42.f),
        const std::string& holder = "",
        unsigned int fSize = Theme::SizeBody
    ) : position(pos), size(sz), placeholder(holder), fontSize(fSize),
        font(&Theme::FontPrimary), isFocused(false), isVisible(true),
        isEnabled(true), showCursor(true), cornerRadius(Theme::RadiusMedium),
        textInset(14.f), backgroundColor(Theme::InputBg),
        borderColor(Theme::InputBorder), focusColor(Theme::InputFocus),
        textColor(Theme::TextPrimary), placeholderColor(Theme::TextPlaceholder),
        onTextChanged(nullptr), onEnter(nullptr) {}

    virtual ~UIInputBox() = default;

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setPlaceholder(const std::string& holder) { placeholder = holder; }
    void setFont(const sf::Font& f) { font = &f; }
    void setFontSize(unsigned int sz) { fontSize = sz; }
    void setFocused(bool focused) {
        isFocused = focused;
        blinkClock.restart();
        showCursor = true;
    }
    void setVisible(bool vis) { isVisible = vis; }
    void setEnabled(bool en) { isEnabled = en; }
    void setCornerRadius(float r) { cornerRadius = r; }
    void setTextInset(float inset) { textInset = inset; }
    void setColors(
        sf::Color background,
        sf::Color border,
        sf::Color focus,
        sf::Color text,
        sf::Color placeholderText
    ) {
        backgroundColor = background;
        borderColor = border;
        focusColor = focus;
        textColor = text;
        placeholderColor = placeholderText;
    }

    void setText(const std::string& utf8Str) {
        unicodeText = sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
    }

    std::string getText() const {
        return sfStringToUtf8(unicodeText);
    }

    bool getFocused() const { return isFocused; }
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }

    void setOnTextChanged(std::function<void(const std::string&)> cb) { onTextChanged = cb; }
    void setOnEnter(std::function<void(const std::string&)> cb) { onEnter = cb; }

    bool contains(sf::Vector2f pt) const {
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y && pt.y <= position.y + size.y;
    }

    virtual bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isVisible || !isEnabled) return false;
        if (button == sf::Mouse::Left) {
            bool wasFocused = isFocused;
            isFocused = contains(mousePos);
            if (isFocused) {
                blinkClock.restart();
                showCursor = true;
            }
            return isFocused || wasFocused;
        }
        return false;
    }

    virtual bool handleTextEntered(sf::Uint32 unicode) {
        if (!isVisible || !isEnabled || !isFocused) return false;

        // Backspace
        if (unicode == 8) {
            if (!unicodeText.isEmpty()) {
                unicodeText.erase(unicodeText.getSize() - 1, 1);
                if (onTextChanged) onTextChanged(getText());
            }
            return true;
        }

        // Enter
        if (unicode == 13) {
            if (onEnter) onEnter(getText());
            return true;
        }

        // Tab or Escape or Control characters
        if (unicode < 32 && unicode != 9) return false;

        // Append character
        unicodeText += unicode;
        if (onTextChanged) onTextChanged(getText());
        return true;
    }

    virtual void update(float dt) {
        if (!isVisible || !isEnabled || !isFocused) return;
        if (blinkClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            blinkClock.restart();
        }
    }

    virtual void draw(sf::RenderTarget& target) {
        if (!isVisible) return;

        // Background & Border
        sf::Color bgColor = backgroundColor;
        sf::Color currentBorderColor = isFocused ? focusColor : borderColor;
        float borderThick = isFocused ? 2.0f : 1.0f;

        RoundedRectRenderer::render(
            target,
            position,
            size,
            cornerRadius,
            bgColor,
            borderThick,
            currentBorderColor
        );

        // Draw Text or Placeholder
        if (!font) return;

        sf::Text sfText;
        sfText.setFont(*font);
        sfText.setCharacterSize(fontSize);

        float textX = position.x + textInset;
        float textY = position.y + (size.y - fontSize) / 2.0f - 2.f;

        if (unicodeText.isEmpty() && !placeholder.empty() && !isFocused) {
            sfText.setString(toSfString(placeholder));
            sfText.setFillColor(placeholderColor);
            sfText.setPosition(textX, textY);
            target.draw(sfText);
        } else {
            sfText.setString(getDisplayText());
            sfText.setFillColor(textColor);
            sfText.setPosition(textX, textY);
            target.draw(sfText);

            // Draw cursor
            if (isFocused && showCursor) {
                sf::FloatRect bounds = sfText.getLocalBounds();
                float cursorX = textX + (unicodeText.isEmpty() ? 0.f : (bounds.left + bounds.width + 2.f));
                sf::RectangleShape cursor(sf::Vector2f(2.f, static_cast<float>(fontSize) + 4.f));
                cursor.setFillColor(focusColor);
                cursor.setPosition(cursorX, textY - 2.f);
                target.draw(cursor);
            }
        }
    }

protected:
    virtual sf::String getDisplayText() const {
        return unicodeText;
    }
};

} // namespace UI

#endif // UI_INPUT_BOX_H
