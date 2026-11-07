#ifndef STAT_CARD_H
#define STAT_CARD_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include <string>

namespace UI {

class StatCard {
private:
    sf::Vector2f position;
    sf::Vector2f size;
    std::string title;
    std::string value;
    std::string subtitle;
    sf::Color accentColor;
    const sf::Font* font;

public:
    StatCard(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(280.f, 130.f),
        const std::string& t = "Metric",
        const std::string& v = "0",
        const std::string& sub = "",
        sf::Color accent = Theme::Primary
    ) : position(pos), size(sz), title(t), value(v), subtitle(sub),
        accentColor(accent), font(&Theme::FontPrimary) {}

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) { size = sz; }
    void setTitle(const std::string& t) { title = t; }
    void setValue(const std::string& v) { value = v; }
    void setSubtitle(const std::string& s) { subtitle = s; }
    void setAccentColor(sf::Color c) { accentColor = c; }

    void draw(sf::RenderTarget& target) {
        if (!font) return;

        // Base card background
        RoundedRectRenderer::render(
            target,
            position,
            size,
            Theme::RadiusLarge,
            Theme::CardBg,
            1.0f,
            Theme::CardBorder
        );

        // Accent indicator left bar
        RoundedRectRenderer::render(
            target,
            sf::Vector2f(position.x + 4.f, position.y + 12.f),
            sf::Vector2f(4.f, size.y - 24.f),
            2.f,
            accentColor
        );

        // Title (Small uppercase label)
        sf::Text titleText;
        titleText.setFont(*font);
        titleText.setCharacterSize(Theme::SizeSmall);
        titleText.setFillColor(Theme::TextSecondary);
        titleText.setString(toSfString(title));
        titleText.setPosition(position.x + 22.f, position.y + 18.f);
        target.draw(titleText);

        // Value (Big Bold number)
        sf::Text valText;
        valText.setFont(Theme::FontTitle);
        valText.setCharacterSize(28);
        valText.setStyle(sf::Text::Bold);
        valText.setFillColor(Theme::TextPrimary);
        valText.setString(toSfString(value));
        valText.setPosition(position.x + 22.f, position.y + 44.f);
        target.draw(valText);

        // Subtitle / Trend
        if (!subtitle.empty()) {
            sf::Text subText;
            subText.setFont(*font);
            subText.setCharacterSize(Theme::SizeSmall);
            subText.setFillColor(accentColor);
            subText.setString(toSfString(subtitle));
            subText.setPosition(position.x + 22.f, position.y + size.y - 32.f);
            target.draw(subText);
        }
    }
};

} // namespace UI

#endif // STAT_CARD_H

