#ifndef EDITABLE_TABLE_H
#define EDITABLE_TABLE_H

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include "RoundRectButton.h"
#include <vector>
#include <string>
#include <functional>

namespace UI {

struct TableColumn {
    std::string header;
    float width;
    bool isAction;
};

class EditableTable {
private:
    sf::Vector2f position;
    sf::Vector2f size;
    std::vector<TableColumn> columns;
    std::vector<std::vector<std::string>> rows;

    int selectedRow;
    int hoveredRow;
    float rowHeight;
    float headerHeight;
    const sf::Font* font;
    unsigned int fontSize;

    int scrollOffset;
    int maxVisibleRows;

    std::function<void(int)> onRowClick;
    std::function<void(int, const std::string&)> onActionClick;

public:
    EditableTable(
        sf::Vector2f pos = sf::Vector2f(0.f, 0.f),
        sf::Vector2f sz = sf::Vector2f(900.f, 400.f)
    ) : position(pos), size(sz), selectedRow(-1), hoveredRow(-1),
        rowHeight(42.f), headerHeight(44.f), font(&Theme::FontPrimary),
        fontSize(Theme::SizeBody), scrollOffset(0), onRowClick(nullptr),
        onActionClick(nullptr)
    {
        updateMaxVisibleRows();
    }

    void setPosition(sf::Vector2f pos) { position = pos; }
    void setSize(sf::Vector2f sz) {
        size = sz;
        updateMaxVisibleRows();
    }

    void updateMaxVisibleRows() {
        maxVisibleRows = static_cast<int>((size.y - headerHeight) / rowHeight);
        if (maxVisibleRows < 1) maxVisibleRows = 1;
    }

    void setColumns(const std::vector<TableColumn>& cols) { columns = cols; }
    void setRows(const std::vector<std::vector<std::string>>& r) {
        rows = r;
        if (selectedRow >= static_cast<int>(rows.size())) selectedRow = -1;
    }

    void clearRows() {
        rows.clear();
        selectedRow = -1;
        hoveredRow = -1;
    }

    void addRow(const std::vector<std::string>& rowData) {
        rows.push_back(rowData);
    }

    int getSelectedRow() const { return selectedRow; }
    void setSelectedRow(int r) { selectedRow = r; }

    void setOnRowClick(std::function<void(int)> cb) { onRowClick = cb; }
    void setOnActionClick(std::function<void(int, const std::string&)> cb) { onActionClick = cb; }

    bool handleMouseMove(sf::Vector2f mousePos) {
        hoveredRow = -1;
        float startY = position.y + headerHeight;
        for (int i = 0; i < maxVisibleRows; ++i) {
            int rIdx = scrollOffset + i;
            if (rIdx >= static_cast<int>(rows.size())) break;

            float ry = startY + static_cast<float>(i) * rowHeight;
            if (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
                mousePos.y >= ry && mousePos.y < ry + rowHeight) {
                hoveredRow = rIdx;
                return true;
            }
        }
        return false;
    }

    bool handleMouseWheelScrolled(float delta, sf::Vector2f mousePos) {
        if (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
            mousePos.y >= position.y && mousePos.y <= position.y + size.y) {
            int maxOffset = std::max(0, static_cast<int>(rows.size()) - maxVisibleRows);
            scrollOffset -= static_cast<int>(delta);
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > maxOffset) scrollOffset = maxOffset;
            return true;
        }
        return false;
    }

    bool handleMouseButtonPressed(sf::Mouse::Button button, sf::Vector2f mousePos) {
        if (button != sf::Mouse::Left) return false;

        float startY = position.y + headerHeight;
        for (int i = 0; i < maxVisibleRows; ++i) {
            int rIdx = scrollOffset + i;
            if (rIdx >= static_cast<int>(rows.size())) break;

            float ry = startY + static_cast<float>(i) * rowHeight;
            if (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
                mousePos.y >= ry && mousePos.y < ry + rowHeight) {

                // Check action button clicks inside action column
                float curX = position.x;
                for (size_t c = 0; c < columns.size(); ++c) {
                    if (columns[c].isAction) {
                        float colW = columns[c].width;
                        if (mousePos.x >= curX && mousePos.x <= curX + colW) {
                            if (onActionClick) onActionClick(rIdx, "action");
                            return true;
                        }
                    }
                    curX += columns[c].width;
                }

                selectedRow = rIdx;
                if (onRowClick) onRowClick(selectedRow);
                return true;
            }
        }
        return false;
    }

