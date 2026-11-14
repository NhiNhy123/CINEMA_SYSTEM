#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <sqlite3.h>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace UI {

void CinemaUIApp::reloadFnbImageTextures() {
    fnbImageTextures.clear();

    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
        sqlite3_exec(
            database,
            "CREATE TABLE IF NOT EXISTS fnb_images (maMatHang TEXT PRIMARY KEY, imagePath TEXT);",
            nullptr,
            nullptr,
            nullptr
        );
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(database, "SELECT maMatHang, imagePath FROM fnb_images", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* c0 = sqlite3_column_text(stmt, 0);
                const unsigned char* c1 = sqlite3_column_text(stmt, 1);
                std::string k = c0 ? reinterpret_cast<const char*>(c0) : "";
                std::string v = c1 ? reinterpret_cast<const char*>(c1) : "";
                if (!k.empty() && !v.empty()) {
                    fnbImagePaths[k] = v;
                }
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(database);
    }

    for (const auto& item : fnbList) {
        std::string id = item.getMaMatHang();
        std::string customPath = fnbImagePaths.count(id) ? fnbImagePaths[id] : "";
        std::vector<std::string> candidates;
        if (!customPath.empty()) {
            if (customPath.find('/') == std::string::npos && customPath.find('\\') == std::string::npos) {
                candidates.push_back("assets/fnb/" + customPath);
            }
            candidates.push_back(customPath);
        }
        candidates.push_back("assets/fnb/" + id + ".jpg");
        candidates.push_back("assets/fnb/" + id + ".png");
        candidates.push_back("assets/fnb/" + id + ".jpeg");
        candidates.push_back("assets/posters/" + id + ".jpg");
        candidates.push_back("assets/posters/" + id + ".png");

        for (const auto& path : candidates) {
            std::ifstream f(path.c_str(), std::ios::binary);
            if (!f.good()) continue;
            f.close();
            std::shared_ptr<sf::Texture> tex(new sf::Texture());
            if (tex->loadFromFile(path)) {
                tex->setSmooth(true);
                fnbImageTextures[id] = tex;
                if (fnbImagePaths[id].empty()) {
                    fnbImagePaths[id] = path;
                }
                break;
            }
        }
    }
}


