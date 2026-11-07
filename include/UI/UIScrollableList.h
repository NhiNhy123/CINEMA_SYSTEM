#ifndef UI_SCROLLABLE_LIST_H
#define UI_SCROLLABLE_LIST_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <algorithm>

namespace UI {

class UIScrollableList {
private:
    sf::Vector2f position;
    sf::Vector2f size;
    float contentHeight;
    float scrollOffsetY;
    float targetScrollOffsetY;
    float scrollSpeed;

    bool isDraggingScrollbar;
    float dragStartY;
    float dragStartScrollY;

    bool isVisible;
    bool isHovered;

    float scrollbarWidth;

public:
    UIScrollableList(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(600.f, 400.f)
    ) : position(pos), size(sz), contentHeight(0.f),
        scrollOffsetY(0.f), targetScrollOffsetY(0.f), scrollSpeed(35.f),
        isDraggingScrollbar(false), dragStartY(0.f), dragStartScrollY(0.f),
        isVisible(true), isHovered(false), scrollbarWidth(8.f) {}

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setContentHeight(float h) {
        contentHeight = h;
        clampScroll();
    }

    float getScrollOffsetY() const { return scrollOffsetY; }
    void setScrollOffsetY(float y) {
        targetScrollOffsetY = y;
        clampScroll();
        scrollOffsetY = targetScrollOffsetY;
    }

    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    void setVisible(bool vis) { isVisible = vis; }

    bool contains(sf::Vector2f pt) const {
        return pt.x >= position.x && pt.x <= position.x + size.x &&
               pt.y >= position.y && pt.y <= position.y + size.y;
    }

    float getMaxScroll() const {
        return std::max(0.0f, contentHeight - size.y);
    }

    void clampScroll() {
        float maxScroll = getMaxScroll();
        if (targetScrollOffsetY < 0.f) targetScrollOffsetY = 0.f;
        if (targetScrollOffsetY > maxScroll) targetScrollOffsetY = maxScroll;
    }

    bool handleMouseWheelScrolled(float delta, sf::Vector2f mousePos) {
        if (!isVisible || !contains(mousePos)) return false;
        targetScrollOffsetY -= delta * scrollSpeed;
        clampScroll();
        return true;
    }

    bool handleMouseMove(sf::Vector2f mousePos) {
        if (!isVisible) return false;
        isHovered = contains(mousePos);

        if (isDraggingScrollbar) {
            float maxScroll = getMaxScroll();
            float trackHeight = size.y;
            float thumbHeight = getThumbHeight();
            float travel = trackHeight - thumbHeight;

            if (travel > 0.f) {
                float dy = mousePos.y - dragStartY;
                targetScrollOffsetY = dragStartScrollY + (dy / travel) * maxScroll;
                clampScroll();
                scrollOffsetY = targetScrollOffsetY;
            }
            return true;
        }

        return isHovered;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (!isVisible || button != sf::Mouse::Left) return false;

        if (isScrollbarNeeded()) {
            sf::FloatRect thumbRect = getThumbRect();
            if (thumbRect.contains(mousePos)) {
                isDraggingScrollbar = true;
                dragStartY = mousePos.y;
                dragStartScrollY = scrollOffsetY;
                return true;
            }
        }
        return false;
    }

    bool handleMouseButtonReleased(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (button == sf::Mouse::Left && isDraggingScrollbar) {
            isDraggingScrollbar = false;
            return true;
        }
        return false;
    }

    void update(float dt) {
        if (!isVisible) return;
        // Smooth lerp scrolling
        float diff = targetScrollOffsetY - scrollOffsetY;
        if (std::abs(diff) > 0.1f) {
            scrollOffsetY += diff * std::min(1.0f, dt * 15.0f);
        } else {
            scrollOffsetY = targetScrollOffsetY;
        }
    }

    bool isScrollbarNeeded() const {
        return contentHeight > size.y;
    }

    float getThumbHeight() const {
        if (contentHeight <= 0.f) return size.y;
        float h = (size.y / contentHeight) * size.y;
        return std::max(24.0f, h);
    }

    sf::FloatRect getThumbRect() const {
        float maxScroll = getMaxScroll();
        float trackHeight = size.y;
        float thumbHeight = getThumbHeight();
        float travel = trackHeight - thumbHeight;
        float thumbY = (maxScroll > 0.f) ? (scrollOffsetY / maxScroll) * travel : 0.f;

        return sf::FloatRect(
            position.x + size.x - scrollbarWidth - 2.f,
            position.y + thumbY,
            scrollbarWidth,
            thumbHeight
        );
    }

    // Call beginDraw before drawing items to clip to list viewport
    void beginDraw(sf::RenderTarget& target, sf::View& previousView) {
        if (!isVisible) return;
        previousView = target.getView();

        sf::Vector2u winSize = target.getSize();
        sf::FloatRect viewport(
            position.x / static_cast<float>(winSize.x),
            position.y / static_cast<float>(winSize.y),
            size.x / static_cast<float>(winSize.x),
            size.y / static_cast<float>(winSize.y)
        );

        sf::View listView(sf::FloatRect(position.x, position.y + scrollOffsetY, size.x, size.y));
        listView.setViewport(viewport);
        target.setView(listView);
    }

    // Call endDraw after drawing items to restore view & render scrollbar
    void endDraw(sf::RenderTarget& target, const sf::View& previousView) {
        if (!isVisible) return;
        target.setView(previousView);

        // Render scrollbar
        if (isScrollbarNeeded()) {
            // Scrollbar track
            RoundedRectRenderer::render(
                target,
                sf::Vector2f(position.x + size.x - scrollbarWidth - 2.f, position.y),
                sf::Vector2f(scrollbarWidth, size.y),
                Theme::RadiusSmall,
                sf::Color(255, 255, 255, 15)
            );

            // Scrollbar thumb
            sf::FloatRect thumb = getThumbRect();
            sf::Color thumbColor = isDraggingScrollbar ? Theme::Primary : (isHovered ? Theme::Cyan : Theme::CardBorder);
            RoundedRectRenderer::render(
                target,
                sf::Vector2f(thumb.left, thumb.top),
                sf::Vector2f(thumb.width, thumb.height),
                Theme::RadiusSmall,
                thumbColor
            );
        }
    }
};

} // namespace UI

#endif // UI_SCROLLABLE_LIST_H

