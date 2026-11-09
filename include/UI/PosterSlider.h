#ifndef POSTER_SLIDER_H
#define POSTER_SLIDER_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include "RoundRectButton.h"
#include <vector>
#include <string>
#include <functional>
#include <map>

namespace UI {

struct PosterSlideItem {
    std::string maPhim;
    std::string tenPhim;
    std::string theLoai;
    int thoiLuong;
    int nhanDoTuoi;
    std::string moTa;
    std::string posterPath;
    sf::Color bannerColor;
};

class PosterSlider {
private:
    sf::Vector2f position;
    sf::Vector2f size;
    std::vector<PosterSlideItem> items;
    int currentIndex;

    sf::Clock autoSlideClock;
    float autoSlideInterval;

    RoundRectButton btnPrev;
    RoundRectButton btnNext;

    std::function<void(int)> onSelectMovie;
    std::map<std::string, sf::Texture> posterCache;

public:
    PosterSlider(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(1000.f, 320.f)
    ) : position(pos), size(sz), currentIndex(0),
        autoSlideInterval(5.0f),
        btnPrev("<", sf::Vector2f(0.f, 0.f), sf::Vector2f(40.f, 40.f), ButtonStyle::Dark, Theme::RadiusPill),
        btnNext(">", sf::Vector2f(0.f, 0.f), sf::Vector2f(40.f, 40.f), ButtonStyle::Dark, Theme::RadiusPill),
        onSelectMovie(nullptr)
    {
        updateLayout();

        btnPrev.setOnClick([this]() {
            if (items.empty()) return;
            currentIndex = (currentIndex - 1 + static_cast<int>(items.size())) % static_cast<int>(items.size());
            autoSlideClock.restart();
        });

        btnNext.setOnClick([this]() {
            if (items.empty()) return;
            currentIndex = (currentIndex + 1) % static_cast<int>(items.size());
            autoSlideClock.restart();
        });
    }

    void updateLayout() {
        btnPrev.setPosition(sf::Vector2f(position.x + 16.f, position.y + (size.y - 40.f) / 2.0f));
        btnNext.setPosition(sf::Vector2f(position.x + size.x - 56.f, position.y + (size.y - 40.f) / 2.0f));
    }

    void setPosition(sf::Vector2f pos) {
        position = pos;
        updateLayout();
    }

    void setSize(sf::Vector2f sz) {
        size = sz;
        updateLayout();
    }

    void setItems(const std::vector<PosterSlideItem>& list) {
        items = list;
        if (currentIndex >= static_cast<int>(items.size())) currentIndex = 0;
        autoSlideClock.restart();
    }

    void addItem(const PosterSlideItem& item) {
        items.push_back(item);
    }

    int getCurrentIndex() const { return currentIndex; }
    void setOnSelectMovie(std::function<void(int)> cb) { onSelectMovie = cb; }

    bool handleMouseMove(sf::Vector2f mousePos) {
        btnPrev.handleMouseMove(mousePos);
        btnNext.handleMouseMove(mousePos);
        return mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
               mousePos.y >= position.y && mousePos.y <= position.y + size.y;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (btnPrev.handleMouseButtonPressed(button, mousePos)) return true;
        if (btnNext.handleMouseButtonPressed(button, mousePos)) return true;

        if (button == sf::Mouse::Left) {
            if (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
                mousePos.y >= position.y && mousePos.y <= position.y + size.y) {
                if (onSelectMovie && !items.empty()) {
                    onSelectMovie(currentIndex);
                }
                return true;
            }
        }
        return false;
    }

    bool handleMouseButtonReleased(sf::Mouse::Button button, sf::Vector2f mousePos) {
        btnPrev.handleMouseButtonReleased(button, mousePos);
        btnNext.handleMouseButtonReleased(button, mousePos);
        return false;
    }

    void update(float dt) {
        if (items.size() > 1) {
            if (autoSlideClock.getElapsedTime().asSeconds() >= autoSlideInterval) {
                currentIndex = (currentIndex + 1) % static_cast<int>(items.size());
                autoSlideClock.restart();
            }
        }
    }