void CinemaUIApp::setupConcessionsView() {
    float leftColX = sidebarWidth + 24.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;
    float gInputX = rightColX + 24.f;
    float gInputW = rightColW - 48.f;
    float halfBtnW = (gInputW - 14.f) / 2.f;

    fnbSubTab = 0;
    selectedFnbRowIdx = fnbList.empty() ? -1 : (static_cast<int>(fnbList.size()) - 1);
    fnbSortAsc = true;
    fnbFormLoai = (selectedFnbRowIdx >= 0) ? fnbList[selectedFnbRowIdx].getLoai() : "Combo";
    fnbFormActive = (selectedFnbRowIdx >= 0) ? (fnbList[selectedFnbRowIdx].getTrangThaiKinhDoanh() == "DangBan") : true;

    btnFnbTabAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX, 26.f),
        sf::Vector2f(215.f, 42.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnFnbTabAll->setOutline(0.f, sf::Color::Transparent);
    btnFnbTabAll->setCustomColors(sf::Color::Transparent, sf::Color(226, 232, 240, 70), sf::Color(203, 213, 225, 110), sf::Color::Transparent);
    btnFnbTabAll->setOnClick([this]() {
        fnbSubTab = 0;
    });

    btnFnbTabCombo = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 228.f, 26.f),
        sf::Vector2f(245.f, 42.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnFnbTabCombo->setOutline(0.f, sf::Color::Transparent);
    btnFnbTabCombo->setCustomColors(sf::Color::Transparent, sf::Color(226, 232, 240, 70), sf::Color(203, 213, 225, 110), sf::Color::Transparent);
    btnFnbTabCombo->setOnClick([this]() {
        fnbSubTab = 1;
    });

    fnbSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX - 165.f, 26.f),
        sf::Vector2f(310.f, 42.f),
        "Tìm tên hoặc mã món...",
        14
    ));
    fnbSearchInput->setTextInset(38.f);
    fnbSearchInput->setColors(sf::Color::White, sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    fnbSearchInput->setCornerRadius(21.f);

    btnFnbExport = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xuất danh sách",
        sf::Vector2f(rightColX + 158.f, 26.f),
        sf::Vector2f(165.f, 42.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnFnbExport->setFontSize(14);
    btnFnbExport->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnFnbExport->setOutline(1.f, sf::Color(226, 232, 240));
    btnFnbExport->setTextColor(sf::Color(30, 41, 59));
    btnFnbExport->setOnClick([this]() {
        std::ofstream out("DATA/danh_sach_fnb.csv", std::ios::out | std::ios::trunc);
        if (out.is_open()) {
            out << "MaSP,TenSanPham,PhanLoai,GiaBan,TrangThai\n";
            for (const auto& item : fnbList) {
                out << item.toCSV() << "\n";
            }
            out.close();
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_FNB", "Xuat danh sach san pham F&B");
        confirmPopup->show("Thành công", "Đã xuất danh sách F&B!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnFnbQuickAdd = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm món",
        sf::Vector2f(rightColX + 335.f, 26.f),
        sf::Vector2f(151.f, 42.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnFnbQuickAdd->setFontSize(14);
    btnFnbQuickAdd->setCustomColors(sf::Color(3, 105, 161), sf::Color(2, 132, 199), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnFnbQuickAdd->setOnClick([this]() {
        if (btnFnbClear) btnFnbClear->triggerClick();
    });

    inputFnbId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 318.f),
        sf::Vector2f(205.f, 42.f),
        "Nhập mã SP (VD: RB01)...",
        15
    ));
    inputFnbId->setColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputFnbId->setCornerRadius(10.f);

    inputFnbName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 402.f),
        sf::Vector2f(gInputW, 42.f),
        "Nhập tên món hoặc combo...",
        15
    ));
    inputFnbName->setColors(sf::Color(248, 250, 252), sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputFnbName->setCornerRadius(10.f);

    inputFnbPrice = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 656.f),
        sf::Vector2f(210.f, 42.f),
        "55000",
        16
    ));
    inputFnbPrice->setRightInset(50.f);
    inputFnbPrice->setColors(sf::Color(240, 249, 255), sf::Color(186, 230, 253), sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(148, 163, 184));
    inputFnbPrice->setCornerRadius(10.f);

    inputFnbNote = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 740.f),
        sf::Vector2f(gInputW, 40.f),
        "Ghi chú phục vụ quầy...",
        13
    ));
    inputFnbNote->setColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(51, 65, 85), sf::Color(148, 163, 184));
    inputFnbNote->setCornerRadius(8.f);
    inputFnbNote->setText("Kèm 02 ống hút giấy tiệt trùng + 02 khăn lạnh Cinestar");

    inputFnbImage = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX + 114.f, 212.f),
        sf::Vector2f(gInputW - 114.f, 38.f),
        "VD: RB01.jpg, bapphomai.jpg...",
        13
    ));
    inputFnbImage->setTextInset(34.f);
    inputFnbImage->setColors(sf::Color(248, 250, 252), sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputFnbImage->setCornerRadius(10.f);

    if (selectedFnbRowIdx >= 0 && selectedFnbRowIdx < static_cast<int>(fnbList.size())) {
        std::string initCode = fnbList[selectedFnbRowIdx].getMaMatHang();
        inputFnbId->setText(initCode);
        inputFnbId->setEnabled(false);
        inputFnbName->setText(fnbList[selectedFnbRowIdx].getTenMatHang());
        std::ostringstream pOss;
        pOss << static_cast<long long>(fnbList[selectedFnbRowIdx].getGiaTien());
        inputFnbPrice->setText(pOss.str());
        std::string storedImg = fnbImagePaths.count(initCode) ? fnbImagePaths[initCode] : (initCode + ".jpg");
        size_t slashPos = storedImg.find_last_of("/\\");
        std::string dispImg = (slashPos != std::string::npos) ? storedImg.substr(slashPos + 1) : storedImg;
        inputFnbImage->setText(dispImg);
    } else {
        inputFnbId->setText("");
        inputFnbId->setEnabled(true);
    }

    auto saveFnbImagePathToDb = [this](const std::string& code, const std::string& imgPath) {
        if (code.empty()) return;
        fnbImagePaths[code] = imgPath;
        sqlite3* database = nullptr;
        if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
            sqlite3_exec(
                database,
                "CREATE TABLE IF NOT EXISTS fnb_images (maMatHang TEXT PRIMARY KEY, imagePath TEXT);",
                nullptr,
                nullptr,
                nullptr
            );
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(database, "INSERT OR REPLACE INTO fnb_images(maMatHang, imagePath) VALUES(?, ?);", -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, imgPath.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            sqlite3_close(database);
        }
        reloadFnbImageTextures();
    };

    float btnRow1Y = 792.f;
    float btnRow2Y = 840.f;

    btnFnbUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Lưu thay đổi",
        sf::Vector2f(gInputX, btnRow1Y),
        sf::Vector2f(halfBtnW, 40.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnFnbUpdate->setFontSize(14);
    btnFnbUpdate->setCustomColors(sf::Color(3, 105, 161), sf::Color(2, 132, 199), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnFnbUpdate->setOnClick([this, saveFnbImagePathToDb]() {
        if (selectedFnbRowIdx < 0 || selectedFnbRowIdx >= static_cast<int>(fnbList.size())) {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 sản phẩm từ danh sách bên trái để cập nhật!", ButtonStyle::Primary, "Đóng", "Quay lại");
            return;
        }
        std::string name = inputFnbName->getText();
        if (name.find_first_not_of(" \t\r\n") == std::string::npos) {
            confirmPopup->show("Lỗi", "Tên sản phẩm không được để trống!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        std::string priceDigits = "";
        for (char c : inputFnbPrice->getText()) {
            if (c >= '0' && c <= '9') priceDigits += c;
        }
        double price = priceDigits.empty() ? 0.0 : std::stod(priceDigits);
        if (price <= 0.0) {
            confirmPopup->show("Lỗi", "Giá bán phải lớn hơn 0 VNĐ!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        std::string newSt = fnbFormActive ? "DangBan" : "NgungBan";
        FoodAndBeverage& cur = fnbList[selectedFnbRowIdx];
        std::string code = cur.getMaMatHang();

        std::string rawImg = inputFnbImage ? inputFnbImage->getText() : "";
        size_t imgStart = rawImg.find_first_not_of(" \t\r\n");
        std::string newImgName = (imgStart != std::string::npos) ? rawImg.substr(imgStart, rawImg.find_last_not_of(" \t\r\n") - imgStart + 1) : "";
        if (newImgName.empty()) {
            newImgName = code + ".jpg";
        }
        std::string fullImgPath = newImgName;
        if (fullImgPath.find('/') == std::string::npos && fullImgPath.find('\\') == std::string::npos) {
            fullImgPath = "assets/fnb/" + fullImgPath;
        }

        std::string curImg = fnbImagePaths.count(code) ? fnbImagePaths[code] : ("assets/fnb/" + code + ".jpg");

        bool nameChanged = (cur.getTenMatHang() != name);
        bool loaiChanged = (cur.getLoai() != fnbFormLoai);
        bool priceChanged = (std::fabs(cur.getGiaTien() - price) >= 0.5);
        bool stChanged = (cur.getTrangThaiKinhDoanh() != newSt);
        bool imgChanged = (!newImgName.empty() && newImgName != curImg && fullImgPath != curImg);

        if (!nameChanged && !loaiChanged && !priceChanged && !stChanged && !imgChanged) {
            confirmPopup->show("Thông báo", "Chưa có thông tin nào thay đổi!", ButtonStyle::Primary, "Đóng", "Quay lại");
            return;
        }

        cur.setTenMatHang(name);
        cur.setLoai(fnbFormLoai);
        cur.setGiaTien(price);
        cur.setTrangThaiKinhDoanh(newSt);
        db.ghiDanhSachFAndB(fnbList);
        saveFnbImagePathToDb(code, fullImgPath);

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_FNB", "Cập nhật F&B: " + name + " (" + code + ")");
        confirmPopup->show("Thành công", "Đã lưu thay đổi cho sản phẩm " + code + "!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnFnbCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm món",
        sf::Vector2f(gInputX + halfBtnW + 14.f, btnRow1Y),
        sf::Vector2f(halfBtnW, 40.f),
        ButtonStyle::Success,
        8.f
    ));
    btnFnbCreate->setFontSize(14);
    btnFnbCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnFnbCreate->setOnClick([this, saveFnbImagePathToDb]() {
        if (selectedFnbRowIdx >= 0) {
            if (btnFnbClear) btnFnbClear->triggerClick();
            confirmPopup->show("Thêm món mới", "Đã chuyển sang chế độ thêm mới. Vui lòng nhập mã và thông tin món ăn!", ButtonStyle::Primary, "Đồng ý", "Đóng");
            return;
        }
        std::string code = inputFnbId->getText();
        size_t cStart = code.find_first_not_of(" \t\r\n");
        if (cStart == std::string::npos) {
            confirmPopup->show("Lỗi", "Vui lòng nhập mã sản phẩm!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        code = code.substr(cStart, code.find_last_not_of(" \t\r\n") - cStart + 1);

        std::string name = inputFnbName->getText();
        size_t nStart = name.find_first_not_of(" \t\r\n");
        if (nStart == std::string::npos) {
            confirmPopup->show("Lỗi", "Tên sản phẩm không được để trống!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        name = name.substr(nStart, name.find_last_not_of(" \t\r\n") - nStart + 1);

        for (const auto& item : fnbList) {
            if (item.getMaMatHang() == code) {
                confirmPopup->show("Lỗi", "Mã sản phẩm '" + code + "' đã tồn tại! Vui lòng nhập mã khác.", ButtonStyle::Danger, "Đóng", "Quay lại");
                return;
            }
        }
        std::string priceDigits = "";
        for (char c : inputFnbPrice->getText()) {
            if (c >= '0' && c <= '9') priceDigits += c;
        }
        double price = priceDigits.empty() ? 0.0 : std::stod(priceDigits);
        if (price <= 0.0) {
            confirmPopup->show("Lỗi", "Giá bán phải lớn hơn 0 VNĐ!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }

        std::string rawImg = inputFnbImage ? inputFnbImage->getText() : "";
        size_t imgStart = rawImg.find_first_not_of(" \t\r\n");
        std::string newImgName = (imgStart != std::string::npos) ? rawImg.substr(imgStart, rawImg.find_last_not_of(" \t\r\n") - imgStart + 1) : (code + ".jpg");
        std::string fullImgPath = newImgName;
        if (fullImgPath.find('/') == std::string::npos && fullImgPath.find('\\') == std::string::npos) {
            fullImgPath = "assets/fnb/" + fullImgPath;
        }

        std::string st = fnbFormActive ? "DangBan" : "NgungBan";
        fnbList.push_back(FoodAndBeverage(code, name, fnbFormLoai, price, st));
        fnbQuantities.assign(fnbList.size(), 0);
        db.ghiDanhSachFAndB(fnbList);
        saveFnbImagePathToDb(code, fullImgPath);

        selectedFnbRowIdx = static_cast<int>(fnbList.size()) - 1;
        inputFnbId->setEnabled(false);

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_FNB", "Thêm sản phẩm F&B: " + name + " (" + code + ")");
        confirmPopup->show("Thành công", "Đã thêm sản phẩm mới " + code + "!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnFnbDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  Xóa món",
        sf::Vector2f(gInputX, btnRow2Y),
        sf::Vector2f(halfBtnW, 38.f),
        ButtonStyle::Danger,
        8.f
    ));
    btnFnbDelete->setFontSize(13);
    btnFnbDelete->setCustomColors(sf::Color(220, 38, 38), sf::Color(185, 28, 28), sf::Color(153, 27, 27), sf::Color(203, 213, 225));
    btnFnbDelete->setOnClick([this]() {
        if (selectedFnbRowIdx >= 0 && selectedFnbRowIdx < static_cast<int>(fnbList.size())) {
            std::string code = fnbList[selectedFnbRowIdx].getMaMatHang();
            std::string name = fnbList[selectedFnbRowIdx].getTenMatHang();
            confirmPopup->show("Xác nhận xóa", "Bạn có chắc chắn muốn xóa '" + name + "' (" + code + ")?", ButtonStyle::Danger, "Xóa món", "Quay lại");
            confirmPopup->setOnConfirm([this, code, name]() {
                fnbList.erase(fnbList.begin() + selectedFnbRowIdx);
                fnbQuantities.assign(fnbList.size(), 0);
                db.ghiDanhSachFAndB(fnbList);
                reloadFnbImageTextures();
                if (!fnbList.empty()) {
                    selectedFnbRowIdx = 0;
                    std::string c0 = fnbList[0].getMaMatHang();
                    inputFnbId->setText(c0);
                    inputFnbId->setEnabled(false);
                    inputFnbName->setText(fnbList[0].getTenMatHang());
                    inputFnbPrice->setText(std::to_string(static_cast<long long>(fnbList[0].getGiaTien())));
                    std::string storedImg = fnbImagePaths.count(c0) ? fnbImagePaths[c0] : (c0 + ".jpg");
                    size_t slashPos = storedImg.find_last_of("/\\");
                    std::string dispImg = (slashPos != std::string::npos) ? storedImg.substr(slashPos + 1) : storedImg;
                    if (inputFnbImage) {
                        inputFnbImage->setText(dispImg);
                    }
                    fnbFormLoai = fnbList[0].getLoai();
                    fnbFormActive = (fnbList[0].getTrangThaiKinhDoanh() == "DangBan");
                } else {
                    selectedFnbRowIdx = -1;
                    inputFnbId->setText("");
                    inputFnbId->setEnabled(true);
                    inputFnbName->setText("");
                    inputFnbPrice->setText("");
                    if (inputFnbImage) inputFnbImage->setText("");
                }
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_FNB", "Xóa sản phẩm F&B: " + name + " (" + code + ")");
            });
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 sản phẩm từ danh sách để xóa!", ButtonStyle::Primary, "Đóng", "Quay lại");
        }
    });

    btnFnbClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  Làm mới",
        sf::Vector2f(gInputX + halfBtnW + 14.f, btnRow2Y),
        sf::Vector2f(halfBtnW, 38.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnFnbClear->setFontSize(13);
    btnFnbClear->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnFnbClear->setOutline(1.f, sf::Color(226, 232, 240));
    btnFnbClear->setTextColor(sf::Color(51, 65, 85));
    btnFnbClear->setOnClick([this]() {
        selectedFnbRowIdx = -1;
        inputFnbId->setText("");
        inputFnbId->setEnabled(true);
        inputFnbName->setText("");
        inputFnbPrice->setText("");
        if (inputFnbImage) inputFnbImage->setText("");
        fnbFormLoai = "Bap";
        fnbFormActive = true;
        inputFnbId->setFocused(true);
    });
}

// Layout coordinates for right form card (consistent across setup & render)
// Layout coordinates for right form card (consistent across setup & render)
static const float ST_FORM_MOVIE_LBL_Y = 276.f;
static const float ST_FORM_MOVIE_Y     = 298.f;
static const float ST_FORM_MOVIE_INFO_Y= 342.f;
static const float ST_FORM_ROOM_LBL_Y  = 374.f;
static const float ST_FORM_ROOM_Y      = 396.f;
static const float ST_FORM_ROOM_INFO_Y = 440.f;
static const float ST_FORM_DATE_LBL_Y  = 470.f;
static const float ST_FORM_DATE_Y      = 492.f;
static const float ST_FORM_EST_Y       = 546.f;
static const float ST_FORM_EST_H       = 92.f;
static const float ST_FORM_BTN_ROW_Y   = 778.f;


void CinemaUIApp::renderConcessionsView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float leftColX = sidebarWidth + 24.f;
    float leftColW = 928.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;
    float gInputX = rightColX + 24.f;
    float gInputW = rightColW - 48.f;

    // 1. Clean Light Blue-Gray Canvas Background (#f8fafc)
    sf::RectangleShape fnbBg(sf::Vector2f(contentAreaW, 972.f));
    fnbBg.setPosition(sidebarWidth, 0.f);
    fnbBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(fnbBg);

    // 2. Top Navigation & Action Bar (y = 26.f, height = 42.f)
    float tabY = 26.f;
    float tabH = 42.f;
    float tab1W = 215.f;
    float tab2W = 245.f;
    float tab1X = leftColX;
    float tab2X = leftColX + 228.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab1X, tabY),
        sf::Vector2f(tab1W, tabH),
        10.f,
        (fnbSubTab == 0) ? sf::Color::White : sf::Color(241, 245, 249),
        1.5f,
        (fnbSubTab == 0) ? sf::Color(3, 105, 161) : sf::Color(226, 232, 240)
    );
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab2X, tabY),
        sf::Vector2f(tab2W, tabH),
        10.f,
        (fnbSubTab == 1) ? sf::Color::White : sf::Color(241, 245, 249),
        1.5f,
        (fnbSubTab == 1) ? sf::Color(3, 105, 161) : sf::Color(226, 232, 240)
    );

    if (btnFnbTabAll) btnFnbTabAll->draw(window);
    if (btnFnbTabCombo) btnFnbTabCombo->draw(window);

    // Tab 1 Content: "Quản lý F&B [08]"
    sf::Text t1Txt;
    t1Txt.setFont(Theme::FontPrimary);
    t1Txt.setCharacterSize(15);
    t1Txt.setStyle(sf::Text::Bold);
    t1Txt.setFillColor((fnbSubTab == 0) ? sf::Color(3, 105, 161) : sf::Color(100, 116, 139));
    t1Txt.setString(toSfString("Quản lý F&B"));
    sf::FloatRect t1B = t1Txt.getLocalBounds();
    float bW = 32.f, bH = 22.f, gGap = 8.f;
    float tot1W = t1B.width + gGap + bW;
    float start1X = tab1X + (tab1W - tot1W) / 2.f;
    t1Txt.setPosition(std::round(start1X - t1B.left), std::round(tabY + (tabH - t1B.height) / 2.f - t1B.top));
    window.draw(t1Txt);

    float b1X = std::round(start1X + t1B.width + gGap);
    float b1Y = std::round(tabY + (tabH - bH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(b1X, b1Y),
        sf::Vector2f(bW, bH),
        11.f,
        (fnbSubTab == 0) ? sf::Color(3, 105, 161) : sf::Color(226, 232, 240)
    );
    sf::Text b1Txt;
    b1Txt.setFont(Theme::FontPrimary);
    b1Txt.setCharacterSize(12);
    b1Txt.setStyle(sf::Text::Bold);
    b1Txt.setFillColor((fnbSubTab == 0) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream b1Oss;
    b1Oss << std::setw(2) << std::setfill('0') << fnbList.size();
    b1Txt.setString(b1Oss.str());
    sf::FloatRect b1B = b1Txt.getLocalBounds();
    b1Txt.setPosition(std::round(b1X + (bW - b1B.width) / 2.f - b1B.left), std::round(b1Y + (bH - b1B.height) / 2.f - b1B.top));
    window.draw(b1Txt);

    // Tab 2 Content: "Combo Khuyến Mãi [02]"
    int comboCount = 0;
    for (const auto& item : fnbList) {
        if (item.getLoai() == "Combo") comboCount++;
    }
    sf::Text t2Txt;
    t2Txt.setFont(Theme::FontPrimary);
    t2Txt.setCharacterSize(15);
    t2Txt.setStyle(sf::Text::Bold);
    t2Txt.setFillColor((fnbSubTab == 1) ? sf::Color(3, 105, 161) : sf::Color(100, 116, 139));
    t2Txt.setString(toSfString("Combo Khuyến Mãi"));
    sf::FloatRect t2B = t2Txt.getLocalBounds();
    float tot2W = t2B.width + gGap + bW;
    float start2X = tab2X + (tab2W - tot2W) / 2.f;
    t2Txt.setPosition(std::round(start2X - t2B.left), std::round(tabY + (tabH - t2B.height) / 2.f - t2B.top));
    window.draw(t2Txt);

    float b2X = std::round(start2X + t2B.width + gGap);
    float b2Y = std::round(tabY + (tabH - bH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(b2X, b2Y),
        sf::Vector2f(bW, bH),
        11.f,
        (fnbSubTab == 1) ? sf::Color(3, 105, 161) : sf::Color(226, 232, 240)
    );
    sf::Text b2Txt;
    b2Txt.setFont(Theme::FontPrimary);
    b2Txt.setCharacterSize(12);
    b2Txt.setStyle(sf::Text::Bold);
    b2Txt.setFillColor((fnbSubTab == 1) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream b2Oss;
    b2Oss << std::setw(2) << std::setfill('0') << std::max(1, comboCount);
    b2Txt.setString(b2Oss.str());
    sf::FloatRect b2B = b2Txt.getLocalBounds();
    b2Txt.setPosition(std::round(b2X + (bW - b2B.width) / 2.f - b2B.left), std::round(b2Y + (bH - b2B.height) / 2.f - b2B.top));
    window.draw(b2Txt);

    // Top-right Search, Export & Quick Add
    if (fnbSearchInput) fnbSearchInput->draw(window);
    sf::CircleShape sLens(5.5f);
    sLens.setPosition(rightColX - 149.f, 40.f);
    sLens.setFillColor(sf::Color::Transparent);
    sLens.setOutlineThickness(1.6f);
    sLens.setOutlineColor(sf::Color(148, 163, 184));
    window.draw(sLens);
    sf::RectangleShape sHdl(sf::Vector2f(5.f, 1.6f));
    sHdl.setPosition(rightColX - 140.f, 50.f);
    sHdl.setRotation(45.f);
    sHdl.setFillColor(sf::Color(148, 163, 184));
    window.draw(sHdl);

    if (btnFnbExport) btnFnbExport->draw(window);
    RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 172.f, 39.f), sf::Vector2f(13.f, 15.f), 2.f, sf::Color(240, 249, 255), 1.3f, sf::Color(3, 105, 161));

    if (btnFnbQuickAdd) btnFnbQuickAdd->draw(window);

    // ==========================================
    // 3. LEFT CARD: Danh Mục Concession Rạp
    // ==========================================
    float cardY = 92.f;
    float cardH = 858.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, cardY),
        sf::Vector2f(leftColW, cardH),
        14.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Grid icon + Card Header Title
    for (int gy = 0; gy < 2; ++gy) {
        for (int gx = 0; gx < 2; ++gx) {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(leftColX + 24.f + gx * 8.f, cardY + 19.f + gy * 8.f),
                sf::Vector2f(6.f, 6.f),
                1.5f,
                sf::Color(3, 105, 161)
            );
        }
    }

    sf::Text cTitle;
    cTitle.setFont(Theme::FontPrimary);
    cTitle.setCharacterSize(18);
    cTitle.setStyle(sf::Text::Bold);
    cTitle.setFillColor(sf::Color(15, 23, 42));
    cTitle.setString(toSfString("Danh Mục Concession Rạp"));
    cTitle.setPosition(leftColX + 46.f, cardY + 15.f);
    window.draw(cTitle);

    sf::FloatRect ctB = cTitle.getLocalBounds();
    sf::Text cSub;
    cSub.setFont(Theme::FontPrimary);
    cSub.setCharacterSize(13);
    cSub.setFillColor(sf::Color(100, 116, 139));
    cSub.setString(toSfString("(Khu vực Hồ Chí Minh)"));
    cSub.setPosition(leftColX + 56.f + ctB.width, cardY + 19.f);
    window.draw(cSub);

    sf::Text sLbl;
    sLbl.setFont(Theme::FontPrimary);
    sLbl.setCharacterSize(13);
    sLbl.setFillColor(sf::Color(100, 116, 139));
    sLbl.setString(toSfString("Sắp xếp theo:"));
    sLbl.setPosition(leftColX + leftColW - 232.f, cardY + 19.f);
    window.draw(sLbl);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX + leftColW - 142.f, cardY + 14.f),
        sf::Vector2f(118.f, 30.f),
        7.f,
        sf::Color(248, 250, 252),
        1.f,
        sf::Color(226, 232, 240)
    );
    sf::Text sBtnTxt;
    sBtnTxt.setFont(Theme::FontPrimary);
    sBtnTxt.setCharacterSize(12);
    sBtnTxt.setStyle(sf::Text::Bold);
    sBtnTxt.setFillColor(sf::Color(30, 41, 59));
    sBtnTxt.setString(toSfString(fnbSortAsc ? "Mã sản phẩm ↓" : "Mã sản phẩm ↑"));
    sf::FloatRect sbB = sBtnTxt.getLocalBounds();
    sBtnTxt.setPosition(
        std::round(leftColX + leftColW - 142.f + (118.f - sbB.width) / 2.f - sbB.left),
        std::round(cardY + 14.f + (30.f - sbB.height) / 2.f - sbB.top)
    );
    window.draw(sBtnTxt);

    // Table Header Bar
    float tblHeaderY = cardY + 56.f;
    sf::RectangleShape thBg(sf::Vector2f(leftColW - 2.f, 42.f));
    thBg.setPosition(leftColX + 1.f, tblHeaderY);
    thBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(thBg);

    std::vector<std::string> cols = {"HÌNH ẢNH", "MÃ SP", "TÊN & QUY CÁCH", "GIÁ BÁN", "PHÂN LOẠI", "TRẠNG THÁI"};
    std::vector<float> colX = {
        leftColX + 24.f,
        leftColX + 108.f,
        leftColX + 210.f,
        leftColX + 510.f,
        leftColX + 635.f,
        leftColX + 782.f
    };

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(12);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(sf::Color(100, 116, 139));
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblHeaderY + 13.f);
        window.draw(ch);
    }

    // Filter & sort F&B items
    std::string q = fnbSearchInput ? fnbSearchInput->getText() : "";
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    std::vector<int> visibleIdx;
    for (size_t i = 0; i < fnbList.size(); ++i) {
        if (fnbSubTab == 1 && fnbList[i].getLoai() != "Combo") continue;
        if (!q.empty()) {
            std::string cLow = fnbList[i].getMaMatHang();
            std::string nLow = fnbList[i].getTenMatHang();
            std::transform(cLow.begin(), cLow.end(), cLow.begin(), ::tolower);
            std::transform(nLow.begin(), nLow.end(), nLow.begin(), ::tolower);
            if (cLow.find(q) == std::string::npos && nLow.find(q) == std::string::npos) continue;
        }
        visibleIdx.push_back(static_cast<int>(i));
    }
    if (!fnbSortAsc) std::reverse(visibleIdx.begin(), visibleIdx.end());

    // Helper for formatted price "25.000 đ"
    auto fmtShortPrice = [](double val) -> std::string {
        long long v = static_cast<long long>(val);
        if (v < 0) v = 0;
        std::string s = std::to_string(v);
        int n = static_cast<int>(s.length()) - 3;
        while (n > 0) {
            s.insert(n, ".");
            n -= 3;
        }
        return s + " đ";
    };

    // Helper lambda to draw an F&B Image Slot (loads real image if present, or clean photo-frame placeholder slot)
    auto drawFnbImageBox = [&](const std::string& itemCode, const std::string& customPath, float bx, float by, float bw, float bh, float radius, bool isLargePreview) {
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(bx, by),
            sf::Vector2f(bw, bh),
            radius,
            sf::Color(240, 249, 255),
            1.f,
            sf::Color(186, 230, 253)
        );

        std::shared_ptr<sf::Texture> tex = nullptr;
        auto it = fnbImageTextures.find(itemCode);
        if (it != fnbImageTextures.end() && it->second && it->second->getSize().x > 0) {
            tex = it->second;
        }
        if (!customPath.empty()) {
            std::string loadPath = customPath;
            if (loadPath.find('/') == std::string::npos && loadPath.find('\\') == std::string::npos) {
                loadPath = "assets/fnb/" + loadPath;
            }
            std::ifstream f(loadPath.c_str(), std::ios::binary);
            if (f.good()) {
                f.close();
                std::shared_ptr<sf::Texture> loaded(new sf::Texture());
                if (loaded->loadFromFile(loadPath)) {
                    loaded->setSmooth(true);
                    fnbImageTextures[itemCode] = loaded;
                    tex = loaded;
                }
            }
        }
        if (!tex && !itemCode.empty()) {
            std::string defPath = "assets/fnb/" + itemCode + ".jpg";
            std::ifstream df(defPath.c_str(), std::ios::binary);
            if (df.good()) {
                df.close();
                std::shared_ptr<sf::Texture> loaded(new sf::Texture());
                if (loaded->loadFromFile(defPath)) {
                    loaded->setSmooth(true);
                    fnbImageTextures[itemCode] = loaded;
                    tex = loaded;
                }
            }
        }

        if (tex && tex->getSize().x > 0 && tex->getSize().y > 0) {
            sf::Sprite sp(*tex);
            float pad = 2.f;
            float sx = (bw - pad * 2.f) / static_cast<float>(tex->getSize().x);
            float sy = (bh - pad * 2.f) / static_cast<float>(tex->getSize().y);
            sp.setScale(sx, sy);
            sp.setPosition(bx + pad, by + pad);
            window.draw(sp);
        } else {
            // Clean Square Photo / Image Placeholder Icon (scaled proportionally for both 68x68 and 104x104 square frames)
            float scale = isLargePreview ? 1.45f : 1.0f;
            float icW = std::round(32.f * scale);
            float icH = std::round(24.f * scale);
            float icX = std::round(bx + (bw - icW) / 2.f);
            float icY = std::round(by + (bh - icH) / 2.f);

            RoundedRectRenderer::render(
                window,
                sf::Vector2f(icX, icY),
                sf::Vector2f(icW, icH),
                4.f * scale,
                sf::Color::White,
                1.5f * scale,
                sf::Color(2, 132, 199)
            );
            // Sun dot inside photo frame
            sf::CircleShape sunDot(2.8f * scale);
            sunDot.setFillColor(sf::Color(56, 189, 248));
            sunDot.setPosition(icX + 5.f * scale, icY + 4.5f * scale);
            window.draw(sunDot);

            // Mountain landscape peak inside photo frame
            sf::ConvexShape mtn(4);
            mtn.setPoint(0, sf::Vector2f(icX + 3.f * scale, icY + icH - 2.5f * scale));
            mtn.setPoint(1, sf::Vector2f(icX + 13.f * scale, icY + 10.f * scale));
            mtn.setPoint(2, sf::Vector2f(icX + 21.f * scale, icY + 14.f * scale));
            mtn.setPoint(3, sf::Vector2f(icX + icW - 3.f * scale, icY + icH - 2.5f * scale));
            mtn.setFillColor(sf::Color(125, 211, 252));
            window.draw(mtn);
        }
    };

    float rowStartY = tblHeaderY + 42.f;
    float rowH = 82.f;
    int shownCount = 0;

    for (size_t v = 0; v < visibleIdx.size() && v < 8; ++v) {
        int idx = visibleIdx[v];
        float ry = rowStartY + static_cast<float>(v) * rowH;
        bool isSel = (selectedFnbRowIdx == idx);
        const FoodAndBeverage& item = fnbList[idx];
        std::string code = item.getMaMatHang();
        std::string name = item.getTenMatHang();
        std::string loai = item.getLoai();
        bool isSelling = (item.getTrangThaiKinhDoanh() == "DangBan");

        sf::RectangleShape rBg(sf::Vector2f(leftColW - 2.f, rowH - 1.f));
        rBg.setPosition(leftColX + 1.f, ry);
        rBg.setFillColor(isSel ? sf::Color(240, 249, 255) : sf::Color::White);
        window.draw(rBg);

        if (isSel) {
            sf::RectangleShape leftBar(sf::Vector2f(4.f, rowH - 1.f));
            leftBar.setPosition(leftColX + 1.f, ry);
            leftBar.setFillColor(sf::Color(2, 132, 199));
            window.draw(leftBar);
        }

        sf::RectangleShape rowDiv(sf::Vector2f(leftColW - 32.f, 1.f));
        rowDiv.setPosition(leftColX + 16.f, ry + rowH - 1.f);
        rowDiv.setFillColor(sf::Color(241, 245, 249));
        window.draw(rowDiv);

        // 1. HÌNH ẢNH Large Thumbnail Slot (68x68, clean without size badge)
        float tx = colX[0] - 4.f, ty = ry + 7.f;
        std::string rowImgPath = fnbImagePaths.count(code) ? fnbImagePaths[code] : "";
        drawFnbImageBox(code, rowImgPath, tx, ty, 68.f, 68.f, 12.f, false);

        // 2. MÃ SP
        float maxCodeW = colX[2] - colX[1] - 10.f;
        std::size_t codeLineCnt = 1;
        sf::String wrappedCode = wrapSfTextString(
            Theme::FontPrimary,
            toSfString(code),
            13,
            maxCodeW,
            2,
            sf::Text::Bold,
            &codeLineCnt
        );
        sf::Text cCode;
        cCode.setFont(Theme::FontPrimary);
        cCode.setCharacterSize(13);
        cCode.setStyle(sf::Text::Bold);
        cCode.setLineSpacing(0.90f);
        cCode.setFillColor(sf::Color(3, 105, 161));
        cCode.setString(wrappedCode);
        cCode.setPosition(colX[1], ry + (codeLineCnt > 1 ? 21.f : 30.f));
        window.draw(cCode);

        // 3. TÊN & QUY CÁCH (Large Bold Name + Specification Line, wrapped cleanly inside column)
        float maxFnbNameW = colX[3] - colX[2] - 24.f;
        std::size_t fnbNameLines = 1;
        sf::String wrappedFnbName = wrapSfTextString(
            Theme::FontPrimary,
            toSfString(name),
            15,
            maxFnbNameW,
            2,
            sf::Text::Bold,
            &fnbNameLines
        );
        sf::Text cName;
        cName.setFont(Theme::FontPrimary);
        cName.setCharacterSize(15);
        cName.setStyle(sf::Text::Bold);
        cName.setLineSpacing(0.88f);
        cName.setFillColor(isSel ? sf::Color(3, 105, 161) : sf::Color(15, 23, 42));
        cName.setString(wrappedFnbName);
        cName.setPosition(colX[2], ry + (fnbNameLines > 1 ? 10.f : 16.f));
        window.draw(cName);

        std::string specDesc = "Phục vụ tiêu chuẩn tại quầy Cinestar";
        if (code == "FB01") specDesc = "Nước giải khát có ga • Cốc giấy lạnh";
        else if (code == "FB02") specDesc = "Vị cam hoặc chanh • Rót tươi tại vòi";
        else if (code == "FB03") specDesc = "Bỏng ngô hạt nấm phủ caramel vàng giòn";
        else if (code == "PRD001") specDesc = "Xô 64oz bắp nóng giòn thơm nức";
        else if (code == "PRD002") specDesc = "Xô 32oz phần cá nhân tiêu chuẩn";
        else if (code == "PRD003") specDesc = "Ly 32oz giữ lạnh 120 phút";
        else if (code == "PRD004") specDesc = "Ly 22oz dung tích phổ thông";
        else if (loai == "Combo") specDesc = "1 Bắp lớn ngọt + 1 Nước ngọt lớn • Tiết kiệm 15k";
        else if (loai == "Bap") specDesc = "Bắp rang bơ thơm giòn nóng hổi";
        else if (loai == "Nuoc") specDesc = "Nước giải khát ướp lạnh tiêu chuẩn";

        std::size_t fnbSpecLines = 1;
        sf::String wrappedSpec = wrapSfTextString(
            Theme::FontPrimary,
            toSfString(specDesc),
            12,
            maxFnbNameW,
            (fnbNameLines > 1) ? 1 : 2,
            sf::Text::Regular,
            &fnbSpecLines
        );
        sf::Text cSpec;
        cSpec.setFont(Theme::FontPrimary);
        cSpec.setCharacterSize(12);
        cSpec.setLineSpacing(0.88f);
        cSpec.setFillColor(sf::Color(100, 116, 139));
        cSpec.setString(wrappedSpec);
        cSpec.setPosition(colX[2], ry + (fnbNameLines > 1 ? 49.f : 40.f));
        window.draw(cSpec);

        // 4. GIÁ BÁN (Large Bold Price)
        sf::Text cPrice;
        cPrice.setFont(Theme::FontPrimary);
        cPrice.setCharacterSize(16);
        cPrice.setStyle(sf::Text::Bold);
        cPrice.setFillColor(sf::Color(3, 105, 161));
        cPrice.setString(toSfString(fmtShortPrice(item.getGiaTien())));
        cPrice.setPosition(colX[3], ry + 29.f);
        window.draw(cPrice);

        // 5. PHÂN LOẠI Pill Badge
        bool isCombo = (loai == "Combo");
        std::string catText = isCombo ? "Combo tiết kiệm" : "Món lẻ";
        float catW = isCombo ? 118.f : 76.f;
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(colX[4], ry + 26.f),
            sf::Vector2f(catW, 28.f),
            14.f,
            isCombo ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249)
        );
        sf::Text cCat;
        cCat.setFont(Theme::FontPrimary);
        cCat.setCharacterSize(12);
        cCat.setStyle(sf::Text::Bold);
        cCat.setFillColor(isCombo ? sf::Color(3, 105, 161) : sf::Color(51, 65, 85));
        cCat.setString(toSfString(catText));
        sf::FloatRect catB = cCat.getLocalBounds();
        cCat.setPosition(
            std::round(colX[4] + (catW - catB.width) / 2.f - catB.left),
            std::round(ry + 26.f + (28.f - catB.height) / 2.f - catB.top)
        );
        window.draw(cCat);

        // 6. TRẠNG THÁI Clickable Pill Badge ("● Đang bán" / "● Ngừng bán")
        sf::Color stBg = isSelling ? sf::Color(224, 242, 254) : sf::Color(255, 228, 230);
        sf::Color stFg = isSelling ? sf::Color(3, 105, 161) : sf::Color(225, 29, 72);
        RoundedRectRenderer::render(window, sf::Vector2f(colX[5], ry + 25.f), sf::Vector2f(108.f, 30.f), 15.f, stBg);

        sf::CircleShape stDot(3.5f);
        stDot.setFillColor(stFg);
        stDot.setPosition(colX[5] + 12.f, ry + 36.5f);
        window.draw(stDot);

        sf::Text cSt;
        cSt.setFont(Theme::FontPrimary);
        cSt.setCharacterSize(12);
        cSt.setStyle(sf::Text::Bold);
        cSt.setFillColor(stFg);
        cSt.setString(toSfString(isSelling ? "Đang bán" : "Ngừng bán"));
        cSt.setPosition(colX[5] + 24.f, ry + 31.f);
        window.draw(cSt);

        shownCount++;
    }

    // Left Card Footer ("Hiển thị 8 trên 8 sản phẩm F&B" + [<] [1] [>])
    float footerY = cardY + cardH - 42.f;
    sf::Text fnbFoot;
    fnbFoot.setFont(Theme::FontPrimary);
    fnbFoot.setCharacterSize(13);
    fnbFoot.setFillColor(sf::Color(100, 116, 139));
    fnbFoot.setString(toSfString("Hiển thị " + std::to_string(shownCount) + " trên " + std::to_string(fnbList.size()) + " sản phẩm F&B"));
    fnbFoot.setPosition(leftColX + 24.f, footerY + 4.f);
    window.draw(fnbFoot);

    float pgX = leftColX + leftColW - 118.f;
    RoundedRectRenderer::render(window, sf::Vector2f(pgX, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
    sf::Text pgPrev("<", Theme::FontPrimary, 13);
    pgPrev.setStyle(sf::Text::Bold);
    pgPrev.setFillColor(sf::Color(148, 163, 184));
    pgPrev.setPosition(pgX + 10.f, footerY + 5.f);
    window.draw(pgPrev);

    RoundedRectRenderer::render(window, sf::Vector2f(pgX + 34.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color(3, 105, 161));
    sf::Text pgOne("1", Theme::FontPrimary, 13);
    pgOne.setStyle(sf::Text::Bold);
    pgOne.setFillColor(sf::Color::White);
    pgOne.setPosition(pgX + 44.f, footerY + 5.f);
    window.draw(pgOne);

    RoundedRectRenderer::render(window, sf::Vector2f(pgX + 68.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
    sf::Text pgNext(">", Theme::FontPrimary, 13);
    pgNext.setStyle(sf::Text::Bold);
    pgNext.setFillColor(sf::Color(148, 163, 184));
    pgNext.setPosition(pgX + 78.f, footerY + 5.f);
    window.draw(pgNext);

    // ==========================================
    // 4. RIGHT CARD: Thông tin sản phẩm F&B
    // ==========================================
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, cardY),
        sf::Vector2f(rightColW, cardH),
        14.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    // Header Icon + Title + Mode Subtitle + [Xem / Sửa] Pill
    RoundedRectRenderer::render(window, sf::Vector2f(gInputX, cardY + 18.f), sf::Vector2f(40.f, 40.f), 10.f, sf::Color(224, 242, 254));
    for (int sl = 0; sl < 3; ++sl) {
        sf::RectangleShape sBar(sf::Vector2f(16.f, 2.f));
        sBar.setPosition(gInputX + 12.f, cardY + 29.f + sl * 7.f);
        sBar.setFillColor(sf::Color(3, 105, 161));
        window.draw(sBar);
        sf::CircleShape sKnob(2.5f);
        sKnob.setFillColor(sf::Color(3, 105, 161));
        sKnob.setPosition(gInputX + 15.f + (sl % 2) * 6.f, cardY + 27.5f + sl * 7.f);
        window.draw(sKnob);
    }

    sf::Text rTitle;
    rTitle.setFont(Theme::FontPrimary);
    rTitle.setCharacterSize(18);
    rTitle.setStyle(sf::Text::Bold);
    rTitle.setFillColor(sf::Color(15, 23, 42));
    rTitle.setString(toSfString("Thông tin sản phẩm F&B"));
    rTitle.setPosition(gInputX + 52.f, cardY + 16.f);
    window.draw(rTitle);

    bool isEditingFnb = (selectedFnbRowIdx >= 0 && selectedFnbRowIdx < static_cast<int>(fnbList.size()));
    std::string modeSubStr = isEditingFnb
        ? ("Chế độ: Đang chỉnh sửa (" + fnbList[selectedFnbRowIdx].getMaMatHang() + ")")
        : "Chế độ: Thêm sản phẩm mới";
    sf::Text rSub;
    rSub.setFont(Theme::FontPrimary);
    rSub.setCharacterSize(13);
    rSub.setStyle(sf::Text::Bold);
    rSub.setFillColor(isEditingFnb ? sf::Color(3, 105, 161) : sf::Color(21, 128, 61));
    rSub.setString(toSfString(modeSubStr));
    rSub.setPosition(gInputX + 52.f, cardY + 41.f);
    window.draw(rSub);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX + rightColW - 106.f, cardY + 22.f),
        sf::Vector2f(82.f, 28.f),
        8.f,
        sf::Color(240, 249, 255),
        1.f,
        sf::Color(186, 230, 253)
    );
    sf::Text modePillTxt;
    modePillTxt.setFont(Theme::FontPrimary);
    modePillTxt.setCharacterSize(12);
    modePillTxt.setStyle(sf::Text::Bold);
    modePillTxt.setFillColor(sf::Color(3, 105, 161));
    modePillTxt.setString(toSfString(isEditingFnb ? "Xem / Sửa" : "Thêm mới"));
    sf::FloatRect mpB = modePillTxt.getLocalBounds();
    modePillTxt.setPosition(
        std::round(rightColX + rightColW - 106.f + (82.f - mpB.width) / 2.f - mpB.left),
        std::round(cardY + 22.f + (28.f - mpB.height) / 2.f - mpB.top)
    );
    window.draw(modePillTxt);

    // Large Square Image Slot (96x96, same square style as left table) & Path Input
    sf::Text banLbl;
    banLbl.setFont(Theme::FontPrimary);
    banLbl.setCharacterSize(12);
    banLbl.setStyle(sf::Text::Bold);
    banLbl.setFillColor(sf::Color(71, 85, 105));
    banLbl.setString(toSfString("HÌNH ẢNH MÓN ĂN / COMBO QUẦY"));
    banLbl.setPosition(gInputX, cardY + 68.f);
    window.draw(banLbl);

    // 96x96 Large Square Product Image Frame (same clean square style as Left Table, larger size)
    std::string previewCode = inputFnbId ? inputFnbId->getText() : "PRD005";
    std::string previewPath = inputFnbImage ? inputFnbImage->getText() : "";
    drawFnbImageBox(previewCode, previewPath, gInputX, 182.f, 96.f, 96.f, 14.f, true);

    // Label, Image File (.jpg) Input Box & Hint right beside the 96x96 Square Image Frame
    sf::Text pathLbl;
    pathLbl.setFont(Theme::FontPrimary);
    pathLbl.setCharacterSize(12);
    pathLbl.setStyle(sf::Text::Bold);
    pathLbl.setFillColor(sf::Color(3, 105, 161));
    pathLbl.setString(toSfString("Tệp hình ảnh (.jpg):"));
    pathLbl.setPosition(gInputX + 114.f, 188.f);
    window.draw(pathLbl);

    if (inputFnbImage) inputFnbImage->draw(window);
    RoundedRectRenderer::render(window, sf::Vector2f(gInputX + 126.f, 224.f), sf::Vector2f(16.f, 14.f), 2.5f, sf::Color::White, 1.3f, sf::Color(2, 132, 199));
    sf::CircleShape miniSun(1.6f);
    miniSun.setFillColor(sf::Color(2, 132, 199));
    miniSun.setPosition(gInputX + 129.f, 227.f);
    window.draw(miniSun);

    sf::Text imgHint;
    imgHint.setFont(Theme::FontPrimary);
    imgHint.setCharacterSize(11);
    imgHint.setFillColor(sf::Color(100, 116, 139));
    std::string hintExample = previewCode.empty() ? "RB01.jpg" : (previewCode + ".jpg");
    imgHint.setString(toSfString("Đặt ảnh vào assets/fnb/ (VD: " + hintExample + ")"));
    imgHint.setPosition(gInputX + 114.f, 258.f);
    window.draw(imgHint);

    // Row 1: MÃ SẢN PHẨM & HÌNH THỨC PHỤC VỤ
    sf::Text lblCode;
    lblCode.setFont(Theme::FontPrimary);
    lblCode.setCharacterSize(12);
    lblCode.setStyle(sf::Text::Bold);
    lblCode.setFillColor(sf::Color(71, 85, 105));
    if (isEditingFnb) {
        lblCode.setString(toSfString("MÃ SẢN PHẨM (CỐ ĐỊNH)"));
    } else {
        lblCode.setString(toSfString("MÃ SẢN PHẨM *"));
    }
    lblCode.setPosition(gInputX, 294.f);
    window.draw(lblCode);

    if (inputFnbId) {
        if (isEditingFnb) {
            inputFnbId->setEnabled(false);
            inputFnbId->setColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(226, 232, 240), sf::Color(100, 116, 139), sf::Color(148, 163, 184));
        } else {
            inputFnbId->setEnabled(true);
            inputFnbId->setColors(sf::Color(248, 250, 252), sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
        }
        inputFnbId->draw(window);
    }

    sf::Text lblType;
    lblType.setFont(Theme::FontPrimary);
    lblType.setCharacterSize(12);
    lblType.setStyle(sf::Text::Bold);
    lblType.setFillColor(sf::Color(71, 85, 105));
    lblType.setString(toSfString("HÌNH THỨC PHỤC VỤ"));
    lblType.setPosition(gInputX + 220.f, 294.f);
    window.draw(lblType);

    // Segmented Toggle: [ Món lẻ | Combo gói ]
    float segX = gInputX + 220.f;
    float segY = 318.f;
    float segW = 218.f;
    float segH = 42.f;
    RoundedRectRenderer::render(window, sf::Vector2f(segX, segY), sf::Vector2f(segW, segH), 10.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));

    bool isComboMode = (fnbFormLoai == "Combo");
    float halfSegW = (segW - 8.f) / 2.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(isComboMode ? (segX + 4.f + halfSegW) : (segX + 4.f), segY + 4.f),
        sf::Vector2f(halfSegW, segH - 8.f),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(186, 230, 253)
    );

    sf::Text seg1Txt;
    seg1Txt.setFont(Theme::FontPrimary);
    seg1Txt.setCharacterSize(13);
    seg1Txt.setStyle(sf::Text::Bold);
    seg1Txt.setFillColor(!isComboMode ? sf::Color(3, 105, 161) : sf::Color(100, 116, 139));
    seg1Txt.setString(toSfString(fnbFormLoai == "Nuoc" ? "Món lẻ (Nước)" : "Món lẻ"));
    sf::FloatRect s1B = seg1Txt.getLocalBounds();
    seg1Txt.setPosition(std::round(segX + 4.f + (halfSegW - s1B.width) / 2.f - s1B.left), std::round(segY + (segH - s1B.height) / 2.f - s1B.top));
    window.draw(seg1Txt);

    sf::Text seg2Txt;
    seg2Txt.setFont(Theme::FontPrimary);
    seg2Txt.setCharacterSize(13);
    seg2Txt.setStyle(sf::Text::Bold);
    seg2Txt.setFillColor(isComboMode ? sf::Color(3, 105, 161) : sf::Color(100, 116, 139));
    seg2Txt.setString(toSfString("Combo gói"));
    sf::FloatRect s2B = seg2Txt.getLocalBounds();
    seg2Txt.setPosition(std::round(segX + 4.f + halfSegW + (halfSegW - s2B.width) / 2.f - s2B.left), std::round(segY + (segH - s2B.height) / 2.f - s2B.top));
    window.draw(seg2Txt);

    // Row 2: TÊN SẢN PHẨM HIỂN THỊ QUẦY
    sf::Text lblName;
    lblName.setFont(Theme::FontPrimary);
    lblName.setCharacterSize(12);
    lblName.setStyle(sf::Text::Bold);
    lblName.setFillColor(sf::Color(71, 85, 105));
    lblName.setString(toSfString("TÊN SẢN PHẨM HIỂN THỊ QUẦY"));
    lblName.setPosition(gInputX, 378.f);
    window.draw(lblName);

    if (inputFnbName) inputFnbName->draw(window);

    float curRow4Y = 632.f;
    float curRow5Y = 716.f;

    // Row 3: Chi tiết định lượng thành phần Combo Box (Chỉ hiển thị khi là Combo)
    if (isComboMode) {
        float boxY = 462.f;
        float boxH = 152.f;
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX, boxY), sf::Vector2f(gInputW, boxH), 12.f, sf::Color(240, 249, 255), 1.f, sf::Color(224, 242, 254));

        sf::Text cBoxTitle;
        cBoxTitle.setFont(Theme::FontPrimary);
        cBoxTitle.setCharacterSize(13);
        cBoxTitle.setStyle(sf::Text::Bold);
        cBoxTitle.setFillColor(sf::Color(15, 23, 42));
        cBoxTitle.setString(toSfString("Chi tiết định lượng thành phần Combo"));
        cBoxTitle.setPosition(gInputX + 16.f, boxY + 12.f);
        window.draw(cBoxTitle);

        std::vector<int> retailIdxs = getRetailFnbIndices();

        // Món 1
        std::string name1 = "Bắp rang lớn (Vị ngọt bơ)";
        std::string unit1 = "01 Xô lớn 64oz";
        double price1 = 45000.0;
        if (!retailIdxs.empty()) {
            int idx1 = retailIdxs[comboItem1Idx % retailIdxs.size()];
            name1 = fnbList[idx1].getTenMatHang();
            price1 = fnbList[idx1].getGiaTien();
            unit1 = (fnbList[idx1].getLoai() == "Bap") ? "01 Xô lớn 64oz" : ((fnbList[idx1].getLoai() == "Nuoc") ? "01 Ly vòi 32oz" : "01 Phần tiêu chuẩn");
        }

        // Món 2
        std::string name2 = "Nước ngọt ly lớn (Coke/Sprite)";
        std::string unit2 = "01 Ly vòi 32oz";
        double price2 = 30000.0;
        if (!retailIdxs.empty()) {
            int rIdx2 = (retailIdxs.size() > 1) ? (comboItem2Idx % retailIdxs.size()) : 0;
            int idx2 = retailIdxs[rIdx2];
            name2 = fnbList[idx2].getTenMatHang();
            price2 = fnbList[idx2].getGiaTien();
            unit2 = (fnbList[idx2].getLoai() == "Bap") ? "01 Xô lớn 64oz" : ((fnbList[idx2].getLoai() == "Nuoc") ? "01 Ly vòi 32oz" : "01 Phần tiêu chuẩn");
        }

        // Tính toán chi phí và tiết kiệm
        double totalRetail = comboItem1Qty * price1 + comboItem2Qty * price2;
        std::string curPriceDigits = "";
        if (inputFnbPrice) {
            for (char c : inputFnbPrice->getText()) {
                if (c >= '0' && c <= '9') curPriceDigits += c;
            }
        }
        double curFormPrice = curPriceDigits.empty() ? 45000.0 : std::stod(curPriceDigits);
        double saving = (totalRetail > curFormPrice) ? (totalRetail - curFormPrice) : 0.0;

        // Badge Tiết kiệm
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 132.f, boxY + 9.f), sf::Vector2f(120.f, 22.f), 11.f, sf::Color(224, 242, 254));
        sf::Text saveBadge;
        saveBadge.setFont(Theme::FontPrimary);
        saveBadge.setCharacterSize(11);
        saveBadge.setStyle(sf::Text::Bold);
        saveBadge.setFillColor(sf::Color(3, 105, 161));
        saveBadge.setString(toSfString("Tiết kiệm " + fmtShortPrice(saving)));
        sf::FloatRect svB = saveBadge.getLocalBounds();
        saveBadge.setPosition(std::round(gInputX + gInputW - 132.f + (120.f - svB.width) / 2.f - svB.left), std::round(boxY + 9.f + (22.f - svB.height) / 2.f - svB.top));
        window.draw(saveBadge);

        // Sub-item 1 inside Combo box
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + 12.f, boxY + 38.f), sf::Vector2f(gInputW - 24.f, 38.f), 8.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        // Tên món 1 (có nút đổi món ◀ ▶)
        sf::Text sub1Title;
        sub1Title.setFont(Theme::FontPrimary);
        sub1Title.setCharacterSize(12);
        sub1Title.setStyle(sf::Text::Bold);
        sub1Title.setFillColor(sf::Color(15, 23, 42));
        sub1Title.setString(toSfString("◀  " + name1 + "  ▶"));
        sub1Title.setPosition(gInputX + 22.f, boxY + 42.f);
        window.draw(sub1Title);

        sf::Text sub1Unit;
        sub1Unit.setFont(Theme::FontPrimary);
        sub1Unit.setCharacterSize(11);
        sub1Unit.setFillColor(sf::Color(100, 116, 139));
        sub1Unit.setString(toSfString("Đơn vị: " + unit1 + " • " + fmtShortPrice(price1)));
        sub1Unit.setPosition(gInputX + 24.f, boxY + 58.f);
        window.draw(sub1Unit);

        sf::Text qtyLbl1;
        qtyLbl1.setFont(Theme::FontPrimary);
        qtyLbl1.setCharacterSize(12);
        qtyLbl1.setFillColor(sf::Color(100, 116, 139));
        qtyLbl1.setString(toSfString("Số lượng:"));
        qtyLbl1.setPosition(gInputX + gInputW - 138.f, boxY + 49.f);
        window.draw(qtyLbl1);

        // Stepper: [-] [qty1] [+]
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 74.f, boxY + 45.f), sf::Vector2f(22.f, 24.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
        sf::Text minus1("-", Theme::FontPrimary, 13);
        minus1.setStyle(sf::Text::Bold); minus1.setFillColor(sf::Color(71, 85, 105));
        minus1.setPosition(gInputX + gInputW - 68.f, boxY + 47.f);
        window.draw(minus1);

        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 48.f, boxY + 45.f), sf::Vector2f(24.f, 24.f), 5.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        sf::Text q1Val(std::to_string(comboItem1Qty), Theme::FontPrimary, 12);
        q1Val.setStyle(sf::Text::Bold);
        q1Val.setFillColor(sf::Color(3, 105, 161));
        sf::FloatRect q1B = q1Val.getLocalBounds();
        q1Val.setPosition(gInputX + gInputW - 48.f + (24.f - q1B.width) / 2.f, boxY + 49.f);
        window.draw(q1Val);

        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 20.f, boxY + 45.f), sf::Vector2f(22.f, 24.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
        sf::Text plus1("+", Theme::FontPrimary, 13);
        plus1.setStyle(sf::Text::Bold); plus1.setFillColor(sf::Color(71, 85, 105));
        plus1.setPosition(gInputX + gInputW - 14.f, boxY + 47.f);
        window.draw(plus1);

        // Sub-item 2 inside Combo box
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + 12.f, boxY + 80.f), sf::Vector2f(gInputW - 24.f, 38.f), 8.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        // Tên món 2 (có nút đổi món ◀ ▶)
        sf::Text sub2Title;
        sub2Title.setFont(Theme::FontPrimary);
        sub2Title.setCharacterSize(12);
        sub2Title.setStyle(sf::Text::Bold);
        sub2Title.setFillColor(sf::Color(15, 23, 42));
        sub2Title.setString(toSfString("◀  " + name2 + "  ▶"));
        sub2Title.setPosition(gInputX + 22.f, boxY + 84.f);
        window.draw(sub2Title);

        sf::Text sub2Unit;
        sub2Unit.setFont(Theme::FontPrimary);
        sub2Unit.setCharacterSize(11);
        sub2Unit.setFillColor(sf::Color(100, 116, 139));
        sub2Unit.setString(toSfString("Đơn vị: " + unit2 + " • " + fmtShortPrice(price2)));
        sub2Unit.setPosition(gInputX + 24.f, boxY + 100.f);
        window.draw(sub2Unit);

        sf::Text qtyLbl2;
        qtyLbl2.setFont(Theme::FontPrimary);
        qtyLbl2.setCharacterSize(12);
        qtyLbl2.setFillColor(sf::Color(100, 116, 139));
        qtyLbl2.setString(toSfString("Số lượng:"));
        qtyLbl2.setPosition(gInputX + gInputW - 138.f, boxY + 91.f);
        window.draw(qtyLbl2);

        // Stepper: [-] [qty2] [+]
        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 74.f, boxY + 87.f), sf::Vector2f(22.f, 24.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
        sf::Text minus2("-", Theme::FontPrimary, 13);
        minus2.setStyle(sf::Text::Bold); minus2.setFillColor(sf::Color(71, 85, 105));
        minus2.setPosition(gInputX + gInputW - 68.f, boxY + 89.f);
        window.draw(minus2);

        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 48.f, boxY + 87.f), sf::Vector2f(24.f, 24.f), 5.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
        sf::Text q2Val(std::to_string(comboItem2Qty), Theme::FontPrimary, 12);
        q2Val.setStyle(sf::Text::Bold);
        q2Val.setFillColor(sf::Color(3, 105, 161));
        sf::FloatRect q2B = q2Val.getLocalBounds();
        q2Val.setPosition(gInputX + gInputW - 48.f + (24.f - q2B.width) / 2.f, boxY + 91.f);
        window.draw(q2Val);

        RoundedRectRenderer::render(window, sf::Vector2f(gInputX + gInputW - 20.f, boxY + 87.f), sf::Vector2f(22.f, 24.f), 5.f, sf::Color(241, 245, 249), 1.f, sf::Color(203, 213, 225));
        sf::Text plus2("+", Theme::FontPrimary, 13);
        plus2.setStyle(sf::Text::Bold); plus2.setFillColor(sf::Color(71, 85, 105));
        plus2.setPosition(gInputX + gInputW - 14.f, boxY + 89.f);
        window.draw(plus2);

        // Bottom summary line inside Combo box
        sf::Text retailTxt;
        retailTxt.setFont(Theme::FontPrimary);
        retailTxt.setCharacterSize(12);
        retailTxt.setFillColor(sf::Color(100, 116, 139));
        retailTxt.setString(toSfString("Tổng giá trị tách lẻ: " + fmtShortPrice(totalRetail)));
        retailTxt.setPosition(gInputX + 14.f, boxY + 124.f);
        window.draw(retailTxt);

        sf::Text comboPriceTxt;
        comboPriceTxt.setFont(Theme::FontPrimary);
        comboPriceTxt.setCharacterSize(12);
        comboPriceTxt.setStyle(sf::Text::Bold);
        comboPriceTxt.setFillColor(sf::Color(3, 105, 161));
        comboPriceTxt.setString(toSfString("Giá bán ra: " + fmtShortPrice(curFormPrice)));
        sf::FloatRect cpB = comboPriceTxt.getLocalBounds();
        comboPriceTxt.setPosition(gInputX + gInputW - 14.f - cpB.width, boxY + 124.f);
        window.draw(comboPriceTxt);

        curRow4Y = 632.f;
        curRow5Y = 716.f;
    } else {
        // Khi chọn Món lẻ: Không hiện box định lượng, kéo các dòng bên dưới lên
        curRow4Y = 462.f;
        curRow5Y = 546.f;
    }

    // Row 4: GIÁ BÁN NIÊM YẾT (VNĐ) & TRẠNG THÁI KINH DOANH
    sf::Text lblPrice;
    lblPrice.setFont(Theme::FontPrimary);
    lblPrice.setCharacterSize(12);
    lblPrice.setStyle(sf::Text::Bold);
    lblPrice.setFillColor(sf::Color(71, 85, 105));
    lblPrice.setString(toSfString("GIÁ BÁN NIÊM YẾT (VNĐ)"));
    lblPrice.setPosition(gInputX, curRow4Y);
    window.draw(lblPrice);

    if (inputFnbPrice) {
        inputFnbPrice->setPosition(sf::Vector2f(gInputX, curRow4Y + 24.f));
        inputFnbPrice->draw(window);
    }

    sf::Text vndUnit;
    vndUnit.setFont(Theme::FontPrimary);
    vndUnit.setCharacterSize(12);
    vndUnit.setStyle(sf::Text::Bold);
    vndUnit.setFillColor(sf::Color(100, 116, 139));
    vndUnit.setString(toSfString("VNĐ"));
    vndUnit.setPosition(gInputX + 170.f, curRow4Y + 37.f);
    window.draw(vndUnit);

    sf::Text lblStatus;
    lblStatus.setFont(Theme::FontPrimary);
    lblStatus.setCharacterSize(12);
    lblStatus.setStyle(sf::Text::Bold);
    lblStatus.setFillColor(sf::Color(71, 85, 105));
    lblStatus.setString(toSfString("TRẠNG THÁI KINH DOANH"));
    lblStatus.setPosition(gInputX + 224.f, curRow4Y);
    window.draw(lblStatus);

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(gInputX + 224.f, curRow4Y + 24.f),
        sf::Vector2f(214.f, 42.f),
        10.f,
        sf::Color(248, 250, 252),
        1.f,
        sf::Color(226, 232, 240)
    );
    sf::Text stBoxTxt;
    stBoxTxt.setFont(Theme::FontPrimary);
    stBoxTxt.setCharacterSize(14);
    stBoxTxt.setStyle(sf::Text::Bold);
    stBoxTxt.setFillColor(fnbFormActive ? sf::Color(3, 105, 161) : sf::Color(225, 29, 72));
    stBoxTxt.setString(toSfString(fnbFormActive ? "Đang bán" : "Ngừng bán"));
    stBoxTxt.setPosition(gInputX + 238.f, curRow4Y + 35.f);
    window.draw(stBoxTxt);

    // Toggle Switch inside TRẠNG THÁI KINH DOANH box
    float swX = gInputX + 382.f;
    float swY = curRow4Y + 33.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(swX, swY),
        sf::Vector2f(44.f, 24.f),
        12.f,
        fnbFormActive ? sf::Color(2, 132, 199) : sf::Color(203, 213, 225)
    );
    sf::CircleShape swKnob(9.f);
    swKnob.setFillColor(sf::Color::White);
    swKnob.setPosition(fnbFormActive ? (swX + 23.f) : (swX + 3.f), swY + 3.f);
    window.draw(swKnob);

    // Row 5: GHI CHÚ QUẦY CONCESSION
    sf::Text lblNote;
    lblNote.setFont(Theme::FontPrimary);
    lblNote.setCharacterSize(12);
    lblNote.setStyle(sf::Text::Bold);
    lblNote.setFillColor(sf::Color(71, 85, 105));
    lblNote.setString(toSfString("GHI CHÚ QUẦY CONCESSION"));
    lblNote.setPosition(gInputX, curRow5Y);
    window.draw(lblNote);

    if (inputFnbNote) {
        inputFnbNote->setPosition(sf::Vector2f(gInputX, curRow5Y + 24.f));
        inputFnbNote->draw(window);
    }

    // 4 Action Buttons at bottom of Right Card
    if (btnFnbCreate) btnFnbCreate->draw(window);
    if (btnFnbDelete) btnFnbDelete->draw(window);
    if (btnFnbUpdate) btnFnbUpdate->draw(window);
    if (btnFnbClear) btnFnbClear->draw(window);
}

} // namespace UI