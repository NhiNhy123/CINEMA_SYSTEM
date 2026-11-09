#ifndef USER_SELECT_POPUP_H
#define USER_SELECT_POPUP_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include "RoundRectButton.h"
#include <string>
#include <functional>
#include <cmath>

namespace UI {

class UserSelectPopup {
private:
    bool isOpen;
    sf::Vector2f windowSize;
    sf::Vector2f cardSize;
    sf::Vector2f cardPos;

    std::string title;
    std::string message;
    ButtonStyle currentConfirmStyle;

    RoundRectButton btnConfirm;
    RoundRectButton btnCancel;

    std::function<void()> onConfirmCallback;
    std::function<void()> onCancelCallback;

public:
    UserSelectPopup(sf::Vector2f winSize = sf::Vector2f(1728.f, 972.f))
        : isOpen(false), windowSize(winSize), cardSize(sf::Vector2f(460.f, 210.f)),
          title("Xác nhận thao tác"), message("Bạn có chắc chắn muốn thực hiện hành động này?"),
          currentConfirmStyle(ButtonStyle::Primary),
          btnConfirm("Xác nhận", sf::Vector2f(0.f, 0.f), sf::Vector2f(136.f, 42.f), ButtonStyle::Primary, 10.f),
          btnCancel("Hủy bỏ", sf::Vector2f(0.f, 0.f), sf::Vector2f(136.f, 42.f), ButtonStyle::Ghost, 10.f)
    {
        applyButtonTheme(currentConfirmStyle);
        updateLayout();

        btnConfirm.setOnClick([this]() {
            isOpen = false;
            if (onConfirmCallback) onConfirmCallback();
        });

        btnCancel.setOnClick([this]() {
            isOpen = false;
            if (onCancelCallback) onCancelCallback();
        });
    }

    void applyButtonTheme(ButtonStyle confirmStyle) {
        currentConfirmStyle = confirmStyle;
        btnCancel.setFontSize(14);
        btnCancel.setCustomColors(
            sf::Color(241, 245, 249),
            sf::Color(226, 232, 240),
            sf::Color(203, 213, 225),
            sf::Color(241, 245, 249)
        );
        btnCancel.setOutline(1.f, sf::Color(203, 213, 225));
        btnCancel.setTextColor(sf::Color(51, 65, 85));

        btnConfirm.setFontSize(14);
        if (confirmStyle == ButtonStyle::Danger) {
            btnConfirm.setCustomColors(
                sf::Color(220, 38, 38),
                sf::Color(185, 28, 28),
                sf::Color(153, 27, 27),
                sf::Color(226, 232, 240)
            );
            btnConfirm.setOutline(0.f, sf::Color::Transparent);
            btnConfirm.setTextColor(sf::Color::White);
        } else {
            // Primary sky-blue tone matching app's main theme
            btnConfirm.setCustomColors(
                sf::Color(2, 132, 199),
                sf::Color(3, 105, 161),
                sf::Color(7, 89, 133),
                sf::Color(226, 232, 240)
            );
            btnConfirm.setOutline(0.f, sf::Color::Transparent);
            btnConfirm.setTextColor(sf::Color::White);
        }
    }

    void updateLayout() {
        cardPos = sf::Vector2f(
            std::round((windowSize.x - cardSize.x) / 2.0f),
            std::round((windowSize.y - cardSize.y) / 2.0f)
        );

        float btnW = 136.f;
        float btnGap = 16.f;
        float totalBtnW = btnW * 2.f + btnGap;
        float startX = std::round(cardPos.x + (cardSize.x - totalBtnW) / 2.0f);
        float btnY = std::round(cardPos.y + cardSize.y - 62.f);

        btnCancel.setPosition(sf::Vector2f(startX, btnY));
        btnConfirm.setPosition(sf::Vector2f(startX + btnW + btnGap, btnY));
    }

    void show(
        const std::string& t,
        const std::string& msg,
        ButtonStyle confirmStyle = ButtonStyle::Primary,
        const std::string& confirmLabel = "Xác nhận",
        const std::string& cancelLabel = "Hủy bỏ"
    ) {
        title = t;
        message = msg;
        applyButtonTheme(confirmStyle);
        btnConfirm.setText(confirmLabel);
        btnCancel.setText(cancelLabel);
        isOpen = true;
        updateLayout();
    }

