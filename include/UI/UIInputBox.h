#ifndef UI_INPUT_BOX_H
#define UI_INPUT_BOX_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

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
    bool isMultiLine;

    sf::Clock blinkClock;
    bool showCursor;

    float cornerRadius;
    float textInset;
    float rightInset;
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

    void normalizePlaceholderAndInset() {
        std::size_t firstNonSpace = placeholder.find_first_not_of(' ');
        if (firstNonSpace != std::string::npos && firstNonSpace >= 2) {
            placeholder = placeholder.substr(firstNonSpace);
            if (textInset < 38.f) {
                textInset = 38.f;
            }
        }
    }

public:
    UIInputBox(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(280.f, 42.f),
        const std::string& holder = "",
        unsigned int fSize = Theme::SizeBody
    ) : position(pos), size(sz), placeholder(holder), fontSize(fSize),
        font(&Theme::FontPrimary), isFocused(false), isVisible(true),
        isEnabled(true), isMultiLine(false), showCursor(true), cornerRadius(Theme::RadiusMedium),
        textInset(14.f), rightInset(14.f), backgroundColor(Theme::InputBg),
        borderColor(Theme::InputBorder), focusColor(Theme::InputFocus),
        textColor(Theme::TextPrimary), placeholderColor(Theme::TextPlaceholder),
        onTextChanged(nullptr), onEnter(nullptr) {
        normalizePlaceholderAndInset();
    }

    virtual ~UIInputBox() = default;

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setPlaceholder(const std::string& holder) {
        placeholder = holder;
        normalizePlaceholderAndInset();
    }
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
    void setRightInset(float inset) { rightInset = inset; }
    void setMultiLine(bool ml) { isMultiLine = ml; }
    bool getMultiLine() const { return isMultiLine; }
    bool isActuallyMultiLine() const { return isMultiLine; }
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
        if (unicode == 13 || unicode == 10) {
            if (isActuallyMultiLine()) {
                unicodeText += '\n';
                if (onTextChanged) onTextChanged(getText());
                return true;
            }
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

    // Thuật toán bẻ dòng (word-wrap) tự động cho văn bản nhiều dòng
    static std::vector<sf::String> wrapText(
        const sf::String& str,
        float maxWidth,
        const sf::Font& f,
        unsigned int charSize
    ) {
        std::vector<sf::String> result;
        if (str.isEmpty()) {
            result.push_back(sf::String());
            return result;
        }

        sf::Text measureText;
        measureText.setFont(f);
        measureText.setCharacterSize(charSize);

        // Tách các đoạn theo ký tự xuống dòng '\n'
        std::vector<sf::String> paragraphs;
        sf::String currentPara;
        for (std::size_t i = 0; i < str.getSize(); ++i) {
            if (str[i] == '\n') {
                paragraphs.push_back(currentPara);
                currentPara.clear();
            } else if (str[i] != '\r') {
                currentPara += str[i];
            }
        }
        paragraphs.push_back(currentPara);

        for (std::size_t p = 0; p < paragraphs.size(); ++p) {
            const sf::String& para = paragraphs[p];
            if (para.isEmpty()) {
                result.push_back(sf::String());
                continue;
            }

            sf::String currentLine;
            int lastSpaceInLine = -1;

            for (std::size_t i = 0; i < para.getSize(); ++i) {
                sf::Uint32 ch = para[i];
                sf::String testLine = currentLine + ch;
                measureText.setString(testLine);
                float w = measureText.getLocalBounds().width;

                if (w <= maxWidth) {
                    currentLine += ch;
                    if (ch == ' ' || ch == '\t') {
                        lastSpaceInLine = static_cast<int>(currentLine.getSize()) - 1;
                    }
                } else {
                    // Đã vượt quá độ rộng tối đa của ô
                    if (lastSpaceInLine != -1 && lastSpaceInLine > 0) {
                        // Cắt dòng ngay tại khoảng trắng gần nhất
                        sf::String lineToPush = currentLine.substring(0, lastSpaceInLine);
                        result.push_back(lineToPush);

                        // Phần còn lại bắt đầu từ sau khoảng trắng
                        currentLine = currentLine.substring(lastSpaceInLine + 1) + ch;

                        // Tìm lại khoảng trắng trong currentLine mới
                        lastSpaceInLine = -1;
                        for (std::size_t k = 0; k < currentLine.getSize(); ++k) {
                            if (currentLine[k] == ' ' || currentLine[k] == '\t') {
                                lastSpaceInLine = static_cast<int>(k);
                            }
                        }
                    } else {
                        // Từ quá dài không có khoảng trắng, bắt buộc bẻ theo ký tự
                        if (!currentLine.isEmpty()) {
                            result.push_back(currentLine);
                        }
                        currentLine = sf::String(ch);
                        lastSpaceInLine = (ch == ' ' || ch == '\t') ? 0 : -1;
                    }
                }
            }

            if (!currentLine.isEmpty()) {
                result.push_back(currentLine);
            }
        }

        if (result.empty()) {
            result.push_back(sf::String());
        }
        return result;
    }

    virtual void draw(sf::RenderTarget& target) {
        if (!isVisible) return;

        // 1. Vẽ nền (Background) & Đường viền (Border) bo góc
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

        if (!font) return;

        // Lưu lại View ban đầu của target
        sf::View origView = target.getView();
        sf::Vector2u targetSize = target.getSize();

        // Thiết lập vùng Scissor View clipping để chữ tuyệt đối không bao giờ tràn ra ngoài khung viền
        float insetBorder = 3.f;
        sf::Vector2i tlPx = target.mapCoordsToPixel(sf::Vector2f(position.x + insetBorder, position.y + insetBorder), origView);
        sf::Vector2i brPx = target.mapCoordsToPixel(sf::Vector2f(position.x + size.x - insetBorder, position.y + size.y - insetBorder), origView);

        if (targetSize.x > 0 && targetSize.y > 0 && brPx.x > tlPx.x && brPx.y > tlPx.y) {
            float vpX = static_cast<float>(tlPx.x) / targetSize.x;
            float vpY = static_cast<float>(tlPx.y) / targetSize.y;
            float vpW = static_cast<float>(brPx.x - tlPx.x) / targetSize.x;
            float vpH = static_cast<float>(brPx.y - tlPx.y) / targetSize.y;

            sf::View clipView;
            clipView.reset(sf::FloatRect(position.x + insetBorder, position.y + insetBorder, size.x - 2.f * insetBorder, size.y - 2.f * insetBorder));
            clipView.setViewport(sf::FloatRect(vpX, vpY, vpW, vpH));
            target.setView(clipView);
        }

        bool multiLineMode = isActuallyMultiLine();
        float textX = position.x + textInset;
        float maxTextWidth = size.x - textInset - rightInset;
        if (maxTextWidth < 10.f) maxTextWidth = 10.f;

        if (multiLineMode) {
            // Chế độ văn bản nhiều dòng (Multi-line / Textarea): Tự động xuống dòng nằm gọn trong ô
            float topPadding = 10.f;
            float lineSpacing = font->getLineSpacing(fontSize);
            if (lineSpacing < static_cast<float>(fontSize) + 4.f) {
                lineSpacing = static_cast<float>(fontSize) + 6.f;
            }

            if (unicodeText.isEmpty() && !placeholder.empty() && !isFocused) {
                // Hiển thị placeholder có bẻ dòng nếu quá dài
                std::vector<sf::String> pLines = wrapText(toSfString(placeholder), maxTextWidth, *font, fontSize);
                for (std::size_t i = 0; i < pLines.size(); ++i) {
                    float lineY = position.y + topPadding + i * lineSpacing;
                    sf::Text sfLine;
                    sfLine.setFont(*font);
                    sfLine.setCharacterSize(fontSize);
                    sfLine.setFillColor(placeholderColor);
                    sfLine.setString(pLines[i]);
                    sfLine.setPosition(textX, lineY);
                    target.draw(sfLine);
                }
            } else {
                // Hiển thị nội dung text đã được bẻ dòng tự động
                sf::String disp = getDisplayText();
                std::vector<sf::String> lines = wrapText(disp, maxTextWidth, *font, fontSize);

                float visibleH = size.y - 2.0f * topPadding;
                float totalH = lines.size() * lineSpacing;
                float scrollOffsetY = 0.f;
                if (totalH > visibleH) {
                    // Tự động cuộn theo dòng cuối cùng khi người dùng gõ vượt quá chiều cao ô
                    scrollOffsetY = totalH - visibleH;
                }

                for (std::size_t i = 0; i < lines.size(); ++i) {
                    float lineY = position.y + topPadding + i * lineSpacing - scrollOffsetY;
                    if (!lines[i].isEmpty()) {
                        sf::Text sfLine;
                        sfLine.setFont(*font);
                        sfLine.setCharacterSize(fontSize);
                        sfLine.setFillColor(textColor);
                        sfLine.setString(lines[i]);
                        sfLine.setPosition(textX, lineY);
                        target.draw(sfLine);
                    }
                }

                // Vẽ con trỏ nhấp nháy tại vị trí cuối cùng của dòng cuối
                if (isFocused && showCursor) {
                    const sf::String& lastLine = lines.back();
                    sf::Text measureText;
                    measureText.setFont(*font);
                    measureText.setCharacterSize(fontSize);
                    measureText.setString(lastLine);
                    sf::FloatRect lastBounds = measureText.getLocalBounds();
                    float lastLineW = lastLine.isEmpty() ? 0.f : (lastBounds.left + lastBounds.width);

                    float cursorX = textX + lastLineW + 2.f;
                    float cursorY = position.y + topPadding + (lines.size() - 1) * lineSpacing - scrollOffsetY;

                    sf::RectangleShape cursor(sf::Vector2f(2.f, static_cast<float>(fontSize) + 4.f));
                    cursor.setFillColor(focusColor);
                    cursor.setPosition(cursorX, cursorY - 1.f);
                    target.draw(cursor);
                }
            }
        } else {
            // Chế độ một dòng (Single-line)
            float textY = position.y + (size.y - fontSize) / 2.0f - 2.f;
            if (unicodeText.isEmpty() && !placeholder.empty() && !isFocused) {
                sf::Text sfText;
                sfText.setFont(*font);
                sfText.setCharacterSize(fontSize);
                sfText.setString(toSfString(placeholder));
                sfText.setFillColor(placeholderColor);
                sfText.setPosition(textX, textY);
                target.draw(sfText);
            } else {
                sf::Text sfText;
                sfText.setFont(*font);
                sfText.setCharacterSize(fontSize);
                sfText.setString(getDisplayText());
                sfText.setFillColor(textColor);

                // Tự động dịch ngang khi chuỗi dài hơn độ rộng ô
                sf::FloatRect bounds = sfText.getLocalBounds();
                float totalW = bounds.left + bounds.width;
                float drawX = textX;
                if (totalW > maxTextWidth) {
                    drawX = textX - (totalW - maxTextWidth);
                }
                sfText.setPosition(drawX, textY);
                target.draw(sfText);

                // Vẽ con trỏ nhấp nháy
                if (isFocused && showCursor) {
                    float cursorX = drawX + (unicodeText.isEmpty() ? 0.f : (bounds.left + bounds.width + 2.f));
                    sf::RectangleShape cursor(sf::Vector2f(2.f, static_cast<float>(fontSize) + 4.f));
                    cursor.setFillColor(focusColor);
                    cursor.setPosition(cursorX, textY - 2.f);
                    target.draw(cursor);
                }
            }
        }

        // Khôi phục View ban đầu
        target.setView(origView);
    }

protected:
    virtual sf::String getDisplayText() const {
        return unicodeText;
    }
};

} // namespace UI

#endif // UI_INPUT_BOX_H
