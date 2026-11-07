#ifndef USER_SELECT_POPUP_H
#define USER_SELECT_POPUP_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include "RoundRectButton.h"
#include <string>
#include <functional>

namespace UI {

class UserSelectPopup {
private:
    bool isOpen;
    sf::Vector2f windowSize;
    sf::Vector2f cardSize;
    sf::Vector2f cardPos;

    std::string title;
    std::string message;

    RoundRectButton btnConfirm;
    RoundRectButton btnCancel;

    std::function<void()> onConfirmCallback;
    std::function<void()> onCancelCallback;

public:
    UserSelectPopup(sf::Vector2f winSize = sf::Vector2f(1728.f, 972.f))
        : isOpen(false), windowSize(winSize), cardSize(sf::Vector2f(480.f, 220.f)),
          title("Xác nhận thao tác"), message("Bạn có chắc chắn muốn thực hiện hành động này?"),
          btnConfirm("Xác nhận", sf::Vector2f(0.f, 0.f), sf::Vector2f(130.f, 42.f), ButtonStyle::Danger),
          btnCancel("Hủy bỏ", sf::Vector2f(0.f, 0.f), sf::Vector2f(130.f, 42.f), ButtonStyle::Dark)
    {
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

    void updateLayout() {
        cardPos = sf::Vector2f(
            (windowSize.x - cardSize.x) / 2.0f,
            (windowSize.y - cardSize.y) / 2.0f
        );

        float btnY = cardPos.y + cardSize.y - 60.f;
        btnCancel.setPosition(sf::Vector2f(cardPos.x + cardSize.x - 290.f, btnY));
        btnConfirm.setPosition(sf::Vector2f(cardPos.x + cardSize.x - 150.f, btnY));
    }

    void show(
        const std::string& t,
        const std::string& msg,
        ButtonStyle confirmStyle = ButtonStyle::Danger,
        const std::string& confirmLabel = "Xác nhận",
        const std::string& cancelLabel = "Hủy bỏ"
    ) {
        title = t;
        message = msg;
        btnConfirm.setStyle(confirmStyle);
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

        // Semi-transparent backdrop
        sf::RectangleShape backdrop(windowSize);
        backdrop.setFillColor(sf::Color(0, 0, 0, 185));
        target.draw(backdrop);

        // Dialog Card
        RoundedRectRenderer::render(
            target,
            cardPos,
            cardSize,
            Theme::RadiusLarge,
            Theme::CardBg,
            1.5f,
            Theme::CardBorder
        );

        // Title
        sf::Text titleText;
        titleText.setFont(Theme::FontPrimary);
        titleText.setCharacterSize(Theme::SizeTitle);
        titleText.setStyle(sf::Text::Bold);
        titleText.setFillColor(Theme::TextPrimary);
        titleText.setString(toSfString(title));
        titleText.setPosition(cardPos.x + 28.f, cardPos.y + 24.f);
        target.draw(titleText);

        // Message
        sf::Text msgText;
        msgText.setFont(Theme::FontPrimary);
        msgText.setCharacterSize(Theme::SizeBody);
        msgText.setFillColor(Theme::TextSecondary);
        msgText.setString(toSfString(message));
        msgText.setPosition(cardPos.x + 28.f, cardPos.y + 76.f);
        target.draw(msgText);

        // Buttons
        btnCancel.draw(target);
        btnConfirm.draw(target);
    }
};

} // namespace UI

#endif // USER_SELECT_POPUP_H