    void close() { isOpen = false; }
    bool getIsOpen() const { return isOpen; }

    void setOnConfirm(std::function<void()> cb) { onConfirmCallback = cb; }
    void setOnCancel(std::function<void()> cb) { onCancelCallback = cb; }

    bool handleMouseMove(sf::Vector2f mousePos) {
        if (!isOpen) return false;
        btnConfirm.handleMouseMove(mousePos);
        btnCancel.handleMouseMove(mousePos);
        return true; // Modal captures all mouse movement
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isOpen) return false;
        btnConfirm.handleMouseButtonPressed(button, mousePos);
        btnCancel.handleMouseButtonPressed(button, mousePos);
        return true; // Modal captures all clicks
    }

    bool handleMouseButtonReleased(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isOpen) return false;
        btnConfirm.handleMouseButtonReleased(button, mousePos);
        btnCancel.handleMouseButtonReleased(button, mousePos);
        return true;
    }

    void draw(sf::RenderTarget& target) {
        if (!isOpen) return;

        // Soft semi-transparent backdrop matching light theme
        sf::RectangleShape backdrop(windowSize);
        backdrop.setFillColor(sf::Color(15, 23, 42, 115));
        target.draw(backdrop);

        // Subtle drop shadow
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(cardPos.x, cardPos.y + 6.f),
            cardSize,
            18.f,
            sf::Color(15, 23, 42, 35)
        );

        // Dialog Card (Clean White + Soft Sky-Blue Border)
        RoundedRectRenderer::render(
            target,
            cardPos,
            cardSize,
            18.f,
            sf::Color::White,
            1.5f,
            sf::Color(186, 230, 253)
        );

        // Top Accent Pill Bar
        sf::Color accentCol = (currentConfirmStyle == ButtonStyle::Danger)
            ? sf::Color(220, 38, 38)
            : sf::Color(2, 132, 199);
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(cardPos.x + (cardSize.x - 64.f) / 2.f, cardPos.y + 14.f),
            sf::Vector2f(64.f, 4.f),
            2.f,
            accentCol
        );

        // Centered Title
        sf::Text titleText;
        titleText.setFont(Theme::FontPrimary);
        titleText.setCharacterSize(22);
        titleText.setStyle(sf::Text::Bold);
        titleText.setFillColor(sf::Color(15, 23, 42));
        titleText.setString(toSfString(title));
        sf::FloatRect tb = titleText.getLocalBounds();
        titleText.setPosition(
            std::round(cardPos.x + (cardSize.x - tb.width) / 2.0f - tb.left),
            std::round(cardPos.y + 32.f - tb.top)
        );
        target.draw(titleText);

        // Centered Message (with automatic line wrapping inside card)
        std::vector<sf::String> msgLines = wrapSfTextLines(
            Theme::FontPrimary,
            toSfString(message),
            15,
            cardSize.x - 48.f
        );
        if (msgLines.size() > 3) msgLines.resize(3);
        float startMsgY = (msgLines.size() <= 1) ? (cardPos.y + 84.f) : (cardPos.y + 72.f);
        for (std::size_t idx = 0; idx < msgLines.size(); ++idx) {
            sf::Text msgText;
            msgText.setFont(Theme::FontPrimary);
            msgText.setCharacterSize(15);
            msgText.setFillColor(sf::Color(71, 85, 105));
            msgText.setString(msgLines[idx]);
            sf::FloatRect mb = msgText.getLocalBounds();
            msgText.setPosition(
                std::round(cardPos.x + (cardSize.x - mb.width) / 2.0f - mb.left),
                std::round(startMsgY + static_cast<float>(idx) * 22.f - mb.top)
            );
            target.draw(msgText);
        }

        // Centered Buttons
        btnCancel.draw(target);
        btnConfirm.draw(target);
    }
};

} // namespace UI

#endif // USER_SELECT_POPUP_H

