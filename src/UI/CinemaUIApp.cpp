#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define byte win_byte_override
#include <windows.h>
#undef byte
#endif

#include "../../include/UI/CinemaUIApp.h"
#include "../../include/UI/CinemaUIApp_ShowtimeHelpers.h"
#include "../../include/ActivityLog.h"
#include "../../include/SysLog.h"
#include "../../include/DateFormat.h"

#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sqlite3.h>
#include <set>
#include <map>

#include "../../include/UI/CinemaUIApp_ShowtimeHelpers.h"

namespace UI {


CinemaUIApp::CinemaUIApp()
    : isLoggedIn(false),
      currentUserId("U001"),
      currentUserName("Chưa đăng nhập"),
      currentUserRole("Admin"),
      loginErrorMsg(""),
      currentTab(ActiveTab::Reports),
      sidebarWidth(250.f),
      topbarHeight(68.f),
      movieSubTab(0),
      selectedMovieRowIdx(-1),
      selectedGenreRowIdx(-1),
      selectedGenreChoiceIdx(0),
      movieAgeFilter(""),
      moviePosActive(true),
      movieCurrentPage(0),
      showMovieSpotlight(false),
      spotlightMovieIdx(-1),
      showAgeInputDialog(false),
      bookingTotalPrice(0.0),
      posSeatTotal(0.0),
      posFnbTotal(0.0),
      posGrandTotal(0.0),
      posSelectedMovieIdx(0),
      posSelectedShowtimeIdx(0),
      posShowSeatMap(true),
      showtimeSubTab(0),
      selectedShowtimeRowIdx(0),
      showtimeCurrentPage(0),
      stLastValidMinutes(540),
      stDateFilterVal(""),
      stRoomFilterVal(""),
      stFormMovieIdx(0),
      stFormRoomIdx(0),
      stCleaningMinutes(15),
      selectedRoomRowIdx(0),
      roomCurrentPage(0),
      roomSearchQuery(""),
      roomFilterType(0),
      roomFormTechIdx(0),
      roomFormCapacity(12),
      roomFormStatusIdx(0),
      seatSubTab(0),
      seatSelectedRoomId("R01"),
      seatViewMode(1),
      seatZoomLevel(1.0f),
      seatListPage(0),
      seatListSearchQuery(""),
      selectedSeatRowIdx(-1),
      selectedSeatTypeRowIdx(0),
      seatTypeFilterIdx(0),
      seatFormTypeCode("SEAT-STD-05"),
      seatTypeColorIdx(0),
      seatTypeSaleActive(true),
      comboItem1Idx(0),
      comboItem1Qty(1),
      comboItem2Idx(1),
      comboItem2Qty(1),
      selectedPromoRowIdx(0),
      promoCurrentPage(0),
      promoFilterTab(0),
      promoScopeVal("All"),
      selectedStaffRowIdx(0),
      staffCurrentPage(0),
      staffFilterTab(0),
      staffFormResigned(false),
      selectedAccountRowIdx(0),
      accountCurrentPage(0),
      accountFilterTab(0),
      accountShowPassword(false),
      auditFilterType(0),
      auditCurrentPage(0),
      auditTableFilterVal(""),
      auditDateFilterVal(""),
      reportFilterTab(0),
      reportCurrentPage(0),
      reportSearchQuery(""),
      reportTotalRevenue(0.0),
      reportCashRevenue(0.0),
      reportQrRevenue(0.0),
      reportCardRevenue(0.0),
      reportTicketCount(0),
      reportFnbCount(0),
      reportTotalTxCount(0),
      reportCashDifference(0.0),
      reportIsBalanced(true)
{
}

bool CinemaUIApp::init() {
    Theme::loadFonts();

    // Create main window 1728 x 972 at 60 FPS
    window.create(
        sf::VideoMode(1728, 972),
        toSfString("HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM - CINEMA MANAGEMENT SYSTEM"),
        sf::Style::Close | sf::Style::Titlebar
    );
    window.setFramerateLimit(60);

    if (cinemaLogoTexture.loadFromFile("assets/cinema_logo.png")) {
        cinemaLogoTexture.setSmooth(true);
        cinemaLogoSprite.setTexture(cinemaLogoTexture);
    }

    loadDataFromDatabase();
    setupLoginView();
    setupUIComponents();

    return true;
}

void CinemaUIApp::loadDataFromDatabase() {
    movieList.clear();
    roomList.clear();
    allSeats.clear();
    showtimeList.clear();
    fnbList.clear();
    genreList.clear();
    customerList.clear();
    promoList.clear();
    shiftList.clear();
    invoiceList.clear();
    ticketList.clear();
    staffUserList.clear();

    // Read real data directly from SQLite DATA/cinema.db via Database class
    db.docDanhSachPhim(movieList);
    db.docDanhSachPhong(roomList);
    db.docDanhSachGhe(allSeats);
    db.docDanhSachSuatChieu(showtimeList);
    db.docDanhSachFAndB(fnbList);
    db.docDanhSachTheLoai(genreList);
    db.docDanhSachUsers(customerList);
    db.docDanhSachKhuyenMai(promoList);
    db.docDanhSachCa(shiftList);
    db.docDanhSachHoaDon(invoiceList);
    db.docDanhSachVe(ticketList);

    // Load staff & admin accounts from SQLite users table
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql =
            "SELECT maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,"
            "taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan "
            "FROM users WHERE vaiTro IN ('Admin','Staff') ORDER BY rowid";
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                auto colStr = [&](int c) -> std::string {
                    const unsigned char* txt = sqlite3_column_text(stmt, c);
                    return txt ? reinterpret_cast<const char*>(txt) : "";
                };
                StaffAccountInfo info;
                info.maNguoiDung = colStr(0);
                info.hoTen = colStr(1);
                info.soDienThoai = colStr(2);
                info.email = colStr(3);
                info.ngaySinh = colStr(4);
                info.gioiTinh = colStr(5);
                info.cccd = colStr(6);
                info.taiKhoan = colStr(7);
                info.matKhau = colStr(8);
                info.vaiTro = colStr(9);
                info.trangThaiLamViec = colStr(10);
                info.trangThaiTaiKhoan = colStr(11);
                staffUserList.push_back(info);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(database);
    }

    fnbQuantities.assign(fnbList.size(), 0);
    reloadMoviePosterTextures();
    reloadFnbImageTextures();
}


void CinemaUIApp::setupUIComponents() {
    setupNavigation();

    // Top Bar Search
    searchBar = std::unique_ptr<GlobalSearchBar>(new GlobalSearchBar(
        sf::Vector2f(sidebarWidth + 24.f, (topbarHeight - 40.f) / 2.0f),
        sf::Vector2f(450.f, 40.f),
        "Tìm kiếm phim, lịch chiếu, khách hàng, vé..."
    ));

    // Confirm Modal Dialog
    confirmPopup = std::unique_ptr<UserSelectPopup>(new UserSelectPopup(sf::Vector2f(1728.f, 972.f)));

    // Initialize Action Buttons for All 13 Tabs (Linked from Cinema_main.cpp)
    float btnTopY = topbarHeight + 20.f;
    float rightBtnX = sidebarWidth + 1360.f + 30.f - 210.f; // ~1460.f

    btnCheckInTicket = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Soat Ve Check-in",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnCheckInTicket->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "SOÁT VÉ / CHECK-IN PHÒNG CHIẾU",
            "Xác nhận quét mã vé xem phim và cho phép khách vào phòng chiếu?",
            ButtonStyle::Primary,
            "Quét & Vào Cửa",
            "Đóng"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHECKIN_VE", "Soát vé thành công cho phòng chiếu");
        });
    });

    btnAddCustomer = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Dang Ky Hoi Vien",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddCustomer->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "ĐĂNG KÝ HỘI VIÊN C-FRIEND",
            "Đăng ký thẻ thành viên mới để nhận tích lũy điểm 5% và ưu đãi sinh nhật?",
            ButtonStyle::Primary,
            "Đăng Ký Ngay",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_KY_CRM", "Đăng ký hội viên C-Friend thành công");
        });
    });

    btnReportAll = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tat Ca",
        sf::Vector2f(rightBtnX - 320.f, btnTopY),
        sf::Vector2f(100.f, 36.f),
        ButtonStyle::Primary,
        6.f
    ));
    btnReportCash = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tien Mat",
        sf::Vector2f(rightBtnX - 210.f, btnTopY),
        sf::Vector2f(100.f, 36.f),
        ButtonStyle::Dark,
        6.f
    ));
    btnReportBank = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "The / QR",
        sf::Vector2f(rightBtnX - 100.f, btnTopY),
        sf::Vector2f(100.f, 36.f),
        ButtonStyle::Dark,
        6.f
    ));

    btnAddRoom = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Them Phong Chieu",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddRoom->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "THÊM PHÒNG CHIẾU MỚI",
            "Thêm phòng chiếu R08 (2D/3D Laser, Sức chứa: 120 Ghế, Dolby Atmos 7.1)?",
            ButtonStyle::Primary,
            "Thêm Phòng",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            Room r("R08", "Phòng Chiếu 8", "2D", 120, "HoatDong");
            roomList.push_back(r);
            db.ghiDanhSachPhong(roomList);
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHONG", "Thêm phòng chiếu mới: R08");
        });
    });

    btnAutoGenerateSeats = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Tao So Do Ghe",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAutoGenerateSeats->setOnClick([this]() {
        if (!confirmPopup) return;
        std::string targetRoomId = seatSelectedRoomId;
        if (targetRoomId.empty() && !roomList.empty()) targetRoomId = roomList[0].getMaPhong();
        int targetCap = 120;
        std::string targetRoomName = targetRoomId;
        for (const auto& rm : roomList) {
            if (rm.getMaPhong() == targetRoomId) {
                targetCap = std::max(1, rm.getSucChua());
                targetRoomName = rm.getTenPhong();
                break;
            }
        }
        confirmPopup->show(
            "TỰ ĐỘNG SINH SƠ ĐỒ GHẾ",
            "Tự động tính toán và tạo sơ đồ ghế tiêu chuẩn (" + std::to_string(targetCap) + " chỗ) cho phòng " + targetRoomName + " (" + targetRoomId + ")?",
            ButtonStyle::Primary,
            "Tạo Tự Động",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this, targetRoomId, targetCap, targetRoomName]() {
            if (targetRoomId.empty()) return;
            for (const auto& s : allSeats) {
                if (s.getMaPhong() == targetRoomId && isSeatBookedInRoom(targetRoomId, s.getMaGhe())) {
                    if (confirmPopup) {
                        confirmPopup->show("CẢNH BÁO NGHIỆP VỤ", "Phòng " + targetRoomName + " đang có ghế đã bán vé trong suất chiếu sắp tới. Không thể tạo đè sơ đồ!", ButtonStyle::Danger, "Đóng", "");
                    }
                    return;
                }
            }

            std::vector<Seat> backupSeats = allSeats;
            allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
                return s.getMaPhong() == targetRoomId;
            }), allSeats.end());

            double stdPrice = 100000.0;
            double vipPrice = 150000.0;
            double cplPrice = 200000.0;
            bool hasStd = false, hasVip = false, hasCpl = false;
            for (const auto& st : seatTypeList) {
                if (st.tenLoai == "Standard" && st.kichHoatBan) { stdPrice = st.donGia; hasStd = true; }
                else if (st.tenLoai == "VIP" && st.kichHoatBan) { vipPrice = st.donGia; hasVip = true; }
                else if ((st.tenLoai == "Couple" || st.tenLoai == "Sweetbox") && st.kichHoatBan) { cplPrice = st.donGia; hasCpl = true; }
            }
            std::string fallbackType = !seatTypeList.empty() ? seatTypeList[0].tenLoai : "Standard";
            double fallbackPrice = !seatTypeList.empty() ? seatTypeList[0].donGia : 100000.0;

            int colsPerRow = (targetCap <= 24) ? 6 : ((targetCap <= 60) ? 10 : ((targetCap <= 120) ? 12 : 15));
            int remainingSlots = targetCap;
            int totalRowsEst = (targetCap + colsPerRow - 1) / colsPerRow;
            int rowIdx = 0;
            int createdRecords = 0;

            while (remainingSlots > 0 && rowIdx < 26) {
                char rowChar = static_cast<char>('A' + rowIdx);
                int rowSlots = std::min(colsPerRow, remainingSlots);
                bool isLastRow = (remainingSlots <= colsPerRow);

                if (isLastRow && hasCpl && rowSlots >= 4 && (rowSlots % 2 == 0)) {
                    for (int c = 1; c < rowSlots; c += 2) {
                        std::string pairCode = std::string(1, rowChar) + std::to_string(c) + "-" + std::to_string(c + 1);
                        allSeats.push_back(Seat(pairCode, targetRoomId, "Couple", cplPrice, "Trong"));
                        createdRecords++;
                    }
                } else {
                    std::string rType = fallbackType;
                    double rPrice = fallbackPrice;
                    if (rowIdx < std::max(1, totalRowsEst / 3) && hasStd) {
                        rType = "Standard"; rPrice = stdPrice;
                    } else if (hasVip) {
                        rType = "VIP"; rPrice = vipPrice;
                    } else if (hasStd) {
                        rType = "Standard"; rPrice = stdPrice;
                    }
                    for (int c = 1; c <= rowSlots; ++c) {
                        std::string code = std::string(1, rowChar) + std::to_string(c);
                        allSeats.push_back(Seat(code, targetRoomId, rType, rPrice, "Trong"));
                        createdRecords++;
                    }
                }
                remainingSlots -= rowSlots;
                rowIdx++;
            }

            if (!db.ghiDanhSachGhe(allSeats)) {
                allSeats = backupSeats;
                if (confirmPopup) {
                    confirmPopup->show("LỖI CƠ SỞ DỮ LIỆU", "Không thể lưu sơ đồ ghế mới vào cơ sở dữ liệu SQLite!", ButtonStyle::Danger, "Đóng", "");
                }
                return;
            }

            seatSelectedCodes.clear();
            refreshSeatsRoomDropdown();
            refreshShowtimesDropdowns();
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "TAO_GHE_TU_DONG", "Khởi tạo sơ đồ " + std::to_string(targetCap) + " chỗ (" + std::to_string(createdRecords) + " ghế) cho phòng " + targetRoomId);
            if (confirmPopup) {
                confirmPopup->show("THÀNH CÔNG", "Đã tạo tự động sơ đồ " + std::to_string(targetCap) + " chỗ (" + std::to_string(createdRecords) + " ghế) cho phòng " + targetRoomName + "!", ButtonStyle::Primary, "Đóng", "");
            }
        });
    });

    btnAddShowtime = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Them Suat Chieu",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddShowtime->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "THÊM SUẤT CHIẾU MỚI",
            "Xếp lịch suất chiếu mới cho phim LAT MAT 7 tại Phòng R01 lúc 21:00 - 23:15?",
            ButtonStyle::Primary,
            "Thêm Suất Chiếu",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            Showtime sc("SC99", "P01", "R01", "10/11/2026", "21:00", "23:15", 85000.0);
            showtimeList.push_back(sc);
            if (!db.ghiDanhSachSuatChieu(showtimeList)) {
                showtimeList.pop_back();
                if (confirmPopup) confirmPopup->show("LỖI CƠ SỞ DỮ LIỆU", "Không thể lưu suất chiếu xuống cơ sở dữ liệu!", ButtonStyle::Danger, "Đóng", "");
                return;
            }
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_SUAT_CHIEU", "Thêm suất chiếu mới: SC99");
        });
    });

    btnManageGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "The Loai Phim",
        sf::Vector2f(rightBtnX - 160.f, topbarHeight + 20.f),
        sf::Vector2f(150.f, 38.f),
        ButtonStyle::Dark,
        8.f
    ));
    btnManageGenres->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "QUẢN LÝ THỂ LOẠI PHIM",
            "Cập nhật danh mục thể loại phim: Hành Động, Hài Hước, Kinh Dị, Hoạt Hình, Tâm Lý?",
            ButtonStyle::Primary,
            "Đồng Bộ Thể Loại",
            "Đóng"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "QUAN_LY_THE_LOAI", "Cập nhật thể loại phim thành công");
        });
    });

    btnAddFnb = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Them Mat Hang F&B",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddFnb->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "THÊM MẶT HÀNG F&B MỚI",
            "Thêm sản phẩm mới vào thực đơn quầy bắp nước CineStar?",
            ButtonStyle::Primary,
            "Thêm Vào Menu",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            FoodAndBeverage f("FB99", "Bắp Rang Bơ Phô Mai Đặc Biệt", "Bap", 75000.0, "DangKinhDoanh");
            fnbList.push_back(f);
            fnbQuantities.push_back(0);
            db.ghiDanhSachFAndB(fnbList);
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_FNB", "Thêm sản phẩm F&B: FB99");
        });
    });

    btnApproveShift = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Duyet Bao Cao Ca",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnApproveShift->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "DUYỆT BÁO CÁO KẾT TOÁN CA",
            "Phê duyệt kết toán quỹ tiền mặt và doanh thu cho ca làm việc hiện tại?",
            ButtonStyle::Primary,
            "Duyệt Báo Cáo",
            "Đóng"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DUYET_BAO_CAO_CA", "Phê duyệt báo cáo ca trực thành công");
        });
    });

    btnCloseShift = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Chot Ca & Nop Tien",
        sf::Vector2f(rightBtnX - 220.f, btnTopY),
        sf::Vector2f(200.f, 38.f),
        ButtonStyle::Dark,
        8.f
    ));
    btnCloseShift->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "CHỐT CA LÀM VIỆC & NỘP DOANH THU",
            "Khóa sổ ca làm việc, in phiếu nộp tiền mặt và bàn giao ca trực tiếp theo?",
            ButtonStyle::Primary,
            "Chốt Ca Ngay",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHOT_CA", "Chốt ca làm việc và nộp tiền thành công");
        });
    });

    btnAddPromo = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Them Khuyen Mai",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddPromo->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "THÊM CHƯƠNG TRÌNH KHUYẾN MÃI",
            "Tạo chương trình khuyến mãi / Voucher giảm giá mới cho rạp CineStar?",
            ButtonStyle::Primary,
            "Tạo Khuyến Mãi",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_KHUYEN_MAI", "Tạo chương trình khuyến mãi mới thành công");
        });
    });

    btnAddStaff = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Them Nhan Vien",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnAddStaff->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "THÊM NHÂN VIÊN MỚI",
            "Tạo hồ sơ nhân sự mới và cấp quyền truy cập hệ thống CineStar?",
            ButtonStyle::Primary,
            "Tạo Nhân Sự",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_NHAN_SU", "Tạo nhân sự mới thành công");
        });
    });

    btnChangePassword = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Doi Mat Khau",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnChangePassword->setOnClick([this]() {
        if (!confirmPopup) return;
        confirmPopup->show(
            "ĐỔI MẬT KHẨU TRUY CẬP",
            "Cập nhật mật khẩu tài khoản bảo mật định kỳ 90 ngày của hệ thống?",
            ButtonStyle::Primary,
            "Lưu Mật Khẩu",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DOI_MAT_KHAU", "Đổi mật khẩu tài khoản thành công");
        });
    });

    btnRefreshLogs = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Lam Moi Nhat Ky",
        sf::Vector2f(rightBtnX, btnTopY),
        sf::Vector2f(210.f, 38.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnRefreshLogs->setOnClick([this]() {
        setupLogsView();
        if (confirmPopup) {
            confirmPopup->show(
                "ĐỒNG BỘ NHẬT KÝ HỆ THỐNG",
                "Đã tải lại toàn bộ nhật ký kiểm toán mới nhất từ SQLite (DATA/cinema.db)!",
                ButtonStyle::Primary,
                "OK",
                "Đóng"
            );
        }
    });

    setupDashboardView();
    setupMoviesView();
    setupRoomsView();
    setupSeatsView();
    setupSeatBookingView();
    setupCRMView();
    setupShiftReportView();
    setupConcessionsView();
    setupShowtimesView();
    setupPromotionsView();
    setupStaffView();
    setupAccountsView();
    setupLogsView();
    setupReportsView();
}