    void draw(sf::RenderTarget& target) {
        if (items.empty()) return;

        const auto& cur = items[currentIndex];

        // Banner backdrop card
        RoundedRectRenderer::render(
            target,
            position,
            size,
            Theme::RadiusLarge,
            Theme::CardBg,
            1.5f,
            Theme::CardBorder
        );

        // Gradient or tinted accent card on the left
        RoundedRectRenderer::render(
            target,
            position,
            sf::Vector2f(size.x * 0.45f, size.y),
            Theme::RadiusLarge,
            cur.bannerColor
        );

        // Poster Thumbnail Rect
        float posterW = 150.f;
        float posterH = 220.f;
        float posterX = position.x + 36.f;
        float posterY = position.y + (size.y - posterH) / 2.0f;

        RoundedRectRenderer::render(
            target,
            sf::Vector2f(posterX, posterY),
            sf::Vector2f(posterW, posterH),
            Theme::RadiusMedium,
            sf::Color(15, 23, 42, 220),
            2.f,
            Theme::Cyan
        );

        bool hasTexture = false;
        if (!cur.posterPath.empty()) {
            if (posterCache.find(cur.posterPath) == posterCache.end()) {
                sf::Texture tex;
                if (tex.loadFromFile(cur.posterPath)) {
                    tex.setSmooth(true);
                    posterCache[cur.posterPath] = tex;
                }
            }
            if (posterCache.find(cur.posterPath) != posterCache.end()) {
                sf::Sprite sp(posterCache[cur.posterPath]);
                float pad = 2.f;
                float sx = (posterW - pad * 2.f) / static_cast<float>(posterCache[cur.posterPath].getSize().x);
                float sy = (posterH - pad * 2.f) / static_cast<float>(posterCache[cur.posterPath].getSize().y);
                sp.setScale(sx, sy);
                sp.setPosition(posterX + pad, posterY + pad);
                target.draw(sp);
                hasTexture = true;
            }
        }

        if (!hasTexture) {
            // Synchronized Photo Frame Placeholder Icon (same style as F&B image frame)
            float icW = 54.f;
            float icH = 40.f;
            float icX = posterX + (posterW - icW) / 2.f;
            float icY = posterY + (posterH - icH) / 2.f - 12.f;
            RoundedRectRenderer::render(
                target,
                sf::Vector2f(icX, icY),
                sf::Vector2f(icW, icH),
                6.f,
                sf::Color::White,
                2.f,
                sf::Color(6, 182, 212)
            );
            sf::CircleShape sunDot(4.5f);
            sunDot.setFillColor(sf::Color(56, 189, 248));
            sunDot.setPosition(icX + 8.f, icY + 7.f);
            target.draw(sunDot);

            sf::ConvexShape mtn(4);
            mtn.setPoint(0, sf::Vector2f(icX + 4.f, icY + icH - 4.f));
            mtn.setPoint(1, sf::Vector2f(icX + 22.f, icY + 16.f));
            mtn.setPoint(2, sf::Vector2f(icX + 35.f, icY + 23.f));
            mtn.setPoint(3, sf::Vector2f(icX + icW - 4.f, icY + icH - 4.f));
            mtn.setFillColor(sf::Color(125, 211, 252));
            target.draw(mtn);

            sf::Text posterText;
            posterText.setFont(Theme::FontPrimary);
            posterText.setCharacterSize(12);
            posterText.setStyle(sf::Text::Bold);
            posterText.setFillColor(sf::Color(148, 163, 184));
            posterText.setString(toSfString("POSTER PHIM"));
            sf::FloatRect ptb = posterText.getLocalBounds();
            posterText.setPosition(posterX + (posterW - ptb.width) / 2.f - ptb.left,
                                   icY + icH + 12.f);
            target.draw(posterText);
        }

        // Movie Info (Right side of poster)
        float infoX = posterX + posterW + 30.f;
        float infoY = position.y + 45.f;

        // Age Rating Badge
        std::string ageLabel = (cur.nhanDoTuoi <= 0) ? "P (Mọi lứa tuổi)" : ("C" + std::to_string(cur.nhanDoTuoi) + " (Trên " + std::to_string(cur.nhanDoTuoi) + " tuổi)");
        sf::Color badgeBg = (cur.nhanDoTuoi >= 18) ? Theme::Danger : ((cur.nhanDoTuoi >= 16) ? Theme::Warning : Theme::Success);

        RoundedRectRenderer::render(
            target,
            sf::Vector2f(infoX, infoY),
            sf::Vector2f(130.f, 26.f),
            Theme::RadiusSmall,
            badgeBg
        );
        sf::Text badgeText;
        badgeText.setFont(Theme::FontPrimary);
        badgeText.setCharacterSize(Theme::SizeSmall);
        badgeText.setStyle(sf::Text::Bold);
        badgeText.setFillColor(Theme::TextPrimary);
        badgeText.setString(toSfString(ageLabel));
        badgeText.setPosition(infoX + 10.f, infoY + 4.f);
        target.draw(badgeText);

        // Movie Title (Bold Bebas or Primary)
        sf::Text titleText;
        titleText.setFont(Theme::FontTitle);
        titleText.setCharacterSize(Theme::SizeDisplay);
        titleText.setStyle(sf::Text::Bold);
        titleText.setFillColor(Theme::TextPrimary);
        titleText.setString(toSfString(cur.tenPhim));
        titleText.setPosition(infoX, infoY + 38.f);
        target.draw(titleText);

        // Genre & Duration
        sf::Text subText;
        subText.setFont(Theme::FontPrimary);
        subText.setCharacterSize(Theme::SizeSubheader);
        subText.setFillColor(Theme::Cyan);
        std::string subStr = "Thể loại: " + cur.theLoai + "  |  Thời lượng: " + std::to_string(cur.thoiLuong) + " phút";
        subText.setString(toSfString(subStr));
        subText.setPosition(infoX, infoY + 85.f);
        target.draw(subText);

        // Description
        sf::Text descText;
        descText.setFont(Theme::FontPrimary);
        descText.setCharacterSize(Theme::SizeBody);
        descText.setFillColor(Theme::TextSecondary);
        std::string shortDesc = cur.moTa;
        if (shortDesc.length() > 140) shortDesc = shortDesc.substr(0, 137) + "...";
        descText.setString(toSfString(shortDesc));
        descText.setPosition(infoX, infoY + 122.f);
        target.draw(descText);

        // "Đặt vé ngay" pill tag
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(infoX, infoY + 175.f),
            sf::Vector2f(140.f, 38.f),
            Theme::RadiusPill,
            Theme::Primary
        );
        sf::Text bookText;
        bookText.setFont(Theme::FontPrimary);
        bookText.setCharacterSize(Theme::SizeBody);
        bookText.setStyle(sf::Text::Bold);
        bookText.setFillColor(Theme::TextPrimary);
        bookText.setString(toSfString("Đặt Vé Ngay >"));
        sf::FloatRect bkb = bookText.getLocalBounds();
        bookText.setPosition(infoX + (140.f - bkb.width) / 2.f - bkb.left, infoY + 175.f + (38.f - bkb.height) / 2.f - bkb.top);
        target.draw(bookText);

        // Prev and Next navigation buttons
        btnPrev.draw(target);
        btnNext.draw(target);

        // Slide Indicator Dots
        float dotsStartX = position.x + (size.x - static_cast<float>(items.size()) * 18.f) / 2.0f;
        float dotsY = position.y + size.y - 20.f;

        for (size_t i = 0; i < items.size(); ++i) {
            bool isActive = (static_cast<int>(i) == currentIndex);
            float dotRadius = isActive ? 5.f : 3.5f;
            sf::CircleShape dot(dotRadius);
            dot.setPosition(dotsStartX + static_cast<float>(i) * 18.f, dotsY - dotRadius);
            dot.setFillColor(isActive ? Theme::Cyan : Theme::CardBorder);
            target.draw(dot);
        }
    }
};

} // namespace UI

#endif // POSTER_SLIDER_H

