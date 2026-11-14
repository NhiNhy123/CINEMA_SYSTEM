#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace UI {
void CinemaUIApp::syncRoomSelectionToForm(int idx) {
    if (idx < 0 || idx >= static_cast<int>(roomList.size())) return;
    selectedRoomRowIdx = idx;
    const auto& r = roomList[idx];

    char uuidBuf[64];
    snprintf(uuidBuf, sizeof(uuidBuf), "63fcfa9d-abd9-4fce-8321-3f86a559%04x", ((idx + 1) * 0x1a2b) & 0xFFFF);
    if (inputRoomUUID) inputRoomUUID->setText(uuidBuf);

    if (inputRoomName) inputRoomName->setText(r.getTenPhong());
    roomFormCapacity = std::max(4, r.getSucChua());
    if (inputRoomCapacity) inputRoomCapacity->setText(std::to_string(roomFormCapacity));

    std::string lp = r.getLoaiPhong();
    std::string lpLow = lp;
    std::transform(lpLow.begin(), lpLow.end(), lpLow.begin(), ::tolower);
    if (lpLow.find("vip") != std::string::npos || lpLow.find("premium") != std::string::npos) {
        roomFormTechIdx = 1; // VIP
    } else if (lpLow.find("imax") != std::string::npos) {
        roomFormTechIdx = 2; // IMAX
    } else if (lpLow.find("4dx") != std::string::npos) {
        roomFormTechIdx = 3; // 4DX
    } else if (lpLow.find("10d") != std::string::npos || lpLow.find("vr") != std::string::npos) {
        roomFormTechIdx = 4; // 10D VR
    } else {
        roomFormTechIdx = 0; // Standard
    }

    roomFormStatusIdx = (r.getTrangThaiPhong() == "TamNgung") ? 1 : 0;
    if (roomStatusDropdown) roomStatusDropdown->setSelectedIndex(roomFormStatusIdx);
}

void CinemaUIApp::generateSeatsForRoom(const std::string& roomId, int capacity, const std::string& techType) {
    if (roomId.empty() || capacity <= 0) return;

    // Erase existing seats for this room
    allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(),
        [&roomId](const Seat& s) { return s.getMaPhong() == roomId; }), allSeats.end());

    int C = 10;
    if (capacity <= 12) C = 4;
    else if (capacity <= 24) C = 6;
    else if (capacity <= 48) C = 8;
    else if (capacity <= 80) C = 10;
    else if (capacity <= 130) C = 12;
    else C = 15;

    int R = (capacity + C - 1) / C;

    for (int i = 0; i < capacity; ++i) {
        int r = i / C;
        int c = i % C;
        char rowLetter = static_cast<char>('A' + r);
        std::string seatId = roomId + "_" + rowLetter + std::to_string(c + 1);

        bool isVip = (R >= 2 && r >= 1 && r < R && c >= C / 4 && c < C - C / 4);

        std::string loaiGhe = isVip ? "VIP" : "Standard";
        double donGia = isVip ? 95000.0 : 75000.0;
        if (techType == "IMAX") {
            donGia += 40000.0;
            loaiGhe = isVip ? "IMAX_VIP" : "IMAX_Standard";
        } else if (techType == "4DX") {
            donGia += 50000.0;
            loaiGhe = isVip ? "4DX_VIP" : "4DX_Standard";
        } else if (techType == "10D VR") {
            donGia += 60000.0;
            loaiGhe = isVip ? "10D_VIP" : "10D_Standard";
        }

        allSeats.push_back(Seat(seatId, roomId, loaiGhe, donGia, "Trong"));
    }

    db.ghiDanhSachGhe(allSeats);
}