void CinemaUIApp::setupNavigation() {
    std::vector<std::pair<std::string, ActiveTab>> navItems;
    if (currentUserRole == "Admin" || currentUserRole == "Khách") {
        navItems = {
            {"Quản lý Doanh thu",    ActiveTab::Reports},
            {"Quản lý phòng",        ActiveTab::Rooms},
            {"Quản lý ghế",          ActiveTab::Seats},
            {"Quản lý suất chiếu",   ActiveTab::Showtimes},
            {"Phim & Thể loại",      ActiveTab::Movies},
            {"Quản lý F&B",          ActiveTab::Concessions},
            {"Quản lý Báo cáo ca",   ActiveTab::ShiftReport},
            {"Khuyến mãi SK",        ActiveTab::Promotions},
            {"Quản lý nhân sự",      ActiveTab::Staff},
            {"Quản lý tài khoản",    ActiveTab::Accounts},
            {"Nhật ký hoạt động",    ActiveTab::SystemLogs}
        };
    } else {
        navItems = {
            {"Bán vé (POS)",         ActiveTab::SeatBooking},
            {"Khách hàng (CRM)",     ActiveTab::CRM},
            {"Chốt ca & Báo cáo",    ActiveTab::ShiftReport}
        };
    }

    navButtons.clear();
    float startY = (currentUserRole == "Admin" || currentUserRole == "Khách") ? 72.f : 92.f;
    float itemH = 39.f;

    for (size_t i = 0; i < navItems.size(); ++i) {
        auto btn = std::unique_ptr<RoundRectButton>(new RoundRectButton(
            "",
            sf::Vector2f(0.f, startY + static_cast<float>(i) * itemH),
            sf::Vector2f(sidebarWidth, itemH),
            ButtonStyle::Dark,
            0.f
        ));

        ActiveTab targetTab = navItems[i].second;
        btn->setOnClick([this, targetTab]() {
            currentTab = targetTab;
            if (targetTab == ActiveTab::Reports) {
                reloadReportsData();
            } else if (targetTab == ActiveTab::Concessions) {
                auto rIndices = getRetailFnbIndices();
                std::vector<std::string> retailOpts;
                for (int ri : rIndices) {
                    if (ri >= 0 && ri < static_cast<int>(fnbList.size())) {
                        retailOpts.push_back(fnbList[ri].getTenMatHang() + " (" + std::to_string(static_cast<long long>(fnbList[ri].getGiaTien())) + " đ)");
                    }
                }
                if (retailOpts.empty()) retailOpts.push_back("Chọn món lẻ...");
                if (comboItem1Dropdown) comboItem1Dropdown->setOptions(retailOpts, (comboItem1Idx >= 0 && comboItem1Idx < static_cast<int>(retailOpts.size())) ? comboItem1Idx : 0);
                if (comboItem2Dropdown) comboItem2Dropdown->setOptions(retailOpts, (comboItem2Idx >= 0 && comboItem2Idx < static_cast<int>(retailOpts.size())) ? comboItem2Idx : (retailOpts.size() > 1 ? 1 : 0));
            } else if (targetTab == ActiveTab::CRM) {
                reloadCRMData();
            } else if (targetTab == ActiveTab::ShiftReport) {
                adminShowHistorySubView = false;
                reloadShiftReportData();
            } else if (targetTab == ActiveTab::Showtimes) {
                refreshShowtimesDropdowns();
            } else if (targetTab == ActiveTab::Promotions) {
                refreshPromoScopeDropdown();
            } else if (targetTab == ActiveTab::Staff) {
                if (!staffUserList.empty()) {
                    if (selectedStaffRowIdx < 0 || selectedStaffRowIdx >= static_cast<int>(staffUserList.size())) {
                        selectedStaffRowIdx = 0;
                    }
                    syncStaffSelectionToForm(selectedStaffRowIdx);
                }
            } else if (targetTab == ActiveTab::Accounts) {
                refreshAccountEmployeeDropdown();
                if (!staffUserList.empty()) {
                    if (selectedAccountRowIdx < 0 || selectedAccountRowIdx >= static_cast<int>(staffUserList.size())) {
                        selectedAccountRowIdx = 0;
                    }
                    syncAccountSelectionToForm(selectedAccountRowIdx);
                }
            } else if (targetTab == ActiveTab::Rooms) {
                if (!roomList.empty()) {
                    if (selectedRoomRowIdx < 0 || selectedRoomRowIdx >= static_cast<int>(roomList.size())) {
                        selectedRoomRowIdx = 0;
                    }
                    syncRoomSelectionToForm(selectedRoomRowIdx);
                }
            }
        });

        navButtons.push_back(std::move(btn));
    }

    // Logout button at bottom of sidebar
    btnLogout = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Đăng Xuất",
        sf::Vector2f(26.f, 918.f),
        sf::Vector2f(sidebarWidth - 52.f, 28.f),
        ButtonStyle::Danger,
        Theme::RadiusSmall
    ));

    btnLogout->setOnClick([this]() {
        if (currentUserRole != "Admin" && !shiftIsClosed) {
            if (confirmPopup) {
                confirmPopup->show(
                    "CHƯA CHỐT CA TRỰC",
                    "Bạn chưa thực hiện Khóa sổ & Chốt ca!\nVui lòng vào mục 'Chốt ca & Báo cáo' để bàn giao và chốt ca trước khi đăng xuất.",
                    ButtonStyle::Danger,
                    "Đã hiểu",
                    ""
                );
            }
            return;
        }
        if (isLoggedIn) {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_XUAT", "Đăng xuất khỏi hệ thống");
        }
        isLoggedIn = false;
        loginInputUser->setText("");
        loginInputPass->setText("");
        loginErrorMsg = "";
    });
}


void CinemaUIApp::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        handleEvents();
        update(dt);
        render();
    }
}