    void draw(sf::RenderTarget& target) {
        if (!font) return;

        // Container background
        RoundedRectRenderer::render(
            target,
            position,
            size,
            Theme::RadiusMedium,
            Theme::CardBg,
            1.0f,
            Theme::CardBorder
        );

        // Header Background
        sf::RectangleShape headerRect(sf::Vector2f(size.x, headerHeight));
        headerRect.setPosition(position.x, position.y);
        headerRect.setFillColor(Theme::BgDark);
        target.draw(headerRect);

        // Header Columns
        float curX = position.x + 12.f;
        for (size_t c = 0; c < columns.size(); ++c) {
            sf::Text hText;
            hText.setFont(*font);
            hText.setCharacterSize(Theme::SizeSmall);
            hText.setStyle(sf::Text::Bold);
            hText.setFillColor(Theme::TextSecondary);
            hText.setString(toSfString(columns[c].header));
            hText.setPosition(curX, position.y + (headerHeight - Theme::SizeSmall) / 2.0f);
            target.draw(hText);

            curX += columns[c].width;
        }

        // Header Divider
        sf::RectangleShape divider(sf::Vector2f(size.x, 1.0f));
        divider.setPosition(position.x, position.y + headerHeight);
        divider.setFillColor(Theme::CardBorder);
        target.draw(divider);

        // Rows
        float startY = position.y + headerHeight;
        for (int i = 0; i < maxVisibleRows; ++i) {
            int rIdx = scrollOffset + i;
            if (rIdx >= static_cast<int>(rows.size())) break;

            float ry = startY + static_cast<float>(i) * rowHeight;
            bool isSelected = (selectedRow == rIdx);
            bool isHovered = (hoveredRow == rIdx);

            // Row background highlight
            if (isSelected) {
                sf::RectangleShape rBg(sf::Vector2f(size.x, rowHeight));
                rBg.setPosition(position.x, ry);
                rBg.setFillColor(sf::Color(99, 102, 241, 50)); // Light Indigo
                target.draw(rBg);
            } else if (isHovered) {
                sf::RectangleShape rBg(sf::Vector2f(size.x, rowHeight));
                rBg.setPosition(position.x, ry);
                rBg.setFillColor(Theme::CardBgHover);
                target.draw(rBg);
            } else if (i % 2 == 1) {
                sf::RectangleShape rBg(sf::Vector2f(size.x, rowHeight));
                rBg.setPosition(position.x, ry);
                rBg.setFillColor(sf::Color(255, 255, 255, 4));
                target.draw(rBg);
            }

            // Cell contents
            float cellX = position.x + 12.f;
            const auto& rowData = rows[rIdx];

            for (size_t c = 0; c < columns.size(); ++c) {
                if (c < rowData.size()) {
                    if (columns[c].isAction) {
                        // Action pill
                        RoundedRectRenderer::render(
                            target,
                            sf::Vector2f(cellX, ry + 6.f),
                            sf::Vector2f(columns[c].width - 24.f, rowHeight - 12.f),
                            Theme::RadiusSmall,
                            Theme::Primary
                        );
                        sf::Text btnText;
                        btnText.setFont(*font);
                        btnText.setCharacterSize(Theme::SizeSmall);
                        btnText.setFillColor(Theme::TextPrimary);
                        btnText.setString(toSfString(rowData[c]));

                        sf::FloatRect bb = btnText.getLocalBounds();
                        float bx = cellX + (columns[c].width - 24.f - bb.width) / 2.0f - bb.left;
                        float by = ry + 6.f + (rowHeight - 12.f - bb.height) / 2.0f - bb.top;
                        btnText.setPosition(bx, by);
                        target.draw(btnText);
                    } else {
                        sf::Text cText;
                        cText.setFont(*font);
                        cText.setCharacterSize(fontSize);
                        cText.setFillColor(isSelected ? Theme::Cyan : Theme::TextPrimary);
                        cText.setString(toSfString(rowData[c]));
                        cText.setPosition(cellX, ry + (rowHeight - fontSize) / 2.0f - 2.f);
                        target.draw(cText);
                    }
                }
                cellX += columns[c].width;
            }

            // Row Bottom Divider
            sf::RectangleShape rDiv(sf::Vector2f(size.x, 1.0f));
            rDiv.setPosition(position.x, ry + rowHeight);
            rDiv.setFillColor(sf::Color(51, 65, 85, 80));
            target.draw(rDiv);
        }
    }
};

} // namespace UI

#endif // EDITABLE_TABLE_H