void CinemaUIApp::setupRoomsView() {
    float leftColX = sidebarWidth + 24.f;  // 284.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1184.f
    float rightColW = 520.f;

    // 1. Top Header Action buttons
    btnRoomExportPDF = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xuất Danh Sách",
        sf::Vector2f(rightColX + rightColW - 344.f, 55.f),
        sf::Vector2f(158.f, 38.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnRoomExportPDF->setFontSize(13);
    btnRoomExportPDF->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomExportPDF->setOutline(1.f, sf::Color(203, 213, 225));
    btnRoomExportPDF->setTextColor(sf::Color(51, 65, 85));
    btnRoomExportPDF->setOnClick([this]() {
        std::ofstream out("reports/DanhSachPhong_SoDoGhe.csv");
        if (out.is_open()) {
            out << "\xEF\xBB\xBF"; // UTF-8 BOM
            out << "Mã Phòng,Tên Phòng,Loại Phòng,Sức Chứa,Trạng Thái\n";
            for (const auto& r : roomList) {
                out << r.getMaPhong() << ",\"" << r.getTenPhong() << "\"," << r.getLoaiPhong() << ","
                    << r.getSucChua() << "," << r.getTrangThaiPhong() << "\n";
            }
            out.close();
            if (confirmPopup) confirmPopup->show("Xuất thành công", "Đã xuất danh sách phòng và cấu hình ra file 'reports/DanhSachPhong_SoDoGhe.csv'!", ButtonStyle::Primary, "Đóng", "Quay lại");
        }
    });

    btnRoomAddNew = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm Phòng Mới",
        sf::Vector2f(rightColX + rightColW - 174.f, 55.f),
        sf::Vector2f(158.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnRoomAddNew->setFontSize(13);
    btnRoomAddNew->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnRoomAddNew->setOnClick([this]() {
        selectedRoomRowIdx = -1;
        char uuidBuf[64];
        snprintf(uuidBuf, sizeof(uuidBuf), "63fcfa9d-abd9-4fce-8321-3f86a559%04x", (static_cast<int>(roomList.size()) + 1) * 0x1234 & 0xFFFF);
        if (inputRoomUUID) inputRoomUUID->setText(uuidBuf);
        if (inputRoomName) {
            char nameBuf[48];
            snprintf(nameBuf, sizeof(nameBuf), "Phòng %d - Standard", static_cast<int>(roomList.size()) + 1);
            inputRoomName->setText(nameBuf);
            inputRoomName->setFocused(true);
        }
        roomFormCapacity = 100;
        if (inputRoomCapacity) inputRoomCapacity->setText("100");
        roomFormTechIdx = 0;
        roomFormStatusIdx = 0;
        if (roomStatusDropdown) roomStatusDropdown->setSelectedIndex(0);
    });

    // 2. Left Card Search & Filter Bar (inside Left Card at y = 166.f)
    roomSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 20.f, 166.f),
        sf::Vector2f(226.f, 38.f),
        "Tìm phòng...",
        13
    ));
    roomSearchInput->setTextInset(34.f);
    roomSearchInput->setColors(sf::Color::White, sf::Color(226, 232, 240), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    roomSearchInput->setCornerRadius(8.f);

    auto makeFilterBtn = [&](const std::string& txt, float x, float w, int filterIdx) {
        auto b = std::unique_ptr<RoundRectButton>(new RoundRectButton(
            txt,
            sf::Vector2f(x, 166.f),
            sf::Vector2f(w, 38.f),
            ButtonStyle::Ghost,
            8.f
        ));
        b->setFontSize(12);
        b->setOnClick([this, filterIdx]() {
            roomFilterType = filterIdx;
            roomCurrentPage = 0;
        });
        return b;
    };

    // Note: Do NOT display count numbers on filter buttons per user instruction
    btnRoomFilterAll = makeFilterBtn("Tất cả", leftColX + 258.f, 84.f, 0);
    btnRoomFilterStandard = makeFilterBtn("Standard", leftColX + 350.f, 96.f, 1);
    btnRoomFilterPremium = makeFilterBtn("Premium", leftColX + 454.f, 96.f, 2);
    btnRoomFilterSpecial = makeFilterBtn("Đặc biệt (IMAX/4DX/10D)", leftColX + 558.f, 194.f, 3);

    // 3. Right Column Form Inputs (matching reference mockup & Room.cpp backend)
    float formX = rightColX + 24.f;
    float formW = rightColW - 48.f; // 472.f

    inputRoomUUID = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formX, 190.f),
        sf::Vector2f(formW, 38.f),
        "UUID phòng...",
        12
    ));
    inputRoomUUID->setColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(100, 116, 139), sf::Color(148, 163, 184));
    inputRoomUUID->setCornerRadius(8.f);
    inputRoomUUID->setTextInset(12.f);

    inputRoomName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formX, 250.f),
        sf::Vector2f(formW, 44.f),
        "Nhập tên phòng chiếu...",
        14
    ));
    inputRoomName->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputRoomName->setCornerRadius(8.f);
    inputRoomName->setTextInset(14.f);

    // Capacity editable input + stepper buttons
    inputRoomCapacity = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(formX, 340.f),
        sf::Vector2f(formW - 96.f, 44.f),
        "Nhập số ghế...",
        14
    ));
    inputRoomCapacity->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputRoomCapacity->setCornerRadius(8.f);
    inputRoomCapacity->setTextInset(14.f);

    btnRoomCapMinus = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "-",
        sf::Vector2f(formX + formW - 88.f, 340.f),
        sf::Vector2f(42.f, 44.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnRoomCapMinus->setFontSize(17);
    btnRoomCapMinus->setCustomColors(sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomCapMinus->setOutline(1.f, sf::Color(203, 213, 225));
    btnRoomCapMinus->setTextColor(sf::Color(51, 65, 85));
    btnRoomCapMinus->setOnClick([this]() {
        if (inputRoomCapacity) {
            int parsed = 0;
            if (sscanf(inputRoomCapacity->getText().c_str(), "%d", &parsed) == 1 && parsed > 0) {
                roomFormCapacity = parsed;
            }
        }
        if (roomFormCapacity > 4) {
            int step = (roomFormCapacity <= 24) ? 2 : 4;
            roomFormCapacity = std::max(4, roomFormCapacity - step);
            if (inputRoomCapacity) inputRoomCapacity->setText(std::to_string(roomFormCapacity));
        }
    });

    btnRoomCapPlus = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+",
        sf::Vector2f(formX + formW - 42.f, 340.f),
        sf::Vector2f(42.f, 44.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnRoomCapPlus->setFontSize(17);
    btnRoomCapPlus->setCustomColors(sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomCapPlus->setOutline(1.f, sf::Color(203, 213, 225));
    btnRoomCapPlus->setTextColor(sf::Color(51, 65, 85));
    btnRoomCapPlus->setOnClick([this]() {
        if (inputRoomCapacity) {
            int parsed = 0;
            if (sscanf(inputRoomCapacity->getText().c_str(), "%d", &parsed) == 1 && parsed > 0) {
                roomFormCapacity = parsed;
            }
        }
        if (roomFormCapacity < 300) {
            int step = (roomFormCapacity < 24) ? 2 : 4;
            roomFormCapacity = std::min(300, roomFormCapacity + step);
            if (inputRoomCapacity) inputRoomCapacity->setText(std::to_string(roomFormCapacity));
        }
    });

    // 5 Technology Options (Tall rectangular cards: circle on top, label on bottom)
    btnRoomTechOptions.clear();
    std::vector<std::string> techNames = {"Standard", "VIP", "IMAX", "4DX", "10D VR"};
    float techGap = 10.f;
    float techW = (formW - 4.f * techGap) / 5.f; // 86.4f
    for (size_t t = 0; t < techNames.size(); ++t) {
        auto tb = std::unique_ptr<RoundRectButton>(new RoundRectButton(
            techNames[t],
            sf::Vector2f(formX + t * (techW + techGap), 434.f),
            sf::Vector2f(techW, 74.f),
            ButtonStyle::Ghost,
            10.f
        ));
        tb->setFontSize(12);
        int tIdx = static_cast<int>(t);
        tb->setOnClick([this, tIdx]() {
            roomFormTechIdx = tIdx;
            if (selectedRoomRowIdx < 0) {
                if (tIdx == 4) roomFormCapacity = 12;
                else if (tIdx == 3) roomFormCapacity = 60;
                else if (tIdx == 2) roomFormCapacity = 60;
                else if (tIdx == 1) roomFormCapacity = 80;
                else roomFormCapacity = 120;
                if (inputRoomCapacity) inputRoomCapacity->setText(std::to_string(roomFormCapacity));
            }
        });
        btnRoomTechOptions.push_back(std::move(tb));
    }

    // Field 4: Trạng thái hoạt động (matching Room.cpp suaPhong option 4: HoatDong / TamNgung)
    roomStatusDropdown = std::unique_ptr<DropdownBox>(new DropdownBox(
        sf::Vector2f(formX, 556.f),
        sf::Vector2f(formW, 44.f),
        {"Sẵn sàng đón khách", "Tạm ngưng / Bảo trì"},
        0
    ));
    roomStatusDropdown->setLightStyle(true);
    roomStatusDropdown->setFontSize(13);
    roomStatusDropdown->setItemHeight(40.f);
    roomStatusDropdown->setOnSelect([this](int idx, const std::string&) {
        roomFormStatusIdx = idx;
    });

    // 4 Action Buttons (2x2 Grid connected 100% to SQLite & Seat Generator)
    float halfBtnW = (formW - 16.f) / 2.f; // 228.f
    btnRoomCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Thêm mới",
        sf::Vector2f(formX, 632.f),
        sf::Vector2f(halfBtnW, 46.f),
        ButtonStyle::Success,
        8.f
    ));
    btnRoomCreate->setFontSize(13);
    btnRoomCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnRoomCreate->setOnClick([this]() {
        std::string name = inputRoomName ? inputRoomName->getText() : "";
        size_t s = name.find_first_not_of(" \t\r\n");
        size_t e = name.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) {
            if (confirmPopup) confirmPopup->show("Thông báo", "Vui lòng nhập tên phòng chiếu!", ButtonStyle::Primary, "Đóng", "Hủy");
            return;
        }
        name = name.substr(s, e - s + 1);

        if (inputRoomCapacity) {
            int parsed = 0;
            if (sscanf(inputRoomCapacity->getText().c_str(), "%d", &parsed) == 1 && parsed > 0) {
                roomFormCapacity = std::clamp(parsed, 4, 500);
            }
        }

        int maxNum = 0;
        for (const auto& rm : roomList) {
            std::string m = rm.getMaPhong();
            int num = 0;
            if (sscanf(m.c_str(), "R%d", &num) >= 1 || sscanf(m.c_str(), "ROOM%d", &num) >= 1) {
                if (num > maxNum) maxNum = num;
            }
        }
        char idBuf[16];
        snprintf(idBuf, sizeof(idBuf), "R%02d", maxNum + 1);
        std::string newId = idBuf;

        std::string techArr[5] = {"Standard", "VIP", "IMAX", "4DX", "10D VR"};
        std::string tech = (roomFormTechIdx >= 0 && roomFormTechIdx < 5) ? techArr[roomFormTechIdx] : "Standard";
        std::string statusStr = (roomFormStatusIdx == 1) ? "TamNgung" : "HoatDong";

        Room newRoom(newId, name, tech, roomFormCapacity, statusStr);
        roomList.push_back(newRoom);
        db.ghiDanhSachPhong(roomList);

        generateSeatsForRoom(newId, roomFormCapacity, tech);

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHONG", "Thêm phòng chiếu mới: " + name + " (" + newId + ")");
        refreshShowtimesDropdowns();
        refreshSeatsRoomDropdown();
        selectedRoomRowIdx = static_cast<int>(roomList.size()) - 1;
        roomCurrentPage = selectedRoomRowIdx / 5;
        syncRoomSelectionToForm(selectedRoomRowIdx);

        if (confirmPopup) confirmPopup->show("Thành công", "Đã thêm phòng chiếu '" + name + "' (" + newId + ") và khởi tạo sơ đồ " + std::to_string(roomFormCapacity) + " ghế trong SQLite!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnRoomUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Cập nhật",
        sf::Vector2f(formX + halfBtnW + 16.f, 632.f),
        sf::Vector2f(halfBtnW, 46.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnRoomUpdate->setFontSize(13);
    btnRoomUpdate->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnRoomUpdate->setOnClick([this]() {
        if (selectedRoomRowIdx < 0 || selectedRoomRowIdx >= static_cast<int>(roomList.size())) {
            if (confirmPopup) confirmPopup->show("Thông báo", "Vui lòng chọn một phòng chiếu trong danh sách để cập nhật (hoặc bấm 'Thêm mới' để tạo phòng mới)!", ButtonStyle::Primary, "Đóng", "Hủy");
            return;
        }
        std::string name = inputRoomName ? inputRoomName->getText() : "";
        size_t s = name.find_first_not_of(" \t\r\n");
        size_t e = name.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) {
            if (confirmPopup) confirmPopup->show("Thông báo", "Tên phòng chiếu không được để trống!", ButtonStyle::Primary, "Đóng", "Hủy");
            return;
        }
        name = name.substr(s, e - s + 1);

        if (inputRoomCapacity) {
            int parsed = 0;
            if (sscanf(inputRoomCapacity->getText().c_str(), "%d", &parsed) == 1 && parsed > 0) {
                roomFormCapacity = std::clamp(parsed, 4, 500);
            }
        }

        std::string techArr[5] = {"Standard", "VIP", "IMAX", "4DX", "10D VR"};
        std::string tech = (roomFormTechIdx >= 0 && roomFormTechIdx < 5) ? techArr[roomFormTechIdx] : "Standard";
        std::string statusStr = (roomFormStatusIdx == 1) ? "TamNgung" : "HoatDong";

        std::string rId = roomList[selectedRoomRowIdx].getMaPhong();
        int oldCap = roomList[selectedRoomRowIdx].getSucChua();
        std::string oldTech = roomList[selectedRoomRowIdx].getLoaiPhong();

        roomList[selectedRoomRowIdx].setTenPhong(name);
        roomList[selectedRoomRowIdx].setLoaiPhong(tech);
        roomList[selectedRoomRowIdx].setSucChua(roomFormCapacity);
        roomList[selectedRoomRowIdx].setTrangThaiPhong(statusStr);
        db.ghiDanhSachPhong(roomList);

        if (oldCap != roomFormCapacity || oldTech != tech) {
            generateSeatsForRoom(rId, roomFormCapacity, tech);
        }

        ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_PHONG", "Cập nhật phòng: " + name + " (" + rId + ")");
        refreshShowtimesDropdowns();
        refreshSeatsRoomDropdown();
        syncRoomSelectionToForm(selectedRoomRowIdx);

        if (confirmPopup) confirmPopup->show("Thành công", "Đã lưu cập nhật phòng '" + name + "' (" + std::to_string(roomFormCapacity) + " ghế) vào cơ sở dữ liệu SQLite!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnRoomDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xóa",
        sf::Vector2f(formX, 690.f),
        sf::Vector2f(halfBtnW, 46.f),
        ButtonStyle::Danger,
        8.f
    ));
    btnRoomDelete->setFontSize(13);
    btnRoomDelete->setCustomColors(sf::Color(225, 29, 72), sf::Color(190, 18, 60), sf::Color(159, 18, 57), sf::Color(203, 213, 225));
    btnRoomDelete->setOnClick([this]() {
        if (selectedRoomRowIdx < 0 || selectedRoomRowIdx >= static_cast<int>(roomList.size())) {
            if (confirmPopup) confirmPopup->show("Thông báo", "Vui lòng chọn một phòng chiếu trong danh sách để xóa!", ButtonStyle::Primary, "Đóng", "Hủy");
            return;
        }
        std::string rId = roomList[selectedRoomRowIdx].getMaPhong();
        std::string rName = roomList[selectedRoomRowIdx].getTenPhong();

        bool isUsed = false;
        for (const auto& st : showtimeList) {
            if (st.getMaPhong() == rId) {
                isUsed = true;
                break;
            }
        }
        if (isUsed) {
            if (confirmPopup) confirmPopup->show("Không thể xóa", "Phòng '" + rName + "' đang có suất chiếu được xếp lịch. Vui lòng chuyển hoặc xóa suất chiếu trước!", ButtonStyle::Danger, "Đã hiểu", "Đóng");
            return;
        }

        if (confirmPopup) {
            confirmPopup->show("XÁC NHẬN XÓA PHÒNG", "Bạn có chắc chắn muốn xóa phòng '" + rName + " (" + rId + ")' cùng toàn bộ sơ đồ ghế khỏi SQLite?", ButtonStyle::Danger, "Xác nhận xóa", "Hủy");
            confirmPopup->setOnConfirm([this, rId, rName]() {
                for (auto it = roomList.begin(); it != roomList.end(); ++it) {
                    if (it->getMaPhong() == rId) {
                        roomList.erase(it);
                        break;
                    }
                }
                db.ghiDanhSachPhong(roomList);

                allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(),
                    [&rId](const Seat& s) { return s.getMaPhong() == rId; }), allSeats.end());
                db.ghiDanhSachGhe(allSeats);

                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_PHONG", "Xóa phòng chiếu: " + rName + " (" + rId + ")");
                refreshShowtimesDropdowns();
                refreshSeatsRoomDropdown();

                selectedRoomRowIdx = !roomList.empty() ? 0 : -1;
                if (selectedRoomRowIdx >= 0) syncRoomSelectionToForm(0);
            });
        }
    });

    btnRoomReset = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Đặt lại form",
        sf::Vector2f(formX + halfBtnW + 16.f, 690.f),
        sf::Vector2f(halfBtnW, 46.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnRoomReset->setFontSize(13);
    btnRoomReset->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomReset->setOutline(1.f, sf::Color(203, 213, 225));
    btnRoomReset->setTextColor(sf::Color(51, 65, 85));
    btnRoomReset->setOnClick([this]() {
        // Reload fresh from SQLite if needed and restore selected room's state
        if (selectedRoomRowIdx >= 0 && selectedRoomRowIdx < static_cast<int>(roomList.size())) {
            syncRoomSelectionToForm(selectedRoomRowIdx);
        } else if (!roomList.empty()) {
            syncRoomSelectionToForm(0);
        } else {
            selectedRoomRowIdx = -1;
            if (inputRoomName) inputRoomName->setText("");
            roomFormCapacity = 100;
            if (inputRoomCapacity) inputRoomCapacity->setText("100");
            roomFormTechIdx = 0;
            roomFormStatusIdx = 0;
            if (roomStatusDropdown) roomStatusDropdown->setSelectedIndex(0);
        }
    });

    // Pagination buttons inside bottom-right of Left Card (y = 768.f, h = 32.f)
    btnRoomPrevPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Trước",
        sf::Vector2f(leftColX + leftColW - 180.f, 768.f),
        sf::Vector2f(58.f, 32.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnRoomPrevPage->setFontSize(12);
    btnRoomPrevPage->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomPrevPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnRoomPrevPage->setTextColor(sf::Color(100, 116, 139));
    btnRoomPrevPage->setOnClick([this]() {
        if (roomCurrentPage > 0) roomCurrentPage--;
    });

    btnRoomNextPage = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Sau",
        sf::Vector2f(leftColX + leftColW - 78.f, 768.f),
        sf::Vector2f(58.f, 32.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnRoomNextPage->setFontSize(12);
    btnRoomNextPage->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnRoomNextPage->setOutline(1.f, sf::Color(226, 232, 240));
    btnRoomNextPage->setTextColor(sf::Color(100, 116, 139));
    btnRoomNextPage->setOnClick([this]() {
        roomCurrentPage++;
    });

    // Initial sync
    if (!roomList.empty()) {
        syncRoomSelectionToForm(0);
    }
}


void CinemaUIApp::renderRoomsView() {
    float leftColX = sidebarWidth + 24.f;       // 284.f
    float leftColW = 880.f;
    float rightColX = leftColX + leftColW + 20.f; // 1184.f
    float rightColW = 520.f;

    // 1. Soft slate-white canvas background (#f8fafc)
    window.clear(sf::Color(248, 250, 252));

    // ==========================================
    // 2. TOP BREADCRUMB & REALTIME SYNC HEADER
    // ==========================================
    float topNavY = 12.f;

    sf::CircleShape pinDot(3.5f);
    pinDot.setFillColor(sf::Color(16, 185, 129));
    pinDot.setPosition(leftColX, topNavY + 3.f);
    window.draw(pinDot);

    sf::Text brBranch(toSfString("Cinestar Quốc Thanh (TP.HCM)"), Theme::FontPrimary, 11);
    brBranch.setStyle(sf::Text::Bold);
    brBranch.setFillColor(sf::Color(15, 23, 42));
    brBranch.setPosition(leftColX + 12.f, topNavY);
    window.draw(brBranch);

    sf::CircleShape sepDot(2.f);
    sepDot.setFillColor(sf::Color(203, 213, 225));
    sepDot.setPosition(leftColX + brBranch.getLocalBounds().width + 20.f, topNavY + 5.f);
    window.draw(sepDot);

    sf::Text brPath(toSfString("Bảng điều khiển  >  Quản lý phòng chiếu  >  Quản Lý Phòng"), Theme::FontPrimary, 11);
    brPath.setFillColor(sf::Color(100, 116, 139));
    brPath.setPosition(leftColX + brBranch.getLocalBounds().width + 30.f, topNavY);
    window.draw(brPath);

    // Right User Pill (CineOS Profile)
    float uPillW = 246.f;
    float uPillX = rightColX + rightColW - uPillW;
    RoundedRectRenderer::render(window, sf::Vector2f(uPillX, topNavY - 4.f), sf::Vector2f(uPillW, 26.f), 13.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::CircleShape uAv(8.f);
    uAv.setFillColor(sf::Color(2, 132, 199));
    uAv.setPosition(uPillX + 5.f, topNavY + 1.f);
    window.draw(uAv);

    sf::Text uIni(toSfString("LT"), Theme::FontPrimary, 9);
    uIni.setStyle(sf::Text::Bold);
    uIni.setFillColor(sf::Color::White);
    uIni.setPosition(uPillX + 7.5f, topNavY + 2.f);
    window.draw(uIni);

    std::string dispUser = currentUserName.empty() ? "Lê Minh Tuấn" : currentUserName;
    std::string dispRole = currentUserRole.empty() ? "Trưởng Ca Vận Hành" : currentUserRole;
    sf::Text uName(toSfString(dispUser + " (" + dispRole + ")"), Theme::FontPrimary, 10);
    uName.setStyle(sf::Text::Bold);
    uName.setFillColor(sf::Color(30, 41, 59));
    uName.setPosition(uPillX + 26.f, topNavY + 1.f);
    window.draw(uName);

    // ==========================================
    // 3. MAIN HEADER TITLE CARD (y = 38.f, h = 72.f)
    // ==========================================
    float titleCardY = 38.f;
    float titleCardH = 72.f;
    float totalW = leftColW + 20.f + rightColW; // 1420.f

    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, titleCardY), sf::Vector2f(totalW, titleCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Left Icon Square (Blue with Cinema Screen / Hall Icon)
    float iconBoxX = leftColX + 18.f;
    float iconBoxY = titleCardY + 14.f;
    RoundedRectRenderer::render(window, sf::Vector2f(iconBoxX, iconBoxY), sf::Vector2f(44.f, 44.f), 10.f, sf::Color(2, 132, 199));

    sf::RectangleShape cSeat(sf::Vector2f(20.f, 12.f));
    cSeat.setPosition(iconBoxX + 12.f, iconBoxY + 20.f);
    cSeat.setFillColor(sf::Color::White);
    window.draw(cSeat);

    sf::RectangleShape cBack(sf::Vector2f(16.f, 10.f));
    cBack.setPosition(iconBoxX + 14.f, iconBoxY + 11.f);
    cBack.setFillColor(sf::Color::White);
    window.draw(cBack);

    sf::RectangleShape cArmL(sf::Vector2f(3.f, 14.f));
    cArmL.setPosition(iconBoxX + 9.f, iconBoxY + 18.f);
    cArmL.setFillColor(sf::Color(224, 242, 254));
    window.draw(cArmL);

    sf::RectangleShape cArmR(sf::Vector2f(3.f, 14.f));
    cArmR.setPosition(iconBoxX + 32.f, iconBoxY + 18.f);
    cArmR.setFillColor(sf::Color(224, 242, 254));
    window.draw(cArmR);

    // Title: QUẢN LÝ PHÒNG CHIẾU (Segoe UI via Theme::FontPrimary + toSfString)
    sf::Text mainTitle(toSfString("QUẢN LÝ PHÒNG CHIẾU"), Theme::FontPrimary, 21);
    mainTitle.setStyle(sf::Text::Bold);
    mainTitle.setFillColor(sf::Color(15, 23, 42));
    mainTitle.setPosition(leftColX + 76.f, titleCardY + 12.f);
    window.draw(mainTitle);

    // Realtime Sync Badge
    float rBadgeX = leftColX + 76.f + mainTitle.getLocalBounds().width + 14.f;
    float rBadgeY = titleCardY + 15.f;
    RoundedRectRenderer::render(window, sf::Vector2f(rBadgeX, rBadgeY), sf::Vector2f(164.f, 22.f), 11.f, sf::Color(236, 253, 245), 1.f, sf::Color(167, 243, 208));

    sf::CircleShape rDot(3.5f);
    rDot.setFillColor(sf::Color(16, 185, 129));
    rDot.setPosition(rBadgeX + 9.f, rBadgeY + 7.5f);
    window.draw(rDot);

    sf::Text rBadgeTxt(toSfString("Dữ liệu đồng bộ Realtime"), Theme::FontPrimary, 11);
    rBadgeTxt.setStyle(sf::Text::Bold);
    rBadgeTxt.setFillColor(sf::Color(5, 150, 105));
    rBadgeTxt.setPosition(rBadgeX + 20.f, rBadgeY + 3.f);
    window.draw(rBadgeTxt);

    // Subtitle
    sf::Text mainSub(toSfString("Quản lý toàn diện danh sách phòng, phân loại công nghệ trải nghiệm, cấu hình sức chứa và thông số kỹ thuật rạp chiếu"), Theme::FontPrimary, 12);
    mainSub.setFillColor(sf::Color(100, 116, 139));
    mainSub.setPosition(leftColX + 76.f, titleCardY + 42.f);
    window.draw(mainSub);

    // Header Action Buttons
    if (btnRoomExportPDF) {
        btnRoomExportPDF->draw(window);
        // Document / Export icon inside button left
        float exX = rightColX + rightColW - 344.f + 16.f;
        float exY = 55.f + 12.f;
        RoundedRectRenderer::render(window, sf::Vector2f(exX, exY), sf::Vector2f(11.f, 14.f), 2.f, sf::Color(148, 163, 184));
        sf::RectangleShape exLine1(sf::Vector2f(7.f, 1.5f));
        exLine1.setFillColor(sf::Color::White);
        exLine1.setPosition(exX + 2.f, exY + 4.f);
        window.draw(exLine1);
        sf::RectangleShape exLine2(sf::Vector2f(5.f, 1.5f));
        exLine2.setFillColor(sf::Color::White);
        exLine2.setPosition(exX + 2.f, exY + 8.f);
        window.draw(exLine2);
    }
    if (btnRoomAddNew) btnRoomAddNew->draw(window);

    // ==========================================
    // 4. LEFT COLUMN: ROOMS LIST & FILTER CARD (y = 124.f, h = 696.f)
    // ==========================================
    float tableCardY = 124.f;
    float tableCardH = 696.f; // Bottom is 820.f

    RoundedRectRenderer::render(window, sf::Vector2f(leftColX, tableCardY), sf::Vector2f(leftColW, tableCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // 4.1 Search & Filter Bar inside top of Left Card
    // Magnifying glass + "Tìm kiếm phòng chiếu:"
    sf::CircleShape sLblLens(4.5f);
    sLblLens.setFillColor(sf::Color::Transparent);
    sLblLens.setOutlineThickness(1.5f);
    sLblLens.setOutlineColor(sf::Color(2, 132, 199));
    sLblLens.setPosition(leftColX + 22.f, tableCardY + 17.f);
    window.draw(sLblLens);
    sf::RectangleShape sLblHandle(sf::Vector2f(4.5f, 1.5f));
    sLblHandle.setFillColor(sf::Color(2, 132, 199));
    sLblHandle.setPosition(leftColX + 30.f, tableCardY + 25.f);
    sLblHandle.setRotation(45.f);
    window.draw(sLblHandle);

    sf::Text sLblTxt(toSfString("Tìm kiếm phòng chiếu:"), Theme::FontPrimary, 12);
    sLblTxt.setStyle(sf::Text::Bold);
    sLblTxt.setFillColor(sf::Color(51, 65, 85));
    sLblTxt.setPosition(leftColX + 38.f, tableCardY + 14.f);
    window.draw(sLblTxt);

    if (roomSearchInput) {
        roomSearchInput->draw(window);
        // Magnifying glass inside search input
        sf::CircleShape inLens(4.5f);
        inLens.setFillColor(sf::Color::Transparent);
        inLens.setOutlineThickness(1.5f);
        inLens.setOutlineColor(sf::Color(148, 163, 184));
        inLens.setPosition(leftColX + 32.f, 166.f + 13.f);
        window.draw(inLens);
        sf::RectangleShape inHandle(sf::Vector2f(4.5f, 1.5f));
        inHandle.setFillColor(sf::Color(148, 163, 184));
        inHandle.setPosition(leftColX + 40.f, 166.f + 21.f);
        inHandle.setRotation(45.f);
        window.draw(inHandle);
    }

    auto updateChipStyle = [&](std::unique_ptr<RoundRectButton>& b, int idx) {
        if (!b) return;
        bool isActive = (roomFilterType == idx);
        if (isActive) {
            b->setCustomColors(sf::Color(15, 23, 42), sf::Color(30, 41, 59), sf::Color(51, 65, 85), sf::Color(15, 23, 42));
            b->setOutline(1.f, sf::Color(15, 23, 42));
            b->setTextColor(sf::Color::White);
        } else {
            b->setCustomColors(sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
            b->setOutline(1.f, sf::Color(226, 232, 240));
            b->setTextColor(sf::Color(71, 85, 105));
        }
        b->draw(window);
    };

    updateChipStyle(btnRoomFilterAll, 0);
    updateChipStyle(btnRoomFilterStandard, 1);
    updateChipStyle(btnRoomFilterPremium, 2);
    updateChipStyle(btnRoomFilterSpecial, 3);

    // Divider below filter bar (y = 216.f)
    sf::RectangleShape fDiv(sf::Vector2f(leftColW - 40.f, 1.f));
    fDiv.setPosition(leftColX + 20.f, 216.f);
    fDiv.setFillColor(sf::Color(241, 245, 249));
    window.draw(fDiv);

    // 4.2 Table Column Headers (y = 228.f)
    float colMaCenterX   = leftColX + 60.f;  // 344.f
    float colTenLeftX    = leftColX + 120.f; // 404.f
    float colCapCenterX  = leftColX + 492.f; // 776.f
    float colStatCenterX = leftColX + 634.f; // 918.f
    float colActCenterX  = leftColX + 784.f; // 1068.f

    auto drawHeaderCentered = [&](const std::string& str, float cx) {
        sf::Text t(toSfString(str), Theme::FontPrimary, 11);
        t.setStyle(sf::Text::Bold);
        t.setFillColor(sf::Color(100, 116, 139));
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(std::round(cx - b.width / 2.f), 228.f);
        window.draw(t);
    };

    drawHeaderCentered("MÃ PHÒNG", colMaCenterX);
    {
        sf::Text t(toSfString("TÊN PHÒNG & TRẢI NGHIỆM"), Theme::FontPrimary, 11);
        t.setStyle(sf::Text::Bold);
        t.setFillColor(sf::Color(100, 116, 139));
        t.setPosition(colTenLeftX, 228.f);
        window.draw(t);
    }
    drawHeaderCentered("SỨC CHỨA", colCapCenterX);
    drawHeaderCentered("TRẠNG THÁI", colStatCenterX);
    drawHeaderCentered("THAO TÁC", colActCenterX);

    // Divider line under table header (y = 250.f)
    sf::RectangleShape hDiv(sf::Vector2f(leftColW - 40.f, 1.f));
    hDiv.setPosition(leftColX + 20.f, 250.f);
    hDiv.setFillColor(sf::Color(226, 232, 240));
    window.draw(hDiv);

    // 4.3 Filter room list
    std::string q = roomSearchInput ? roomSearchInput->getText() : "";
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    std::vector<int> filteredIndices;
    for (size_t i = 0; i < roomList.size(); ++i) {
        const auto& rm = roomList[i];
        std::string lp = rm.getLoaiPhong();
        std::string lpLow = lp;
        std::transform(lpLow.begin(), lpLow.end(), lpLow.begin(), ::tolower);

        if (roomFilterType == 1) { // Standard
            if (lpLow.find("standard") == std::string::npos && lpLow.find("2d") == std::string::npos) continue;
        } else if (roomFilterType == 2) { // Premium / VIP
            if (lpLow.find("vip") == std::string::npos && lpLow.find("premium") == std::string::npos) continue;
        } else if (roomFilterType == 3) { // Đặc biệt
            if (lpLow.find("imax") == std::string::npos && lpLow.find("4dx") == std::string::npos && lpLow.find("10d") == std::string::npos && lpLow.find("vr") == std::string::npos) continue;
        }

        if (!q.empty()) {
            std::string nameLow = rm.getTenPhong();
            std::transform(nameLow.begin(), nameLow.end(), nameLow.begin(), ::tolower);
            std::string idLow = rm.getMaPhong();
            std::transform(idLow.begin(), idLow.end(), idLow.begin(), ::tolower);
            if (nameLow.find(q) == std::string::npos && idLow.find(q) == std::string::npos && lpLow.find(q) == std::string::npos) continue;
        }
        filteredIndices.push_back(static_cast<int>(i));
    }

    int pageSize = 5;
    int maxPage = std::max(0, (static_cast<int>(filteredIndices.size()) + pageSize - 1) / pageSize - 1);
    if (roomCurrentPage > maxPage) roomCurrentPage = maxPage;
    if (roomCurrentPage < 0) roomCurrentPage = 0;
    int startIdx = roomCurrentPage * pageSize;

    sf::Vector2i mPixel = sf::Mouse::getPosition(window);
    sf::Vector2f mPos = window.mapPixelToCoords(mPixel);

    float rowStartY = 258.f;
    float rowH = 88.f;
    float rowGap = 8.f;

    for (int row = 0; row < pageSize; ++row) {
        int itemIdx = startIdx + row;
        if (itemIdx >= static_cast<int>(filteredIndices.size())) break;
        int actualIdx = filteredIndices[itemIdx];
        const auto& rm = roomList[actualIdx];

        float ry = rowStartY + row * (rowH + rowGap);
        bool isSelected = (selectedRoomRowIdx == actualIdx);
        bool isHovered = (mPos.x >= leftColX + 16.f && mPos.x <= leftColX + leftColW - 16.f && mPos.y >= ry && mPos.y <= ry + rowH);

        // Row background card
        sf::Color rowBg = isSelected ? sf::Color(240, 253, 250) : (isHovered ? sf::Color(248, 250, 252) : sf::Color::White);
        sf::Color rowBorder = isSelected ? sf::Color(165, 243, 252) : (isHovered ? sf::Color(226, 232, 240) : sf::Color(241, 245, 249));
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 14.f, ry), sf::Vector2f(leftColW - 28.f, rowH), 10.f, rowBg, 1.f, rowBorder);

        if (isSelected) {
            RoundedRectRenderer::render(window, sf::Vector2f(leftColX + 14.f, ry + 8.f), sf::Vector2f(4.f, rowH - 16.f), 2.f, sf::Color(6, 182, 212));
        }

        // Col 1: Mã phòng rounded badge (centered at colMaCenterX, ry + 44.f)
        float codeBoxW = 64.f;
        float codeBoxH = 44.f;
        float codeBoxX = colMaCenterX - codeBoxW / 2.f;
        float codeBoxY = ry + (rowH - codeBoxH) / 2.f;
        sf::Color codeBg = isSelected ? sf::Color(207, 250, 254) : sf::Color(241, 245, 249);
        sf::Color codeBdr = isSelected ? sf::Color(103, 232, 249) : sf::Color(226, 232, 240);
        sf::Color codeFg = isSelected ? sf::Color(14, 116, 144) : sf::Color(51, 65, 85);
        RoundedRectRenderer::render(window, sf::Vector2f(codeBoxX, codeBoxY), sf::Vector2f(codeBoxW, codeBoxH), 8.f, codeBg, 1.f, codeBdr);

        std::string rawCode = rm.getMaPhong();
        int parsedNum = 0;
        if (sscanf(rawCode.c_str(), "R%d", &parsedNum) == 1 || sscanf(rawCode.c_str(), "ROOM%d", &parsedNum) == 1) {
            char numStr[16];
            snprintf(numStr, sizeof(numStr), "%02d", parsedNum);
            sf::Text l1(toSfString("ROOM"), Theme::FontPrimary, 10);
            l1.setStyle(sf::Text::Bold);
            l1.setFillColor(codeFg);
            l1.setPosition(std::round(colMaCenterX - l1.getLocalBounds().width / 2.f), codeBoxY + 7.f);
            window.draw(l1);

            sf::Text l2(toSfString(numStr), Theme::FontPrimary, 12);
            l2.setStyle(sf::Text::Bold);
            l2.setFillColor(codeFg);
            l2.setPosition(std::round(colMaCenterX - l2.getLocalBounds().width / 2.f), codeBoxY + 22.f);
            window.draw(l2);
        } else {
            sf::Text mCodeTxt(toSfString(rawCode), Theme::FontPrimary, 11);
            mCodeTxt.setStyle(sf::Text::Bold);
            mCodeTxt.setFillColor(codeFg);
            mCodeTxt.setPosition(std::round(colMaCenterX - mCodeTxt.getLocalBounds().width / 2.f), codeBoxY + 14.f);
            window.draw(mCodeTxt);
        }

        // Col 2: Color dot + Room Name + Tech sticker + "Đang chọn" sticker + Subtitle
        std::string lp = rm.getLoaiPhong();
        std::string lpLow = lp;
        std::transform(lpLow.begin(), lpLow.end(), lpLow.begin(), ::tolower);

        sf::Color dotColor = sf::Color(14, 165, 233); // Blue Standard
        std::string techBadgeStr = "2D/3D";
        sf::Color badgeBg = sf::Color(224, 242, 254);
        sf::Color badgeBdr = sf::Color(186, 230, 253);
        sf::Color badgeFg = sf::Color(2, 132, 199);
        std::string descSub = "Màn bạc Silver Screen • Dolby Atmos 7.1";

        if (lpLow.find("vip") != std::string::npos || lpLow.find("premium") != std::string::npos) {
            dotColor = sf::Color(245, 158, 11); // Amber VIP
            techBadgeStr = "VIP LOUNGE";
            badgeBg = sf::Color(254, 243, 199);
            badgeBdr = sf::Color(253, 230, 138);
            badgeFg = sf::Color(180, 83, 9);
            descSub = "Ghế da hạng thương gia • Phục vụ ẩm thực tại chỗ";
        } else if (lpLow.find("imax") != std::string::npos) {
            dotColor = sf::Color(147, 51, 234); // Purple IMAX
            techBadgeStr = "LASER 4K";
            badgeBg = sf::Color(243, 232, 255);
            badgeBdr = sf::Color(233, 213, 255);
            badgeFg = sf::Color(126, 34, 206);
            descSub = "Màn hình cong vĩ tuyến • Hệ thống âm thanh 12 kênh";
        } else if (lpLow.find("4dx") != std::string::npos) {
            dotColor = sf::Color(249, 115, 22); // Orange 4DX
            techBadgeStr = "MOTION FX";
            badgeBg = sf::Color(255, 237, 213);
            badgeBdr = sf::Color(254, 215, 170);
            badgeFg = sf::Color(194, 65, 12);
            descSub = "Ghế chuyển động đa chiều • Hiệu ứng môi trường thực";
        } else if (lpLow.find("10d") != std::string::npos || lpLow.find("vr") != std::string::npos) {
            dotColor = sf::Color(6, 182, 212); // Cyan 10D VR
            techBadgeStr = "VR CINEMA";
            badgeBg = sf::Color(8, 145, 178); // Solid teal like reference mockup!
            badgeBdr = sf::Color(6, 182, 212);
            badgeFg = sf::Color::White;
            descSub = "Kính thực tế ảo tương tác • Ghế rung cảm ứng chuyển động";
        }

        // Colored circle dot with soft halo
        sf::CircleShape cHalo(7.5f);
        cHalo.setFillColor(sf::Color(dotColor.r, dotColor.g, dotColor.b, 38));
        cHalo.setPosition(colTenLeftX - 3.f, ry + 21.f);
        window.draw(cHalo);

        sf::CircleShape cDot(4.5f);
        cDot.setFillColor(dotColor);
        cDot.setPosition(colTenLeftX, ry + 24.f);
        window.draw(cDot);

        // Room Name text (top line of Col 2, y = ry + 17.f)
        sf::Text rNameTxt(toSfString(rm.getTenPhong()), Theme::FontPrimary, 14);
        rNameTxt.setStyle(sf::Text::Bold);
        rNameTxt.setFillColor(sf::Color(15, 23, 42));
        rNameTxt.setPosition(colTenLeftX + 18.f, ry + 17.f);
        window.draw(rNameTxt);

        // Technology sticker (generous padding, centered text)
        float bX = colTenLeftX + 18.f + rNameTxt.getLocalBounds().width + 10.f;
        sf::Text bTxt(toSfString(techBadgeStr), Theme::FontPrimary, 10);
        bTxt.setStyle(sf::Text::Bold);
        float bW = bTxt.getLocalBounds().width + 16.f;
        float bH = 22.f;
        float bY = ry + 16.f;
        RoundedRectRenderer::render(window, sf::Vector2f(bX, bY), sf::Vector2f(bW, bH), 6.f, badgeBg, 1.f, badgeBdr);
        bTxt.setFillColor(badgeFg);
        bTxt.setPosition(std::round(bX + (bW - bTxt.getLocalBounds().width) / 2.f), bY + 4.f);
        window.draw(bTxt);

        // "Đang chọn" sticker when row is selected (matching reference mockup)
        if (isSelected) {
            float selX = bX + bW + 8.f;
            sf::Text selTxt(toSfString("Đang chọn"), Theme::FontPrimary, 10);
            selTxt.setStyle(sf::Text::Bold);
            float selW = selTxt.getLocalBounds().width + 16.f;
            RoundedRectRenderer::render(window, sf::Vector2f(selX, bY), sf::Vector2f(selW, bH), 6.f,
                                        sf::Color(207, 250, 254), 1.f, sf::Color(165, 243, 252));
            selTxt.setFillColor(sf::Color(14, 116, 144));
            selTxt.setPosition(std::round(selX + (selW - selTxt.getLocalBounds().width) / 2.f), bY + 4.f);
            window.draw(selTxt);
        }

        // Subline description (bottom line of Col 2, y = ry + 49.f — 32px below Room Name so 0 overlap!)
        sf::Text rSubTxt(toSfString(descSub), Theme::FontPrimary, 12);
        rSubTxt.setFillColor(sf::Color(100, 116, 139));
        rSubTxt.setPosition(colTenLeftX + 18.f, ry + 49.f);
        window.draw(rSubTxt);

        // Col 3: Sức chứa pill centered at colCapCenterX
        float capBoxW = 84.f;
        float capBoxH = 34.f;
        float capBoxX = colCapCenterX - capBoxW / 2.f;
        float capBoxY = ry + (rowH - capBoxH) / 2.f;
        RoundedRectRenderer::render(window, sf::Vector2f(capBoxX, capBoxY), sf::Vector2f(capBoxW, capBoxH), 8.f,
                                    isSelected ? sf::Color::White : sf::Color(248, 250, 252), 1.f,
                                    isSelected ? sf::Color(165, 243, 252) : sf::Color(226, 232, 240));

        // Mini sofa/armchair icon inside capacity pill
        float chX = capBoxX + 14.f;
        float chY = capBoxY + 11.f;
        RoundedRectRenderer::render(window, sf::Vector2f(chX + 2.f, chY), sf::Vector2f(10.f, 7.f), 2.f, sf::Color(100, 116, 139));
        RoundedRectRenderer::render(window, sf::Vector2f(chX, chY + 5.f), sf::Vector2f(14.f, 6.f), 2.f, sf::Color(71, 85, 105));

        sf::Text capNum(toSfString(std::to_string(rm.getSucChua())), Theme::FontPrimary, 13);
        capNum.setStyle(sf::Text::Bold);
        capNum.setFillColor(sf::Color(15, 23, 42));
        capNum.setPosition(capBoxX + 34.f, capBoxY + 8.f);
        window.draw(capNum);

        // Col 4: Trạng thái pill centered at colStatCenterX (linked to SQLite trangThaiPhong & showtimeList)
        bool isMaintenance = (rm.getTrangThaiPhong() == "TamNgung");
        bool hasShowtime = false;
        if (!isMaintenance) {
            for (const auto& st : showtimeList) {
                if (st.getMaPhong() == rm.getMaPhong()) {
                    hasShowtime = true;
                    break;
                }
            }
        }

        std::string sText = isMaintenance ? "Tạm ngưng" : (hasShowtime ? "Đang chiếu" : "Sẵn sàng");
        sf::Color sBg = isMaintenance ? sf::Color(254, 243, 199) : sf::Color(236, 253, 245);
        sf::Color sBdr = isMaintenance ? sf::Color(253, 230, 138) : sf::Color(167, 243, 208);
        sf::Color sFg = isMaintenance ? sf::Color(180, 83, 9) : sf::Color(5, 150, 105);
        sf::Color sDotCol = isMaintenance ? sf::Color(245, 158, 11) : sf::Color(16, 185, 129);

        float statBoxW = 108.f;
        float statBoxH = 34.f;
        float statBoxX = colStatCenterX - statBoxW / 2.f;
        float statBoxY = ry + (rowH - statBoxH) / 2.f;
        RoundedRectRenderer::render(window, sf::Vector2f(statBoxX, statBoxY), sf::Vector2f(statBoxW, statBoxH), 17.f, sBg, 1.f, sBdr);

        sf::Text stTxt(toSfString(sText), Theme::FontPrimary, 11);
        stTxt.setStyle(sf::Text::Bold);
        stTxt.setFillColor(sFg);
        float totalStContentW = 12.f + stTxt.getLocalBounds().width;
        float stStartX = statBoxX + (statBoxW - totalStContentW) / 2.f;

        sf::CircleShape stDot(3.5f);
        stDot.setFillColor(sDotCol);
        stDot.setPosition(stStartX, statBoxY + 13.5f);
        window.draw(stDot);

        stTxt.setPosition(stStartX + 12.f, statBoxY + 9.f);
        window.draw(stTxt);

        // Col 5: Thao tác centered at colActCenterX
        if (isSelected) {
            float actW = 96.f;
            float actH = 34.f;
            float actX = colActCenterX - actW / 2.f;
            float actY = ry + (rowH - actH) / 2.f;
            RoundedRectRenderer::render(window, sf::Vector2f(actX, actY), sf::Vector2f(actW, actH), 8.f, sf::Color(2, 132, 199));

            // Small white pencil icon
            sf::RectangleShape pShaft(sf::Vector2f(9.f, 2.5f));
            pShaft.setFillColor(sf::Color::White);
            pShaft.setPosition(actX + 12.f, actY + 19.f);
            pShaft.setRotation(-45.f);
            window.draw(pShaft);

            sf::Text actTxt(toSfString("Đang sửa"), Theme::FontPrimary, 11);
            actTxt.setStyle(sf::Text::Bold);
            actTxt.setFillColor(sf::Color::White);
            actTxt.setPosition(actX + 26.f, actY + 9.f);
            window.draw(actTxt);
        } else {
            float edW = 34.f;
            float edH = 34.f;
            float edX = colActCenterX - edW / 2.f;
            float edY = ry + (rowH - edH) / 2.f;
            RoundedRectRenderer::render(window, sf::Vector2f(edX, edY), sf::Vector2f(edW, edH), 8.f,
                                        isHovered ? sf::Color(224, 242, 254) : sf::Color(248, 250, 252), 1.f, sf::Color(203, 213, 225));
            // Vector square + pencil edit icon (replaces ASCII "[edit]")
            float sqX = edX + 10.f;
            float sqY = edY + 10.f;
            sf::RectangleShape sqBox(sf::Vector2f(13.f, 13.f));
            sqBox.setPosition(sqX, sqY + 1.f);
            sqBox.setFillColor(sf::Color::Transparent);
            sqBox.setOutlineThickness(1.5f);
            sqBox.setOutlineColor(sf::Color(100, 116, 139));
            window.draw(sqBox);

            sf::RectangleShape penLine(sf::Vector2f(11.f, 2.5f));
            penLine.setPosition(sqX + 4.f, sqY + 9.f);
            penLine.setFillColor(sf::Color(71, 85, 105));
            penLine.setRotation(-45.f);
            window.draw(penLine);
        }
    }

    // 4.4 Table Footer INSIDE Left Card (y = 754.f .. 812.f)
    sf::RectangleShape footDiv(sf::Vector2f(leftColW - 40.f, 1.f));
    footDiv.setPosition(leftColX + 20.f, 754.f);
    footDiv.setFillColor(sf::Color(241, 245, 249));
    window.draw(footDiv);

    int totalFiltered = static_cast<int>(filteredIndices.size());
    int shownCount = std::min(pageSize, std::max(0, totalFiltered - startIdx));
    char sumBuf[128];
    snprintf(sumBuf, sizeof(sumBuf), "Hiển thị %d trên tổng số %d phòng chiếu hoạt động", shownCount, totalFiltered);
    sf::Text tableSum(toSfString(sumBuf), Theme::FontPrimary, 12);
    tableSum.setFillColor(sf::Color(100, 116, 139));
    tableSum.setPosition(leftColX + 22.f, 775.f);
    window.draw(tableSum);

    if (btnRoomPrevPage) btnRoomPrevPage->draw(window);

    // Active page number pill between "Trước" and "Sau"
    float pagePillX = leftColX + leftColW - 116.f;
    float pagePillY = 768.f;
    RoundedRectRenderer::render(window, sf::Vector2f(pagePillX, pagePillY), sf::Vector2f(32.f, 32.f), 6.f, sf::Color(2, 132, 199));
    sf::Text pageNumTxt(toSfString(std::to_string(roomCurrentPage + 1)), Theme::FontPrimary, 12);
    pageNumTxt.setStyle(sf::Text::Bold);
    pageNumTxt.setFillColor(sf::Color::White);
    pageNumTxt.setPosition(std::round(pagePillX + (32.f - pageNumTxt.getLocalBounds().width) / 2.f), pagePillY + 7.f);
    window.draw(pageNumTxt);

    if (btnRoomNextPage) btnRoomNextPage->draw(window);

    // ==========================================
    // 5. RIGHT COLUMN: ROOM CONFIGURATION CARD (y = 124.f, h = 696.f)
    // ==========================================
    float rightCardY = 124.f;
    float rightCardH = 696.f;
    float formX = rightColX + 24.f;
    float formW = rightColW - 48.f; // 472.f

    RoundedRectRenderer::render(window, sf::Vector2f(rightColX, rightCardY), sf::Vector2f(rightColW, rightCardH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    // Right Card Header (cleanly separated so 0 overlap!)
    // Small sliders icon + "CHỈNH SỬA THUỘC TÍNH"
    for (int sl = 0; sl < 2; ++sl) {
        sf::RectangleShape sLine(sf::Vector2f(11.f, 1.5f));
        sLine.setFillColor(sf::Color(2, 132, 199));
        sLine.setPosition(formX, 151.f + sl * 5.f);
        window.draw(sLine);
        sf::CircleShape sKnob(2.f);
        sKnob.setFillColor(sf::Color(2, 132, 199));
        sKnob.setPosition(formX + (sl == 0 ? 6.f : 2.f), 149.5f + sl * 5.f);
        window.draw(sKnob);
    }

    sf::Text editSub(toSfString("CHỈNH SỬA THUỘC TÍNH"), Theme::FontPrimary, 11);
    editSub.setStyle(sf::Text::Bold);
    editSub.setFillColor(sf::Color(2, 132, 199));
    editSub.setPosition(formX + 16.f, 146.f);
    window.draw(editSub);

    sf::Text editTitle(toSfString("Thông tin phòng & Cấu hình"), Theme::FontPrimary, 18);
    editTitle.setStyle(sf::Text::Bold);
    editTitle.setFillColor(sf::Color(15, 23, 42));
    editTitle.setPosition(formX, 168.f);
    window.draw(editTitle);

    // Badge showing selected room ("Phòng #1", "Phòng #5", or "Tạo mới")
    std::string numBadgeStr = (selectedRoomRowIdx >= 0) ? ("Phòng #" + std::to_string(selectedRoomRowIdx + 1)) : "Tạo mới";
    float nbW = 86.f;
    float nbX = formX + formW - nbW;
    RoundedRectRenderer::render(window, sf::Vector2f(nbX, 156.f), sf::Vector2f(nbW, 28.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
    sf::Text nbTxt(toSfString(numBadgeStr), Theme::FontPrimary, 12);
    nbTxt.setStyle(sf::Text::Bold);
    nbTxt.setFillColor(sf::Color(2, 132, 199));
    nbTxt.setPosition(std::round(nbX + (nbW - nbTxt.getLocalBounds().width) / 2.f), 162.f);
    window.draw(nbTxt);

    // Header separator line (y = 204.f)
    sf::RectangleShape rHeadDiv(sf::Vector2f(formW, 1.f));
    rHeadDiv.setPosition(formX, 204.f);
    rHeadDiv.setFillColor(sf::Color(241, 245, 249));
    window.draw(rHeadDiv);

    // Field 1: TÊN PHÒNG CHIẾU * (y = 224.f, input at y = 250.f)
    sf::Text f1Lbl(toSfString("TÊN PHÒNG CHIẾU *"), Theme::FontPrimary, 11);
    f1Lbl.setStyle(sf::Text::Bold);
    f1Lbl.setFillColor(sf::Color(51, 65, 85));
    f1Lbl.setPosition(formX, 224.f);
    window.draw(f1Lbl);

    // Blue verified badge on right
    sf::CircleShape vCircle(6.f);
    vCircle.setFillColor(sf::Color(2, 132, 199));
    vCircle.setPosition(formX + formW - 86.f, 225.f);
    window.draw(vCircle);
    sf::Text vCheck(toSfString("✓"), Theme::FontPrimary, 9);
    vCheck.setStyle(sf::Text::Bold);
    vCheck.setFillColor(sf::Color::White);
    vCheck.setPosition(formX + formW - 83.5f, 224.5f);
    window.draw(vCheck);

    sf::Text f1Valid(toSfString("Đã xác thực"), Theme::FontPrimary, 11);
    f1Valid.setStyle(sf::Text::Bold);
    f1Valid.setFillColor(sf::Color(2, 132, 199));
    f1Valid.setPosition(formX + formW - 70.f, 224.f);
    window.draw(f1Valid);

    if (inputRoomName) inputRoomName->draw(window);

    // Field 2: SỨC CHỨA (TỔNG SỐ GHẾ) * (y = 314.f, input at y = 340.f)
    sf::Text f2Lbl(toSfString("SỨC CHỨA (TỔNG SỐ GHẾ) *"), Theme::FontPrimary, 11);
    f2Lbl.setStyle(sf::Text::Bold);
    f2Lbl.setFillColor(sf::Color(51, 65, 85));
    f2Lbl.setPosition(formX, 314.f);
    window.draw(f2Lbl);

    sf::Text f2Guide(toSfString("Ghế cố định / phòng"), Theme::FontPrimary, 11);
    f2Guide.setFillColor(sf::Color(148, 163, 184));
    f2Guide.setPosition(formX + formW - f2Guide.getLocalBounds().width, 314.f);
    window.draw(f2Guide);

    if (inputRoomCapacity) {
        inputRoomCapacity->draw(window);
        // Right-aligned unit suffix "ghế ngồi" inside inputRoomCapacity
        sf::Text unitTxt(toSfString("ghế ngồi"), Theme::FontPrimary, 12);
        unitTxt.setFillColor(sf::Color(148, 163, 184));
        unitTxt.setPosition(formX + (formW - 96.f) - unitTxt.getLocalBounds().width - 14.f, 340.f + 13.f);
        window.draw(unitTxt);
    }
    if (btnRoomCapMinus) btnRoomCapMinus->draw(window);
    if (btnRoomCapPlus) btnRoomCapPlus->draw(window);

    // Field 3: LOẠI CÔNG NGHỆ & MÀU NHẬN DIỆN (y = 406.f, 5 tall cards at y = 434.f)
    sf::Text f3Lbl(toSfString("LOẠI CÔNG NGHỆ & MÀU NHẬN DIỆN"), Theme::FontPrimary, 11);
    f3Lbl.setStyle(sf::Text::Bold);
    f3Lbl.setFillColor(sf::Color(51, 65, 85));
    f3Lbl.setPosition(formX, 406.f);
    window.draw(f3Lbl);

    sf::Color techDotColors[5] = {
        sf::Color(14, 165, 233),  // Standard (Sky blue)
        sf::Color(245, 158, 11),  // VIP (Amber)
        sf::Color(147, 51, 234),  // IMAX (Purple)
        sf::Color(249, 115, 22),  // 4DX (Orange)
        sf::Color(6, 182, 212)    // 10D VR (Teal/Cyan)
    };
    std::string techCardLabels[5] = {"Standard", "VIP", "IMAX", "4DX", "10D VR"};
    float techGap = 10.f;
    float techW = (formW - 4.f * techGap) / 5.f; // 86.4f
    float techY = 434.f;
    float techH = 74.f;

    for (size_t t = 0; t < 5; ++t) {
        float cardX = formX + t * (techW + techGap);
        bool isSelTech = (roomFormTechIdx == static_cast<int>(t));
        bool isHoverTech = (mPos.x >= cardX && mPos.x <= cardX + techW && mPos.y >= techY && mPos.y <= techY + techH);

        sf::Color cBg = isSelTech ? sf::Color(236, 254, 255) : (isHoverTech ? sf::Color(248, 250, 252) : sf::Color::White);
        sf::Color cBdr = isSelTech ? sf::Color(8, 145, 178) : sf::Color(226, 232, 240);
        float bdrThick = isSelTech ? 2.f : 1.f;

        RoundedRectRenderer::render(window, sf::Vector2f(cardX, techY), sf::Vector2f(techW, techH), 10.f, cBg, bdrThick, cBdr);

        // Colored circle centered in upper half of card
        float circleR = 9.5f;
        sf::CircleShape tCircle(circleR);
        tCircle.setFillColor(techDotColors[t]);
        tCircle.setPosition(std::round(cardX + (techW - circleR * 2.f) / 2.f), techY + 14.f);
        window.draw(tCircle);

        // Label centered in lower half of card
        sf::Text tLbl(toSfString(techCardLabels[t]), Theme::FontPrimary, 12);
        tLbl.setStyle(sf::Text::Bold);
        tLbl.setFillColor(isSelTech ? sf::Color(15, 23, 42) : sf::Color(71, 85, 105));
        tLbl.setPosition(std::round(cardX + (techW - tLbl.getLocalBounds().width) / 2.f), techY + 44.f);
        window.draw(tLbl);
    }

    // Field 4: TRẠNG THÁI HOẠT ĐỘNG (y = 530.f, dropdown at y = 556.f)
    sf::Text f4Lbl(toSfString("TRẠNG THÁI HOẠT ĐỘNG"), Theme::FontPrimary, 11);
    f4Lbl.setStyle(sf::Text::Bold);
    f4Lbl.setFillColor(sf::Color(51, 65, 85));
    f4Lbl.setPosition(formX, 530.f);
    window.draw(f4Lbl);

    if (roomStatusDropdown) {
        roomStatusDropdown->draw(window);
    }

    // 4 Action Buttons (2x2 Grid at y = 632.f and y = 690.f)
    float halfBtnW = (formW - 16.f) / 2.f; // 228.f
    if (btnRoomCreate) {
        btnRoomCreate->draw(window);
        // Plus-in-circle icon
        sf::CircleShape ic(7.5f);
        ic.setFillColor(sf::Color::White);
        ic.setPosition(formX + 58.f, 632.f + 15.5f);
        window.draw(ic);
        sf::Text pTxt(toSfString("+"), Theme::FontPrimary, 13);
        pTxt.setStyle(sf::Text::Bold);
        pTxt.setFillColor(sf::Color(5, 150, 105));
        pTxt.setPosition(formX + 61.5f, 632.f + 13.5f);
        window.draw(pTxt);
    }
    if (btnRoomUpdate) {
        btnRoomUpdate->draw(window);
        // Floppy save icon
        float ux = formX + halfBtnW + 16.f + 62.f;
        float uy = 632.f + 16.f;
        RoundedRectRenderer::render(window, sf::Vector2f(ux, uy), sf::Vector2f(13.f, 13.f), 2.f, sf::Color::Transparent, 1.5f, sf::Color::White);
        sf::RectangleShape uTop(sf::Vector2f(7.f, 4.f));
        uTop.setFillColor(sf::Color::White);
        uTop.setPosition(ux + 3.f, uy + 1.f);
        window.draw(uTop);
    }
    if (btnRoomDelete) {
        btnRoomDelete->draw(window);
        // Trash bin icon
        float dx = formX + 74.f;
        float dy = 690.f + 16.f;
        sf::RectangleShape dLid(sf::Vector2f(13.f, 2.f));
        dLid.setFillColor(sf::Color::White);
        dLid.setPosition(dx - 1.f, dy);
        window.draw(dLid);
        RoundedRectRenderer::render(window, sf::Vector2f(dx + 1.f, dy + 3.f), sf::Vector2f(9.f, 11.f), 1.5f, sf::Color::Transparent, 1.5f, sf::Color::White);
    }
    if (btnRoomReset) {
        btnRoomReset->draw(window);
        // Undo / Reset circular arrow icon
        float rx = formX + halfBtnW + 16.f + 50.f;
        float ry = 690.f + 16.f;
        sf::CircleShape rArc(6.f);
        rArc.setFillColor(sf::Color::Transparent);
        rArc.setOutlineThickness(1.6f);
        rArc.setOutlineColor(sf::Color(71, 85, 105));
        rArc.setPosition(rx, ry + 1.f);
        window.draw(rArc);
    }

    // Bottom info banner inside Right Card (y = 752.f, h = 50.f)
    RoundedRectRenderer::render(window, sf::Vector2f(formX, 752.f), sf::Vector2f(formW, 50.f), 8.f,
                                sf::Color(240, 249, 255), 1.f, sf::Color(186, 230, 253));
    sf::CircleShape infoDot(4.f);
    infoDot.setFillColor(sf::Color(2, 132, 199));
    infoDot.setPosition(formX + 14.f, 764.f);
    window.draw(infoDot);

    sf::Text infoTitle(toSfString("Đồng bộ dữ liệu SQLite (DATA/cinema.db):"), Theme::FontPrimary, 11);
    infoTitle.setStyle(sf::Text::Bold);
    infoTitle.setFillColor(sf::Color(2, 132, 199));
    infoTitle.setPosition(formX + 28.f, 759.f);
    window.draw(infoTitle);

    sf::Text infoDesc(toSfString("Tự động khởi tạo và cập nhật sơ đồ " + std::to_string(roomFormCapacity) + " ghế ngồi theo công nghệ " + techCardLabels[std::clamp(roomFormTechIdx, 0, 4)] + "."), Theme::FontPrimary, 11);
    infoDesc.setFillColor(sf::Color(71, 85, 105));
    infoDesc.setPosition(formX + 28.f, 779.f);
    window.draw(infoDesc);

    // ==========================================
    // 6. BOTTOM STATISTICAL FOOTER (y = 838.f, h = 76.f)
    // Linked to live SQLite data as statistical display
    // ==========================================
    float kpiY = 838.f;
    float kpiH = 76.f;
    float kpiGap = 16.f;

    // 3 Statistical Cards + 1 Support/Copyright Card
    float card1W = 240.f;
    float card2W = 260.f;
    float card3W = 280.f;
    float card4W = totalW - (card1W + card2W + card3W + 3.f * kpiGap); // 592.f

    // KPI 1: TỔNG SỐ PHÒNG (Soft Sky-Blue Card)
    float c1X = leftColX;
    RoundedRectRenderer::render(window, sf::Vector2f(c1X, kpiY), sf::Vector2f(card1W, kpiH), 12.f, sf::Color(224, 242, 254), 1.f, sf::Color(125, 211, 252));
    RoundedRectRenderer::render(window, sf::Vector2f(c1X + 16.f, kpiY + 17.f), sf::Vector2f(42.f, 42.f), 10.f, sf::Color(2, 132, 199));
    sf::RectangleShape c1Ic(sf::Vector2f(18.f, 14.f));
    c1Ic.setPosition(c1X + 28.f, kpiY + 31.f);
    c1Ic.setFillColor(sf::Color::White);
    window.draw(c1Ic);

    sf::Text c1Lbl(toSfString("TỔNG SỐ PHÒNG"), Theme::FontPrimary, 10);
    c1Lbl.setStyle(sf::Text::Bold);
    c1Lbl.setFillColor(sf::Color(3, 105, 161));
    c1Lbl.setPosition(c1X + 70.f, kpiY + 16.f);
    window.draw(c1Lbl);

    char c1ValBuf[32];
    snprintf(c1ValBuf, sizeof(c1ValBuf), "%02zu Phòng", roomList.size());
    sf::Text c1Val(toSfString(c1ValBuf), Theme::FontPrimary, 17);
    c1Val.setStyle(sf::Text::Bold);
    c1Val.setFillColor(sf::Color(12, 74, 110));
    c1Val.setPosition(c1X + 70.f, kpiY + 36.f);
    window.draw(c1Val);

    // KPI 2: SỨC CHỨA TOÀN CỤM (Soft Cyan-Blue Card)
    float c2X = c1X + card1W + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c2X, kpiY), sf::Vector2f(card2W, kpiH), 12.f, sf::Color(207, 250, 254), 1.f, sf::Color(103, 232, 249));
    RoundedRectRenderer::render(window, sf::Vector2f(c2X + 16.f, kpiY + 17.f), sf::Vector2f(42.f, 42.f), 10.f, sf::Color(8, 145, 178));
    // Sofa icon inside box
    RoundedRectRenderer::render(window, sf::Vector2f(c2X + 29.f, kpiY + 30.f), sf::Vector2f(16.f, 10.f), 2.f, sf::Color::White);
    RoundedRectRenderer::render(window, sf::Vector2f(c2X + 26.f, kpiY + 37.f), sf::Vector2f(22.f, 8.f), 2.f, sf::Color(207, 250, 254));

    sf::Text c2Lbl(toSfString("SỨC CHỨA TOÀN CỤM"), Theme::FontPrimary, 10);
    c2Lbl.setStyle(sf::Text::Bold);
    c2Lbl.setFillColor(sf::Color(14, 116, 144));
    c2Lbl.setPosition(c2X + 70.f, kpiY + 16.f);
    window.draw(c2Lbl);

    int totalSeatsSum = 0;
    for (const auto& rm : roomList) totalSeatsSum += rm.getSucChua();
    char c2ValBuf[32];
    snprintf(c2ValBuf, sizeof(c2ValBuf), "%d Ghế", totalSeatsSum);
    sf::Text c2Val(toSfString(c2ValBuf), Theme::FontPrimary, 17);
    c2Val.setStyle(sf::Text::Bold);
    c2Val.setFillColor(sf::Color(2, 132, 199));
    c2Val.setPosition(c2X + 70.f, kpiY + 36.f);
    window.draw(c2Val);

    // KPI 3: TỶ LỆ BẢO DƯỠNG (Soft Emerald-Green Card)
    float c3X = c2X + card2W + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c3X, kpiY), sf::Vector2f(card3W, kpiH), 12.f, sf::Color(220, 252, 231), 1.f, sf::Color(134, 239, 172));
    RoundedRectRenderer::render(window, sf::Vector2f(c3X + 16.f, kpiY + 17.f), sf::Vector2f(42.f, 42.f), 10.f, sf::Color(22, 163, 74));
    // Shield / Check icon inside box
    RoundedRectRenderer::render(window, sf::Vector2f(c3X + 29.f, kpiY + 29.f), sf::Vector2f(16.f, 18.f), 4.f, sf::Color::White);

    sf::Text c3Lbl(toSfString("TỶ LỆ BẢO DƯỠNG"), Theme::FontPrimary, 10);
    c3Lbl.setStyle(sf::Text::Bold);
    c3Lbl.setFillColor(sf::Color(21, 128, 61));
    c3Lbl.setPosition(c3X + 70.f, kpiY + 16.f);
    window.draw(c3Lbl);

    int activeRoomsCount = 0;
    for (const auto& rm : roomList) {
        if (rm.getTrangThaiPhong() != "TamNgung") activeRoomsCount++;
    }
    int readyPct = roomList.empty() ? 100 : static_cast<int>(std::round((activeRoomsCount * 100.0) / roomList.size()));
    char c3ValBuf[48];
    snprintf(c3ValBuf, sizeof(c3ValBuf), "%d%% Sẵn sàng", readyPct);
    sf::Text c3Val(toSfString(c3ValBuf), Theme::FontPrimary, 17);
    c3Val.setStyle(sf::Text::Bold);
    c3Val.setFillColor(sf::Color(22, 163, 74));
    c3Val.setPosition(c3X + 70.f, kpiY + 36.f);
    window.draw(c3Val);

    // Card 4: Support & Copyright Card (matching reference bottom-right footer)
    float c4X = c3X + card3W + kpiGap;
    RoundedRectRenderer::render(window, sf::Vector2f(c4X, kpiY), sf::Vector2f(card4W, kpiH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

    sf::Text supTxt(toSfString("Hỗ trợ kỹ thuật: 1900 8080 (Ext: 104)   •   Đồng bộ SQLite Realtime"), Theme::FontPrimary, 12);
    supTxt.setStyle(sf::Text::Bold);
    supTxt.setFillColor(sf::Color(51, 65, 85));
    supTxt.setPosition(c4X + 20.f, kpiY + 16.f);
    window.draw(supTxt);

    sf::Text footCpr(toSfString("© 2026 Cinestar CineOS • Bản quyền thuộc Cinestar Cinema Complex"), Theme::FontPrimary, 12);
    footCpr.setFillColor(sf::Color(100, 116, 139));
    footCpr.setPosition(c4X + 20.f, kpiY + 42.f);
    window.draw(footCpr);
}
} // namespace UI