void CinemaUIApp::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
            return;
        }

        sf::Vector2i pixelMouse = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelMouse);
        if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased) {
            mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
        } else if (event.type == sf::Event::MouseMoved) {
            mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        } else if (event.type == sf::Event::MouseWheelScrolled) {
            mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y));
        }

        // ==========================================
        // 1. IF NOT LOGGED IN -> ONLY PROCESS LOGIN
        // ==========================================
        if (!isLoggedIn) {
            if (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Enter)) {
                std::string u = loginInputUser ? loginInputUser->getText() : "";
                std::string p = loginInputPass ? loginInputPass->getText() : "";
                authenticateUser(u, p);
                continue;
            }
            if (loginInputUser) {
                if (event.type == sf::Event::MouseButtonPressed) loginInputUser->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                else if (event.type == sf::Event::TextEntered) loginInputUser->handleTextEntered(event.text.unicode);
            }
            if (loginInputPass) {
                if (event.type == sf::Event::MouseButtonPressed) loginInputPass->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                else if (event.type == sf::Event::TextEntered) loginInputPass->handleTextEntered(event.text.unicode);
            }
            if (chkRememberMe) {
                if (event.type == sf::Event::MouseMoved) chkRememberMe->handleMouseMove(mousePos);
                else if (event.type == sf::Event::MouseButtonPressed) chkRememberMe->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            }
            if (btnLogin) {
                if (event.type == sf::Event::MouseMoved) btnLogin->handleMouseMove(mousePos);
                else if (event.type == sf::Event::MouseButtonPressed) btnLogin->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                else if (event.type == sf::Event::MouseButtonReleased) btnLogin->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            }
            continue;
        }

        // ==========================================
        // 2. LOGGED IN -> PROCESS MAIN UI
        // ==========================================
        if (confirmPopup && confirmPopup->getIsOpen()) {
            if (event.type == sf::Event::MouseMoved) confirmPopup->handleMouseMove(mousePos);
            else if (event.type == sf::Event::MouseButtonPressed) confirmPopup->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            else if (event.type == sf::Event::MouseButtonReleased) confirmPopup->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            continue;
        }

        if (showAgeInputDialog) {
            if (event.type == sf::Event::MouseButtonPressed) {
                if (inputNewAgeRating) inputNewAgeRating->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                if (btnCancelNewAge) btnCancelNewAge->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                if (btnSaveNewAge) btnSaveNewAge->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseButtonReleased) {
                if (btnCancelNewAge) btnCancelNewAge->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                if (btnSaveNewAge) btnSaveNewAge->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseMoved) {
                if (btnCancelNewAge) btnCancelNewAge->handleMouseMove(mousePos);
                if (btnSaveNewAge) btnSaveNewAge->handleMouseMove(mousePos);
            } else if (event.type == sf::Event::TextEntered) {
                if (inputNewAgeRating) inputNewAgeRating->handleTextEntered(event.text.unicode);
            }
            continue;
        }

        // Dropdowns top priority
        bool capturedByDropdown = false;
        if (currentTab == ActiveTab::SeatBooking) {
            std::vector<DropdownBox*> posDrops = {
                posDateDropdown.get(),
                posMovieDropdown.get(),
                posShowtimeDropdown.get(),
                posFnbDropdown.get(),
                showtimeDropdown.get()
            };
            for (auto* drp : posDrops) {
                if (drp && drp->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        if (drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos)) {
                            capturedByDropdown = true;
                            break;
                        }
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseMoved) {
                        if (drp->handleMouseMove(mousePos)) {
                            capturedByDropdown = true;
                        }
                    }
                }
            }
            if (capturedByDropdown) continue;
        }

        if (currentTab == ActiveTab::CRM && crmTierDropdown && crmTierDropdown->getExpanded()) {
            if (event.type == sf::Event::MouseWheelScrolled) {
                capturedByDropdown = crmTierDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
            } else if (event.type == sf::Event::MouseButtonPressed) {
                capturedByDropdown = crmTierDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseButtonReleased) {
                capturedByDropdown = crmTierDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseMoved) {
                capturedByDropdown = crmTierDropdown->handleMouseMove(mousePos);
            }
            if (capturedByDropdown) continue;
        }

        if (currentTab == ActiveTab::Movies) {
            std::vector<DropdownBox*> allMovieDrops = {
                movieAgeFilterDropdown.get(),
                movieFormAgeDropdown.get()
            };
            for (auto* drp : allMovieDrops) {
                if (drp && drp->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        if (drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos)) {
                            capturedByDropdown = true;
                            break;
                        }
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseMoved) {
                        if (drp->handleMouseMove(mousePos)) {
                            capturedByDropdown = true;
                        }
                    }
                }
            }
            if (capturedByDropdown) continue;
        }

        if (currentTab == ActiveTab::Showtimes) {
            std::vector<DropdownBox*> allStDrops = {
                stMovieFilterDropdown.get(),
                stDateDropdown.get(),
                stRoomDropdown.get(),
                stMovieDropdown.get(),
                stRoomFormDropdown.get()
            };
            for (auto* drp : allStDrops) {
                if (drp && drp->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        if (drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos)) {
                            capturedByDropdown = true;
                            break;
                        }
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        if (capturedByDropdown) break;
                    } else if (event.type == sf::Event::MouseMoved) {
                        if (drp->handleMouseMove(mousePos)) {
                            capturedByDropdown = true;
                        }
                    }
                }
            }
            if (capturedByDropdown) continue;
        }

        if (currentTab == ActiveTab::Rooms) {
            std::vector<DropdownBox*> allRoomDrops = {
                roomStatusDropdown.get(),
                roomFilterDropdown.get()
            };
            for (auto* drp : allRoomDrops) {
                if (drp && drp->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        capturedByDropdown = drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseMoved) {
                        capturedByDropdown = drp->handleMouseMove(mousePos);
                    }
                    if (capturedByDropdown) break;
                }
            }
            if (capturedByDropdown) continue;
        }
        if (currentTab == ActiveTab::Seats && seatSubTab == 0) {
            auto captureSeatDrp = [&](std::unique_ptr<DropdownBox>& drp) {
                if (!drp || !drp->getExpanded()) return false;
                if (event.type == sf::Event::MouseWheelScrolled) {
                    return drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                } else if (event.type == sf::Event::MouseButtonPressed) {
                    return drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                } else if (event.type == sf::Event::MouseButtonReleased) {
                    return drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                } else if (event.type == sf::Event::MouseMoved) {
                    return drp->handleMouseMove(mousePos);
                }
                return false;
            };
            if (seatSubTab == 0) {
                if (captureSeatDrp(seatRoomDropdown)) continue;
                if (captureSeatDrp(seatBatchTypeDropdown)) continue;
            } else if (seatSubTab == 1) {
                if (captureSeatDrp(seatTypeFilterDropdown)) continue;
            }
        }
        if (currentTab == ActiveTab::Promotions && promoScopeDropdown && promoScopeDropdown->getExpanded()) {
            if (event.type == sf::Event::MouseWheelScrolled) {
                capturedByDropdown = promoScopeDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
            } else if (event.type == sf::Event::MouseButtonPressed) {
                capturedByDropdown = promoScopeDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseButtonReleased) {
                capturedByDropdown = promoScopeDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseMoved) {
                capturedByDropdown = promoScopeDropdown->handleMouseMove(mousePos);
            }
            if (capturedByDropdown) continue;
        }
        if (currentTab == ActiveTab::Staff && staffGenderDropdown && staffGenderDropdown->getExpanded()) {
            if (event.type == sf::Event::MouseWheelScrolled) {
                capturedByDropdown = staffGenderDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
            } else if (event.type == sf::Event::MouseButtonPressed) {
                capturedByDropdown = staffGenderDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseButtonReleased) {
                capturedByDropdown = staffGenderDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseMoved) {
                capturedByDropdown = staffGenderDropdown->handleMouseMove(mousePos);
            }
            if (capturedByDropdown) continue;
        }
        if (currentTab == ActiveTab::Accounts && accountRoleDropdown && accountRoleDropdown->getExpanded()) {
            if (event.type == sf::Event::MouseWheelScrolled) {
                capturedByDropdown = accountRoleDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
            } else if (event.type == sf::Event::MouseButtonPressed) {
                capturedByDropdown = accountRoleDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseButtonReleased) {
                capturedByDropdown = accountRoleDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            } else if (event.type == sf::Event::MouseMoved) {
                capturedByDropdown = accountRoleDropdown->handleMouseMove(mousePos);
            }
            if (capturedByDropdown) continue;
        }
        if (currentTab == ActiveTab::Reports) {
            auto checkReportDrop = [&](std::unique_ptr<DropdownBox>& drop) {
                if (drop && drop->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        capturedByDropdown = drop->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drop->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drop->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseMoved) {
                        capturedByDropdown = drop->handleMouseMove(mousePos);
                    }
                }
            };
            checkReportDrop(reportShiftDropdown);
            if (!capturedByDropdown) checkReportDrop(reportCashierDropdown);
            if (!capturedByDropdown) checkReportDrop(reportPaymentDropdown);
            if (capturedByDropdown) continue;
        }
        if (currentTab == ActiveTab::ShiftReport && (currentUserRole == "Admin" || currentUserRole == "Khách") && !adminShowHistorySubView) {
            if (adminNextCashierDropdown && adminNextCashierDropdown->getExpanded()) {
                if (event.type == sf::Event::MouseWheelScrolled) {
                    capturedByDropdown = adminNextCashierDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                } else if (event.type == sf::Event::MouseButtonPressed) {
                    capturedByDropdown = adminNextCashierDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                } else if (event.type == sf::Event::MouseButtonReleased) {
                    capturedByDropdown = adminNextCashierDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                } else if (event.type == sf::Event::MouseMoved) {
                    capturedByDropdown = adminNextCashierDropdown->handleMouseMove(mousePos);
                }
                if (capturedByDropdown) continue;
            }
        }
        if (currentTab == ActiveTab::Concessions && fnbFormLoai == "Combo") {
            auto checkFnbDrop = [&](std::unique_ptr<DropdownBox>& drop) {
                if (drop && drop->getExpanded()) {
                    if (event.type == sf::Event::MouseWheelScrolled) {
                        capturedByDropdown = drop->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    } else if (event.type == sf::Event::MouseButtonPressed) {
                        capturedByDropdown = drop->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        capturedByDropdown = drop->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseMoved) {
                        capturedByDropdown = drop->handleMouseMove(mousePos);
                    }
                }
            };
            checkFnbDrop(comboItem1Dropdown);
            if (!capturedByDropdown) checkFnbDrop(comboItem2Dropdown);
            if (capturedByDropdown) continue;
        }


        // Search Bar Events
        if (searchBar) {
            if (event.type == sf::Event::MouseButtonPressed) searchBar->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            else if (event.type == sf::Event::TextEntered) searchBar->handleTextEntered(event.text.unicode);
        }

        // Navigation Sidebar Buttons & Logout Button
        if (event.type == sf::Event::MouseMoved) {
            for (auto& btn : navButtons) btn->handleMouseMove(mousePos);
            if (btnLogout) btnLogout->handleMouseMove(mousePos);
        } else if (event.type == sf::Event::MouseButtonPressed) {
            for (auto& btn : navButtons) btn->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            if (btnLogout) btnLogout->handleMouseButtonPressed(event.mouseButton.button, mousePos);
        } else if (event.type == sf::Event::MouseButtonReleased) {
            for (auto& btn : navButtons) btn->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            if (btnLogout) btnLogout->handleMouseButtonReleased(event.mouseButton.button, mousePos);
        }

        // Helper lambda for button events
        auto handleBtn = [&](std::unique_ptr<RoundRectButton>& b) {
            if (!b) return;
            if (event.type == sf::Event::MouseMoved) b->handleMouseMove(mousePos);
            else if (event.type == sf::Event::MouseButtonPressed) b->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            else if (event.type == sf::Event::MouseButtonReleased) b->handleMouseButtonReleased(event.mouseButton.button, mousePos);
        };

        // Active View specific events
        switch (currentTab) {
            case ActiveTab::Dashboard:
                if (dashboardSlider) {
                    if (event.type == sf::Event::MouseMoved) dashboardSlider->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) dashboardSlider->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) dashboardSlider->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                }
                break;

            case ActiveTab::Movies:
                handleBtn(btnSubTabMovies);
                handleBtn(btnSubTabGenres);

                if (movieSubTab == 0) {
                    handleBtn(btnQuickAddMovie);
                    if (movieSearchInput) {
                        if (event.type == sf::Event::MouseButtonPressed) {
                            movieSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        } else if (event.type == sf::Event::TextEntered) {
                            movieSearchInput->handleTextEntered(event.text.unicode);

                            // Lọc danh sách phim theo từ khóa mới
                            std::string q = movieSearchInput->getText();
                            std::transform(q.begin(), q.end(), q.begin(), ::tolower);

                            std::vector<int> matched;
                            for (size_t i = 0; i < movieList.size(); ++i) {
                                std::string pName = movieList[i].getTenPhim();
                                std::string pCode = movieList[i].getMaPhim();
                                std::string pGenre = db.layTheLoaiCuaPhim(pCode);
                                int pAge = movieList[i].getNhanDoTuoi();

                                if (!movieAgeFilter.empty()) {
                                    if (movieAgeFilter == "P") {
                                        if (pAge > 0) continue;
                                    } else if (movieAgeFilter == "K") {
                                        if (pAge > 12) continue;
                                    } else {
                                        int targetAge = 0;
                                        for (char c : movieAgeFilter) if (c >= '0' && c <= '9') targetAge = targetAge * 10 + (c - '0');
                                        if (targetAge > 0 && pAge != targetAge) continue;
                                    }
                                }

                                if (!q.empty()) {
                                    std::string pNameLower = pName;
                                    std::string pCodeLower = pCode;
                                    std::string pGenreLower = pGenre;
                                    std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                                    std::transform(pCodeLower.begin(), pCodeLower.end(), pCodeLower.begin(), ::tolower);
                                    std::transform(pGenreLower.begin(), pGenreLower.end(), pGenreLower.begin(), ::tolower);
                                    if (pNameLower.find(q) == std::string::npos &&
                                        pCodeLower.find(q) == std::string::npos &&
                                        pGenreLower.find(q) == std::string::npos) {
                                        continue;
                                    }
                                }
                                matched.push_back(static_cast<int>(i));
                            }

                            movieCurrentPage = 0;
                            showMovieSpotlight = false;
                            spotlightMovieIdx = -1;
                            if (matched.empty()) {
                                showMovieSpotlight = false;
                                spotlightMovieIdx = -1;
                            } else {
                                // Typing in movieSearchInput only filters the table, does NOT overwrite movie form inputs
                            }
                        }
                    }
                    if (inputMovieId) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieId->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieId->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieTitle) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieTitle->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieTitle->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieDuration) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieDuration->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieDuration->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieReleaseDate) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieReleaseDate->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieReleaseDate->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieEndDate) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieEndDate->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieEndDate->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieAgeRating) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieAgeRating->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieAgeRating->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieGenre) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieGenre->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieGenre->handleTextEntered(event.text.unicode);
                    }
                    if (inputMoviePoster) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMoviePoster->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMoviePoster->handleTextEntered(event.text.unicode);
                    }
                    if (inputMovieDesc) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieDesc->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieDesc->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnSelectGenre);
                    handleBtn(btnMovieCreate);
                    handleBtn(btnMovieUpdate);
                    handleBtn(btnMovieDelete);
                    handleBtn(btnMovieClear);

                    if (event.type == sf::Event::MouseMoved) {
                        if (movieAgeFilterDropdown) movieAgeFilterDropdown->handleMouseMove(mousePos);
                        if (movieFormAgeDropdown) movieFormAgeDropdown->handleMouseMove(mousePos);
                    } else if (event.type == sf::Event::MouseWheelScrolled) {
                        if (movieAgeFilterDropdown) movieAgeFilterDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                        if (movieFormAgeDropdown) movieFormAgeDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }

                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 24.f;
                        float leftColW = 928.f;
                        float rightColX = sidebarWidth + 968.f;
                        float rightColW = 486.f;
                        float inputX = rightColX + 136.f;

                        // 1. Age Filter Dropdown & Form Age Dropdown
                        auto handleMovieDrop = [&](std::unique_ptr<DropdownBox>& drp) {
                            if (!drp) return;
                            if (drp->containsMain(mousePos)) {
                                if (movieAgeFilterDropdown && movieAgeFilterDropdown.get() != drp.get()) movieAgeFilterDropdown->setExpanded(false);
                                if (movieFormAgeDropdown && movieFormAgeDropdown.get() != drp.get()) movieFormAgeDropdown->setExpanded(false);
                                drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                            }
                        };
                        handleMovieDrop(movieAgeFilterDropdown);
                        handleMovieDrop(movieFormAgeDropdown);

                        // 3. POS Toggle Switch inside Right Form (y = 736.f .. 784.f)
                        if (mousePos.x >= rightColX + 20.f && mousePos.x <= rightColX + rightColW - 20.f &&
                            mousePos.y >= 736.f && mousePos.y <= 784.f) {
                            moviePosActive = !moviePosActive;
                        }

                        // 4. Movie Table: Filtered indices helper
                        std::string query = movieSearchInput ? movieSearchInput->getText() : "";
                        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

                        std::vector<int> filteredMovieIndices;
                        for (size_t i = 0; i < movieList.size(); ++i) {
                            std::string pName = movieList[i].getTenPhim();
                            std::string pCode = movieList[i].getMaPhim();
                            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
                            int pAge = movieList[i].getNhanDoTuoi();

                            if (!movieAgeFilter.empty()) {
                                if (movieAgeFilter == "P") {
                                    if (pAge > 0) continue;
                                } else if (movieAgeFilter == "K") {
                                    if (pAge > 12) continue;
                                } else {
                                    int targetAge = 0;
                                    for (char c : movieAgeFilter) if (c >= '0' && c <= '9') targetAge = targetAge * 10 + (c - '0');
                                    if (targetAge > 0 && pAge != targetAge) continue;
                                }
                            }
                        }

                        // Re-filter with search query
                        filteredMovieIndices.clear();
                        for (size_t i = 0; i < movieList.size(); ++i) {
                            std::string pName = movieList[i].getTenPhim();
                            std::string pCode = movieList[i].getMaPhim();
                            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
                            int pAge = movieList[i].getNhanDoTuoi();

                            if (!movieAgeFilter.empty()) {
                                if (movieAgeFilter == "P") {
                                    if (pAge > 0) continue;
                                } else if (movieAgeFilter == "K") {
                                    if (pAge > 12) continue;
                                } else {
                                    int targetAge = 0;
                                    for (char c : movieAgeFilter) if (c >= '0' && c <= '9') targetAge = targetAge * 10 + (c - '0');
                                    if (targetAge > 0 && pAge != targetAge) continue;
                                }
                            }

                            if (!query.empty()) {
                                std::string pNameLower = pName;
                                std::string pCodeLower = pCode;
                                std::string pGenreLower = pGenre;
                                std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                                std::transform(pCodeLower.begin(), pCodeLower.end(), pCodeLower.begin(), ::tolower);
                                std::transform(pGenreLower.begin(), pGenreLower.end(), pGenreLower.begin(), ::tolower);
                                if (pNameLower.find(query) == std::string::npos &&
                                    pCodeLower.find(query) == std::string::npos &&
                                    pGenreLower.find(query) == std::string::npos) {
                                    continue;
                                }
                            }
                            filteredMovieIndices.push_back(static_cast<int>(i));
                        }

                        int totalFiltered = static_cast<int>(filteredMovieIndices.size());
                        int pageSize = showMovieSpotlight ? 2 : 5;
                        int totalMoviePages = std::max(1, (totalFiltered + pageSize - 1) / pageSize);
                        if (movieCurrentPage >= totalMoviePages) movieCurrentPage = totalMoviePages - 1;
                        if (movieCurrentPage < 0) movieCurrentPage = 0;

                        auto syncMovieFormByIdx = [this](int targetIdx) {
                            if (targetIdx >= 0 && targetIdx < static_cast<int>(movieList.size())) {
                                selectedMovieRowIdx = targetIdx;
                                if (inputMovieId) inputMovieId->setText(movieList[targetIdx].getMaPhim());
                                if (inputMovieTitle) inputMovieTitle->setText(movieList[targetIdx].getTenPhim());
                                if (inputMovieDuration) inputMovieDuration->setText(std::to_string(movieList[targetIdx].getThoiLuong()));
                                if (inputMovieReleaseDate) inputMovieReleaseDate->setText(movieList[targetIdx].getNgayKhoiChieu());
                                if (inputMovieEndDate) inputMovieEndDate->setText(movieList[targetIdx].getNgayKetThuc());
                                int ageVal = movieList[targetIdx].getNhanDoTuoi();
                                std::string codeVal = (ageVal <= 0) ? "P" : ("T" + std::to_string(ageVal));
                                if (inputMovieAgeRating) inputMovieAgeRating->setText(codeVal);
                                syncMovieFormAgeDropdown(codeVal);
                                if (inputMovieGenre) inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[targetIdx].getMaPhim()));
                                std::string pPath = movieList[targetIdx].getPoster();
                                if (pPath.empty() || pPath == "assets/posters/poster_default.jpg") {
                                    pPath = "assets/posters/" + movieList[targetIdx].getMaPhim() + ".jpg";
                                }
                                if (inputMoviePoster) inputMoviePoster->setText(pPath);
                                if (inputMovieDesc) inputMovieDesc->setText(movieList[targetIdx].getMoTa());
                            }
                        };

                        // 4.1 Footer Pagination clicks
                        float footerY = 232.f + 716.f - 42.f; // 906.f
                        if (mousePos.y >= footerY && mousePos.y <= footerY + 32.f &&
                            mousePos.x >= leftColX + leftColW - 260.f && mousePos.x <= leftColX + leftColW - 16.f) {
                            float pageBtnEndX = leftColX + leftColW - 20.f;
                            float pNextW = 50.f;
                            float pNextX = pageBtnEndX - pNextW;
                            if (mousePos.x >= pNextX && mousePos.x <= pNextX + pNextW) {
                                if (movieCurrentPage < totalMoviePages - 1) {
                                    movieCurrentPage++;
                                    int newStart = movieCurrentPage * pageSize;
                                    if (newStart < totalFiltered) {
                                        if (showMovieSpotlight) {
                                            spotlightMovieIdx = filteredMovieIndices[newStart];
                                            selectedMovieRowIdx = spotlightMovieIdx;
                                        }
                                        syncMovieFormByIdx(filteredMovieIndices[newStart]);
                                    }
                                }
                            } else {
                                float curNumX = pNextX - 8.f;
                                bool clickedNum = false;
                                for (int p = std::min(totalMoviePages - 1, 4); p >= 0; --p) {
                                    float pNumW = 28.f;
                                    curNumX -= pNumW;
                                    if (mousePos.x >= curNumX && mousePos.x <= curNumX + pNumW) {
                                        movieCurrentPage = p;
                                        int newStart = movieCurrentPage * pageSize;
                                        if (newStart < totalFiltered) {
                                            if (showMovieSpotlight) {
                                                spotlightMovieIdx = filteredMovieIndices[newStart];
                                                selectedMovieRowIdx = spotlightMovieIdx;
                                            }
                                            syncMovieFormByIdx(filteredMovieIndices[newStart]);
                                        }
                                        clickedNum = true;
                                        break;
                                    }
                                    curNumX -= 4.f;
                                }
                                if (!clickedNum) {
                                    float pPrevW = 54.f;
                                    float pPrevX = curNumX - pPrevW;
                                    if (mousePos.x >= pPrevX && mousePos.x <= pPrevX + pPrevW) {
                                        if (movieCurrentPage > 0) {
                                            movieCurrentPage--;
                                            int newStart = movieCurrentPage * pageSize;
                                            if (newStart < totalFiltered) {
                                                if (showMovieSpotlight) {
                                                    spotlightMovieIdx = filteredMovieIndices[newStart];
                                                    selectedMovieRowIdx = spotlightMovieIdx;
                                                }
                                                syncMovieFormByIdx(filteredMovieIndices[newStart]);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 4.2 Close Spotlight Button Click (when Spotlight is open)
                        if (showMovieSpotlight) {
                            float spotlightX = leftColX + 16.f;
                            float spotlightW = leftColW - 32.f;
                            float spotlightY = 554.f;
                            float closeBtnW = 145.f;
                            float closeBtnH = 28.f;
                            float closeBtnX = spotlightX + spotlightW - closeBtnW - 10.f;
                            float closeBtnY = spotlightY + 4.f;

                            if (mousePos.x >= closeBtnX && mousePos.x <= closeBtnX + closeBtnW &&
                                mousePos.y >= closeBtnY && mousePos.y <= closeBtnY + closeBtnH) {
                                showMovieSpotlight = false;
                                spotlightMovieIdx = -1;
                                int curPos = 0;
                                for (int i = 0; i < totalFiltered; ++i) {
                                    if (filteredMovieIndices[i] == selectedMovieRowIdx) {
                                        curPos = i;
                                        break;
                                    }
                                }
                                movieCurrentPage = curPos / 5;
                            }
                        }

                        // 4.3 Row selection & Action Icon clicks in Movie Table (rowH = 104.f, 2 rows in Spotlight mode, 5 rows normal)
                        float rowStartY = 232.f + 64.f + 42.f; // 338.f
                        float rowH = 104.f;
                        if (mousePos.x >= leftColX + 16.f && mousePos.x <= leftColX + leftColW - 16.f &&
                            mousePos.y >= rowStartY && mousePos.y <= rowStartY + static_cast<float>(pageSize) * rowH) {
                            int clickedVisualRow = static_cast<int>((mousePos.y - rowStartY) / rowH);
                            if (clickedVisualRow >= 0 && clickedVisualRow < pageSize) {
                                int actualIdx = movieCurrentPage * pageSize + clickedVisualRow;
                                int targetIdx = -1;
                                if (actualIdx >= 0 && actualIdx < totalFiltered) {
                                    targetIdx = filteredMovieIndices[actualIdx];
                                }

                                if (targetIdx >= 0 && targetIdx < static_cast<int>(movieList.size())) {
                                    bool clickedEditIcon = (mousePos.x >= leftColX + 844.f && mousePos.x <= leftColX + 874.f);
                                    bool clickedDeleteIcon = (mousePos.x >= leftColX + 876.f && mousePos.x <= leftColX + 908.f);

                                    bool hasFormChanges = false;
                                    if (selectedMovieRowIdx == targetIdx) {
                                        std::string curId = movieList[targetIdx].getMaPhim();
                                        int ageVal = movieList[targetIdx].getNhanDoTuoi();
                                        std::string curAgeStr = (ageVal <= 0) ? "P" : ("T" + std::to_string(ageVal));
                                        std::string curGenre = db.layTheLoaiCuaPhim(curId);
                                        std::string curPoster = movieList[targetIdx].getPoster();
                                        if (curPoster.empty() || curPoster == "assets/posters/poster_default.jpg") {
                                            curPoster = "assets/posters/" + curId + ".jpg";
                                        }
                                        if (inputMovieTitle && inputMovieTitle->getText() != movieList[targetIdx].getTenPhim()) hasFormChanges = true;
                                        if (inputMovieDuration && inputMovieDuration->getText() != std::to_string(movieList[targetIdx].getThoiLuong())) hasFormChanges = true;
                                        if (inputMovieReleaseDate && DateFormat::normalizeDate(inputMovieReleaseDate->getText()) != DateFormat::normalizeDate(movieList[targetIdx].getNgayKhoiChieu())) hasFormChanges = true;
                                        if (inputMovieEndDate && DateFormat::normalizeDate(inputMovieEndDate->getText()) != DateFormat::normalizeDate(movieList[targetIdx].getNgayKetThuc())) hasFormChanges = true;
                                        if (inputMovieAgeRating && inputMovieAgeRating->getText() != curAgeStr) hasFormChanges = true;
                                        if (inputMovieGenre && inputMovieGenre->getText() != curGenre) hasFormChanges = true;
                                        if (inputMoviePoster && inputMoviePoster->getText() != curPoster) hasFormChanges = true;
                                        if (inputMovieDesc && inputMovieDesc->getText() != movieList[targetIdx].getMoTa()) hasFormChanges = true;
                                    }

                                    // Only trigger backend update when clicking Pencil Edit icon if the user has already edited the form
                                    if (clickedEditIcon && selectedMovieRowIdx == targetIdx && hasFormChanges && btnMovieUpdate) {
                                        btnMovieUpdate->triggerClick();
                                    } else if (clickedEditIcon) {
                                        selectedMovieRowIdx = targetIdx;
                                        syncMovieFormByIdx(targetIdx);
                                        if (inputMovieTitle) inputMovieTitle->setFocused(true);
                                    } else if (clickedDeleteIcon && btnMovieDelete) {
                                        selectedMovieRowIdx = targetIdx;
                                        syncMovieFormByIdx(targetIdx);
                                        btnMovieDelete->triggerClick();
                                    } else {
                                        // Click on the movie row itself: Toggle Spotlight!
                                        if (showMovieSpotlight && targetIdx == spotlightMovieIdx) {
                                            // Clicked again on the same movie -> TURN OFF spotlight!
                                            showMovieSpotlight = false;
                                            spotlightMovieIdx = -1;
                                            movieCurrentPage = actualIdx / 5;
                                        } else if (showMovieSpotlight && targetIdx != spotlightMovieIdx) {
                                            // Clicked on a different movie while spotlight is open -> Switch spotlight to it
                                            spotlightMovieIdx = targetIdx;
                                            selectedMovieRowIdx = targetIdx;
                                            syncMovieFormByIdx(targetIdx);
                                        } else {
                                            // Normal mode: Clicked on a movie -> TURN ON spotlight!
                                            showMovieSpotlight = true;
                                            spotlightMovieIdx = targetIdx;
                                            selectedMovieRowIdx = targetIdx;
                                            syncMovieFormByIdx(targetIdx);
                                            movieCurrentPage = actualIdx / 2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (genreSearchInput) {
                        if (event.type == sf::Event::MouseButtonPressed) genreSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) genreSearchInput->handleTextEntered(event.text.unicode);
                    }
                    if (inputGenreName) {
                        if (event.type == sf::Event::MouseButtonPressed) inputGenreName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputGenreName->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnExportGenres);
                    handleBtn(btnQuickAddGenre);
                    handleBtn(btnGenreCreate);
                    handleBtn(btnGenreDelete);
                    handleBtn(btnGenreUpdate);
                    handleBtn(btnGenreClear);

                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 24.f;
                        float leftColW = 928.f;
                        float rightColX = sidebarWidth + 968.f;

                        // 1. Sort Toggle button [Mã thể loại ↓↑]
                        if (mousePos.x >= leftColX + leftColW - 140.f && mousePos.x <= leftColX + leftColW - 20.f &&
                            mousePos.y >= 246.f && mousePos.y <= 278.f) {
                            genreSortAsc = !genreSortAsc;
                        }

                        // 2. Color Swatch Selection (5 circles under MÀU SẮC BIỂU THỊ)
                        for (int c = 0; c < 5; ++c) {
                            float cx = rightColX + 42.f + c * 40.f;
                            float cy = 574.f;
                            float dx = mousePos.x - cx;
                            float dy = mousePos.y - cy;
                            if (dx * dx + dy * dy <= 18.f * 18.f) {
                                selectedGenreColorIdx = c;
                                break;
                            }
                        }

                        // 3. Genre Table Row Click (filtered & sorted)
                        float rowStartY = 232.f + 58.f + 40.f;
                        float rowH = 56.f;
                        if (mousePos.x >= leftColX && mousePos.x <= leftColX + leftColW &&
                            mousePos.y >= rowStartY && mousePos.y <= rowStartY + 10 * rowH) {
                            int clickedVisualRow = static_cast<int>((mousePos.y - rowStartY) / rowH);
                            std::string q = genreSearchInput ? genreSearchInput->getText() : "";
                            std::transform(q.begin(), q.end(), q.begin(), ::tolower);

                            std::vector<int> visibleIndices;
                            for (size_t i = 0; i < genreList.size(); ++i) {
                                if (!q.empty()) {
                                    std::string cLow = genreList[i].first;
                                    std::string nLow = genreList[i].second;
                                    std::transform(cLow.begin(), cLow.end(), cLow.begin(), ::tolower);
                                    std::transform(nLow.begin(), nLow.end(), nLow.begin(), ::tolower);
                                    if (cLow.find(q) == std::string::npos && nLow.find(q) == std::string::npos) {
                                        continue;
                                    }
                                }
                                visibleIndices.push_back(static_cast<int>(i));
                            }
                            if (!genreSortAsc) {
                                std::reverse(visibleIndices.begin(), visibleIndices.end());
                            }

                            if (clickedVisualRow >= 0 && clickedVisualRow < static_cast<int>(visibleIndices.size())) {
                                int actualIdx = visibleIndices[clickedVisualRow];
                                selectedGenreRowIdx = actualIdx;
                                std::string gId = genreList[actualIdx].first;
                                inputGenreId->setText(gId);
                                inputGenreName->setText(genreList[actualIdx].second);
                                int colIdx = genreColorMap.count(gId) ? genreColorMap[gId] : actualIdx;
                                selectedGenreColorIdx = colIdx % 5;
                            }
                        }
                    }
                }
                break;

            case ActiveTab::SeatBooking:
                handleBtn(btnPosLock);
                handleBtn(btnPosLogout);
                handleBtn(btnPosRefresh);
                handleBtn(btnPosSeatClear);
                handleBtn(btnPosAddFnb);
                handleBtn(btnPosApplyMember);
                handleBtn(btnPosApplyVoucher);
                handleBtn(btnPosCheckout);
                handleBtn(btnPosPrintProforma);
                handleBtn(btnPosCancel);

                if (inputPosMemberPhone) {
                    if (event.type == sf::Event::MouseButtonPressed) inputPosMemberPhone->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputPosMemberPhone->handleTextEntered(event.text.unicode);
                }
                if (inputPosVoucher) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        inputPosVoucher->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        posPromoDropdownOpen = true;
                    } else if (event.type == sf::Event::TextEntered) {
                        inputPosVoucher->handleTextEntered(event.text.unicode);
                        posPromoDropdownOpen = true;
                        posPromoScrollOffset = 0;
                    }
                }
                if (inputPosCashGiven) {
                    if (event.type == sf::Event::MouseButtonPressed) inputPosCashGiven->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        inputPosCashGiven->handleTextEntered(event.text.unicode);
                        posCustomCashEdited = true;
                        recalculatePosTotals();
                    }
                }

                // Mouse wheel scroll over NGÀY, PHIM rows or Promo Dropdown
                if (event.type == sf::Event::MouseWheelScrolled) {
                    float leftColX = sidebarWidth + 14.f;
                    float leftColW = 1000.f;
                    float rightStartX = leftColX + leftColW + 14.f;

                    if (posPromoDropdownOpen) {
                        float popX = rightStartX + 14.f;
                        float popY = 566.f;
                        float popW = 408.f;
                        float popH = 224.f;
                        if (mousePos.x >= popX && mousePos.x <= popX + popW && mousePos.y >= popY && mousePos.y <= popY + popH) {
                            if (event.mouseWheelScroll.delta < 0) posPromoScrollOffset++;
                            else if (event.mouseWheelScroll.delta > 0 && posPromoScrollOffset > 0) posPromoScrollOffset--;
                        }
                    }

                    if (mousePos.x >= leftColX && mousePos.x <= leftColX + leftColW) {
                        if (mousePos.y >= 80.f && mousePos.y <= 125.f) {
                            int maxDates = std::max(0, static_cast<int>(posAvailableDates.size()) - 5);
                            if (event.mouseWheelScroll.delta < 0 && posDateScrollOffset < maxDates) posDateScrollOffset++;
                            else if (event.mouseWheelScroll.delta > 0 && posDateScrollOffset > 0) posDateScrollOffset--;
                        } else if (mousePos.y >= 126.f && mousePos.y <= 180.f) {
                            int maxMov = std::max(0, static_cast<int>(posAvailableMovieIds.size()) - 4);
                            if (event.mouseWheelScroll.delta < 0 && posMovieScrollOffset < maxMov) posMovieScrollOffset++;
                            else if (event.mouseWheelScroll.delta > 0 && posMovieScrollOffset > 0) posMovieScrollOffset--;
                        }
                    }
                }

                // Keyboard shortcuts (F5 Checkout, F6 Proforma, ESC Cancel)
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::F5) {
                        if (btnPosCheckout) btnPosCheckout->triggerClick();
                    } else if (event.key.code == sf::Keyboard::F6) {
                        if (btnPosPrintProforma) btnPosPrintProforma->triggerClick();
                    } else if (event.key.code == sf::Keyboard::Escape) {
                        if (btnPosCancel) btnPosCancel->triggerClick();
                    }
                }

                // Interactive Clicks on Filter Rows, Seat Matrix, Cart Remove Buttons, and Payment Methods
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 14.f;
                    float leftColW = 1000.f;
                    float rightStartX = leftColX + leftColW + 14.f;
                    float rightW = 436.f;

                    // Check click on Floating Promo Dropdown if open
                    if (posPromoDropdownOpen) {
                        float popX = rightStartX + 14.f;
                        float popY = 566.f;
                        float popW = 408.f;
                        float popH = 224.f;

                        if (mousePos.x >= popX && mousePos.x <= popX + popW && mousePos.y >= popY && mousePos.y <= popY + popH) {
                            float itemH = 46.f;
                            int maxItems = 4;
                            float startItemsY = popY + 28.f;

                            std::string query = inputPosVoucher ? inputPosVoucher->getText() : "";
                            std::string qUpper = query;
                            std::transform(qUpper.begin(), qUpper.end(), qUpper.begin(), ::toupper);
                            qUpper.erase(0, qUpper.find_first_not_of(" \t\r\n"));
                            if (!qUpper.empty()) qUpper.erase(qUpper.find_last_not_of(" \t\r\n") + 1);

                            std::vector<std::pair<std::string, std::string>> matchingList;
                            sqlite3* dbConn = nullptr;
                            if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
                                std::string q = "SELECT maCode, tenKhuyenMai FROM promotions;";
                                sqlite3_stmt* stmt = nullptr;
                                if (sqlite3_prepare_v2(dbConn, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        const char* cCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                                        const char* cTen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                                        std::string codeStr = cCode ? cCode : "";
                                        std::string tenStr = cTen ? cTen : "";
                                        std::string codeUpper = codeStr;
                                        std::string tenUpper = tenStr;
                                        std::transform(codeUpper.begin(), codeUpper.end(), codeUpper.begin(), ::toupper);
                                        std::transform(tenUpper.begin(), tenUpper.end(), tenUpper.begin(), ::toupper);
                                        if (qUpper.empty() || codeUpper.find(qUpper) != std::string::npos || tenUpper.find(qUpper) != std::string::npos) {
                                            matchingList.push_back({codeStr, tenStr});
                                        }
                                    }
                                    sqlite3_finalize(stmt);
                                }
                                sqlite3_close(dbConn);
                            }

                            int totalMatching = static_cast<int>(matchingList.size());
                            for (int i = 0; i < maxItems && (posPromoScrollOffset + i) < totalMatching; ++i) {
                                float iy = startItemsY + static_cast<float>(i) * itemH;
                                if (mousePos.y >= iy && mousePos.y <= iy + itemH) {
                                    std::string selCode = matchingList[posPromoScrollOffset + i].first;
                                    if (inputPosVoucher) inputPosVoucher->setText(selCode);
                                    posVoucherCode = selCode;
                                    posVoucherApplied = true;
                                    if (btnPosApplyVoucher) {
                                        btnPosApplyVoucher->setText("Đã áp dụng");
                                        btnPosApplyVoucher->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(148, 163, 184));
                                    }
                                    posPromoDropdownOpen = false;
                                    recalculatePosTotals();
                                    break;
                                }
                            }
                            break;
                        } else if (mousePos.x >= rightStartX + 14.f && mousePos.x <= rightStartX + 14.f + 304.f &&
                                   mousePos.y >= 530.f && mousePos.y <= 530.f + 32.f) {
                            // Click inside input box, keep open
                        } else {
                            posPromoDropdownOpen = false;
                        }
                    }

                    // 1. Row 1: NGÀY pills & scroll arrows (y = 86.f .. 120.f)
                    float dateRowY = 86.f;
                    int maxVisibleDates = 5;
                    int totalDates = static_cast<int>(posAvailableDates.size());
                    float dateStartX = leftColX + 82.f;
                    float dateBtnW = 136.f;
                    float dateBtnH = 34.f;
                    float dateGap = 10.f;

                    if (mousePos.y >= dateRowY && mousePos.y <= dateRowY + dateBtnH) {
                        if (totalDates > maxVisibleDates) {
                            float arrX = leftColX + leftColW - 68.f;
                            if (mousePos.x >= arrX && mousePos.x <= arrX + 24.f) {
                                if (posDateScrollOffset > 0) posDateScrollOffset--;
                            } else if (mousePos.x >= arrX + 28.f && mousePos.x <= arrX + 52.f) {
                                if (posDateScrollOffset < totalDates - maxVisibleDates) posDateScrollOffset++;
                            }
                        }
                        for (int i = 0; i < maxVisibleDates && (posDateScrollOffset + i) < totalDates; ++i) {
                            float bx = dateStartX + static_cast<float>(i) * (dateBtnW + dateGap);
                            if (mousePos.x >= bx && mousePos.x <= bx + dateBtnW) {
                                posSelectedDateStr = posAvailableDates[posDateScrollOffset + i];
                                // Ensure selected movie has showtimes on this date if possible
                                bool movieHasStOnDate = false;
                                for (const auto& st : showtimeList) {
                                    if (st.getNgayChieu() == posSelectedDateStr && st.getMaPhim() == posSelectedMovieId) {
                                        movieHasStOnDate = true;
                                        break;
                                    }
                                }
                                if (!movieHasStOnDate) {
                                    for (const auto& st : showtimeList) {
                                        if (st.getNgayChieu() == posSelectedDateStr) {
                                            posSelectedMovieId = st.getMaPhim();
                                            break;
                                        }
                                    }
                                }
                                refreshPosShowtimesDropdown();
                                refreshSeatsForShowtime(currentShowtimeId);
                                break;
                            }
                        }
                    }

                    // 2. Row 2: PHIM cards & scroll arrows (y = 130.f .. 176.f)
                    float movieRowY = 130.f;
                    int maxVisibleMovies = 4;
                    int totalMovies = static_cast<int>(posAvailableMovieIds.size());
                    float movieStartX = leftColX + 82.f;
                    float movieCardW = 204.f;
                    float movieCardH = 46.f;
                    float movieGap = 10.f;

                    if (mousePos.y >= movieRowY && mousePos.y <= movieRowY + movieCardH) {
                        if (totalMovies > maxVisibleMovies) {
                            float arrX = leftColX + leftColW - 68.f;
                            if (mousePos.x >= arrX && mousePos.x <= arrX + 24.f) {
                                if (posMovieScrollOffset > 0) posMovieScrollOffset--;
                            } else if (mousePos.x >= arrX + 28.f && mousePos.x <= arrX + 52.f) {
                                if (posMovieScrollOffset < totalMovies - maxVisibleMovies) posMovieScrollOffset++;
                            }
                        }
                        for (int i = 0; i < maxVisibleMovies && (posMovieScrollOffset + i) < totalMovies; ++i) {
                            float mx = movieStartX + static_cast<float>(i) * (movieCardW + movieGap);
                            if (mousePos.x >= mx && mousePos.x <= mx + movieCardW) {
                                posSelectedMovieId = posAvailableMovieIds[posMovieScrollOffset + i];
                                refreshPosShowtimesDropdown();
                                refreshSeatsForShowtime(currentShowtimeId);
                                break;
                            }
                        }
                    }

                    // 3. Row 3: SUẤT cards (y = 186.f .. 240.f)
                    float stRowY = 186.f;
                    float stStartX = leftColX + 82.f;
                    float stCardW = 172.f;
                    float stCardH = 54.f;
                    float stGap = 10.f;
                    int maxVisibleSt = 5;
                    int totalSt = static_cast<int>(posFilteredShowtimeIds.size());

                    if (mousePos.y >= stRowY && mousePos.y <= stRowY + stCardH) {
                        for (int i = 0; i < maxVisibleSt && i < totalSt; ++i) {
                            float sx = stStartX + static_cast<float>(i) * (stCardW + stGap);
                            if (mousePos.x >= sx && mousePos.x <= sx + stCardW) {
                                if (!posFilteredShowtimeIds[i].empty()) {
                                    currentShowtimeId = posFilteredShowtimeIds[i];
                                    refreshSeatsForShowtime(currentShowtimeId);
                                }
                                break;
                            }
                        }
                    }

                    // 4. Seat Matrix Clicks
                    for (auto& s : currentSeats) {
                        if (s.trangThai != "DaDat") {
                            if (mousePos.x >= s.position.x && mousePos.x <= s.position.x + s.size.x &&
                                mousePos.y >= s.position.y && mousePos.y <= s.position.y + s.size.y) {
                                s.isSelected = !s.isSelected;
                                if (s.isSelected) {
                                    selectedSeatCodes.push_back(s.maGhe);
                                } else {
                                    for (auto it = selectedSeatCodes.begin(); it != selectedSeatCodes.end(); ++it) {
                                        if (*it == s.maGhe) {
                                            selectedSeatCodes.erase(it);
                                            break;
                                        }
                                    }
                                }
                                recalculatePosTotals();
                                break;
                            }
                        }
                    }

                    // 5. Remove single seat ('×') in Card 1 (GHẾ ĐANG CHỌN)
                    float rc1Y = 74.f;
                    for (size_t i = 0; i < selectedSeatCodes.size() && i < 2; ++i) {
                        float itemY = rc1Y + 38.f + static_cast<float>(i) * 44.f;
                        if (mousePos.x >= rightStartX + rightW - 38.f && mousePos.x <= rightStartX + rightW - 14.f &&
                            mousePos.y >= itemY && mousePos.y <= itemY + 38.f) {
                            std::string codeToRemove = selectedSeatCodes[i];
                            selectedSeatCodes.erase(selectedSeatCodes.begin() + static_cast<long long>(i));
                            for (auto& s : currentSeats) {
                                if (s.maGhe == codeToRemove) {
                                    s.isSelected = false;
                                    break;
                                }
                            }
                            recalculatePosTotals();
                            break;
                        }
                    }

                    // 6. Remove F&B item ('×') in Card 2 (F&B ĐÃ CHỌN)
                    float rc2Y = 248.f;
                    std::vector<std::string> uniqueFnbNames;
                    for (const auto& f : posSelectedFnb) {
                        if (std::find(uniqueFnbNames.begin(), uniqueFnbNames.end(), f.first) == uniqueFnbNames.end()) {
                            uniqueFnbNames.push_back(f.first);
                        }
                    }
                    for (size_t i = 0; i < uniqueFnbNames.size() && i < 2; ++i) {
                        float fy = rc2Y + 38.f + static_cast<float>(i) * 42.f;
                        float boxH = (uniqueFnbNames.size() == 1) ? 54.f : 38.f;
                        if (mousePos.x >= rightStartX + rightW - 38.f && mousePos.x <= rightStartX + rightW - 14.f &&
                            mousePos.y >= fy && mousePos.y <= fy + boxH) {
                            std::string targetName = uniqueFnbNames[i];
                            // Remove one instance of targetName
                            for (auto it = posSelectedFnb.begin(); it != posSelectedFnb.end(); ++it) {
                                if (it->first == targetName) {
                                    posSelectedFnb.erase(it);
                                    break;
                                }
                            }
                            recalculatePosTotals();
                            break;
                        }
                    }

                    // 7. Payment Method Cards Click (Tiền mặt, Quét QR, Thẻ POS)
                    float pmY = 582.f + 32.f;
                    float pmW = 130.f;
                    float pmH = 50.f;
                    float pmGap = 9.f;
                    if (mousePos.y >= pmY && mousePos.y <= pmY + pmH) {
                        for (int p = 0; p < 3; ++p) {
                            float px = rightStartX + 14.f + static_cast<float>(p) * (pmW + pmGap);
                            if (mousePos.x >= px && mousePos.x <= px + pmW) {
                                posPaymentMethod = p;
                                break;
                            }
                        }
                    }
                }
                break;

            case ActiveTab::CRM:
                if (showCrmAddModal) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        if (inputCrmName) inputCrmName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (inputCrmPhone) inputCrmPhone->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (inputCrmEmail) inputCrmEmail->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::TextEntered) {
                        if (inputCrmName) inputCrmName->handleTextEntered(event.text.unicode);
                        if (inputCrmPhone) inputCrmPhone->handleTextEntered(event.text.unicode);
                        if (inputCrmEmail) inputCrmEmail->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnCrmSave);
                    handleBtn(btnCrmCancel);
                    break;
                }

                if (crmTierDropdown) {
                    if (event.type == sf::Event::MouseButtonPressed) crmTierDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) crmTierDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseMoved) crmTierDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseWheelScrolled) crmTierDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                }

                if (crmSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) crmSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        crmSearchInput->handleTextEntered(event.text.unicode);
                        crmCurrentPage = 0;
                    }
                }
                if (crmDateRangeInput) {
                    if (event.type == sf::Event::MouseButtonPressed) crmDateRangeInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) crmDateRangeInput->handleTextEntered(event.text.unicode);
                }

                // Handle clicks on Sub-Tab Switcher Bar & Customer Row Actions
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float startX = sidebarWidth + 24.f;
                    float tabBarY = 174.f;
                    float tabW = 176.f;

                    // Sub-tab 0: Quản lý Khách hàng
                    if (mousePos.x >= startX + 10.f && mousePos.x <= startX + 10.f + tabW &&
                        mousePos.y >= tabBarY + 4.f && mousePos.y <= tabBarY + 42.f) {
                        if (crmSubTab != 0) {
                            crmSubTab = 0;
                            crmCurrentPage = 0;
                            if (crmTierDropdown) {
                                crmTierDropdown->setOptions({"Tất cả hạng hội viên", "Hạng Thường (Standard)", "Hạng Thân Thiết / VIP"}, 0);
                            }
                        }
                    }
                    // Sub-tab 1: Lịch sử tích điểm
                    else if (mousePos.x >= startX + 194.f && mousePos.x <= startX + 194.f + tabW &&
                             mousePos.y >= tabBarY + 4.f && mousePos.y <= tabBarY + 42.f) {
                        if (crmSubTab != 1) {
                            crmSubTab = 1;
                            crmCurrentPage = 0;
                            if (crmTierDropdown) {
                                crmTierDropdown->setOptions({"Tất cả loại GD", "+ Tích điểm", "- Đổi điểm"}, 0);
                            }
                        }
                    }

                    // Customer Row Actions in Sub-tab 0: [+ Tích 100đ] and [- Đổi 100đ]
                    if (crmSubTab == 0) {
                        std::string q = crmSearchInput ? crmSearchInput->getText() : "";
                        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                        int filterIdx = crmTierDropdown ? crmTierDropdown->getSelectedIndex() : 0;

                        std::vector<int> filteredIndices;
                        for (size_t idx = 0; idx < customerList.size(); ++idx) {
                            const auto& c = customerList[idx];
                            std::string h = c.getHangThanhVien();
                            if (filterIdx == 1 && (h != "Thuong" && h != "Standard" && !h.empty())) continue;
                            if (filterIdx == 2 && (h != "ThanThiet" && h != "VIP" && h != "Bac")) continue;
                            if (!q.empty()) {
                                std::string hay = c.getMaKH() + " " + c.getHoTen() + " " + c.getSoDienThoai() + " " + c.getEmail();
                                std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
                                if (hay.find(q) == std::string::npos) continue;
                            }
                            filteredIndices.push_back(static_cast<int>(idx));
                        }

                        int rowsPerPage = 8;
                        int startIdx = crmCurrentPage * rowsPerPage;
                        int endIdx = std::min(startIdx + rowsPerPage, static_cast<int>(filteredIndices.size()));
                        float rowStartY = 428.f + 44.f + 42.f;
                        float rowH = 50.f;

                        for (int i = startIdx; i < endIdx; ++i) {
                            float ry = rowStartY + static_cast<float>(i - startIdx) * rowH;
                            int custIdx = filteredIndices[i];
                            Customer& targetCust = customerList[custIdx];

                            // [+ Tích 100đ] button
                            if (mousePos.x >= startX + 1230.f && mousePos.x <= startX + 1308.f &&
                                mousePos.y >= ry + 12.f && mousePos.y <= ry + 38.f) {
                                targetCust.tichDiem(100, "Tích điểm giao dịch tại quầy POS", "06/05/2026 19:15");
                                targetCust.themChiTieu(200000);

                                sqlite3* dbConn = nullptr;
                                if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
                                    std::string upSql = "UPDATE customers SET diemTichLuy='" + std::to_string(targetCust.getDiemTichLuy()) +
                                                        "', hangThanhVien='" + targetCust.getHangThanhVien() +
                                                        "', tongChiTieu='" + std::to_string(targetCust.getTongChiTieu()) +
                                                        "' WHERE maKhachHang='" + targetCust.getMaKH() + "';";
                                    sqlite3_exec(dbConn, upSql.c_str(), nullptr, nullptr, nullptr);

                                    std::string gdId = "GD" + std::to_string(crmPointHistoryList.size() + 1);
                                    std::string insSql = "INSERT INTO customer_points_history VALUES ('" + gdId + "', '" + targetCust.getMaKH() +
                                                         "', 'TichDiem', '100', 'Tích điểm giao dịch tại quầy POS', '06/05/2026 19:15');";
                                    sqlite3_exec(dbConn, insSql.c_str(), nullptr, nullptr, nullptr);
                                    sqlite3_close(dbConn);
                                }
                                reloadCRMData();
                                break;
                            }
                            // [- Đổi 100đ] button
                            else if (mousePos.x >= startX + 1316.f && mousePos.x <= startX + 1394.f &&
                                     mousePos.y >= ry + 12.f && mousePos.y <= ry + 38.f) {
                                if (targetCust.doiDiem(100, "Đổi 100 điểm giảm giá vé & combo", "06/05/2026 19:15")) {
                                    sqlite3* dbConn = nullptr;
                                    if (sqlite3_open("DATA/cinema.db", &dbConn) == SQLITE_OK) {
                                        std::string upSql = "UPDATE customers SET diemTichLuy='" + std::to_string(targetCust.getDiemTichLuy()) +
                                                            "', hangThanhVien='" + targetCust.getHangThanhVien() +
                                                            "' WHERE maKhachHang='" + targetCust.getMaKH() + "';";
                                        sqlite3_exec(dbConn, upSql.c_str(), nullptr, nullptr, nullptr);

                                        std::string gdId = "GD" + std::to_string(crmPointHistoryList.size() + 1);
                                        std::string insSql = "INSERT INTO customer_points_history VALUES ('" + gdId + "', '" + targetCust.getMaKH() +
                                                             "', 'DoiDiem', '100', 'Đổi 100 điểm giảm giá vé & combo', '06/05/2026 19:15');";
                                        sqlite3_exec(dbConn, insSql.c_str(), nullptr, nullptr, nullptr);
                                        sqlite3_close(dbConn);
                                    }
                                    reloadCRMData();
                                } else if (confirmPopup) {
                                    confirmPopup->show("KHÔNG ĐỦ ĐIỂM", "Hội viên " + targetCust.getHoTen() + " không đủ 100 điểm tích lũy để quy đổi!", ButtonStyle::Danger, "Đóng", "");
                                }
                                break;
                            }
                        }
                    }
                }

                handleBtn(btnCrmSearch);
                handleBtn(btnCrmExport);
                handleBtn(btnCrmRefresh);
                handleBtn(btnCrmAddModal);
                handleBtn(btnCrmPrevPage);
                handleBtn(btnCrmNextPage);
                break;

            case ActiveTab::Reports: {
                handleBtn(btnReportExport);
                handleBtn(btnReportFilter);
                handleBtn(btnTxFilterAll);
                handleBtn(btnTxFilterTicket);
                handleBtn(btnTxFilterFnb);
                handleBtn(btnReconcileBalance);
                handleBtn(btnPrintShiftReceipt);
                handleBtn(btnReportPrevPage);
                handleBtn(btnReportNextPage);

                if (reportSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) reportSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        reportSearchInput->handleTextEntered(event.text.unicode);
                        reportSearchQuery = reportSearchInput->getText();
                        applyReportFilters();
                    }
                }
                if (inputReportStartDate) {
                    if (event.type == sf::Event::MouseButtonPressed) inputReportStartDate->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputReportStartDate->handleTextEntered(event.text.unicode);
                }
                if (inputReportEndDate) {
                    if (event.type == sf::Event::MouseButtonPressed) inputReportEndDate->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputReportEndDate->handleTextEntered(event.text.unicode);
                }

                auto handleReportDropEvents = [&](std::unique_ptr<DropdownBox>& drop) {
                    if (!drop) return;
                    if (event.type == sf::Event::MouseMoved) drop->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseWheelScrolled) drop->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) drop->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) drop->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                };

                handleReportDropEvents(reportShiftDropdown);
                handleReportDropEvents(reportCashierDropdown);
                handleReportDropEvents(reportPaymentDropdown);

                if (event.type == sf::Event::KeyPressed && event.key.control && event.key.code == sf::Keyboard::K) {
                    if (reportSearchInput) reportSearchInput->setFocused(true);
                }
                break;
            }

            case ActiveTab::Rooms: {
                handleBtn(btnRoomExportPDF);
                handleBtn(btnRoomAddNew);
                handleBtn(btnRoomCapMinus);
                handleBtn(btnRoomCapPlus);
                for (auto& tb : btnRoomTechOptions) handleBtn(tb);
                handleBtn(btnRoomCreate);
                handleBtn(btnRoomUpdate);
                handleBtn(btnRoomDelete);
                handleBtn(btnRoomReset);
                handleBtn(btnRoomPrevPage);
                handleBtn(btnRoomNextPage);

                if (roomFilterDropdown) {
                    if (event.type == sf::Event::MouseMoved) roomFilterDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseWheelScrolled) roomFilterDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) roomFilterDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) roomFilterDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                }

                if (roomStatusDropdown) {
                    if (event.type == sf::Event::MouseMoved) roomStatusDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseWheelScrolled) roomStatusDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) roomStatusDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                }

                if (roomSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) roomSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        roomSearchInput->handleTextEntered(event.text.unicode);
                        roomCurrentPage = 0;
                    }
                }
                if (inputRoomName) {
                    if (event.type == sf::Event::MouseButtonPressed) inputRoomName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputRoomName->handleTextEntered(event.text.unicode);
                }
                if (inputRoomCapacity) {
                    if (event.type == sf::Event::MouseButtonPressed) inputRoomCapacity->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        if (event.text.unicode == 8 || (event.text.unicode >= '0' && event.text.unicode <= '9')) {
                            inputRoomCapacity->handleTextEntered(event.text.unicode);
                            int parsed = 0;
                            if (sscanf(inputRoomCapacity->getText().c_str(), "%d", &parsed) == 1 && parsed > 0) {
                                roomFormCapacity = std::clamp(parsed, 1, 500);
                            }
                        }
                    }
                }

                // Table row clicks & pagination page number clicks
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 880.f;

                    // Filter rooms
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
                    int totalPages = std::max(1, (static_cast<int>(filteredIndices.size()) + pageSize - 1) / pageSize);
                    if (roomCurrentPage >= totalPages) roomCurrentPage = totalPages - 1;
                    if (roomCurrentPage < 0) roomCurrentPage = 0;

                    int startIdx = roomCurrentPage * pageSize;
                    float rowStartY = 258.f;
                    float rowH = 88.f;
                    float rowGap = 8.f;

                    for (int row = 0; row < pageSize; ++row) {
                        int itemIdx = startIdx + row;
                        if (itemIdx >= static_cast<int>(filteredIndices.size())) break;

                        float ry = rowStartY + row * (rowH + rowGap);
                        if (mousePos.x >= leftColX + 12.f && mousePos.x <= leftColX + leftColW - 12.f &&
                            mousePos.y >= ry && mousePos.y <= ry + rowH) {
                            int actualIdx = filteredIndices[itemIdx];
                            syncRoomSelectionToForm(actualIdx);
                            break;
                        }
                    }
                }
                break;
            }

            case ActiveTab::Seats: {
                handleBtn(btnSeatSubTabMatrix);
                handleBtn(btnSeatSubTabTypes);

                if (seatSubTab == 0) {
                    handleBtn(btnSeatCopyLayout);
                    handleBtn(btnSeatExportMap);
                    handleBtn(btnSeatAddRowQuick);
                    handleBtn(btnSeatView3D);
                    handleBtn(btnSeatView2D);
                    handleBtn(btnSeatViewPOV);
                    handleBtn(btnSeatZoomIn);
                    handleBtn(btnSeatZoomOut);
                    handleBtn(btnSeatZoomReset);
                    handleBtn(btnSeatListPrev);
                    handleBtn(btnSeatListNext);

                    handleBtn(btnSeatBatchAdd);
                    handleBtn(btnSeatUpdateType);
                    handleBtn(btnSeatDeleteBatch);
                    handleBtn(btnSeatResetForm);

                    // Dropdowns
                    bool dropdownConsumedClick = false;
                    if (seatRoomDropdown) {
                        if (event.type == sf::Event::MouseMoved) seatRoomDropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) seatRoomDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) {
                            bool wasExp = seatRoomDropdown->getExpanded();
                            if (seatRoomDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos)) {
                                dropdownConsumedClick = true;
                            }
                            if (!wasExp && seatRoomDropdown->getExpanded() && seatBatchTypeDropdown) {
                                seatBatchTypeDropdown->setExpanded(false);
                            }
                        } else if (event.type == sf::Event::MouseButtonReleased) {
                            seatRoomDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        }
                    }
                    if (seatBatchTypeDropdown && !dropdownConsumedClick) {
                        if (event.type == sf::Event::MouseMoved) seatBatchTypeDropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) seatBatchTypeDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) {
                            bool wasExp = seatBatchTypeDropdown->getExpanded();
                            if (seatBatchTypeDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos)) {
                                dropdownConsumedClick = true;
                            }
                            if (!wasExp && seatBatchTypeDropdown->getExpanded() && seatRoomDropdown) {
                                seatRoomDropdown->setExpanded(false);
                            }
                        } else if (event.type == sf::Event::MouseButtonReleased) {
                            seatBatchTypeDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        }
                    }

                    // Inputs
                    if (seatSearchInput && !dropdownConsumedClick) {
                        if (event.type == sf::Event::MouseButtonPressed) seatSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) {
                            seatSearchInput->handleTextEntered(event.text.unicode);
                            seatListPage = 0;
                        }
                    }
                    if (inputSeatBatchRows && !dropdownConsumedClick) {
                        if (event.type == sf::Event::MouseButtonPressed) inputSeatBatchRows->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputSeatBatchRows->handleTextEntered(event.text.unicode);
                    }
                    if (inputSeatBatchCols && !dropdownConsumedClick) {
                        if (event.type == sf::Event::MouseButtonPressed) inputSeatBatchCols->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputSeatBatchCols->handleTextEntered(event.text.unicode);
                    }

                    // Click on Live Interactive Seat Map & Individual Seat Table
                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && !dropdownConsumedClick) {
                        float leftColX = sidebarWidth + 24.f;
                        float leftColW = 880.f;

                        // 1. Click on Seat Map Boxes
                        std::vector<SeatMapBox> mapBoxes = buildCurrentRoomSeatMapBoxes();
                        for (const auto& box : mapBoxes) {
                            if (box.bounds.contains(mousePos)) {
                                auto it = std::find(seatSelectedCodes.begin(), seatSelectedCodes.end(), box.maGhe);
                                if (it != seatSelectedCodes.end()) {
                                    seatSelectedCodes.erase(it);
                                } else {
                                    seatSelectedCodes.push_back(box.maGhe);
                                }

                                // Sync selected seats to Right Batch Form
                                if (!seatSelectedCodes.empty()) {
                                    std::set<char> selR;
                                    std::set<int> selC;
                                    for (const auto& sc : seatSelectedCodes) {
                                        size_t aLen = 0;
                                        std::string pfx;
                                        while (aLen < sc.size() && std::isalpha(static_cast<unsigned char>(sc[aLen]))) {
                                            pfx.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(sc[aLen]))));
                                            ++aLen;
                                        }
                                        if (aLen > 0 && aLen < sc.size()) {
                                            char rowCh = (pfx == "VIP") ? 'V' : ((pfx == "SW") ? 'S' : pfx[0]);
                                            selR.insert(rowCh);
                                            int c1 = 0, c2 = 0;
                                            std::string rest = sc.substr(aLen);
                                            if (std::sscanf(rest.c_str(), "%d-%d", &c1, &c2) == 2 && c1 > 0 && c2 >= c1) {
                                                for (int k = c1; k <= c2; ++k) selC.insert(k);
                                            } else if (std::sscanf(rest.c_str(), "%d", &c1) == 1 && c1 > 0) {
                                                selC.insert(c1);
                                            }
                                        }
                                    }
                                    std::string rStr = "";
                                    for (char rc : selR) {
                                        if (!rStr.empty()) rStr += ", ";
                                        rStr += rc;
                                    }
                                    std::string cStr = "";
                                    for (int cn : selC) {
                                        if (!cStr.empty()) cStr += ", ";
                                        cStr += std::to_string(cn);
                                    }
                                    if (inputSeatBatchRows) inputSeatBatchRows->setText(rStr);
                                    if (inputSeatBatchCols) inputSeatBatchCols->setText(cStr);

                                    for (size_t ti = 0; ti < seatTypeList.size(); ++ti) {
                                        if (seatTypeList[ti].tenLoai == box.loaiGhe && seatBatchTypeDropdown) {
                                            seatBatchTypeDropdown->setSelectedIndex(static_cast<int>(ti));
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }

                        // 2. Click on Bottom-Left Seat Table ("Danh sách ghế đã cấu hình")
                        std::vector<int> filteredSeatIndices = getFilteredRoomSeatIndices();
                        int pageSize = 5;
                        int totalItems = static_cast<int>(filteredSeatIndices.size());
                        int totalPages = std::max(1, (totalItems + pageSize - 1) / pageSize);
                        if (seatListPage >= totalPages) seatListPage = totalPages - 1;
                        if (seatListPage < 0) seatListPage = 0;

                        float rStartY = 730.f;
                        float rH = 31.f;
                        float colCenter5 = leftColX + 775.f;
                        int startIdx = seatListPage * pageSize;

                        for (int i = 0; i < pageSize && (startIdx + i) < totalItems; ++i) {
                            float ry = rStartY + i * rH;
                            int actualSeatIdx = filteredSeatIndices[startIdx + i];
                            const Seat& targetSeat = allSeats[actualSeatIdx];
                            std::string targetCode = targetSeat.getMaGhe();

                            // Click Trash icon (colCenter5 + 4 .. colCenter5 + 30)
                            if (mousePos.x >= colCenter5 + 4.f && mousePos.x <= colCenter5 + 30.f &&
                                mousePos.y >= ry && mousePos.y <= ry + rH) {
                                if (isSeatBookedInRoom(seatSelectedRoomId, targetCode)) {
                                    if (confirmPopup) {
                                        confirmPopup->show("CẢNH BÁO NGHIỆP VỤ", "Ghế " + targetCode + " đã có vé bán trong suất chiếu sắp tới. Không thể xóa ghế này!", ButtonStyle::Danger, "Đóng", "");
                                    }
                                    break;
                                }
                                if (confirmPopup) {
                                    std::string selRoom = seatSelectedRoomId;
                                    std::string tCode = targetCode;
                                    confirmPopup->show("XÁC NHẬN XÓA GHẾ", "Bạn có chắc chắn muốn xóa ghế " + tCode + " khỏi phòng " + selRoom + "?", ButtonStyle::Danger, "Xác nhận xóa", "Hủy");
                                    confirmPopup->setOnConfirm([this, selRoom, tCode]() {
                                        allSeats.erase(std::remove_if(allSeats.begin(), allSeats.end(), [&](const Seat& s) {
                                            return s.getMaPhong() == selRoom && s.getMaGhe() == tCode;
                                        }), allSeats.end());
                                        db.ghiDanhSachGhe(allSeats);
                                        refreshSeatsRoomDropdown();
                                        refreshShowtimesDropdowns();
                                    });
                                }
                                break;
                            }

                            // Click Pencil icon or Row -> Load into Right Form & Select on Map
                            if (mousePos.x >= leftColX + 20.f && mousePos.x <= leftColX + leftColW - 20.f &&
                                mousePos.y >= ry && mousePos.y <= ry + rH) {
                                size_t aLen = 0;
                                std::string pfx;
                                while (aLen < targetCode.size() && std::isalpha(static_cast<unsigned char>(targetCode[aLen]))) {
                                    pfx.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(targetCode[aLen]))));
                                    ++aLen;
                                }
                                if (aLen > 0 && aLen < targetCode.size()) {
                                    char rc = (pfx == "VIP") ? 'V' : ((pfx == "SW") ? 'S' : pfx[0]);
                                    std::string colPart = targetCode.substr(aLen);
                                    if (inputSeatBatchRows) inputSeatBatchRows->setText(std::string(1, rc));
                                    if (inputSeatBatchCols) inputSeatBatchCols->setText(colPart);
                                    seatSelectedCodes = {targetCode};

                                    for (size_t ti = 0; ti < seatTypeList.size(); ++ti) {
                                        if (seatTypeList[ti].tenLoai == targetSeat.getLoaiGhe() && seatBatchTypeDropdown) {
                                            seatBatchTypeDropdown->setSelectedIndex(static_cast<int>(ti));
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }

                        // 3. Click on Numbered Pagination Buttons (1, 2, 3) at y = 892..916
                        int startPage = std::max(0, std::min(seatListPage - 1, totalPages - 3));
                        for (int p = 0; p < 3 && (startPage + p) < totalPages; ++p) {
                            float px = leftColX + leftColW - 118.f + p * 23.f;
                            if (mousePos.x >= px && mousePos.x <= px + 20.f && mousePos.y >= 892.f && mousePos.y <= 916.f) {
                                seatListPage = startPage + p;
                                break;
                            }
                        }
                    }
                } else if (seatSubTab == 1) {
                    bool filterDrpClicked = false;
                    if (seatTypeFilterDropdown) {
                        if (event.type == sf::Event::MouseMoved) seatTypeFilterDropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) seatTypeFilterDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) {
                            if (seatTypeFilterDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos)) {
                                filterDrpClicked = true;
                            }
                        } else if (event.type == sf::Event::MouseButtonReleased) {
                            seatTypeFilterDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        }
                    }
                    if (filterDrpClicked) continue;

                    handleBtn(btnSeatExportPricing);
                    handleBtn(btnSeatAddNewType);
                    handleBtn(btnSeatPriceHistory);
                    handleBtn(btnSeatTypeFilterAll);
                    handleBtn(btnSeatTypeFilterSingle);
                    handleBtn(btnSeatTypeFilterCouple);
                    handleBtn(btnSeatTypeFilterSpecial);
                    handleBtn(btnSeatTypeAdd);
                    handleBtn(btnSeatTypeUpdate);
                    handleBtn(btnSeatTypeDelete);
                    handleBtn(btnSeatTypeReset);

                    if (seatTypeSearchInput) {
                        if (event.type == sf::Event::MouseButtonPressed) seatTypeSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) seatTypeSearchInput->handleTextEntered(event.text.unicode);
                    }
                    if (inputSeatTypeName) {
                        if (event.type == sf::Event::MouseButtonPressed) inputSeatTypeName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputSeatTypeName->handleTextEntered(event.text.unicode);
                    }
                    if (inputSeatTypePrice) {
                        if (event.type == sf::Event::MouseButtonPressed) inputSeatTypePrice->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) {
                            if (event.text.unicode == 8 || (event.text.unicode >= '0' && event.text.unicode <= '9')) {
                                inputSeatTypePrice->handleTextEntered(event.text.unicode);
                            }
                        }
                    }
                    if (inputSeatTypeDesc) {
                        if (event.type == sf::Event::MouseButtonPressed) inputSeatTypeDesc->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputSeatTypeDesc->handleTextEntered(event.text.unicode);
                    }

                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 24.f;
                        float leftColW = 880.f;
                        float rightColX = leftColX + leftColW + 20.f;
                        float rightColW = 500.f;

                        // Build filtered seatType indices (diacritic-insensitive & filtered by seatTypeFilterIdx dropdown)
                        std::string q = seatTypeSearchInput ? UI::removeVietnameseDiacritics(seatTypeSearchInput->getText()) : "";
                        std::vector<int> visTypeIndices;
                        for (size_t i = 0; i < seatTypeList.size(); ++i) {
                            if (seatTypeFilterIdx > 0 && seatTypeFilterIdx <= static_cast<int>(seatTypeList.size())) {
                                if (seatTypeList[i].tenLoai != seatTypeList[seatTypeFilterIdx - 1].tenLoai) continue;
                            }
                            std::string tlLow = UI::removeVietnameseDiacritics(seatTypeList[i].tenLoai);
                            if (!q.empty()) {
                                std::string descLow = UI::removeVietnameseDiacritics(seatTypeList[i].moTa + " " + seatTypeList[i].maLoai);
                                if (tlLow.find(q) == std::string::npos && descLow.find(q) == std::string::npos) continue;
                            }
                            visTypeIndices.push_back(static_cast<int>(i));
                        }

                        float tblCardY = 228.f;
                        float rowStartY = tblCardY + 88.f;
                        float rowH = 66.f;

                        // Click row in Seat Types table
                        for (size_t vi = 0; vi < visTypeIndices.size() && vi < 5; ++vi) {
                            float ry = rowStartY + vi * rowH;
                            if (mousePos.x >= leftColX + 16.f && mousePos.x <= leftColX + leftColW - 16.f && mousePos.y >= ry && mousePos.y <= ry + rowH) {
                                int actualIdx = visTypeIndices[vi];
                                if (mousePos.x >= leftColX + 775.f && mousePos.x <= leftColX + 860.f) {
                                    seatTypeList[actualIdx].kichHoatBan = !seatTypeList[actualIdx].kichHoatBan;
                                }
                                syncSeatTypeSelectionToForm(actualIdx);
                                break;
                            }
                        }

                        // Click Price Up/Down Stepper inside inputSeatTypePrice (y = 414.f, h = 40.f)
                        float prBoxRight = rightColX + rightColW - 24.f;
                        if (mousePos.x >= prBoxRight - 28.f && mousePos.x <= prBoxRight - 4.f &&
                            mousePos.y >= 414.f && mousePos.y <= 454.f && inputSeatTypePrice) {
                            double curP = 100000.0;
                            try { curP = std::stod(inputSeatTypePrice->getText()); } catch (...) { curP = 100000.0; }
                            if (mousePos.y < 434.f) curP += 10000.0;
                            else curP = std::max(10000.0, curP - 10000.0);
                            char pBuf[32];
                            snprintf(pBuf, sizeof(pBuf), "%.0f", curP);
                            inputSeatTypePrice->setText(pBuf);
                        }

                        // Click Circular Color Swatches (Right Form: y = 486.f, diameter = 32.f)
                        float swatchY = 486.f;
                        for (int s = 0; s < 5; ++s) {
                            float sx = rightColX + 28.f + s * 64.f;
                            if (mousePos.x >= sx - 4.f && mousePos.x <= sx + 36.f && mousePos.y >= swatchY - 4.f && mousePos.y <= swatchY + 36.f) {
                                seatTypeColorIdx = s;
                                break;
                            }
                        }

                        // Click Toggle Surcharge / Active Switch (swBoxY = 626.f, h = 46.f)
                        float swBoxY = 626.f;
                        if (mousePos.x >= rightColX + 24.f && mousePos.x <= rightColX + rightColW - 24.f &&
                            mousePos.y >= swBoxY && mousePos.y <= swBoxY + 46.f) {
                            seatTypeSaleActive = !seatTypeSaleActive;
                        }
                    }
                }
                break;
            }

            case ActiveTab::Showtimes: {
                handleBtn(btnStTabList);
                handleBtn(btnStTabTimeline);
                handleBtn(btnStTabByMovie);
                handleBtn(btnStExport);
                handleBtn(btnStNew);

                auto handleStDrop = [&](std::unique_ptr<DropdownBox>& drp) {
                    if (!drp) return;
                    if (event.type == sf::Event::MouseButtonPressed) {
                        bool wasExpanded = drp->getExpanded();
                        drp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (!wasExpanded && drp->getExpanded()) {
                            if (stMovieFilterDropdown && stMovieFilterDropdown.get() != drp.get()) stMovieFilterDropdown->setExpanded(false);
                            if (stDateDropdown && stDateDropdown.get() != drp.get()) stDateDropdown->setExpanded(false);
                            if (stRoomDropdown && stRoomDropdown.get() != drp.get()) stRoomDropdown->setExpanded(false);
                            if (stMovieDropdown && stMovieDropdown.get() != drp.get()) stMovieDropdown->setExpanded(false);
                            if (stRoomFormDropdown && stRoomFormDropdown.get() != drp.get()) stRoomFormDropdown->setExpanded(false);
                        }
                    } else if (event.type == sf::Event::MouseButtonReleased) {
                        drp->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::MouseMoved) {
                        drp->handleMouseMove(mousePos);
                    } else if (event.type == sf::Event::MouseWheelScrolled) {
                        drp->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                };
                handleStDrop(stMovieFilterDropdown);
                handleStDrop(stDateDropdown);
                handleStDrop(stRoomDropdown);
                handleStDrop(stMovieDropdown);
                handleStDrop(stRoomFormDropdown);

                handleBtn(btnStSave);
                handleBtn(btnStCreate);
                handleBtn(btnStDelete);
                handleBtn(btnStReset);

                if (stSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) stSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) stSearchInput->handleTextEntered(event.text.unicode);
                }
                if (inputStDate) {
                    if (event.type == sf::Event::MouseButtonPressed) inputStDate->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) {
                        inputStDate->handleTextEntered(event.text.unicode);
                        refreshFormMovieDropdownForDate();
                    }
                }
                if (inputStTime) {
                    if (event.type == sf::Event::MouseButtonPressed) inputStTime->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputStTime->handleTextEntered(event.text.unicode);
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 880.f;

                    // 4 Footer KPI Cards Click Events
                    float kpiY = ST_KPI_Y;
                    float kpiH = 66.f;
                    float kpiGap = 16.f;
                    float kpiTotalW = 1420.f;
                    float kpiCardW = (kpiTotalW - 3 * kpiGap) / 4.f; // 343.f

                    if (mousePos.y >= kpiY && mousePos.y <= kpiY + kpiH) {
                        // Card 1: Tổng Suất Chiếu -> Bấm vào để TRỞ VỀ QUẢN LÝ NHIỀU SUẤT CHIẾU (Hiển thị đầy đủ tất cả suất chiếu)
                        float c1X = leftColX;
                        if (mousePos.x >= c1X && mousePos.x <= c1X + kpiCardW) {
                            if (stSearchInput) stSearchInput->setText("");
                            if (stMovieFilterDropdown) stMovieFilterDropdown->setSelectedIndex(0);
                            if (stDateDropdown) stDateDropdown->setSelectedIndex(0); // Trở về "Tất cả các ngày"
                            if (stRoomDropdown) stRoomDropdown->setSelectedIndex(0); // Trở về "Tất cả phòng"
                            stMovieFilterVal = "";
                            stDateFilterVal = "";
                            stRoomFilterVal = "";
                            showtimeCurrentPage = 0;
                            selectedShowtimeRowIdx = -1;
                            refreshShowtimesDropdowns();
                        }
                        // Card 2, Card 3, Card 4: Chỉ liên kết với data để hiển thị, khi bấm vào không dịch chuyển qua đâu cả
                    }

                    if (showtimeSubTab == 0) {
                        float tblHeaderY = ST_CARD_Y + 44.f;
                        float rowStartY = tblHeaderY + 34.f;
                        float rowH = 118.f;

                        // Calculate visibleIdx (Case- and diacritic-insensitive!)
                        std::string q = stSearchInput ? stSearchInput->getText() : "";
                        std::string qClean = removeVietnameseDiacritics(q);

                        std::vector<int> visibleIdx;
                        for (size_t i = 0; i < showtimeList.size(); ++i) {
                            if (!stDateFilterVal.empty() && showtimeList[i].getNgayChieu() != stDateFilterVal) continue;
                            if (!stRoomFilterVal.empty() && showtimeList[i].getMaPhong() != stRoomFilterVal) continue;
                            if (!stMovieFilterVal.empty() && showtimeList[i].getMaPhim() != stMovieFilterVal) continue;
                            if (!qClean.empty()) {
                                std::string scClean = removeVietnameseDiacritics(showtimeList[i].getMaSuatChieu());
                                std::string spClean = removeVietnameseDiacritics(showtimeList[i].getMaPhong());
                                std::string smClean = removeVietnameseDiacritics(showtimeList[i].getMaPhim());
                                std::string pName = "";
                                for (const auto& m : movieList) { if (m.getMaPhim() == showtimeList[i].getMaPhim()) { pName = m.getTenPhim(); break; } }
                                std::string pNameClean = removeVietnameseDiacritics(pName);
                                std::string rName = "";
                                for (const auto& r : roomList) { if (r.getMaPhong() == showtimeList[i].getMaPhong()) { rName = r.getTenPhong() + " - " + r.getLoaiPhong(); break; } }
                                std::string rNameClean = removeVietnameseDiacritics(rName);

                                if (scClean.find(qClean) == std::string::npos &&
                                    spClean.find(qClean) == std::string::npos &&
                                    smClean.find(qClean) == std::string::npos &&
                                    pNameClean.find(qClean) == std::string::npos &&
                                    rNameClean.find(qClean) == std::string::npos) {
                                    continue;
                                }
                            }
                            visibleIdx.push_back(static_cast<int>(i));
                        }

                        int totalVisible = static_cast<int>(visibleIdx.size());
                        int totalPages = std::max(1, (totalVisible + 3) / 4);
                        if (showtimeCurrentPage >= totalPages) showtimeCurrentPage = totalPages - 1;
                        if (showtimeCurrentPage < 0) showtimeCurrentPage = 0;

                        int startIdx = showtimeCurrentPage * 4;
                        int endIdx = std::min(startIdx + 4, totalVisible);

                        // Table row clicks
                        for (int r = startIdx; r < endIdx; ++r) {
                            int actualIdx = visibleIdx[r];
                            float ry = rowStartY + (r - startIdx) * rowH;

                            // Sửa Button click (Pencil icon at aColX = leftColX + 780.f, ry + 46.f, 26x26)
                            float aColX = leftColX + 780.f;
                            float icY = ry + 46.f;
                            if (mousePos.x >= aColX && mousePos.x <= aColX + 26.f &&
                                mousePos.y >= icY && mousePos.y <= icY + 26.f) {
                                syncShowtimeSelectionToForm(actualIdx);
                                break;
                            }

                            // Xóa Button click (Trash icon at aColX + 32.f, ry + 46.f, 26x26)
                            if (mousePos.x >= aColX + 32.f && mousePos.x <= aColX + 58.f &&
                                mousePos.y >= icY && mousePos.y <= icY + 26.f) {
                                std::string code = showtimeList[actualIdx].getMaSuatChieu();
                                std::vector<Showtime> backup = showtimeList;
                                showtimeList.erase(showtimeList.begin() + actualIdx);
                                if (!db.ghiDanhSachSuatChieu(showtimeList)) {
                                    showtimeList = backup;
                                    if (confirmPopup) confirmPopup->show("LỖI CƠ SỞ DỮ LIỆU", "Không thể xóa suất chiếu khỏi cơ sở dữ liệu!", ButtonStyle::Danger, "Đóng", "");
                                    break;
                                }
                                refreshShowtimesDropdowns();
                                if (selectedShowtimeRowIdx >= static_cast<int>(showtimeList.size())) {
                                    selectedShowtimeRowIdx = static_cast<int>(showtimeList.size()) - 1;
                                }
                                if (selectedShowtimeRowIdx >= 0) {
                                    syncShowtimeSelectionToForm(selectedShowtimeRowIdx);
                                }
                                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_SUAT_CHIEU", "Xoa suat chieu: " + code);
                                if (confirmPopup) confirmPopup->show("THÀNH CÔNG", "Đã xóa suất chiếu " + code + "!", ButtonStyle::Primary, "Đóng", "");
                                break;
                            }

                            // Row body click selection
                            if (mousePos.x >= leftColX + 8.f && mousePos.x <= leftColX + 770.f &&
                                mousePos.y >= ry && mousePos.y <= ry + rowH - 6.f) {
                                syncShowtimeSelectionToForm(actualIdx);
                                break;
                            }
                        }

                        // Pagination buttons click detection (Exact math alignment!)
                        float footY = ST_CARD_Y + ST_CARD_H - 36.f;
                        float btnSz = 28.f;
                        float btnGap = 6.f;
                        float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
                        float pgX = leftColX + (leftColW - totalPagingW) / 2.f;

                        // Click Prev '<'
                        if (mousePos.x >= pgX && mousePos.x <= pgX + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            if (showtimeCurrentPage > 0) showtimeCurrentPage--;
                        }
                        // Click page numbers
                        for (int p = 0; p < totalPages; ++p) {
                            float px = pgX + (p + 1) * (btnSz + btnGap);
                            if (mousePos.x >= px && mousePos.x <= px + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                                showtimeCurrentPage = p;
                            }
                        }
                        // Click Next '>'
                        float nextX = pgX + (totalPages + 1) * (btnSz + btnGap);
                        if (mousePos.x >= nextX && mousePos.x <= nextX + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            if (showtimeCurrentPage < totalPages - 1) showtimeCurrentPage++;
                        }
                    } else if (showtimeSubTab == 1) {
                        if (showtimeTimelineDate.empty()) showtimeTimelineDate = stTodayStr();
                        if (!stDateFilterVal.empty()) showtimeTimelineDate = stDateFilterVal;

                        // Date Navigator button clicks [<] and [>]
                        float dtNavStartX = leftColX + leftColW - 190.f;
                        if (mousePos.x >= dtNavStartX && mousePos.x <= dtNavStartX + 24.f &&
                            mousePos.y >= ST_CARD_Y + 12.f && mousePos.y <= ST_CARD_Y + 36.f) {
                            stDateFilterVal = "";
                            if (stDateDropdown) stDateDropdown->setSelectedIndex(0);
                            showtimeTimelineDate = stAddDays(showtimeTimelineDate, -1);
                        }
                        if (mousePos.x >= dtNavStartX + 162.f && mousePos.x <= dtNavStartX + 186.f &&
                            mousePos.y >= ST_CARD_Y + 12.f && mousePos.y <= ST_CARD_Y + 36.f) {
                            stDateFilterVal = "";
                            if (stDateDropdown) stDateDropdown->setSelectedIndex(0);
                            showtimeTimelineDate = stAddDays(showtimeTimelineDate, 1);
                        }

                        // Timeline click detection across all 8 rooms
                        float tlStartX = leftColX + 116.f;
                        float tlW = 744.f;
                        float startLaneY = ST_CARD_Y + 76.f;
                        float laneH = 54.f;
                        float laneGap = 6.f;
                        size_t totalLanes = roomList.size();
                        std::string curDateDisplay = showtimeTimelineDate;

                        for (size_t r = 0; r < totalLanes; ++r) {
                            float ly = startLaneY + r * (laneH + laneGap);
                            for (size_t i = 0; i < showtimeList.size(); ++i) {
                                const auto& st = showtimeList[i];
                                if (st.getMaPhong() != roomList[r].getMaPhong()) continue;
                                if (st.getNgayChieu() != curDateDisplay) continue;

                                int sMins = 540;
                                if (!stParseTimeToMinutes(st.getGioBatDau(), sMins)) continue;
                                int dur = 120;
                                for (const auto& m : movieList) { if (m.getMaPhim() == st.getMaPhim()) { dur = m.getThoiLuong(); break; } }

                                float bx = tlStartX + ((sMins - 480) / 960.f) * tlW;
                                float bw = std::max(68.f, (dur / 960.f) * tlW);
                                float by = ly + 4.f;
                                float bh = laneH - 8.f;

                                if (mousePos.x >= bx && mousePos.x <= bx + bw && mousePos.y >= by && mousePos.y <= by + bh) {
                                    syncShowtimeSelectionToForm(static_cast<int>(i));
                                    break;
                                }
                            }
                        }
                    } else if (showtimeSubTab == 2) {
                        // Movie grouped schedule click detection (3 movies per page)
                        int totalMovies = static_cast<int>(movieList.size());
                        int moviesPerPage = 3;
                        int totalPages = std::max(1, (totalMovies + moviesPerPage - 1) / moviesPerPage);
                        if (showtimeCurrentPage >= totalPages) showtimeCurrentPage = totalPages - 1;
                        if (showtimeCurrentPage < 0) showtimeCurrentPage = 0;

                        int startM = showtimeCurrentPage * moviesPerPage;
                        int endM = std::min(startM + moviesPerPage, totalMovies);

                        float startCardY = ST_CARD_Y + 48.f;
                        float cardBlockH = 154.f;
                        float cardGap = 12.f;

                        for (int m = startM; m < endM; ++m) {
                            float cy = startCardY + (m - startM) * (cardBlockH + cardGap);
                            const auto& movie = movieList[m];

                            // Check "+ Thêm suất" button
                            float addBtnX = leftColX + leftColW - 146.f;
                            float addBtnY = cy + 18.f;
                            float addBtnW = 120.f;
                            float addBtnH = 28.f;
                            if (mousePos.x >= addBtnX && mousePos.x <= addBtnX + addBtnW &&
                                mousePos.y >= addBtnY && mousePos.y <= addBtnY + addBtnH) {
                                selectedShowtimeRowIdx = -1;
                                stFormMovieIdx = m;
                                if (inputStTime) inputStTime->setText("");
                                if (inputStDate) inputStDate->setText("");
                                refreshFormMovieDropdownForDate();
                                int mappedDropIdx = 0;
                                for (size_t fi = 0; fi < stFilteredMovieIndices.size(); ++fi) {
                                    if (stFilteredMovieIndices[fi] == m) {
                                        mappedDropIdx = static_cast<int>(fi);
                                        break;
                                    }
                                }
                                if (stMovieDropdown) stMovieDropdown->setSelectedIndex(mappedDropIdx);
                                stFormMovieIdx = m;
                                stLastValidMinutes = -1;
                                break;
                            }

                            // Check showtime pills
                            float pillStartX = leftColX + 120.f;
                            float pillY = cy + 86.f;
                            float pillW = 136.f;
                            float pillH = 42.f;
                            float pillGap = 10.f;

                            int pIdx = 0;
                            for (size_t i = 0; i < showtimeList.size(); ++i) {
                                const auto& st = showtimeList[i];
                                if (st.getMaPhim() != movie.getMaPhim()) continue;
                                if (!stDateFilterVal.empty() && st.getNgayChieu() != stDateFilterVal) continue;

                                if (pIdx < 5) {
                                    float px = pillStartX + pIdx * (pillW + pillGap);
                                    if (mousePos.x >= px && mousePos.x <= px + pillW &&
                                        mousePos.y >= pillY && mousePos.y <= pillY + pillH) {
                                        syncShowtimeSelectionToForm(static_cast<int>(i));
                                        break;
                                    }
                                    pIdx++;
                                }
                            }
                        }

                        // SubTab 2 Pagination Clicks
                        float footY = ST_CARD_Y + ST_CARD_H - 36.f;
                        float btnSz = 28.f;
                        float btnGap = 6.f;
                        float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
                        float pgX = leftColX + (leftColW - totalPagingW) / 2.f;

                        // Click Prev '<'
                        if (mousePos.x >= pgX && mousePos.x <= pgX + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            if (showtimeCurrentPage > 0) showtimeCurrentPage--;
                        }
                        // Click page numbers
                        for (int p = 0; p < totalPages; ++p) {
                            float px = pgX + (p + 1) * (btnSz + btnGap);
                            if (mousePos.x >= px && mousePos.x <= px + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                                showtimeCurrentPage = p;
                            }
                        }
                        // Click Next '>'
                        float nextX = pgX + (totalPages + 1) * (btnSz + btnGap);
                        if (mousePos.x >= nextX && mousePos.x <= nextX + btnSz && mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            if (showtimeCurrentPage < totalPages - 1) showtimeCurrentPage++;
                        }
                    }
                }
                break;
            }

            case ActiveTab::Concessions:
                handleBtn(btnFnbTabAll);
                handleBtn(btnFnbTabCombo);
                handleBtn(btnFnbExport);
                handleBtn(btnFnbQuickAdd);
                handleBtn(btnFnbCreate);
                handleBtn(btnFnbDelete);
                handleBtn(btnFnbUpdate);
                handleBtn(btnFnbClear);

                if (fnbFormLoai == "Combo") {
                    if (comboItem1Dropdown) {
                        if (event.type == sf::Event::MouseMoved) comboItem1Dropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) comboItem1Dropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseButtonReleased) comboItem1Dropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) comboItem1Dropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                    if (comboItem2Dropdown) {
                        if (event.type == sf::Event::MouseMoved) comboItem2Dropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) comboItem2Dropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseButtonReleased) comboItem2Dropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) comboItem2Dropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                }

                if (fnbSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) fnbSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) fnbSearchInput->handleTextEntered(event.text.unicode);
                }
                if (inputFnbId && selectedFnbRowIdx < 0) {
                    if (event.type == sf::Event::MouseButtonPressed) inputFnbId->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputFnbId->handleTextEntered(event.text.unicode);
                }
                if (inputFnbName) {
                    if (event.type == sf::Event::MouseButtonPressed) inputFnbName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputFnbName->handleTextEntered(event.text.unicode);
                }
                if (inputFnbPrice) {
                    if (event.type == sf::Event::MouseButtonPressed) inputFnbPrice->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputFnbPrice->handleTextEntered(event.text.unicode);
                }
                if (inputFnbNote) {
                    if (event.type == sf::Event::MouseButtonPressed) inputFnbNote->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputFnbNote->handleTextEntered(event.text.unicode);
                }
                if (inputFnbImage) {
                    if (event.type == sf::Event::MouseButtonPressed) inputFnbImage->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::TextEntered) inputFnbImage->handleTextEntered(event.text.unicode);
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 928.f;
                    float rightColX = sidebarWidth + 968.f;
                    float rightColW = 486.f;
                    float gInputX = rightColX + 24.f;
                    float gInputW = rightColW - 48.f;

                    // 1. Sort toggle button [Mã sản phẩm ↓] on Left Card Header
                    if (mousePos.x >= leftColX + leftColW - 144.f && mousePos.x <= leftColX + leftColW - 22.f &&
                        mousePos.y >= 106.f && mousePos.y <= 138.f) {
                        fnbSortAsc = !fnbSortAsc;
                    }

                    // 2. Service Type Segmented Toggle [Món lẻ | Combo gói] on Right Card
                    if (mousePos.x >= gInputX + 220.f && mousePos.x <= gInputX + 438.f &&
                        mousePos.y >= 318.f && mousePos.y <= 360.f) {
                        float halfSeg = 218.f / 2.f;
                        if (mousePos.x < gInputX + 220.f + halfSeg) {
                            if (fnbFormLoai == "Combo") fnbFormLoai = "Bap";
                        } else {
                            fnbFormLoai = "Combo";
                        }
                    }

                    // 2b. Combo Recipe items interactions (Only active when fnbFormLoai == "Combo")
                    if (fnbFormLoai == "Combo") {
                        auto rIdxs = getRetailFnbIndices();
                        // Item 1: Stepper [-] & [+] and Name Change
                        if (mousePos.x >= gInputX + gInputW - 76.f && mousePos.x <= gInputX + gInputW - 50.f &&
                            mousePos.y >= 505.f && mousePos.y <= 533.f) {
                            if (comboItem1Qty > 1) comboItem1Qty--;
                        }
                        else if (mousePos.x >= gInputX + gInputW - 22.f && mousePos.x <= gInputX + gInputW + 4.f &&
                                 mousePos.y >= 505.f && mousePos.y <= 533.f) {
                            comboItem1Qty++;
                        }
                        else if (mousePos.x >= gInputX + 14.f && mousePos.x <= gInputX + gInputW - 140.f &&
                                 mousePos.y >= 500.f && mousePos.y <= 538.f) {
                            if (!rIdxs.empty()) comboItem1Idx = (comboItem1Idx + 1) % rIdxs.size();
                        }

                        // Item 2: Stepper [-] & [+] and Name Change
                        if (mousePos.x >= gInputX + gInputW - 76.f && mousePos.x <= gInputX + gInputW - 50.f &&
                            mousePos.y >= 547.f && mousePos.y <= 575.f) {
                            if (comboItem2Qty > 1) comboItem2Qty--;
                        }
                        else if (mousePos.x >= gInputX + gInputW - 22.f && mousePos.x <= gInputX + gInputW + 4.f &&
                                 mousePos.y >= 547.f && mousePos.y <= 575.f) {
                            comboItem2Qty++;
                        }
                        else if (mousePos.x >= gInputX + 14.f && mousePos.x <= gInputX + gInputW - 140.f &&
                                 mousePos.y >= 542.f && mousePos.y <= 580.f) {
                            if (!rIdxs.empty()) comboItem2Idx = (comboItem2Idx + 1) % rIdxs.size();
                        }
                    }

                    // 3. Business Status Toggle Box [Đang bán / Ngừng bán] on Right Card
                    float stY = (fnbFormLoai == "Combo") ? 656.f : (462.f + 24.f);
                    if (mousePos.x >= gInputX + 224.f && mousePos.x <= gInputX + 438.f &&
                        mousePos.y >= stY && mousePos.y <= stY + 42.f) {
                        fnbFormActive = !fnbFormActive;
                    }

                    // 4. Table Row Selection & Direct Status Pill Toggle
                    float rowStartY = 92.f + 58.f + 42.f;
                    float rowH = 82.f;
                    if (mousePos.x >= leftColX && mousePos.x <= leftColX + leftColW &&
                        mousePos.y >= rowStartY && mousePos.y <= rowStartY + 8.f * rowH) {
                        int clickedVisRow = static_cast<int>((mousePos.y - rowStartY) / rowH);
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

                        if (clickedVisRow >= 0 && clickedVisRow < static_cast<int>(visibleIdx.size())) {
                            int targetIdx = visibleIdx[clickedVisRow];
                            // If clicked directly on TRẠNG THÁI pill column (leftColX + 775 .. leftColX + 895), toggle status in DB immediately
                            if (mousePos.x >= leftColX + 775.f && mousePos.x <= leftColX + 895.f) {
                                std::string curSt = fnbList[targetIdx].getTrangThaiKinhDoanh();
                                std::string nextSt = (curSt == "DangBan") ? "NgungBan" : "DangBan";
                                fnbList[targetIdx].setTrangThaiKinhDoanh(nextSt);
                                db.ghiDanhSachFAndB(fnbList);
                            }
                            selectedFnbRowIdx = targetIdx;
                            std::string selCode = fnbList[targetIdx].getMaMatHang();
                            inputFnbId->setText(selCode);
                            inputFnbId->setEnabled(false);
                            inputFnbName->setText(fnbList[targetIdx].getTenMatHang());
                            inputFnbPrice->setText(std::to_string(static_cast<long long>(fnbList[targetIdx].getGiaTien())));
                            std::string storedImg = fnbImagePaths.count(selCode) ? fnbImagePaths[selCode] : (selCode + ".jpg");
                            size_t slashPos = storedImg.find_last_of("/\\");
                            std::string dispImg = (slashPos != std::string::npos) ? storedImg.substr(slashPos + 1) : storedImg;
                            if (inputFnbImage) {
                                inputFnbImage->setText(dispImg);
                            }
                            fnbFormLoai = fnbList[targetIdx].getLoai();
                            fnbFormActive = (fnbList[targetIdx].getTrangThaiKinhDoanh() == "DangBan");
                        }
                    }
                }
                break;

            case ActiveTab::ShiftReport:
                if ((currentUserRole == "Admin" || currentUserRole == "Khách") && !adminShowHistorySubView) {
                    if (adminNextCashierDropdown) {
                        if (event.type == sf::Event::MouseMoved) adminNextCashierDropdown->handleMouseMove(mousePos);
                        else if (event.type == sf::Event::MouseButtonPressed) adminNextCashierDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseButtonReleased) adminNextCashierDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::MouseWheelScrolled) adminNextCashierDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                    if (inputAdminManagerPin) {
                        if (event.type == sf::Event::MouseButtonPressed) inputAdminManagerPin->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputAdminManagerPin->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnAdminShiftRefresh);
                    handleBtn(btnAdminShiftExit);
                    handleBtn(btnAdminShiftHistory);
                    handleBtn(btnAdminCashierHandover);
                    handleBtn(btnAdminManagerApprove);
                } else {
                    if (currentUserRole == "Staff") {
                        if (inputShiftActualCash) {
                            if (event.type == sf::Event::MouseButtonPressed) inputShiftActualCash->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                            else if (event.type == sf::Event::TextEntered) inputShiftActualCash->handleTextEntered(event.text.unicode);
                        }
                        handleBtn(btnShiftCalculate);
                        handleBtn(btnShiftCloseShift);
                    }
                    handleBtn(btnShiftExportReport);
                    handleBtn(btnShiftRefresh);
                    handleBtn(btnShiftPrevPage);
                    handleBtn(btnShiftNextPage);
                    if (currentUserRole == "Admin" || currentUserRole == "Khách") {
                        handleBtn(btnAdminShiftHistory);
                    }
                }
                break;

            case ActiveTab::Promotions: {
                handleBtn(btnPromoExport);
                handleBtn(btnPromoNew);
                handleBtn(btnPromoFilterAll);
                handleBtn(btnPromoFilterActive);
                handleBtn(btnPromoFilterUpcoming);
                handleBtn(btnPromoFilterExpired);
                handleBtn(btnPromoCopyCode);
                handleBtn(btnPromoSelectMovie);
                handleBtn(btnPromoCreate);
                handleBtn(btnPromoUpdate);
                handleBtn(btnPromoDelete);
                handleBtn(btnPromoClear);

                if (promoScopeDropdown) {
                    if (event.type == sf::Event::MouseMoved) promoScopeDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) promoScopeDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) promoScopeDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                }

                if (event.type == sf::Event::KeyPressed && event.key.control && event.key.code == sf::Keyboard::K) {
                    if (promoSearchInput) promoSearchInput->setFocused(true);
                }

                auto handlePromoInput = [&](std::unique_ptr<UIInputBox>& inp) {
                    if (!inp) return;
                    if (event.type == sf::Event::MouseButtonPressed) {
                        std::string prev = inp->getText();
                        inp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::TextEntered) {
                        std::string prev = inp->getText();
                        inp->handleTextEntered(event.text.unicode);
                        if (inp.get() == promoSearchInput.get() && inp->getText() != prev) {
                            promoCurrentPage = 0;
                        }
                    }
                };

                handlePromoInput(promoSearchInput);
                if (selectedPromoRowIdx < 0) handlePromoInput(inputPromoId);
                handlePromoInput(inputPromoName);
                handlePromoInput(inputPromoCode);
                handlePromoInput(inputPromoPercent);
                handlePromoInput(inputPromoMaxDiscount);
                handlePromoInput(inputPromoStartDate);
                handlePromoInput(inputPromoEndDate);

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 860.f;
                    float rowStartY = 334.f;
                    float rowH = 104.f;

                    std::vector<int> filteredIndices = getFilteredPromoIndices();
                    int totalFiltered = static_cast<int>(filteredIndices.size());
                    int rowsPerPage = 5;
                    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
                    if (promoCurrentPage >= totalPages) promoCurrentPage = totalPages - 1;
                    if (promoCurrentPage < 0) promoCurrentPage = 0;

                    int startIdx = promoCurrentPage * rowsPerPage;
                    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);
                    int shownCount = std::max(0, endIdx - startIdx);

                    // 1. Table row & code pill clicks
                    for (int r = 0; r < shownCount; ++r) {
                        int actualIdx = filteredIndices[startIdx + r];
                        float ry = rowStartY + r * rowH;

                        // Check click on MÃ CODE pill (x around leftColX + 310..450, y around ry + 34..70)
                        if (mousePos.x >= leftColX + 310.f && mousePos.x <= leftColX + 450.f &&
                            mousePos.y >= ry + 34.f && mousePos.y <= ry + 70.f) {
                            syncPromoSelectionToForm(actualIdx);
                            std::string cStr = promoList[actualIdx].getMaCode();
                            sf::Clipboard::setString(toSfString(cStr));
                            if (confirmPopup) {
                                confirmPopup->show("ĐÃ SAO CHÉP MÃ CODE", "Đã sao chép mã voucher '" + cStr + "' vào bộ nhớ tạm!", ButtonStyle::Primary, "Đóng", "");
                            }
                            break;
                        }

                        // Row body selection click
                        if (mousePos.x >= leftColX + 4.f && mousePos.x <= leftColX + leftColW - 4.f &&
                            mousePos.y >= ry && mousePos.y <= ry + rowH) {
                            syncPromoSelectionToForm(actualIdx);
                            break;
                        }
                    }

                    // 2. Pagination buttons click detection
                    float footY = 868.f;
                    float btnSz = 28.f;
                    float btnGap = 6.f;
                    float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
                    float pgStartX = leftColX + leftColW - 18.f - totalPagingW;

                    // Prev '<'
                    if (mousePos.x >= pgStartX && mousePos.x <= pgStartX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (promoCurrentPage > 0) promoCurrentPage--;
                    }
                    // Page numbers 1..N
                    for (int p = 0; p < totalPages; ++p) {
                        float px = pgStartX + (p + 1) * (btnSz + btnGap);
                        if (mousePos.x >= px && mousePos.x <= px + btnSz &&
                            mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            promoCurrentPage = p;
                        }
                    }
                    // Next '>'
                    float nextX = pgStartX + (totalPages + 1) * (btnSz + btnGap);
                    if (mousePos.x >= nextX && mousePos.x <= nextX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (promoCurrentPage < totalPages - 1) promoCurrentPage++;
                    }
                }
                break;
            }

            case ActiveTab::Staff: {
                handleBtn(btnStaffExport);
                handleBtn(btnStaffNew);
                handleBtn(btnStaffFilterAll);
                handleBtn(btnStaffFilterActive);
                handleBtn(btnStaffFilterResigned);
                handleBtn(btnStaffCreate);
                handleBtn(btnStaffUpdate);
                handleBtn(btnStaffDelete);
                handleBtn(btnStaffClear);
                handleBtn(btnStaffPrevPage);
                handleBtn(btnStaffNextPage);

                if (staffGenderDropdown) {
                    if (event.type == sf::Event::MouseMoved) staffGenderDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) staffGenderDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) staffGenderDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                }

                if (event.type == sf::Event::KeyPressed && event.key.control && event.key.code == sf::Keyboard::K) {
                    if (staffSearchInput) staffSearchInput->setFocused(true);
                }

                auto handleStaffInput = [&](std::unique_ptr<UIInputBox>& inp) {
                    if (!inp) return;
                    if (event.type == sf::Event::MouseButtonPressed) {
                        inp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::TextEntered) {
                        std::string prev = inp->getText();
                        inp->handleTextEntered(event.text.unicode);
                        if (inp.get() == staffSearchInput.get() && inp->getText() != prev) {
                            staffCurrentPage = 0;
                        }
                    }
                };

                handleStaffInput(staffSearchInput);
                if (selectedStaffRowIdx < 0) handleStaffInput(inputStaffId);
                handleStaffInput(inputStaffName);
                handleStaffInput(inputStaffPhone);
                handleStaffInput(inputStaffBirthDate);
                handleStaffInput(inputStaffCCCD);
                handleStaffInput(inputStaffEmail);

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 910.f;
                    float rowStartY = 378.f;
                    float rowH = 74.f;

                    std::vector<int> filteredIndices = getFilteredStaffIndices();
                    int totalFiltered = static_cast<int>(filteredIndices.size());
                    int rowsPerPage = 5;
                    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
                    if (staffCurrentPage >= totalPages) staffCurrentPage = totalPages - 1;
                    if (staffCurrentPage < 0) staffCurrentPage = 0;

                    int startIdx = staffCurrentPage * rowsPerPage;
                    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);
                    int shownCount = std::max(0, endIdx - startIdx);

                    // Table rows selection & status pill clicks
                    for (int r = 0; r < shownCount; ++r) {
                        int actualIdx = filteredIndices[startIdx + r];
                        float ry = rowStartY + r * rowH;

                        // Click directly on status pill (leftColX + 780 .. leftColX + 890, ry + 16 .. ry + 56)
                        if (mousePos.x >= leftColX + 780.f && mousePos.x <= leftColX + 890.f &&
                            mousePos.y >= ry + 16.f && mousePos.y <= ry + 56.f) {
                            if (staffUserList[actualIdx].vaiTro == "Admin") {
                                if (confirmPopup) confirmPopup->show("CẢNH BÁO", "Không thể chọn nghỉ làm với Admin để đảm bảo hệ thống luôn có ít nhất một Admin đang hoạt động. Vui lòng quản lý tại mục Quản lý tài khoản!", ButtonStyle::Danger, "Đóng", "");
                                break;
                            }
                            std::string curSt = staffUserList[actualIdx].trangThaiLamViec;
                            staffUserList[actualIdx].trangThaiLamViec = (curSt == "DangLam") ? "DaNghiViec" : "DangLam";
                            saveStaffUsersToDatabase();
                            syncStaffSelectionToForm(actualIdx);
                            break;
                        }

                        // Row click selection
                        if (mousePos.x >= leftColX && mousePos.x <= leftColX + leftColW &&
                            mousePos.y >= ry && mousePos.y <= ry + rowH) {
                            syncStaffSelectionToForm(actualIdx);
                            break;
                        }
                    }

                    // Numbered Pagination clicks at footY = 868.f
                    float footY = 868.f;
                    float btnSz = 28.f;
                    float btnGap = 6.f;
                    float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
                    float pgStartX = leftColX + leftColW - 18.f - totalPagingW;

                    // Prev '<'
                    if (mousePos.x >= pgStartX && mousePos.x <= pgStartX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (staffCurrentPage > 0) staffCurrentPage--;
                    }
                    // Page numbers
                    for (int p = 0; p < totalPages; ++p) {
                        float px = pgStartX + (p + 1) * (btnSz + btnGap);
                        if (mousePos.x >= px && mousePos.x <= px + btnSz &&
                            mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            staffCurrentPage = p;
                        }
                    }
                    // Next '>'
                    float nextX = pgStartX + (totalPages + 1) * (btnSz + btnGap);
                    if (mousePos.x >= nextX && mousePos.x <= nextX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (staffCurrentPage < totalPages - 1) staffCurrentPage++;
                    }

                    // Checkbox "Đã nghỉ làm" in Right Form at rightColX = 1200, y = 664
                    float formRightColX = 1200.f;
                    if (mousePos.x >= formRightColX + 100.f && mousePos.x <= formRightColX + 260.f &&
                        mousePos.y >= 655.f && mousePos.y <= 695.f) {
                        if (selectedStaffRowIdx >= 0 && selectedStaffRowIdx < static_cast<int>(staffUserList.size()) &&
                            staffUserList[selectedStaffRowIdx].vaiTro == "Admin") {
                            if (confirmPopup) confirmPopup->show("CẢNH BÁO", "Không thể chọn nghỉ làm với Admin để đảm bảo hệ thống luôn có ít nhất một Admin đang hoạt động. Vui lòng quản lý tại mục Quản lý tài khoản!", ButtonStyle::Danger, "Đóng", "");
                        } else {
                            staffFormResigned = !staffFormResigned;
                        }
                    }
                }
                break;
            }

            case ActiveTab::Accounts: {
                handleBtn(btnAccountExport);
                handleBtn(btnAccountNew);
                handleBtn(btnAccountFilterAll);
                handleBtn(btnAccountFilterActive);
                handleBtn(btnAccountFilterLocked);
                handleBtn(btnAccountSave);
                handleBtn(btnAccountToggleLock);
                handleBtn(btnAccountRefresh);
                handleBtn(btnAccountClear);
                handleBtn(btnAccountPrevPage);
                handleBtn(btnAccountNextPage);

                if (accountRoleDropdown) {
                    if (event.type == sf::Event::MouseMoved) accountRoleDropdown->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) accountRoleDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    else if (event.type == sf::Event::MouseButtonReleased) accountRoleDropdown->handleMouseButtonReleased(event.mouseButton.button, mousePos);
                }

                if (event.type == sf::Event::KeyPressed && event.key.control && event.key.code == sf::Keyboard::K) {
                    if (accountSearchInput) accountSearchInput->setFocused(true);
                }

                auto handleAccountInput = [&](std::unique_ptr<UIInputBox>& inp) {
                    if (!inp) return;
                    if (event.type == sf::Event::MouseButtonPressed) {
                        inp->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::TextEntered) {
                        std::string prev = inp->getText();
                        inp->handleTextEntered(event.text.unicode);
                        if (inp.get() == accountSearchInput.get() && inp->getText() != prev) {
                            accountCurrentPage = 0;
                        }
                    }
                };

                handleAccountInput(accountSearchInput);
                handleAccountInput(inputAccountEmployeeId);
                // inputAccountFullName is read-only
                handleAccountInput(inputAccountUsername);
                handleAccountInput(inputAccountPassword);

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float leftColW = 910.f;
                    float rowStartY = 378.f;
                    float rowH = 74.f;

                    std::vector<int> filteredIndices = getFilteredAccountIndices();
                    int totalFiltered = static_cast<int>(filteredIndices.size());
                    int rowsPerPage = 5;
                    int totalPages = std::max(1, (totalFiltered + rowsPerPage - 1) / rowsPerPage);
                    if (accountCurrentPage >= totalPages) accountCurrentPage = totalPages - 1;
                    if (accountCurrentPage < 0) accountCurrentPage = 0;

                    int startIdx = accountCurrentPage * rowsPerPage;
                    int endIdx = std::min(startIdx + rowsPerPage, totalFiltered);
                    int shownCount = std::max(0, endIdx - startIdx);

                    // Table rows selection & status/action clicks
                    for (int r = 0; r < shownCount; ++r) {
                        int actualIdx = filteredIndices[startIdx + r];
                        float ry = rowStartY + r * rowH;

                        // Click on Edit/Pencil icon (aColX = leftColX + 834.f, ry + 22.f, 28x28)
                        if (mousePos.x >= leftColX + 830.f && mousePos.x <= leftColX + 865.f &&
                            mousePos.y >= ry + 18.f && mousePos.y <= ry + 56.f) {
                            syncAccountSelectionToForm(actualIdx);
                            break;
                        }

                        // Click on Status Pill (leftColX + 680.f .. leftColX + 790.f, ry + 18.f .. ry + 56.f) -> toggle lock
                        if (mousePos.x >= leftColX + 680.f && mousePos.x <= leftColX + 790.f &&
                            mousePos.y >= ry + 18.f && mousePos.y <= ry + 56.f) {
                            std::string curSt = staffUserList[actualIdx].trangThaiTaiKhoan;
                            staffUserList[actualIdx].trangThaiTaiKhoan = (curSt == "KichHoat") ? "BiKhoa" : "KichHoat";
                            saveStaffUsersToDatabase();
                            syncAccountSelectionToForm(actualIdx);
                            break;
                        }

                        // Row click selection
                        if (mousePos.x >= leftColX && mousePos.x <= leftColX + leftColW &&
                            mousePos.y >= ry && mousePos.y <= ry + rowH) {
                            syncAccountSelectionToForm(actualIdx);
                            break;
                        }
                    }

                    // Numbered Pagination clicks at footY = 868.f
                    float footY = 868.f;
                    float btnSz = 28.f;
                    float btnGap = 6.f;
                    float totalPagingW = (totalPages + 2) * btnSz + (totalPages + 1) * btnGap;
                    float pgStartX = leftColX + leftColW - 18.f - totalPagingW;

                    // Prev '<'
                    if (mousePos.x >= pgStartX && mousePos.x <= pgStartX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (accountCurrentPage > 0) accountCurrentPage--;
                    }
                    // Page numbers
                    for (int p = 0; p < totalPages; ++p) {
                        float px = pgStartX + (p + 1) * (btnSz + btnGap);
                        if (mousePos.x >= px && mousePos.x <= px + btnSz &&
                            mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                            accountCurrentPage = p;
                        }
                    }
                    // Next '>'
                    float nextX = pgStartX + (totalPages + 1) * (btnSz + btnGap);
                    if (mousePos.x >= nextX && mousePos.x <= nextX + btnSz &&
                        mousePos.y >= footY && mousePos.y <= footY + btnSz) {
                        if (accountCurrentPage < totalPages - 1) accountCurrentPage++;
                    }

                    // Right Form interactions
                    float rightColX = sidebarWidth + 950.f;
                    // Eye icon toggle in Password Box (rightColX + 484 - 52, y = 530..570)
                    if (mousePos.x >= rightColX + 484.f - 52.f && mousePos.x <= rightColX + 484.f - 24.f &&
                        mousePos.y >= 530.f && mousePos.y <= 570.f) {
                        accountShowPassword = !accountShowPassword;
                    }

                    // "Tạo mật khẩu ngẫu nhiên" link (rightColX + 300 .. rightColX + 470, y = 502 .. 526)
                    if (mousePos.x >= rightColX + 300.f && mousePos.x <= rightColX + 470.f &&
                        mousePos.y >= 502.f && mousePos.y <= 526.f) {
                        const char chars[] = "abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789!@#$%";
                        std::string rndPass = "";
                        for (int i = 0; i < 10; ++i) rndPass += chars[rand() % (sizeof(chars) - 1)];
                        if (inputAccountPassword) inputAccountPassword->setText(rndPass);
                    }
                }
                break;
            }

            case ActiveTab::SystemLogs:
                handleBtn(btnRefreshLogs);
                handleBtn(btnExportLogs);
                handleBtn(btnAuditFilterAll);
                handleBtn(btnAuditFilterInsert);
                handleBtn(btnAuditFilterUpdate);
                handleBtn(btnAuditFilterDelete);

                if (auditSearchInput) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        auditSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                    } else if (event.type == sf::Event::TextEntered) {
                        auditSearchInput->handleTextEntered(event.text.unicode);
                        auditCurrentPage = 0;
                    }
                }

                if (auditDateDropdown) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        bool wasExp = auditDateDropdown->getExpanded();
                        auditDateDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (!wasExp && auditDateDropdown->getExpanded() && auditTableDropdown) {
                            auditTableDropdown->setExpanded(false);
                        }
                    } else if (event.type == sf::Event::MouseMoved) {
                        auditDateDropdown->handleMouseMove(mousePos);
                    } else if (event.type == sf::Event::MouseWheelScrolled) {
                        auditDateDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                }

                if (auditTableDropdown) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        bool wasExp = auditTableDropdown->getExpanded();
                        auditTableDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        if (!wasExp && auditTableDropdown->getExpanded() && auditDateDropdown) {
                            auditDateDropdown->setExpanded(false);
                        }
                    } else if (event.type == sf::Event::MouseMoved) {
                        auditTableDropdown->handleMouseMove(mousePos);
                    } else if (event.type == sf::Event::MouseWheelScrolled) {
                        auditTableDropdown->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    }
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float leftColX = sidebarWidth + 24.f;
                    float cardW = 1420.f;
                    float tableY = 262.f;
                    float tableH = 652.f;
                    float footRowY = tableY + tableH - 42.f;
                    float pgBtnSz = 28.f;
                    float pgGap = 6.f;

                    std::string q = auditSearchInput ? auditSearchInput->getText() : "";
                    std::string qNorm = removeVietnameseDiacritics(q);
                    std::transform(qNorm.begin(), qNorm.end(), qNorm.begin(), ::tolower);
                    int filteredCount = 0;
                    for (const auto& r : auditRecords) {
                        if (auditFilterType == 1 && r.actionType != 1) continue;
                        if (auditFilterType == 2 && r.actionType != 2) continue;
                        if (auditFilterType == 3 && r.actionType != 3) continue;
                        if (!auditDateFilterVal.empty() && r.thoiGian.rfind(auditDateFilterVal, 0) != 0) continue;
                        if (!auditTableFilterVal.empty() && r.tableName != auditTableFilterVal) continue;
                        if (!qNorm.empty()) {
                            std::string hay = r.thoiGian + " " + r.maNguoiDung + " " + r.hoTen + " " + r.vaiTro + " " +
                                              r.tableName + " " + r.fieldChanged + " " + r.oldData + " " + r.newData + " " + r.chiTiet;
                            std::string hayNorm = removeVietnameseDiacritics(hay);
                            std::transform(hayNorm.begin(), hayNorm.end(), hayNorm.begin(), ::tolower);
                            if (hayNorm.find(qNorm) == std::string::npos) continue;
                        }
                        filteredCount++;
                    }
                    int totalPages = std::max(1, (filteredCount + 11) / 12);
                    float totalPgW = (std::min(5, totalPages) + 2) * (pgBtnSz + pgGap);
                    float pgStartX = leftColX + cardW - 24.f - totalPgW;

                    if (mousePos.y >= footRowY && mousePos.y <= footRowY + pgBtnSz) {
                        if (mousePos.x >= pgStartX && mousePos.x <= pgStartX + pgBtnSz) {
                            if (auditCurrentPage > 0) auditCurrentPage--;
                        }
                        int startP = std::max(0, auditCurrentPage - 2);
                        int endP = std::min(totalPages, startP + 5);
                        if (endP - startP < 5) startP = std::max(0, endP - 5);
                        for (int p = startP; p < endP; ++p) {
                            float bx = pgStartX + (p - startP + 1) * (pgBtnSz + pgGap);
                            if (mousePos.x >= bx && mousePos.x <= bx + pgBtnSz) {
                                auditCurrentPage = p;
                            }
                        }
                        float nextBx = pgStartX + (endP - startP + 1) * (pgBtnSz + pgGap);
                        if (mousePos.x >= nextBx && mousePos.x <= nextBx + pgBtnSz) {
                            if (auditCurrentPage < totalPages - 1) auditCurrentPage++;
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}

void CinemaUIApp::update(float dt) {
    if (!isLoggedIn) {
        if (loginInputUser) loginInputUser->update(dt);
        if (loginInputPass) loginInputPass->update(dt);
        return;
    }

    if (searchBar) searchBar->update(dt);
    if (inputCustomerPhone) inputCustomerPhone->update(dt);

    if (currentTab == ActiveTab::SeatBooking) {
        if (inputPosMemberPhone) inputPosMemberPhone->update(dt);
        if (inputPosVoucher) inputPosVoucher->update(dt);
        if (inputPosCashGiven) inputPosCashGiven->update(dt);
    }
    if (currentTab == ActiveTab::CRM) {
        if (crmSearchInput) crmSearchInput->update(dt);
        if (crmDateRangeInput) crmDateRangeInput->update(dt);
        if (inputCrmName) inputCrmName->update(dt);
        if (inputCrmPhone) inputCrmPhone->update(dt);
        if (inputCrmEmail) inputCrmEmail->update(dt);
    }

    if (currentTab == ActiveTab::Dashboard && dashboardSlider) {
        dashboardSlider->update(dt);
    }

    if (currentTab == ActiveTab::Movies) {
        if (movieSearchInput) movieSearchInput->update(dt);
        if (inputMovieId) inputMovieId->update(dt);
        if (inputMovieTitle) inputMovieTitle->update(dt);
        if (inputMovieDuration) inputMovieDuration->update(dt);
        if (inputMovieReleaseDate) inputMovieReleaseDate->update(dt);
        if (inputMovieEndDate) inputMovieEndDate->update(dt);
        if (inputMovieAgeRating) inputMovieAgeRating->update(dt);
        if (inputMovieGenre) inputMovieGenre->update(dt);
        if (inputMoviePoster) inputMoviePoster->update(dt);
        if (inputMovieDesc) inputMovieDesc->update(dt);
        if (inputGenreId) inputGenreId->update(dt);
        if (inputGenreName) inputGenreName->update(dt);
        if (genreSearchInput) genreSearchInput->update(dt);
        if (inputNewAgeRating) inputNewAgeRating->update(dt);
    }

    if (currentTab == ActiveTab::Concessions) {
        if (fnbSearchInput) fnbSearchInput->update(dt);
        if (inputFnbId) inputFnbId->update(dt);
        if (inputFnbName) inputFnbName->update(dt);
        if (inputFnbPrice) inputFnbPrice->update(dt);
        if (inputFnbNote) inputFnbNote->update(dt);
        if (inputFnbImage) inputFnbImage->update(dt);
    }

    if (currentTab == ActiveTab::Rooms) {
        if (roomSearchInput) roomSearchInput->update(dt);
        if (inputRoomName) inputRoomName->update(dt);
        if (inputRoomCapacity) inputRoomCapacity->update(dt);
    }
    if (currentTab == ActiveTab::Seats) {
        if (seatSearchInput) seatSearchInput->update(dt);
        if (inputSeatBatchRows) inputSeatBatchRows->update(dt);
        if (inputSeatBatchCols) inputSeatBatchCols->update(dt);
        if (seatTypeSearchInput) seatTypeSearchInput->update(dt);
        if (inputSeatTypeName) inputSeatTypeName->update(dt);
        if (inputSeatTypePrice) inputSeatTypePrice->update(dt);
        if (inputSeatTypeDesc) inputSeatTypeDesc->update(dt);
    }
    if (currentTab == ActiveTab::Showtimes) {
        if (stSearchInput) stSearchInput->update(dt);
        if (inputStDate) inputStDate->update(dt);
        if (inputStTime) inputStTime->update(dt);
    }
    if (currentTab == ActiveTab::Concessions && fnbFormLoai == "Combo") {
        if (comboItem1Dropdown) comboItem1Dropdown->drawPopup(window);
        if (comboItem2Dropdown) comboItem2Dropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Promotions) {
        if (promoSearchInput) promoSearchInput->update(dt);
        if (inputPromoId) inputPromoId->update(dt);
        if (inputPromoName) inputPromoName->update(dt);
        if (inputPromoCode) inputPromoCode->update(dt);
        if (inputPromoPercent) inputPromoPercent->update(dt);
        if (inputPromoMaxDiscount) inputPromoMaxDiscount->update(dt);
        if (inputPromoStartDate) inputPromoStartDate->update(dt);
        if (inputPromoEndDate) inputPromoEndDate->update(dt);
    }
    if (currentTab == ActiveTab::Staff) {
        if (staffSearchInput) staffSearchInput->update(dt);
        if (inputStaffId) inputStaffId->update(dt);
        if (inputStaffName) inputStaffName->update(dt);
        if (inputStaffPhone) inputStaffPhone->update(dt);
        if (inputStaffBirthDate) inputStaffBirthDate->update(dt);
        if (inputStaffCCCD) inputStaffCCCD->update(dt);
        if (inputStaffEmail) inputStaffEmail->update(dt);
    }
    if (currentTab == ActiveTab::Accounts) {
        if (accountSearchInput) accountSearchInput->update(dt);
        if (inputAccountEmployeeId) inputAccountEmployeeId->update(dt);
        if (inputAccountFullName) inputAccountFullName->update(dt);
        if (inputAccountUsername) inputAccountUsername->update(dt);
        if (inputAccountPassword) inputAccountPassword->update(dt);
    }
}

void CinemaUIApp::render() {
    window.clear(Theme::BgDark);

    // If not logged in, render the Login Screen
    if (!isLoggedIn) {
        renderLoginView();
        window.display();
        return;
    }

    // Render Active View Content
    switch (currentTab) {
        case ActiveTab::SeatBooking:
            renderSeatBookingView();
            break;
        case ActiveTab::CRM:
            renderCRMView();
            break;
        case ActiveTab::Reports:
            renderReportsView();
            break;
        case ActiveTab::Rooms:
            renderRoomsView();
            break;
        case ActiveTab::Seats:
            renderSeatsView();
            break;
        case ActiveTab::Showtimes:
            renderShowtimesView();
            break;
        case ActiveTab::Movies:
            renderMoviesView();
            break;
        case ActiveTab::Concessions:
            renderConcessionsView();
            break;
        case ActiveTab::ShiftReport:
            renderShiftReportView();
            break;
        case ActiveTab::Promotions:
            renderPromotionsView();
            break;
        case ActiveTab::Staff:
            renderStaffView();
            break;
        case ActiveTab::Accounts:
            renderAccountsView();
            break;
        case ActiveTab::SystemLogs:
            renderLogsView();
            break;
        case ActiveTab::Dashboard:
            renderDashboardView();
            break;
    }

    // Render Fixed Chrome (Top Bar & Sidebar)
    renderSidebar();
    if (currentTab != ActiveTab::SeatBooking && currentTab != ActiveTab::CRM && currentTab != ActiveTab::ShiftReport && currentTab != ActiveTab::Movies && currentTab != ActiveTab::Concessions && currentTab != ActiveTab::Showtimes && currentTab != ActiveTab::Rooms && currentTab != ActiveTab::Seats && currentTab != ActiveTab::Promotions && currentTab != ActiveTab::Staff && currentTab != ActiveTab::Accounts && currentTab != ActiveTab::Reports && currentTab != ActiveTab::SystemLogs) {
        renderTopBar();
    }

    // Render Floating Dropdowns on top
    if (currentTab == ActiveTab::SeatBooking) {
        if (showtimeDropdown) showtimeDropdown->drawPopup(window);
        if (posDateDropdown) posDateDropdown->drawPopup(window);
        if (posMovieDropdown) posMovieDropdown->drawPopup(window);
        if (posShowtimeDropdown) posShowtimeDropdown->drawPopup(window);
        if (posFnbDropdown) posFnbDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::CRM && crmTierDropdown) {
        crmTierDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Movies) {
        if (movieAgeFilterDropdown) movieAgeFilterDropdown->drawPopup(window);
        if (movieFormAgeDropdown) movieFormAgeDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Showtimes) {
        if (stMovieFilterDropdown) stMovieFilterDropdown->drawPopup(window);
        if (stDateDropdown) stDateDropdown->drawPopup(window);
        if (stRoomDropdown) stRoomDropdown->drawPopup(window);
        if (stMovieDropdown) stMovieDropdown->drawPopup(window);
        if (stRoomFormDropdown) stRoomFormDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Concessions && fnbFormLoai == "Combo") {
        if (comboItem1Dropdown) comboItem1Dropdown->drawPopup(window);
        if (comboItem2Dropdown) comboItem2Dropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Promotions) {
        if (promoScopeDropdown) promoScopeDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Staff) {
        if (staffGenderDropdown) staffGenderDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Accounts) {
        if (accountRoleDropdown) accountRoleDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Rooms) {
        if (roomStatusDropdown) roomStatusDropdown->drawPopup(window);
        if (roomFilterDropdown) roomFilterDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Seats) {
        if (seatSubTab == 0) {
            if (seatBatchTypeDropdown) seatBatchTypeDropdown->drawPopup(window);
            if (seatRoomDropdown) seatRoomDropdown->drawPopup(window);
        } else if (seatSubTab == 1) {
            if (seatTypeFilterDropdown) seatTypeFilterDropdown->drawPopup(window);
        }
    }
    if (currentTab == ActiveTab::SystemLogs) {
        if (auditDateDropdown) auditDateDropdown->drawPopup(window);
        if (auditTableDropdown) auditTableDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::Reports) {
        if (reportShiftDropdown) reportShiftDropdown->drawPopup(window);
        if (reportCashierDropdown) reportCashierDropdown->drawPopup(window);
        if (reportPaymentDropdown) reportPaymentDropdown->drawPopup(window);
    }
    if (currentTab == ActiveTab::ShiftReport && (currentUserRole == "Admin" || currentUserRole == "Khách") && !adminShowHistorySubView) {
        if (adminNextCashierDropdown) adminNextCashierDropdown->drawPopup(window);
    }

    // Modal dialog for adding custom age limit
    if (showAgeInputDialog) {
        sf::RectangleShape backdrop(sf::Vector2f(1728.f, 972.f));
        backdrop.setFillColor(sf::Color(15, 23, 42, 140));
        window.draw(backdrop);

        float modalW = 440.f;
        float modalH = 220.f;
        float modalX = (1728.f - modalW) / 2.f;
        float modalY = (972.f - modalH) / 2.f;

        RoundedRectRenderer::render(window, sf::Vector2f(modalX, modalY), sf::Vector2f(modalW, modalH), 12.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));

        sf::Text mTitle;
        mTitle.setFont(Theme::FontPrimary);
        mTitle.setCharacterSize(17);
        mTitle.setStyle(sf::Text::Bold);
        mTitle.setFillColor(sf::Color(15, 23, 42));
        mTitle.setString(toSfString("THÊM GIỚI HẠN ĐỘ TUỔI MỚI"));
        mTitle.setPosition(modalX + 30.f, modalY + 22.f);
        window.draw(mTitle);

        sf::Text mSub;
        mSub.setFont(Theme::FontPrimary);
        mSub.setCharacterSize(13);
        mSub.setFillColor(sf::Color(100, 116, 139));
        mSub.setString(toSfString("Nhập mã độ tuổi mới để thêm vào danh sách lựa chọn:"));
        mSub.setPosition(modalX + 30.f, modalY + 52.f);
        window.draw(mSub);

        if (inputNewAgeRating) inputNewAgeRating->draw(window);
        if (btnCancelNewAge) btnCancelNewAge->draw(window);
        if (btnSaveNewAge) btnSaveNewAge->draw(window);
    }

    // Modal dialog on topmost layer
    if (confirmPopup) {
        confirmPopup->draw(window);
    }

    window.display();
}


void CinemaUIApp::renderSidebar() {
    // Sidebar background
    sf::RectangleShape sidebar(sf::Vector2f(sidebarWidth, 972.f));
    sidebar.setFillColor(Theme::SidebarBg);
    window.draw(sidebar);

    // Sidebar right border divider
    sf::RectangleShape div(sf::Vector2f(1.5f, 972.f));
    div.setPosition(sidebarWidth, 0.f);
    div.setFillColor(Theme::CardBorder);
    window.draw(div);

    // Logo & Brand Header
    sf::Text logoText;
    logoText.setFont(Theme::FontCinema);
    logoText.setCharacterSize(22);
    logoText.setStyle(sf::Text::Bold);
    logoText.setFillColor(sf::Color(6, 182, 212)); // Cyan branding
    logoText.setString(toSfString("CINESTAR"));
    logoText.setPosition(22.f, 14.f);
    window.draw(logoText);

    // Pill "CineOS v4.2 PRO"
    RoundedRectRenderer::render(window, sf::Vector2f(130.f, 17.f), sf::Vector2f(96.f, 18.f), 9.f, sf::Color(14, 165, 233, 40));
    sf::Text osBadge;
    osBadge.setFont(Theme::FontPrimary);
    osBadge.setCharacterSize(10);
    osBadge.setStyle(sf::Text::Bold);
    osBadge.setFillColor(sf::Color(14, 165, 233));
    osBadge.setString("CineOS v4.2 PRO");
    osBadge.setPosition(136.f, 19.f);
    window.draw(osBadge);

    sf::Text subLogo;
    subLogo.setFont(Theme::FontPrimary);
    subLogo.setCharacterSize(10);
    subLogo.setFillColor(Theme::TextSecondary);
    subLogo.setString(toSfString("Hệ thống Bán Vé & Rạp Chiếu Phim Cinestar Cinemas Complex"));
    subLogo.setPosition(22.f, 42.f);
    window.draw(subLogo);

    // Menu Items - Admin excludes SeatBooking (POS) and CRM as requested
    std::vector<std::pair<std::string, ActiveTab>> menuItems;
    bool isStaff = (currentUserRole != "Admin" && currentUserRole != "Khách");
    if (!isStaff) {
        menuItems = {
            {"Quản lý Doanh thu",    ActiveTab::Reports},
            {"Quản lý phòng",        ActiveTab::Rooms},
            {"Quản lý ghế",          ActiveTab::Seats},
            {"Quản lý suất chiếu",   ActiveTab::Showtimes},
            {"Phim & Thể loại",      ActiveTab::Movies},
            {"Quản lý F&B",          ActiveTab::Concessions},
            {"Quản lý Báo cáo ca",   ActiveTab::ShiftReport},
            {"Khuyến mãi SK",        ActiveTab::Promotions},
            {"Quản lý nhân sự",      ActiveTab::Staff},
            {"Quản lý tài khoản",    ActiveTab::Accounts},
            {"Nhật ký hoạt động",    ActiveTab::SystemLogs}
        };
    } else {
        menuItems = {
            {"Bán vé (POS)",         ActiveTab::SeatBooking},
            {"Khách hàng (CRM)",     ActiveTab::CRM},
            {"Chốt ca & Báo cáo",    ActiveTab::ShiftReport}
        };

        sf::Text secHeader;
        secHeader.setFont(Theme::FontPrimary);
        secHeader.setCharacterSize(11);
        secHeader.setStyle(sf::Text::Bold);
        secHeader.setFillColor(sf::Color(148, 163, 184));
        secHeader.setString(toSfString("MENU NGHIỆP VỤ"));
        secHeader.setPosition(22.f, 68.f);
        window.draw(secHeader);
    }

    float startY = isStaff ? 92.f : 72.f;
    float itemH = 39.f;

    for (size_t i = 0; i < menuItems.size(); ++i) {
        float itemY = startY + static_cast<float>(i) * itemH;
        bool isActive = (currentTab == menuItems[i].second);
        bool isHovered = (i < navButtons.size() && navButtons[i]->getState() == ButtonState::Hovered);

        if (isActive) {
            sf::RectangleShape activeBg(sf::Vector2f(sidebarWidth, itemH));
            activeBg.setPosition(0.f, itemY);
            activeBg.setFillColor(sf::Color(6, 182, 212, 25));
            window.draw(activeBg);

            sf::RectangleShape activeBar(sf::Vector2f(3.5f, itemH));
            activeBar.setPosition(0.f, itemY);
            activeBar.setFillColor(sf::Color(6, 182, 212));
            window.draw(activeBar);
        } else if (isHovered) {
            sf::RectangleShape hoverBg(sf::Vector2f(sidebarWidth, itemH));
            hoverBg.setPosition(0.f, itemY);
            hoverBg.setFillColor(sf::Color(255, 255, 255, 12));
            window.draw(hoverBg);
        }

        sf::Color itemColor = isActive ? sf::Color(6, 182, 212) :
                              (isHovered ? sf::Color(241, 245, 249) : sf::Color(203, 213, 225));

        // Draw Icon according to target tab
        drawSidebarMenuIcon(window, static_cast<int>(menuItems[i].second), sf::Vector2f(20.f, itemY + 11.f), itemColor);

        // Draw Label
        sf::Text label;
        label.setFont(Theme::FontPrimary);
        label.setCharacterSize(13);
        if (isActive) label.setStyle(sf::Text::Bold);
        label.setFillColor(itemColor);
        label.setString(toSfString(menuItems[i].first));
        label.setPosition(46.f, itemY + 9.f);
        window.draw(label);

        // Badge [KIOSK] for POS Mode
        if (menuItems[i].second == ActiveTab::SeatBooking) {
            float badgeX = 175.f;
            float badgeY = itemY + 10.f;
            RoundedRectRenderer::render(window, sf::Vector2f(badgeX, badgeY), sf::Vector2f(50.f, 20.f), 10.f, sf::Color(14, 165, 233));
            sf::Text bText;
            bText.setFont(Theme::FontPrimary);
            bText.setCharacterSize(10);
            bText.setStyle(sf::Text::Bold);
            bText.setFillColor(sf::Color::White);
            bText.setString("KIOSK");
            bText.setPosition(badgeX + 9.f, badgeY + 3.f);
            window.draw(bText);
        }
    }

    if (isStaff) {
        float infoY = 805.f;
        sf::Text txtShift;
        txtShift.setFont(Theme::FontPrimary);
        txtShift.setCharacterSize(11);
        txtShift.setFillColor(sf::Color(148, 163, 184));
        txtShift.setString(toSfString("Ca hiện tại: • Ca Chiếu (14h-22h)"));
        txtShift.setPosition(22.f, infoY);
        window.draw(txtShift);

        sf::Text txtKiosk;
        txtKiosk.setFont(Theme::FontPrimary);
        txtKiosk.setCharacterSize(11);
        txtKiosk.setFillColor(sf::Color(148, 163, 184));
        txtKiosk.setString(toSfString("Quầy Kiosk: POS-01 (Sảnh A)"));
        txtKiosk.setPosition(22.f, infoY + 20.f);
        window.draw(txtKiosk);
    }

    // Bottom User Profile Card (STRICTLY ROLE ONLY - NO "Le Tuyet Nhi")
    float profY = 855.f;
    float profH = 96.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(14.f, profY),
        sf::Vector2f(sidebarWidth - 28.f, profH),
        Theme::RadiusMedium,
        Theme::CardBg,
        1.0f,
        Theme::CardBorder
    );

    // Avatar Circle
    sf::CircleShape avatar(17.f);
    avatar.setPosition(26.f, profY + 12.f);
    avatar.setFillColor((currentUserRole == "Admin") ? sf::Color(99, 102, 241) : Theme::Cyan);
    window.draw(avatar);

    sf::Text avText;
    avText.setFont(Theme::FontPrimary);
    avText.setCharacterSize(12);
    avText.setStyle(sf::Text::Bold);
    avText.setFillColor(Theme::TextPrimary);
    avText.setString((currentUserRole == "Admin") ? "AD" : "ST");
    sf::FloatRect avb = avText.getLocalBounds();
    avText.setPosition(26.f + (34.f - avb.width) / 2.f - avb.left, profY + 12.f + (34.f - avb.height) / 2.f - avb.top);
    window.draw(avText);

    // Role Display - NO PERSONAL NAME "Le Tuyet Nhi" - ONLY ROLE AS REQUESTED
    sf::Text uRole;
    uRole.setFont(Theme::FontPrimary);
    uRole.setCharacterSize(14);
    uRole.setStyle(sf::Text::Bold);
    uRole.setFillColor(sf::Color(248, 250, 252));
    uRole.setString(toSfString(currentUserRole == "Admin" ? "Admin" : "Nhân Viên"));
    uRole.setPosition(68.f, profY + 11.f);
    window.draw(uRole);

    sf::Text uSub;
    uSub.setFont(Theme::FontPrimary);
    uSub.setCharacterSize(11);
    uSub.setFillColor(Theme::Success);
    uSub.setString(toSfString(currentUserRole == "Admin" ? "Quản Trị Hệ Thống" : "Nhân Sự Vận Hành"));
    uSub.setPosition(68.f, profY + 31.f);
    window.draw(uSub);

    // Logout button
    if (btnLogout) {
        btnLogout->setPosition(sf::Vector2f(26.f, profY + 56.f));
        btnLogout->setSize(sf::Vector2f(sidebarWidth - 52.f, 26.f));
        btnLogout->draw(window);
    }
}

void CinemaUIApp::renderTopBar() {
    float topW = 1728.f - sidebarWidth;
    sf::RectangleShape topBar(sf::Vector2f(topW, topbarHeight));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(Theme::BgDark);
    window.draw(topBar);

    sf::RectangleShape div(sf::Vector2f(topW, 1.5f));
    div.setPosition(sidebarWidth, topbarHeight);
    div.setFillColor(Theme::CardBorder);
    window.draw(div);

    if (searchBar) {
        searchBar->draw(window);
    }

    // Top Right Info & Live Time
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S  |  %d/%m/%Y", localTime);

    sf::Text timeText;
    timeText.setFont(Theme::FontPrimary);
    timeText.setCharacterSize(Theme::SizeBody);
    timeText.setFillColor(Theme::Cyan);
    timeText.setString(timeBuf);
    timeText.setPosition(1728.f - 240.f, (topbarHeight - Theme::SizeBody) / 2.0f);
    window.draw(timeText);
}


} // namespace UI
