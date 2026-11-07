#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "../../include/UI/CinemaUIApp.h"
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

namespace UI {

CinemaUIApp::CinemaUIApp()
    : isLoggedIn(false),
      currentUserId("U001"),
      currentUserName("Chưa đăng nhập"),
      currentUserRole("Admin"),
      loginErrorMsg(""),
      loginBackgroundScale(1.f),
      loginBackgroundOffset(0.f, 0.f),
      currentTab(ActiveTab::Reports),
      sidebarWidth(250.f),
      topbarHeight(68.f),
      bookingTotalPrice(0.0),
      posSeatTotal(0.0),
      posFnbTotal(0.0),
      posGrandTotal(0.0),
      posSelectedMovieIdx(0),
      posSelectedShowtimeIdx(0),
      posShowSeatMap(false),
      movieSubTab(0),
      selectedMovieRowIdx(-1),
      selectedGenreRowIdx(-1),
      selectedGenreChoiceIdx(0),
      movieAgeFilter(""),
      moviePosActive(true)
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

    if (!loginBackgroundTexture.loadFromFile("assets/login_clean_bg.jpg") &&
        !loginBackgroundTexture.loadFromFile("assets/login_bg.jpg") &&
        !loginBackgroundTexture.loadFromFile("assets/login_reference.jpg")) {
        std::cerr << "Không thể mở ảnh nền đăng nhập!\n";
    }
    if (cinemaLogoTexture.loadFromFile("assets/cinema_logo.png")) {
        cinemaLogoTexture.setSmooth(true);
        cinemaLogoSprite.setTexture(cinemaLogoTexture);
    }
    sf::Vector2u backgroundSize = loginBackgroundTexture.getSize();
    loginBackgroundScale = std::max(
        1728.f / static_cast<float>(backgroundSize.x),
        972.f / static_cast<float>(backgroundSize.y)
    );
    loginBackgroundOffset = sf::Vector2f(
        (1728.f - static_cast<float>(backgroundSize.x) * loginBackgroundScale) / 2.f,
        (972.f - static_cast<float>(backgroundSize.y) * loginBackgroundScale) / 2.f
    );
    loginBackgroundSprite.setTexture(loginBackgroundTexture);
    loginBackgroundTexture.setSmooth(true);
    loginBackgroundSprite.setScale(loginBackgroundScale, loginBackgroundScale);
    loginBackgroundSprite.setPosition(loginBackgroundOffset);

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
}

void CinemaUIApp::reloadMoviePosterTextures() {
    moviePosterTextures.clear();
    for (const auto& m : movieList) {
        std::string id = m.getMaPhim();
        std::string dbPath = m.getPoster();
        std::vector<std::string> candidates;
        if (!dbPath.empty() && dbPath != "assets/posters/poster_default.jpg") {
            candidates.push_back(dbPath);
        }
        candidates.push_back("assets/posters/" + id + ".jpg");
        candidates.push_back("assets/posters/" + id + ".png");
        candidates.push_back("assets/posters/" + id + ".jpeg");
        if (!dbPath.empty()) {
            candidates.push_back(dbPath);
        }

        for (const auto& path : candidates) {
            std::ifstream f(path.c_str(), std::ios::binary);
            if (!f.good()) continue;
            f.close();
            std::shared_ptr<sf::Texture> tex(new sf::Texture());
            if (tex->loadFromFile(path)) {
                tex->setSmooth(true);
                moviePosterTextures[id] = tex;
                break;
            }
        }
    }
}

void CinemaUIApp::setupLoginView() {
    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f
    float cardY = (972.f - cardH) / 2.0f;  // 206.f
    float leftW = 420.f;
    float rightW = 520.f;
    float rightX = cardX + leftW + (cardW - leftW - rightW) / 2.0f; // Centered horizontally in right panel (814.f)

    loginInputUser = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightX, cardY + 196.f),
        sf::Vector2f(rightW, 54.f),
        "Nhập mã nhân viên hoặc email...",
        16
    ));
    loginInputUser->setTextInset(48.f);
    loginInputUser->setCornerRadius(10.f);
    loginInputUser->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    loginInputPass = std::unique_ptr<PasswordField>(new PasswordField(
        sf::Vector2f(rightX, cardY + 300.f),
        sf::Vector2f(rightW, 54.f),
        "••••••••••••",
        16
    ));
    loginInputPass->setTextInset(48.f);
    loginInputPass->setCornerRadius(10.f);
    loginInputPass->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    chkRememberMe = std::unique_ptr<UICheckboxItem>(new UICheckboxItem(
        "Ghi nhớ đăng nhập",
        sf::Vector2f(rightX, cardY + 374.f),
        true,
        15
    ));
    chkRememberMe->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(71, 85, 105)
    );

    btnLogin = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "ĐĂNG NHẬP HỆ THỐNG  →",
        sf::Vector2f(rightX, cardY + 416.f),
        sf::Vector2f(rightW, 54.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnLogin->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(203, 213, 225)
    );
    btnLogin->setFont(Theme::FontPrimary);
    btnLogin->setFontSize(17);

    // Handle button actions
    btnLogin->setOnClick([this]() {
        std::string u = loginInputUser->getText();
        std::string p = loginInputPass->getText();
        authenticateUser(u, p);
    });

    loginInputPass->setOnEnter([this](const std::string& p) {
        std::string u = loginInputUser->getText();
        authenticateUser(u, p);
    });
}

bool CinemaUIApp::authenticateUser(const std::string& inputUser, const std::string& inputPass) {
    if (inputUser.empty() || inputPass.empty()) {
        loginErrorMsg = "Vui lòng nhập đầy đủ tài khoản và mật khẩu!";
        return false;
    }

    // Read real users from SQLite DATA/cinema.db (users table)
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) != SQLITE_OK || database == nullptr) {
        if (database != nullptr) sqlite3_close(database);
        loginErrorMsg = "Không thể mở cơ sở dữ liệu SQLite DATA/cinema.db!";
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT maNguoiDung,hoTen,email,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan "
        "FROM users WHERE (taiKhoan=? OR maNguoiDung=? OR email=?) AND matKhau=? ORDER BY rowid LIMIT 1";

    bool found = false;
    std::string statusErr = "";

    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, inputPass.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) == SQLITE_ROW) {
            auto getCol = [&](int idx) -> std::string {
                const unsigned char* txt = sqlite3_column_text(statement, idx);
                return txt ? reinterpret_cast<const char*>(txt) : "";
            };
            std::string uid = getCol(0);
            std::string uname = getCol(1);
            std::string role = getCol(5);
            std::string workStatus = getCol(6);
            std::string accStatus = getCol(7);

            if (role == "Staff" && accStatus == "BiKhoa") {
                statusErr = "Tài khoản nhân viên đã bị khóa!";
            } else if (role == "Staff" && workStatus == "DaNghiViec") {
                statusErr = "Nhân viên đã nghỉ việc, không thể đăng nhập!";
            } else {
                currentUserId = uid;
                currentUserName = uname;
                currentUserRole = role;
                found = true;
            }
        }
        sqlite3_finalize(statement);
    }
    sqlite3_close(database);

    if (found) {
        isLoggedIn = true;
        loginErrorMsg = "";
        loadDataFromDatabase();

        if (currentUserRole == "Admin") {
            currentTab = ActiveTab::Reports;
        } else {
            currentTab = ActiveTab::SeatBooking;
        }
        setupNavigation();

        // Record real activity log into SQLite DATA/cinema.db
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_NHAP", "Dang nhap thanh cong qua giao dien UI");
        setupLogsView(); // refresh log table
        return true;
    } else {
        loginErrorMsg = statusErr.empty() ? "Tài khoản hoặc mật khẩu không chính xác!" : statusErr;
        return false;
    }
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHECKIN_VE", "Soat ve thanh cong cho phong chieu");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_KY_CRM", "Dang ky hoi vien C-Friend thanh cong");
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
            Room r("R08", "Phong Chieu 8", "2D", 120, "HoatDong");
            roomList.push_back(r);
            db.ghiDanhSachPhong(roomList);
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHONG", "Them phong chieu moi: R08");
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
        confirmPopup->show(
            "TỰ ĐỘNG SINH SƠ ĐỒ GHẾ",
            "Tự động tính toán và tạo danh sách ghế (Standard, VIP, Sweetbox) theo phòng chiếu?",
            ButtonStyle::Primary,
            "Tạo Tự Động",
            "Hủy"
        );
        confirmPopup->setOnConfirm([this]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "TAO_GHE_TU_DONG", "Khoi tao so do ghe phong chieu thanh cong");
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
            Showtime sc("SC99", "P01", "R01", "21:00", "23:15", "10/11/2026", 85000.0);
            showtimeList.push_back(sc);
            db.ghiDanhSachSuatChieu(showtimeList);
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_SUAT_CHIEU", "Them suat chieu moi: SC99");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "QUAN_LY_THE_LOAI", "Cap nhat the loai phim thanh cong");
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
            FoodAndBeverage f("FB99", "Bap Rang Bo Pho Mai Dac Biet", "Bap", 75000.0, "DangKinhDoanh");
            fnbList.push_back(f);
            fnbQuantities.push_back(0);
            db.ghiDanhSachFAndB(fnbList);
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_FNB", "Them san pham F&B: FB99");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DUYET_BAO_CAO_CA", "Phe duyet bao cao ca truc thanh cong");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "CHOT_CA", "Chot ca lam viec va nop tien thanh cong");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_KHUYEN_MAI", "Tao chuong trinh khuyen mai moi thanh cong");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_NHAN_SU", "Tao nhan su moi thanh cong");
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
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DOI_MAT_KHAU", "Doi mat khau tai khoan thanh cong");
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
    setupSeatBookingView();
    setupConcessionsView();
    setupLogsView();
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
            {"Quản lý Doanh thu",    ActiveTab::Reports},
            {"Quản lý Báo cáo ca",   ActiveTab::ShiftReport},
            {"Quản lý tài khoản",    ActiveTab::Accounts},
            {"Nhật ký hoạt động",    ActiveTab::SystemLogs}
        };
    }

    navButtons.clear();
    float startY = 72.f;
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
        if (isLoggedIn) {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_XUAT", "Dang xuat khoi he thong");
        }
        isLoggedIn = false;
        loginInputUser->setText("");
        loginInputPass->setText("");
        loginErrorMsg = "";
    });
}

void CinemaUIApp::setupDashboardView() {
    dashboardStats.clear();
    float contentX = sidebarWidth + 30.f;
    float statY = topbarHeight + 25.f;
    float cardW = 325.f;
    float cardH = 120.f;
    float gap = 24.f;

    // Real KPI numbers derived from CSV data
    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX, statY),
        sf::Vector2f(cardW, cardH),
        "PHIM ĐANG CHIẾU (CSV)",
        std::to_string(movieList.size()) + " Tác Phẩm",
        "Lat Mat 7, Mai, Avatar 3...",
        Theme::Primary
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap), statY),
        sf::Vector2f(cardW, cardH),
        "SUẤT CHIẾU HỆ THỐNG (CSV)",
        std::to_string(showtimeList.size()) + " Suất Chiếu",
        "R01 đến R07 (2D, 3D, IMAX)",
        Theme::Cyan
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap) * 2.f, statY),
        sf::Vector2f(cardW, cardH),
        "TỔNG SỐ GHẾ QUẢN LÝ (CSV)",
        std::to_string(allSeats.size()) + " Ghế Ngồi",
        "Standard, VIP, IMAX, Sweetbox",
        Theme::Success
    )));

    dashboardStats.push_back(std::unique_ptr<StatCard>(new StatCard(
        sf::Vector2f(contentX + (cardW + gap) * 3.f, statY),
        sf::Vector2f(cardW, cardH),
        "DANH MỤC F&B / BẮP NƯỚC (CSV)",
        std::to_string(fnbList.size()) + " Mặt Hàng",
        "Bắp ngọt, Bắp phô mai, Coca...",
        Theme::Warning
    )));

    // Poster Slider Banner using EXACT movies from DATA/quan_ly_phim.csv
    float sliderY = statY + cardH + 28.f;
    dashboardSlider = std::unique_ptr<PosterSlider>(new PosterSlider(
        sf::Vector2f(contentX, sliderY),
        sf::Vector2f(1370.f, 320.f)
    ));

    std::vector<PosterSlideItem> slides;
    std::vector<sf::Color> colors = {
        sf::Color(99, 102, 241, 100),
        sf::Color(6, 182, 212, 100),
        sf::Color(245, 158, 11, 100),
        sf::Color(239, 68, 68, 100),
        sf::Color(16, 185, 129, 100)
    };

    for (size_t i = 0; i < movieList.size(); ++i) {
        PosterSlideItem it;
        it.maPhim = movieList[i].getMaPhim();
        it.tenPhim = movieList[i].getTenPhim();
        it.theLoai = db.layTheLoaiCuaPhim(it.maPhim);
        if (it.theLoai.empty()) it.theLoai = "Điện ảnh";
        it.thoiLuong = movieList[i].getThoiLuong();
        it.nhanDoTuoi = movieList[i].getNhanDoTuoi();
        it.moTa = movieList[i].getMoTa();
        if (it.moTa.empty()) it.moTa = "Tác phẩm điện ảnh bom tấn đang chiếu tại hệ thống rạp CineStar.";
        it.posterPath = movieList[i].getPoster();
        it.bannerColor = colors[i % colors.size()];
        slides.push_back(it);
    }

    if (slides.empty()) {
        PosterSlideItem fallback;
        fallback.maPhim = "P01";
        fallback.tenPhim = "LAT MAT 7: MOT DIEU UOC";
        fallback.theLoai = "Gia Dinh, Tam Ly";
        fallback.thoiLuong = 120;
        fallback.nhanDoTuoi = 18;
        fallback.moTa = "Phim ve gia dinh va tinh mau tu.";
        fallback.bannerColor = colors[0];
        slides.push_back(fallback);
    }

    dashboardSlider->setItems(slides);
    dashboardSlider->setOnSelectMovie([this](int idx) {
        currentTab = ActiveTab::SeatBooking;
    });
}

void CinemaUIApp::setupMoviesView() {
    float leftColX = sidebarWidth + 24.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;

    // Sub-tab buttons matching reference image
    btnSubTabMovies = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX, 180.f),
        sf::Vector2f(205.f, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSubTabMovies->setFontSize(16);
    btnSubTabMovies->setOutline(0.f, sf::Color::Transparent);
    btnSubTabMovies->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 70),
        sf::Color(203, 213, 225, 110),
        sf::Color::Transparent
    );
    btnSubTabMovies->setOnClick([this]() {
        movieSubTab = 0;
    });

    btnSubTabGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "",
        sf::Vector2f(leftColX + 215.f, 180.f),
        sf::Vector2f(205.f, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSubTabGenres->setFontSize(16);
    btnSubTabGenres->setOutline(0.f, sf::Color::Transparent);
    btnSubTabGenres->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 70),
        sf::Color(203, 213, 225, 110),
        sf::Color::Transparent
    );
    btnSubTabGenres->setOnClick([this]() {
        movieSubTab = 1;
    });

    // Quick search input inside Left Card (cardY = 232.f)
    movieSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 108.f, 248.f),
        sf::Vector2f(305.f, 36.f),
        "     Tìm theo Mã, Tên, Thể loại...",
        Theme::SizeBody
    ));
    movieSearchInput->setCornerRadius(8.f);
    movieSearchInput->setColors(
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    // "+ Thêm mới phim" top-right button inside Left Card filter bar
    btnQuickAddMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới phim",
        sf::Vector2f(leftColX + 698.f, 248.f),
        sf::Vector2f(210.f, 36.f),
        ButtonStyle::Primary,
        8.f
    ));
    btnQuickAddMovie->setFontSize(14);
    btnQuickAddMovie->setCustomColors(
        sf::Color(3, 105, 161),
        sf::Color(2, 132, 199),
        sf::Color(7, 89, 133),
        sf::Color(203, 213, 225)
    );
    btnQuickAddMovie->setOnClick([this]() {
        selectedMovieRowIdx = -1;
        std::string nextId = "P0" + std::to_string(movieList.size() + 1);
        inputMovieId->setText(nextId);
        inputMovieTitle->setText("");
        inputMovieDuration->setText("120");
        inputMovieReleaseDate->setText("15/11/2026");
        inputMovieAgeRating->setText("T18");
        inputMovieGenre->setText("");
        if (inputMoviePoster) inputMoviePoster->setText("assets/posters/" + nextId + ".jpg");
        inputMovieDesc->setText("");
    });

    // Movie Form Inputs (Right Card: rightColX, cardY = 232.f)
    float inputX = rightColX + 136.f;
    float inputW = 330.f;
    float inputH = 35.f;
    sf::Color softInputBg(241, 245, 249);
    sf::Color softInputBorder(226, 232, 240);
    sf::Color focusBlue(2, 132, 199);

    inputMovieId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 286.f),
        sf::Vector2f(inputW, inputH),
        "Mã phim tự sinh...",
        Theme::SizeBody
    ));
    inputMovieId->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputMovieId->setCornerRadius(8.f);
    inputMovieId->setText("P0" + std::to_string(movieList.size() + 1));

    inputMovieTitle = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 331.f),
        sf::Vector2f(inputW, inputH),
        "Nhập tên phim...",
        Theme::SizeBody
    ));
    inputMovieTitle->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieTitle->setCornerRadius(8.f);

    inputMovieDuration = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 376.f),
        sf::Vector2f(inputW, inputH),
        "120",
        Theme::SizeBody
    ));
    inputMovieDuration->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieDuration->setCornerRadius(8.f);

    inputMovieReleaseDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 421.f),
        sf::Vector2f(inputW, inputH),
        "dd/MM/yyyy",
        Theme::SizeBody
    ));
    inputMovieReleaseDate->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieReleaseDate->setCornerRadius(8.f);
    inputMovieReleaseDate->setText("01/04/2026");

    inputMovieAgeRating = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 466.f),
        sf::Vector2f(inputW, inputH),
        "P, K, T13, T18",
        Theme::SizeBody
    ));
    inputMovieAgeRating->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieAgeRating->setCornerRadius(8.f);
    inputMovieAgeRating->setText("T18");

    inputMovieGenre = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 511.f),
        sf::Vector2f(inputW - 92.f, inputH),
        "Tâm Lý, Hành Động",
        Theme::SizeBody
    ));
    inputMovieGenre->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieGenre->setCornerRadius(8.f);

    btnSelectGenre = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Chọn...",
        sf::Vector2f(inputX + inputW - 84.f, 511.f),
        sf::Vector2f(84.f, inputH),
        ButtonStyle::Ghost,
        8.f
    ));
    btnSelectGenre->setFontSize(13);
    btnSelectGenre->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(241, 245, 249));
    btnSelectGenre->setOutline(1.f, sf::Color(186, 230, 253));
    btnSelectGenre->setTextColor(sf::Color(3, 105, 161));
    btnSelectGenre->setOnClick([this]() {
        if (!genreList.empty()) {
            selectedGenreChoiceIdx = (selectedGenreChoiceIdx + 1) % genreList.size();
            inputMovieGenre->setText(genreList[selectedGenreChoiceIdx].second);
        }
    });

    // Dedicated Poster Path Input (paired with visual Poster Preview slot)
    inputMoviePoster = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX + 58.f, 566.f),
        sf::Vector2f(inputW - 58.f, inputH),
        "assets/posters/P01.jpg",
        Theme::SizeBody
    ));
    inputMoviePoster->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMoviePoster->setCornerRadius(8.f);
    inputMoviePoster->setText("assets/posters/P01.jpg");

    inputMovieDesc = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX + 20.f, 650.f),
        sf::Vector2f(rightColW - 40.f, 78.f),
        "Nhập nội dung tóm tắt phim...",
        Theme::SizeBody
    ));
    inputMovieDesc->setColors(softInputBg, softInputBorder, focusBlue, sf::Color(51, 65, 85), sf::Color(148, 163, 184));
    inputMovieDesc->setCornerRadius(8.f);

    // Form Action Buttons matching reference image
    float btnW = (rightColW - 40.f - 12.f) / 2.0f; // 217.f
    float btnRow1Y = 796.f;
    float btnRow2Y = 846.f;

    btnMovieCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm mới",
        sf::Vector2f(rightColX + 20.f, btnRow1Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Success,
        8.f
    ));
    btnMovieCreate->setFontSize(14);
    btnMovieCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnMovieCreate->setOnClick([this]() {
        std::string title = inputMovieTitle->getText();
        if (title.find_first_not_of(" \t\r\n") == std::string::npos) {
            confirmPopup->show("Lỗi", "Vui lòng nhập tên phim trước khi thêm mới!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }

        std::string id = inputMovieId->getText();
        if (id.empty()) {
            id = "P0" + std::to_string(movieList.size() + 1);
        }
        for (size_t i = 0; i < movieList.size(); ++i) {
            if (movieList[i].getMaPhim() == id) {
                confirmPopup->show("Lỗi", "Mã phim '" + id + "' đã tồn tại trong hệ thống!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }
        }

        int dur = 0;
        try {
            dur = std::stoi(inputMovieDuration->getText());
        } catch (...) {
            dur = 0;
        }
        if (dur <= 0) {
            confirmPopup->show("Lỗi", "Thời lượng phim phải lớn hơn 0 phút!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }

        std::string rel = inputMovieReleaseDate->getText();
        if (rel.empty()) {
            confirmPopup->show("Lỗi", "Vui lòng nhập ngày khởi chiếu (DD/MM/YYYY)!", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }
        if (!DateFormat::isValidDate(rel)) {
            confirmPopup->show("Lỗi", "Ngày khởi chiếu không hợp lệ! Vui lòng nhập định dạng DD/MM/YYYY.", ButtonStyle::Danger, "Đóng", "Hủy");
            return;
        }
        std::string normRel = DateFormat::normalizeDate(rel);
        int todayKey = 0;
        if (DateFormat::todayDateKey(todayKey)) {
            if (DateFormat::dateKey(normRel) < todayKey) {
                confirmPopup->show("Lỗi", "Ngày khởi chiếu không được nằm trong quá khứ!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }
        }

        std::string ageStr = inputMovieAgeRating->getText();
        int age = 0;
        if (ageStr == "P" || ageStr == "p" || ageStr == "K" || ageStr == "k") age = 0;
        else {
            std::string numOnly = "";
            for (char ch : ageStr) {
                if (ch >= '0' && ch <= '9') numOnly += ch;
            }
            try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
        }

        std::string gName = inputMovieGenre->getText();
        std::string desc = inputMovieDesc->getText();
        std::string posterPath = (inputMoviePoster && !inputMoviePoster->getText().empty())
                                 ? inputMoviePoster->getText()
                                 : ("assets/posters/" + id + ".jpg");

        int day = 1, month = 1, year = 2026;
        DateFormat::parseDate(normRel, day, month, year);
        month += 1;
        if (month > 12) { month = 1; year += 1; }
        std::ostringstream ssEnd;
        ssEnd << std::setfill('0') << std::setw(2) << day << "/"
              << std::setw(2) << month << "/" << std::setw(4) << year;
        std::string endDate = ssEnd.str();

        Movie m(id, title, dur, age, normRel, endDate, desc, posterPath);
        movieList.push_back(m);
        db.ghiDanhSachPhim(movieList);
        if (!gName.empty()) {
            std::stringstream gss(gName);
            std::string gToken;
            bool firstGenre = true;
            while (std::getline(gss, gToken, ',')) {
                size_t s = gToken.find_first_not_of(" \t\r\n");
                size_t e = gToken.find_last_not_of(" \t\r\n");
                if (s != std::string::npos && e != std::string::npos) {
                    std::string cleanG = gToken.substr(s, e - s + 1);
                    if (firstGenre) {
                        db.capNhatTheLoaiChoPhim(id, cleanG);
                        firstGenre = false;
                    } else {
                        db.themTheLoaiChoPhim(id, cleanG);
                    }
                }
            }
            db.docDanhSachTheLoai(genreList);
        }
        db.docDanhSachPhim(movieList);
        reloadMoviePosterTextures();
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHIM", "Them phim moi: " + title + " (" + id + ")");
        confirmPopup->show("Thành công", "Đã thêm phim mới!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnMovieUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(rightColX + 20.f + btnW + 12.f, btnRow1Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieUpdate->setFontSize(14);
    btnMovieUpdate->setCustomColors(sf::Color(224, 242, 254), sf::Color(186, 230, 253), sf::Color(125, 211, 252), sf::Color(226, 232, 240));
    btnMovieUpdate->setOutline(1.f, sf::Color(186, 230, 253));
    btnMovieUpdate->setTextColor(sf::Color(3, 105, 161));
    btnMovieUpdate->setOnClick([this]() {
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            std::string title = inputMovieTitle->getText();
            if (title.find_first_not_of(" \t\r\n") == std::string::npos) {
                confirmPopup->show("Lỗi", "Tên phim không được để trống!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }

            int dur = 0;
            try {
                dur = std::stoi(inputMovieDuration->getText());
            } catch (...) {
                dur = 0;
            }
            if (dur <= 0) {
                confirmPopup->show("Lỗi", "Thời lượng phim phải lớn hơn 0 phút!", ButtonStyle::Danger, "Đóng", "Hủy");
                return;
            }

            std::string rel = inputMovieReleaseDate->getText();
            std::string oldRel = DateFormat::normalizeDate(movieList[selectedMovieRowIdx].getNgayKhoiChieu());
            if (!rel.empty()) {
                if (!DateFormat::isValidDate(rel)) {
                    confirmPopup->show("Lỗi", "Ngày khởi chiếu không hợp lệ! Vui lòng nhập định dạng DD/MM/YYYY.", ButtonStyle::Danger, "Đóng", "Hủy");
                    return;
                }
                std::string normRel = DateFormat::normalizeDate(rel);
                int todayKey = 0;
                if (normRel != oldRel && DateFormat::todayDateKey(todayKey)) {
                    if (DateFormat::dateKey(normRel) < todayKey) {
                        confirmPopup->show("Lỗi", "Ngày khởi chiếu mới không được nằm trong quá khứ!", ButtonStyle::Danger, "Đóng", "Hủy");
                        return;
                    }
                }
                rel = normRel;
            }

            std::string ageStr = inputMovieAgeRating->getText();
            int age = 0;
            if (ageStr == "P" || ageStr == "p" || ageStr == "K" || ageStr == "k") age = 0;
            else {
                std::string numOnly = "";
                for (char ch : ageStr) {
                    if (ch >= '0' && ch <= '9') numOnly += ch;
                }
                try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
            }

            std::string gName = inputMovieGenre->getText();
            std::string mId = movieList[selectedMovieRowIdx].getMaPhim();
            std::string newDesc = inputMovieDesc->getText();
            std::string newPoster = inputMoviePoster ? inputMoviePoster->getText() : "";
            std::string curGenre = db.layTheLoaiCuaPhim(mId);
            std::string curPoster = movieList[selectedMovieRowIdx].getPoster();
            if (curPoster.empty() || curPoster == "assets/posters/poster_default.jpg") {
                curPoster = "assets/posters/" + mId + ".jpg";
            }

            bool isUnchanged = (title == movieList[selectedMovieRowIdx].getTenPhim()) &&
                               (dur == movieList[selectedMovieRowIdx].getThoiLuong()) &&
                               (age == movieList[selectedMovieRowIdx].getNhanDoTuoi()) &&
                               (rel.empty() || rel == oldRel) &&
                               (gName == curGenre) &&
                               (newDesc == movieList[selectedMovieRowIdx].getMoTa()) &&
                               (newPoster.empty() || newPoster == curPoster);
            if (isUnchanged) {
                confirmPopup->show("Thông báo", "Chưa có thông tin nào thay đổi!", ButtonStyle::Primary, "Đóng", "Quay lại");
                return;
            }

            movieList[selectedMovieRowIdx].setTenPhim(title);
            movieList[selectedMovieRowIdx].setThoiLuong(dur);
            movieList[selectedMovieRowIdx].setNhanDoTuoi(age);
            if (!rel.empty()) {
                movieList[selectedMovieRowIdx].setNgayKhoiChieu(rel);
                std::string curEnd = DateFormat::normalizeDate(movieList[selectedMovieRowIdx].getNgayKetThuc());
                if (DateFormat::dateKey(rel) > DateFormat::dateKey(curEnd)) {
                    int day = 1, month = 1, year = 2026;
                    DateFormat::parseDate(rel, day, month, year);
                    month += 1;
                    if (month > 12) { month = 1; year += 1; }
                    std::ostringstream ssEnd;
                    ssEnd << std::setfill('0') << std::setw(2) << day << "/"
                          << std::setw(2) << month << "/" << std::setw(4) << year;
                    movieList[selectedMovieRowIdx].setNgayKetThuc(ssEnd.str());
                }
            }
            movieList[selectedMovieRowIdx].setMoTa(newDesc);
            if (!newPoster.empty()) {
                movieList[selectedMovieRowIdx].setPoster(newPoster);
            }

            db.ghiDanhSachPhim(movieList);
            if (!gName.empty()) {
                std::stringstream gss(gName);
                std::string gToken;
                bool firstGenre = true;
                while (std::getline(gss, gToken, ',')) {
                    size_t s = gToken.find_first_not_of(" \t\r\n");
                    size_t e = gToken.find_last_not_of(" \t\r\n");
                    if (s != std::string::npos && e != std::string::npos) {
                        std::string cleanG = gToken.substr(s, e - s + 1);
                        if (firstGenre) {
                            db.capNhatTheLoaiChoPhim(mId, cleanG);
                            firstGenre = false;
                        } else {
                            db.themTheLoaiChoPhim(mId, cleanG);
                        }
                    }
                }
                db.docDanhSachTheLoai(genreList);
            }
            db.docDanhSachPhim(movieList);
            reloadMoviePosterTextures();

            ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_PHIM", "Cap nhat thong tin phim: " + title + " (" + mId + ")");
            confirmPopup->show("Thành công", "Đã cập nhật thông tin!", ButtonStyle::Primary, "Đóng", "Quay lại");
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 phim từ danh sách để cập nhật!", ButtonStyle::Primary, "Đóng", "Hủy");
        }
    });

    btnMovieDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa",
        sf::Vector2f(rightColX + 20.f, btnRow2Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieDelete->setFontSize(14);
    btnMovieDelete->setCustomColors(sf::Color(254, 226, 226), sf::Color(254, 202, 202), sf::Color(252, 165, 165), sf::Color(226, 232, 240));
    btnMovieDelete->setOutline(1.f, sf::Color(252, 165, 165));
    btnMovieDelete->setTextColor(sf::Color(220, 38, 38));
    btnMovieDelete->setOnClick([this]() {
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            std::string pName = movieList[selectedMovieRowIdx].getTenPhim();
            confirmPopup->show("XÁC NHẬN XÓA", "Bạn có chắc chắn muốn xóa phim '" + pName + "' khỏi hệ thống?", ButtonStyle::Danger, "Xác nhận xóa", "Hủy");
            confirmPopup->setOnConfirm([this, pName]() {
                movieList.erase(movieList.begin() + selectedMovieRowIdx);
                db.ghiDanhSachPhim(movieList);
                reloadMoviePosterTextures();
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_PHIM", "Xoa phim: " + pName);
                selectedMovieRowIdx = -1;
            });
        }
    });

    btnMovieClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Clear Form",
        sf::Vector2f(rightColX + 20.f + btnW + 12.f, btnRow2Y),
        sf::Vector2f(btnW, 40.f),
        ButtonStyle::Ghost,
        8.f
    ));
    btnMovieClear->setFontSize(14);
    btnMovieClear->setCustomColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(203, 213, 225), sf::Color(226, 232, 240));
    btnMovieClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnMovieClear->setTextColor(sf::Color(51, 65, 85));
    btnMovieClear->setOnClick([this]() {
        selectedMovieRowIdx = -1;
        std::string nextId = "P0" + std::to_string(movieList.size() + 1);
        inputMovieId->setText(nextId);
        inputMovieTitle->setText("");
        inputMovieDuration->setText("120");
        inputMovieReleaseDate->setText("01/04/2026");
        inputMovieAgeRating->setText("T18");
        inputMovieGenre->setText("");
        if (inputMoviePoster) inputMoviePoster->setText("assets/posters/" + nextId + ".jpg");
        inputMovieDesc->setText("");
    });

    // Populate initial form with 1st movie
    if (!movieList.empty()) {
        selectedMovieRowIdx = 0;
        inputMovieId->setText(movieList[0].getMaPhim());
        inputMovieTitle->setText(movieList[0].getTenPhim());
        inputMovieDuration->setText(std::to_string(movieList[0].getThoiLuong()));
        inputMovieReleaseDate->setText(movieList[0].getNgayKhoiChieu());
        int a0 = movieList[0].getNhanDoTuoi();
        inputMovieAgeRating->setText(a0 <= 0 ? "P" : ("T" + std::to_string(a0)));
        inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[0].getMaPhim()));
        std::string p0 = movieList[0].getPoster();
        if (p0.empty() || p0 == "assets/posters/poster_default.jpg") {
            p0 = "assets/posters/" + movieList[0].getMaPhim() + ".jpg";
        }
        if (inputMoviePoster) inputMoviePoster->setText(p0);
        inputMovieDesc->setText(movieList[0].getMoTa());
    }

    // Genre Form Setup (Matching Reference UI & Linked to SQLite Backend)
    selectedGenreColorIdx = 0;
    genreSortAsc = true;
    genreColorMap["TL01"] = 0; // Red
    genreColorMap["TL02"] = 1; // Purple
    genreColorMap["TL03"] = 2; // Pink
    genreColorMap["TL04"] = 3; // Amber
    genreColorMap["TL05"] = 4; // Blue
    genreColorMap["TL06"] = 5; // Emerald
    genreColorMap["TL07"] = 6; // Cyan
    genreColorMap["TL08"] = 7; // Orange
    genreColorMap["TL09"] = 8; // Crimson

    genreSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightColX - 40.f, 180.f),
        sf::Vector2f(310.f, 40.f),
        "     Tìm tên hoặc mã thể loại...",
        13
    ));
    genreSearchInput->setColors(sf::Color::White, sf::Color(226, 232, 240), focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    genreSearchInput->setCornerRadius(20.f);

    btnExportGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "    Xuất danh sách",
        sf::Vector2f(rightColX + 282.f, 180.f),
        sf::Vector2f(170.f, 40.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnExportGenres->setFontSize(13);
    btnExportGenres->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnExportGenres->setOutline(1.f, sf::Color(226, 232, 240));
    btnExportGenres->setTextColor(sf::Color(30, 41, 59));
    btnExportGenres->setOnClick([this]() {
        std::ofstream out("DATA/danh_sach_the_loai.csv", std::ios::out | std::ios::trunc);
        if (out.is_open()) {
            out << "MaTheLoai,TenTheLoai\n";
            for (const auto& g : genreList) {
                out << g.first << "," << g.second << "\n";
            }
            out.close();
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "XUAT_THE_LOAI", "Xuat danh sach the loai phim");
        confirmPopup->show("Thành công", "Đã xuất danh sách thể loại!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    float gInputX = rightColX + 24.f;
    float gInputW = rightColW - 48.f;
    float gInputH = 44.f;
    float gHalfBtnW = (gInputW - 14.f) / 2.f;

    inputGenreId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 352.f),
        sf::Vector2f(gInputW, gInputH),
        "TL01",
        14
    ));
    inputGenreId->setColors(sf::Color(241, 245, 249), sf::Color(226, 232, 240), focusBlue, sf::Color(30, 41, 59), sf::Color(148, 163, 184));
    inputGenreId->setCornerRadius(10.f);

    inputGenreName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(gInputX, 442.f),
        sf::Vector2f(gInputW, gInputH),
        "Nhập tên thể loại...",
        14
    ));
    inputGenreName->setColors(sf::Color::White, sf::Color(203, 213, 225), focusBlue, sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputGenreName->setCornerRadius(10.f);

    if (!genreList.empty()) {
        selectedGenreRowIdx = 0;
        inputGenreId->setText(genreList[0].first);
        inputGenreName->setText(genreList[0].second);
    }

    btnGenreCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "  + Thêm thể loại",
        sf::Vector2f(gInputX, 616.f),
        sf::Vector2f(gHalfBtnW, 44.f),
        ButtonStyle::Success,
        10.f
    ));
    btnGenreCreate->setFontSize(14);
    btnGenreCreate->setCustomColors(sf::Color(5, 150, 105), sf::Color(4, 120, 87), sf::Color(6, 95, 70), sf::Color(203, 213, 225));
    btnGenreCreate->setOnClick([this]() {
        std::string gName = inputGenreName->getText();
        size_t s = gName.find_first_not_of(" \t\r\n");
        size_t e = gName.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) {
            confirmPopup->show("Lỗi", "Tên thể loại không được để trống!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        gName = gName.substr(s, e - s + 1);
        if (db.themLoaiTheLoai(gName)) {
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
            if (!genreList.empty()) {
                selectedGenreRowIdx = static_cast<int>(genreList.size()) - 1;
                std::string newId = genreList.back().first;
                genreColorMap[newId] = selectedGenreColorIdx;
                inputGenreId->setText(newId);
                inputGenreName->setText(genreList.back().second);
            }
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_THE_LOAI", "Them the loai: " + gName);
            confirmPopup->show("Thành công", "Đã thêm thể loại mới!", ButtonStyle::Primary, "Đóng", "Quay lại");
        } else {
            confirmPopup->show("Thông báo", "Thể loại này đã tồn tại trong hệ thống!", ButtonStyle::Primary, "Đóng", "Quay lại");
        }
    });

    btnGenreDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Xóa thể loại",
        sf::Vector2f(gInputX + gHalfBtnW + 14.f, 616.f),
        sf::Vector2f(gHalfBtnW, 44.f),
        ButtonStyle::Danger,
        10.f
    ));
    btnGenreDelete->setFontSize(14);
    btnGenreDelete->setCustomColors(sf::Color(225, 29, 72), sf::Color(190, 18, 60), sf::Color(159, 18, 57), sf::Color(203, 213, 225));
    btnGenreDelete->setOnClick([this]() {
        if (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size())) {
            std::string gId = genreList[selectedGenreRowIdx].first;
            std::string gName = genreList[selectedGenreRowIdx].second;
            confirmPopup->show("Xác nhận xóa", "Bạn có chắc chắn muốn xóa thể loại '" + gName + "'?", ButtonStyle::Danger, "Xóa thể loại", "Quay lại");
            confirmPopup->setOnConfirm([this, gId, gName]() {
                if (db.xoaLoaiTheLoai(gId)) {
                    genreList.clear();
                    db.docDanhSachTheLoai(genreList);
                    if (!genreList.empty()) {
                        selectedGenreRowIdx = 0;
                        inputGenreId->setText(genreList[0].first);
                        inputGenreName->setText(genreList[0].second);
                    } else {
                        selectedGenreRowIdx = -1;
                        inputGenreId->setText("TL01");
                        inputGenreName->setText("");
                    }
                    ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_THE_LOAI", "Xoa the loai: " + gName + " (" + gId + ")");
                }
            });
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 thể loại từ danh sách để xóa!", ButtonStyle::Primary, "Đóng", "Quay lại");
        }
    });

    btnGenreUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Lưu thay đổi",
        sf::Vector2f(gInputX, 672.f),
        sf::Vector2f(gInputW, 44.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnGenreUpdate->setFontSize(14);
    btnGenreUpdate->setCustomColors(sf::Color(2, 132, 199), sf::Color(3, 105, 161), sf::Color(7, 89, 133), sf::Color(203, 213, 225));
    btnGenreUpdate->setOnClick([this]() {
        if (selectedGenreRowIdx < 0 || selectedGenreRowIdx >= static_cast<int>(genreList.size())) {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 thể loại từ danh sách để cập nhật!", ButtonStyle::Primary, "Đóng", "Quay lại");
            return;
        }
        std::string gId = genreList[selectedGenreRowIdx].first;
        std::string oldName = genreList[selectedGenreRowIdx].second;
        std::string gName = inputGenreName->getText();
        size_t s = gName.find_first_not_of(" \t\r\n");
        size_t e = gName.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) {
            confirmPopup->show("Lỗi", "Tên thể loại không được để trống!", ButtonStyle::Danger, "Đóng", "Quay lại");
            return;
        }
        gName = gName.substr(s, e - s + 1);
        int oldCol = genreColorMap.count(gId) ? genreColorMap[gId] : (selectedGenreRowIdx % 5);
        if (gName == oldName && selectedGenreColorIdx == oldCol) {
            confirmPopup->show("Thông báo", "Chưa có thông tin nào thay đổi!", ButtonStyle::Primary, "Đóng", "Quay lại");
            return;
        }
        if (gName != oldName) {
            if (!db.suaLoaiTheLoai(gId, gName)) {
                confirmPopup->show("Lỗi", "Tên thể loại đã tồn tại hoặc không hợp lệ!", ButtonStyle::Danger, "Đóng", "Quay lại");
                return;
            }
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
        }
        genreColorMap[gId] = selectedGenreColorIdx;
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_THE_LOAI", "Cap nhat the loai: " + gName + " (" + gId + ")");
        confirmPopup->show("Thành công", "Đã cập nhật thông tin!", ButtonStyle::Primary, "Đóng", "Quay lại");
    });

    btnGenreClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "   Làm mới biểu mẫu",
        sf::Vector2f(gInputX, 728.f),
        sf::Vector2f(gInputW, 40.f),
        ButtonStyle::Ghost,
        10.f
    ));
    btnGenreClear->setFontSize(13);
    btnGenreClear->setCustomColors(sf::Color::White, sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(241, 245, 249));
    btnGenreClear->setOutline(1.f, sf::Color(226, 232, 240));
    btnGenreClear->setTextColor(sf::Color(51, 65, 85));
    btnGenreClear->setOnClick([this]() {
        selectedGenreRowIdx = -1;
        size_t nextId = 1;
        std::string nextCode;
        bool exists = false;
        do {
            std::ostringstream ss;
            ss << "TL" << std::setw(2) << std::setfill('0') << nextId++;
            nextCode = ss.str();
            exists = false;
            for (const auto& g : genreList) {
                if (g.first == nextCode) { exists = true; break; }
            }
        } while (exists);
        inputGenreId->setText(nextCode);
        inputGenreName->setText("");
        inputGenreName->setFocused(true);
    });
}

static std::string formatVND(double amount) {
    long long val = static_cast<long long>(amount);
    if (val < 0) val = 0;
    std::string s = std::to_string(val);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ".");
        n -= 3;
    }
    return s + " VNĐ";
}

void CinemaUIApp::setupSeatBookingView() {
    // 1. Refresh button in topbar ("Làm mới lịch chiếu")
    btnPosRefresh = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Làm mới lịch chiếu",
        sf::Vector2f(1728.f - 230.f, (topbarHeight - 36.f) / 2.0f),
        sf::Vector2f(205.f, 36.f),
        ButtonStyle::Secondary,
        18.f
    ));
    btnPosRefresh->setCustomColors(
        sf::Color(14, 165, 233), // Sky blue #0ea5e9
        sf::Color(2, 132, 199),  // #0284c7
        sf::Color(3, 105, 161),  // #0369a1
        sf::Color(148, 163, 184)
    );
    btnPosRefresh->setOnClick([this]() {
        loadDataFromDatabase();
        if (!showtimeList.empty()) {
            if (posSelectedShowtimeIdx >= static_cast<int>(showtimeList.size())) posSelectedShowtimeIdx = 0;
            currentShowtimeId = showtimeList[posSelectedShowtimeIdx].getMaSuatChieu();
            refreshSeatsForShowtime(currentShowtimeId);
        }
    });

    // 2. Filter buttons
    float filterStartX = sidebarWidth + 24.f;
    float filterStartY = topbarHeight + 16.f;

    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char dateBuf[32];
    std::strftime(dateBuf, sizeof(dateBuf), "Hôm nay, %d/%m", lt);

    btnPosDate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        dateBuf,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 14.f),
        sf::Vector2f(140.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    ));
    btnPosDate->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );

    std::string currentMovieTitle = "Thỏ Ơi";
    if (!movieList.empty()) {
        currentMovieTitle = movieList[posSelectedMovieIdx % movieList.size()].getTenPhim();
    }

    btnPosMovie = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        currentMovieTitle,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 54.f),
        sf::Vector2f(140.f, 32.f),
        ButtonStyle::Secondary,
        6.f
    ));
    btnPosMovie->setCustomColors(
        sf::Color(14, 165, 233),
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(148, 163, 184)
    );
    btnPosMovie->setOnClick([this]() {
        if (!movieList.empty()) {
            posSelectedMovieIdx = (posSelectedMovieIdx + 1) % movieList.size();
            std::string mTitle = movieList[posSelectedMovieIdx].getTenPhim();
            std::string mId = movieList[posSelectedMovieIdx].getMaPhim();
            btnPosMovie->setText(mTitle);

            for (size_t i = 0; i < showtimeList.size(); ++i) {
                if (showtimeList[i].getMaPhim() == mId) {
                    posSelectedShowtimeIdx = static_cast<int>(i);
                    currentShowtimeId = showtimeList[i].getMaSuatChieu();
                    std::string stLabel = showtimeList[i].getGioBatDau() + " - " + showtimeList[i].getMaPhong() + " - Standard";
                    btnPosShowtime->setText(stLabel);
                    posShowSeatMap = true;
                    refreshSeatsForShowtime(currentShowtimeId);
                    return;
                }
            }
        }
    });

    std::string currentShowtimeLabel = "17:00 - Phòng 1 - Standard";
    if (!showtimeList.empty()) {
        const auto& st = showtimeList[posSelectedShowtimeIdx % showtimeList.size()];
        currentShowtimeLabel = st.getGioBatDau() + " - " + st.getMaPhong() + " - Standard";
        currentShowtimeId = st.getMaSuatChieu();
    }

    btnPosShowtime = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        currentShowtimeLabel,
        sf::Vector2f(filterStartX + 90.f, filterStartY + 94.f),
        sf::Vector2f(235.f, 34.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnPosShowtime->setCustomColors(
        sf::Color(255, 255, 255),
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240)
    );
    btnPosShowtime->setTextColor(sf::Color(15, 23, 42));
    btnPosShowtime->setOutline(1.0f, sf::Color(203, 213, 225));
    btnPosShowtime->setOnClick([this]() {
        if (!showtimeList.empty()) {
            posSelectedShowtimeIdx = (posSelectedShowtimeIdx + 1) % showtimeList.size();
            const auto& st = showtimeList[posSelectedShowtimeIdx];
            currentShowtimeId = st.getMaSuatChieu();
            std::string stLabel = st.getGioBatDau() + " - " + st.getMaPhong() + " - Standard";
            btnPosShowtime->setText(stLabel);
            for (size_t m = 0; m < movieList.size(); ++m) {
                if (movieList[m].getMaPhim() == st.getMaPhim()) {
                    posSelectedMovieIdx = static_cast<int>(m);
                    btnPosMovie->setText(movieList[m].getTenPhim());
                    break;
                }
            }
        }
        posShowSeatMap = true;
        refreshSeatsForShowtime(currentShowtimeId);
    });

    // Right Column Buttons
    float rightStartX = sidebarWidth + 1084.f;

    btnPosAddFnb = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "+ Thêm bắp/nước",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 434.f),
        sf::Vector2f(175.f, 34.f),
        ButtonStyle::Primary,
        6.f
    ));
    btnPosAddFnb->setCustomColors(
        sf::Color(245, 158, 11), // Amber #f59e0b
        sf::Color(217, 119, 6),  // #d97706
        sf::Color(180, 83, 9),   // #b45309
        sf::Color(203, 213, 225)
    );
    btnPosAddFnb->setOnClick([this]() {
        if (posSelectedFnb.empty()) {
            posSelectedFnb.push_back({"1x Combo Bắp Nước CineStar", 75000.0});
        } else if (posSelectedFnb.size() == 1) {
            posSelectedFnb.push_back({"1x Bắp Rang Phô Mai Lớn", 45000.0});
        } else if (posSelectedFnb.size() == 2) {
            posSelectedFnb.push_back({"1x Nước Ngọt Có Ga Lạnh", 25000.0});
        } else {
            posSelectedFnb.clear();
        }
        posFnbTotal = 0.0;
        for (const auto& item : posSelectedFnb) {
            posFnbTotal += item.second;
        }
        posGrandTotal = posSeatTotal + posFnbTotal;
    });

    btnPosCheckout = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thanh toán (F5)",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 560.f),
        sf::Vector2f(340.f, 48.f),
        ButtonStyle::Success,
        8.f
    ));
    btnPosCheckout->setCustomColors(
        sf::Color(34, 197, 94), // Emerald green #22c55e
        sf::Color(22, 163, 74), // #16a34a
        sf::Color(21, 128, 61), // #15803d
        sf::Color(203, 213, 225)
    );
    btnPosCheckout->setOnClick([this]() {
        if (selectedSeatCodes.empty() && posSelectedFnb.empty()) {
            confirmPopup->show("Thông báo", "Vui lòng chọn ít nhất 1 ghế hoặc món F&B trước khi thanh toán!", ButtonStyle::Primary, "Đồng ý", "Đóng");
            return;
        }
        std::ostringstream oss;
        oss << "XÁC NHẬN THANH TOÁN ĐƠN HÀNG POS (KIOSK)\n"
            << "Số vé: " << selectedSeatCodes.size() << " | F&B: " << posSelectedFnb.size() << " món\n"
            << "Tổng cộng: " << formatVND(posGrandTotal);

        confirmPopup->show("XÁC NHẬN BÁN VÉ", oss.str(), ButtonStyle::Success, "In Hóa Đơn & Xác Nhận", "Hủy");
        confirmPopup->setOnConfirm([this]() {
            for (auto& s : currentSeats) {
                if (s.isSelected) {
                    s.isSelected = false;
                    s.trangThai = "DaDat";
                    db.capNhatTrangThaiGheSuatChieu(currentShowtimeId, s.maGhe, "DaDat");
                }
            }
            std::string logMsg = "Xuat ve POS (" + std::to_string(selectedSeatCodes.size()) + " ve), F&B (" + std::to_string(posSelectedFnb.size()) + " mon), Thu: " + std::to_string((long long)posGrandTotal) + " VND";
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "BAN_VE_POS", logMsg);
            setupLogsView();

            selectedSeatCodes.clear();
            posSelectedFnb.clear();
            posSeatTotal = 0.0;
            posFnbTotal = 0.0;
            posGrandTotal = 0.0;
        });
    });

    btnPosCancel = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Hủy đơn",
        sf::Vector2f(rightStartX + 14.f, topbarHeight + 618.f),
        sf::Vector2f(340.f, 44.f),
        ButtonStyle::Dark,
        8.f
    ));
    btnPosCancel->setCustomColors(
        sf::Color(100, 116, 139), // Slate grey #64748b
        sf::Color(71, 85, 105),   // #475569
        sf::Color(51, 65, 85),    // #334155
        sf::Color(203, 213, 225)
    );
    btnPosCancel->setOnClick([this]() {
        for (auto& s : currentSeats) {
            s.isSelected = false;
        }
        selectedSeatCodes.clear();
        posSelectedFnb.clear();
        posSeatTotal = 0.0;
        posFnbTotal = 0.0;
        posGrandTotal = 0.0;
    });

    if (!showtimeList.empty()) {
        currentShowtimeId = showtimeList[0].getMaSuatChieu();
        refreshSeatsForShowtime(currentShowtimeId);
    }
}

void CinemaUIApp::refreshSeatsForShowtime(const std::string& maSuatChieu) {
    currentSeats.clear();
    selectedSeatCodes.clear();
    bookingTotalPrice = 0.0;

    // 1. Find showtime to get room ID
    std::string roomId = "R01";
    for (const auto& st : showtimeList) {
        if (st.getMaSuatChieu() == maSuatChieu) {
            roomId = st.getMaPhong();
            break;
        }
    }

    // 2. Read REAL seat status from DATA/lich_va_ve.csv
    vector<pair<string, string>> seatStatusList;
    db.docTrangThaiGheSuatChieu(maSuatChieu, seatStatusList);

    // 3. Filter REAL seats from DATA/quan_ly_phong_ghe.csv for this room
    std::vector<Seat> roomSeats;
    for (const auto& s : allSeats) {
        if (s.getMaPhong() == roomId) {
            roomSeats.push_back(s);
        }
    }

    // Layout configuration inside White Seat Card
    float cardX = sidebarWidth + 24.f;
    float cardY = topbarHeight + 172.f;
    float cardW = 1040.f;

    float seatW = 54.f;
    float seatH = 42.f;
    float gapX = 14.f;
    float gapY = 14.f;

    float gridStartX = cardX + (cardW - (10.f * seatW + 9.f * gapX)) / 2.0f;
    float gridStartY = cardY + 98.f;

    if (!roomSeats.empty()) {
        int colsPerRow = 10;
        if (roomSeats.size() <= 12) colsPerRow = 4;
        else if (roomSeats.size() <= 30) colsPerRow = 6;

        gridStartX = cardX + (cardW - (colsPerRow * seatW + (colsPerRow - 1) * gapX)) / 2.0f;

        for (size_t i = 0; i < roomSeats.size(); ++i) {
            VisualSeat vs;
            vs.maGhe = roomSeats[i].getMaGhe();
            vs.loaiGhe = roomSeats[i].getLoaiGhe();
            vs.donGia = roomSeats[i].getDonGiaGhe();
            vs.isSelected = false;
            vs.trangThai = "Trong";

            // Check if booked in CSV
            for (const auto& stat : seatStatusList) {
                if (stat.first == vs.maGhe && (stat.second == "DaDat" || stat.second == "1")) {
                    vs.trangThai = "DaDat";
                    break;
                }
            }

            int r = static_cast<int>(i) / colsPerRow;
            int c = static_cast<int>(i) % colsPerRow;

            vs.position = sf::Vector2f(
                gridStartX + static_cast<float>(c) * (seatW + gapX),
                gridStartY + static_cast<float>(r) * (seatH + gapY)
            );
            vs.size = sf::Vector2f(seatW, seatH);
            currentSeats.push_back(vs);
        }
    } else {
        // Fallback standard layout
        char rows[7] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
        for (int r = 0; r < 7; ++r) {
            for (int c = 1; c <= 10; ++c) {
                VisualSeat s;
                std::string code = std::string(1, rows[r]) + (c < 10 ? "0" : "") + std::to_string(c);
                s.maGhe = code;
                s.isSelected = false;
                s.trangThai = "Trong";
                s.loaiGhe = (r < 3) ? "Standard" : ((r < 6) ? "VIP" : "Sweetbox");
                s.donGia = (r < 3) ? 60000.0 : ((r < 6) ? 90000.0 : 140000.0);

                for (const auto& stat : seatStatusList) {
                    if (stat.first == code && (stat.second == "DaDat" || stat.second == "1")) {
                        s.trangThai = "DaDat";
                        break;
                    }
                }

                s.position = sf::Vector2f(
                    gridStartX + static_cast<float>(c - 1) * (seatW + gapX),
                    gridStartY + static_cast<float>(r) * (seatH + gapY)
                );
                s.size = sf::Vector2f(seatW, seatH);
                currentSeats.push_back(s);
            }
        }
    }
}

void CinemaUIApp::setupConcessionsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    btnCheckoutFnb = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thanh Toán Đơn F&B",
        sf::Vector2f(contentX + 960.f, contentY + 540.f),
        sf::Vector2f(380.f, 46.f),
        ButtonStyle::Success
    ));

    btnCheckoutFnb->setOnClick([this]() {
        double total = 0.0;
        int itemsCount = 0;
        for (size_t i = 0; i < fnbList.size() && i < fnbQuantities.size(); ++i) {
            total += static_cast<double>(fnbQuantities[i]) * fnbList[i].getGiaTien();
            itemsCount += fnbQuantities[i];
        }

        if (itemsCount == 0) {
            confirmPopup->show("Thông báo", "Vui lòng chọn món F&B trước khi thanh toán!", ButtonStyle::Primary, "Đồng ý", "Đóng");
            return;
        }

        std::ostringstream oss;
        oss << "Xác nhận thanh toán " << itemsCount << " món F&B. Tổng cộng: "
            << std::fixed << std::setprecision(0) << total << " đ?";

        confirmPopup->show("Thanh Toán Bắp Nước", oss.str(), ButtonStyle::Success, "Xác nhận", "Hủy bỏ");
        confirmPopup->setOnConfirm([this, itemsCount, total]() {
            ActivityLog::ghiNhan(currentUserId, currentUserRole, "BAN_FNB", "Ban " + std::to_string(itemsCount) + " mon bap nuoc, thu " + std::to_string((long long)total) + " d");
            setupLogsView();
            for (auto& q : fnbQuantities) q = 0;
        });
    });
}

void CinemaUIApp::setupLogsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    logsTable = std::unique_ptr<EditableTable>(new EditableTable(
        sf::Vector2f(contentX, contentY + 45.f),
        sf::Vector2f(1370.f, 740.f)
    ));

    std::vector<TableColumn> cols = {
        {"Thời Gian", 220.f, false},
        {"Mã Người Dùng", 180.f, false},
        {"Vai Trò", 180.f, false},
        {"Hành Động", 280.f, false},
        {"Chi Tiết Nhật Ký (cinema.db)", 510.f, false}
    };
    logsTable->setColumns(cols);

    // Read activity logs directly from SQLite DATA/cinema.db (nhat_ky_hoat_dong table)
    std::vector<std::vector<std::string>> tRows;
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) == SQLITE_OK && database != nullptr) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT thoiGian,maNguoiDung,vaiTro,hanhDong,chiTiet FROM nhat_ky_hoat_dong ORDER BY rowid";
        if (sqlite3_prepare_v2(database, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::vector<std::string> row;
                for (int i = 0; i < 5; ++i) {
                    const unsigned char* txt = sqlite3_column_text(stmt, i);
                    row.push_back(txt ? reinterpret_cast<const char*>(txt) : "");
                }
                tRows.push_back(row);
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(database);
    }

    if (tRows.empty()) {
        tRows.push_back({"07/11/2026 09:00:00", "U001", "Admin", "DANG_NHAP", "Dang nhap he thong thanh cong"});
    }

    // Display newest logs first
    std::reverse(tRows.begin(), tRows.end());
    logsTable->setRows(tRows);
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

        sf::Vector2f mousePos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
        if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased) {
            mousePos = sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        }

        // ==========================================
        // 1. IF NOT LOGGED IN -> ONLY PROCESS LOGIN
        // ==========================================
        if (!isLoggedIn) {
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

        // Dropdowns top priority
        bool capturedByDropdown = false;
        if (currentTab == ActiveTab::SeatBooking && showtimeDropdown && showtimeDropdown->getExpanded()) {
            if (event.type == sf::Event::MouseButtonPressed) {
                capturedByDropdown = showtimeDropdown->handleMouseButtonPressed(event.mouseButton.button, mousePos);
            }
            if (event.type == sf::Event::MouseMoved) showtimeDropdown->handleMouseMove(mousePos);
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
                        if (event.type == sf::Event::MouseButtonPressed) movieSearchInput->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) movieSearchInput->handleTextEntered(event.text.unicode);
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

                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 24.f;
                        float leftColW = 928.f;
                        float rightColX = sidebarWidth + 968.f;
                        float rightColW = 486.f;
                        float inputX = rightColX + 136.f;

                        // 1. Age Filter Pills in Left Card Filter Bar (Tất cả | P | K | T13 | T18)
                        float filterBarX = leftColX + 422.f;
                        float filterBarY = 248.f;
                        if (mousePos.y >= filterBarY && mousePos.y <= filterBarY + 36.f &&
                            mousePos.x >= filterBarX && mousePos.x <= filterBarX + 266.f) {
                            float relX = mousePos.x - filterBarX;
                            if (relX < 62.f) movieAgeFilter = "";
                            else if (relX < 104.f) movieAgeFilter = "P";
                            else if (relX < 146.f) movieAgeFilter = "K";
                            else if (relX < 204.f) movieAgeFilter = "13";
                            else movieAgeFilter = "18";
                        }

                        // 2. Age Selector Pills inside Right Form (P | K | T13 | T18 at y = 466.f)
                        if (mousePos.y >= 466.f && mousePos.y <= 501.f &&
                            mousePos.x >= inputX && mousePos.x <= inputX + 330.f) {
                            float relX = mousePos.x - inputX;
                            float segW = 330.f / 4.f;
                            int seg = static_cast<int>(relX / segW);
                            const char* ratings[] = {"P", "K", "T13", "T18"};
                            if (seg >= 0 && seg < 4 && inputMovieAgeRating) {
                                inputMovieAgeRating->setText(ratings[seg]);
                            }
                        }

                        // 3. POS Toggle Switch inside Right Form (y = 736.f .. 784.f)
                        if (mousePos.x >= rightColX + 20.f && mousePos.x <= rightColX + rightColW - 20.f &&
                            mousePos.y >= 736.f && mousePos.y <= 784.f) {
                            moviePosActive = !moviePosActive;
                        }

                        // 4. Row selection & Action Icon clicks in Movie Table
                        float rowStartY = 232.f + 64.f + 42.f; // 338.f
                        float rowH = 66.f;
                        if (mousePos.x >= leftColX + 16.f && mousePos.x <= leftColX + leftColW - 16.f &&
                            mousePos.y >= rowStartY && mousePos.y <= rowStartY + 8.f * rowH) {
                            int clickedVisualRow = static_cast<int>((mousePos.y - rowStartY) / rowH);

                            std::string query = movieSearchInput ? movieSearchInput->getText() : "";
                            std::transform(query.begin(), query.end(), query.begin(), ::tolower);

                            int matchCount = 0;
                            int targetIdx = -1;
                            for (size_t i = 0; i < movieList.size(); ++i) {
                                std::string pName = movieList[i].getTenPhim();
                                std::string pCode = movieList[i].getMaPhim();
                                std::string pGenre = db.layTheLoaiCuaPhim(pCode);
                                int pAge = movieList[i].getNhanDoTuoi();

                                if (!movieAgeFilter.empty()) {
                                    if (movieAgeFilter == "P" && pAge > 0) continue;
                                    if (movieAgeFilter == "K" && pAge > 12) continue;
                                    if (movieAgeFilter == "13" && pAge != 13) continue;
                                    if (movieAgeFilter == "18" && pAge < 16) continue;
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
                                if (matchCount == clickedVisualRow) {
                                    targetIdx = static_cast<int>(i);
                                    break;
                                }
                                matchCount++;
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
                                    if (inputMovieAgeRating && inputMovieAgeRating->getText() != curAgeStr) hasFormChanges = true;
                                    if (inputMovieGenre && inputMovieGenre->getText() != curGenre) hasFormChanges = true;
                                    if (inputMoviePoster && inputMoviePoster->getText() != curPoster) hasFormChanges = true;
                                    if (inputMovieDesc && inputMovieDesc->getText() != movieList[targetIdx].getMoTa()) hasFormChanges = true;
                                }

                                // Only trigger backend update when clicking Pencil Edit icon if the user has already edited the form
                                if (clickedEditIcon && selectedMovieRowIdx == targetIdx && hasFormChanges && btnMovieUpdate) {
                                    btnMovieUpdate->triggerClick();
                                } else {
                                    selectedMovieRowIdx = targetIdx;
                                    inputMovieId->setText(movieList[targetIdx].getMaPhim());
                                    inputMovieTitle->setText(movieList[targetIdx].getTenPhim());
                                    inputMovieDuration->setText(std::to_string(movieList[targetIdx].getThoiLuong()));
                                    inputMovieReleaseDate->setText(movieList[targetIdx].getNgayKhoiChieu());
                                    int ageVal = movieList[targetIdx].getNhanDoTuoi();
                                    inputMovieAgeRating->setText(ageVal <= 0 ? "P" : ("T" + std::to_string(ageVal)));
                                    inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[targetIdx].getMaPhim()));
                                    std::string pPath = movieList[targetIdx].getPoster();
                                    if (pPath.empty() || pPath == "assets/posters/poster_default.jpg") {
                                        pPath = "assets/posters/" + movieList[targetIdx].getMaPhim() + ".jpg";
                                    }
                                    if (inputMoviePoster) inputMoviePoster->setText(pPath);
                                    inputMovieDesc->setText(movieList[targetIdx].getMoTa());

                                    if (clickedEditIcon && inputMovieTitle) {
                                        inputMovieTitle->setFocused(true);
                                    } else if (clickedDeleteIcon && btnMovieDelete) {
                                        btnMovieDelete->triggerClick();
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
                handleBtn(btnPosRefresh);
                handleBtn(btnPosDate);
                handleBtn(btnPosMovie);
                handleBtn(btnPosShowtime);
                handleBtn(btnPosAddFnb);
                handleBtn(btnPosCheckout);
                handleBtn(btnPosCancel);

                // Keyboard F5 shortcut for instant checkout
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F5) {
                    if (btnPosCheckout) btnPosCheckout->triggerClick();
                }

                // Interactive Seat Map Clicks
                if (posShowSeatMap && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    for (auto& s : currentSeats) {
                        if (s.trangThai != "DaDat") {
                            if (mousePos.x >= s.position.x && mousePos.x <= s.position.x + s.size.x &&
                                mousePos.y >= s.position.y && mousePos.y <= s.position.y + s.size.y) {
                                s.isSelected = !s.isSelected;
                                if (s.isSelected) {
                                    selectedSeatCodes.push_back(s.maGhe);
                                    posSeatTotal += s.donGia;
                                } else {
                                    for (auto it = selectedSeatCodes.begin(); it != selectedSeatCodes.end(); ++it) {
                                        if (*it == s.maGhe) {
                                            selectedSeatCodes.erase(it);
                                            break;
                                        }
                                    }
                                    posSeatTotal -= s.donGia;
                                }
                                if (posSeatTotal < 0.0) posSeatTotal = 0.0;
                                posGrandTotal = posSeatTotal + posFnbTotal;
                                break;
                            }
                        }
                    }
                }
                break;

            case ActiveTab::CRM:
                handleBtn(btnAddCustomer);
                break;

            case ActiveTab::Reports:
                handleBtn(btnReportAll);
                handleBtn(btnReportCash);
                handleBtn(btnReportBank);
                break;

            case ActiveTab::Rooms:
                handleBtn(btnAddRoom);
                break;

            case ActiveTab::Seats:
                handleBtn(btnAutoGenerateSeats);
                break;

            case ActiveTab::Showtimes:
                handleBtn(btnAddShowtime);
                break;

            case ActiveTab::Concessions:
                handleBtn(btnCheckoutFnb);
                handleBtn(btnAddFnb);

                // F&B Quantity Plus / Minus clicks
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    float startX = sidebarWidth + 30.f;
                    float startY = topbarHeight + 90.f;
                    float cardW = 420.f;
                    float cardH = 110.f;
                    float gap = 20.f;

                    for (size_t i = 0; i < fnbList.size(); ++i) {
                        int row = static_cast<int>(i) / 2;
                        int col = static_cast<int>(i) % 2;
                        float cx = startX + static_cast<float>(col) * (cardW + gap);
                        float cy = startY + static_cast<float>(row) * (cardH + gap);

                        // Minus button
                        float minusX = cx + cardW - 90.f;
                        float btnY = cy + 55.f;
                        if (mousePos.x >= minusX && mousePos.x <= minusX + 28.f &&
                            mousePos.y >= btnY && mousePos.y <= btnY + 28.f) {
                            if (fnbQuantities[i] > 0) fnbQuantities[i]--;
                            break;
                        }

                        // Plus button
                        float plusX = cx + cardW - 38.f;
                        if (mousePos.x >= plusX && mousePos.x <= plusX + 28.f &&
                            mousePos.y >= btnY && mousePos.y <= btnY + 28.f) {
                            fnbQuantities[i]++;
                            break;
                        }
                    }
                }
                break;

            case ActiveTab::ShiftReport:
                handleBtn(btnApproveShift);
                handleBtn(btnCloseShift);
                break;

            case ActiveTab::Promotions:
                handleBtn(btnAddPromo);
                break;

            case ActiveTab::Staff:
                handleBtn(btnAddStaff);
                break;

            case ActiveTab::Accounts:
                handleBtn(btnChangePassword);
                break;

            case ActiveTab::SystemLogs:
                handleBtn(btnRefreshLogs);
                if (logsTable) {
                    if (event.type == sf::Event::MouseMoved) logsTable->handleMouseMove(mousePos);
                    else if (event.type == sf::Event::MouseWheelScrolled) logsTable->handleMouseWheelScrolled(event.mouseWheelScroll.delta, mousePos);
                    else if (event.type == sf::Event::MouseButtonPressed) logsTable->handleMouseButtonPressed(event.mouseButton.button, mousePos);
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

    if (currentTab == ActiveTab::Dashboard && dashboardSlider) {
        dashboardSlider->update(dt);
    }

    if (currentTab == ActiveTab::Movies) {
        if (movieSearchInput) movieSearchInput->update(dt);
        if (inputMovieId) inputMovieId->update(dt);
        if (inputMovieTitle) inputMovieTitle->update(dt);
        if (inputMovieDuration) inputMovieDuration->update(dt);
        if (inputMovieReleaseDate) inputMovieReleaseDate->update(dt);
        if (inputMovieAgeRating) inputMovieAgeRating->update(dt);
        if (inputMovieGenre) inputMovieGenre->update(dt);
        if (inputMoviePoster) inputMoviePoster->update(dt);
        if (inputMovieDesc) inputMovieDesc->update(dt);
        if (inputGenreId) inputGenreId->update(dt);
        if (inputGenreName) inputGenreName->update(dt);
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
    if (currentTab != ActiveTab::SeatBooking && currentTab != ActiveTab::Movies) {
        renderTopBar();
    }

    // Render Floating Dropdowns on top
    if (currentTab == ActiveTab::SeatBooking && showtimeDropdown) {
        showtimeDropdown->drawPopup(window);
    }

    // Modal dialog on topmost layer
    if (confirmPopup) {
        confirmPopup->draw(window);
    }

    window.display();
}

void CinemaUIApp::renderLoginView() {
    // 1. Draw modern clean blue-white canvas (Full Screen 1728x972)
    sf::RectangleShape bg(sf::Vector2f(1728.f, 972.f));
    bg.setFillColor(sf::Color(240, 244, 248)); // #f0f4f8 soft blue-white
    window.draw(bg);

    // Decorative soft blue ambient shapes in background
    sf::CircleShape glow1(380.f);
    glow1.setPosition(80.f, -120.f);
    glow1.setFillColor(sf::Color(224, 242, 254, 180)); // Soft sky blue #e0f2fe
    window.draw(glow1);

    sf::CircleShape glow2(420.f);
    glow2.setPosition(1100.f, 480.f);
    glow2.setFillColor(sf::Color(224, 242, 254, 160));
    window.draw(glow2);

    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f (Centered)
    float cardY = (972.f - cardH) / 2.0f;  // 206.f (Centered)
    float leftW = 420.f;
    float rightW = 520.f;
    float rightX = cardX + leftW + (cardW - leftW - rightW) / 2.0f; // 814.f (Centered in right panel)

    // 2. Drop shadow behind the card
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX - 4.f, cardY + 6.f),
        sf::Vector2f(cardW + 8.f, cardH + 8.f),
        22.f,
        sf::Color(15, 23, 42, 25)
    );

    // 3. Main Crisp White Card Background
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, cardH),
        16.f,
        sf::Color(255, 255, 255),
        1.f,
        sf::Color(226, 232, 240)
    );

    // 4. Left Panel Background: Soft Ice-Blue #f0f9ff
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(leftW, cardH),
        16.f,
        sf::Color(240, 249, 255)
    );
    // Square off the right side of the left panel
    sf::RectangleShape leftFiller(sf::Vector2f(16.f, cardH));
    leftFiller.setPosition(cardX + leftW - 16.f, cardY);
    leftFiller.setFillColor(sf::Color(240, 249, 255));
    window.draw(leftFiller);

    // Left panel right separator line
    sf::RectangleShape leftSep(sf::Vector2f(1.f, cardH));
    leftSep.setPosition(cardX + leftW, cardY);
    leftSep.setFillColor(sf::Color(224, 242, 254));
    window.draw(leftSep);

    // 5. Top Blue Accent Strip
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, 4.f),
        2.f,
        sf::Color(2, 132, 199)
    );

    // ==========================================
    // LEFT PANEL - BRANDING & SECURITY (CRISP)
    // ==========================================
    float leftCenterX = cardX + leftW / 2.0f;

    // 1. Logo Badge (Rounded Square with CINESTAR logo - unchanged)
    float badgeSize = 110.f;
    float badgeX = leftCenterX - badgeSize / 2.0f;
    float badgeY = cardY + 68.f;

    // Drop shadow behind logo badge
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX - 1.f, badgeY + 2.f),
        sf::Vector2f(badgeSize + 2.f, badgeSize + 2.f),
        22.f,
        sf::Color(2, 132, 199, 25)
    );

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX, badgeY),
        sf::Vector2f(badgeSize, badgeSize),
        20.f,
        sf::Color(255, 255, 255),
        2.f,
        sf::Color(2, 132, 199)
    );

    if (cinemaLogoTexture.getSize().x > 0 && cinemaLogoTexture.getSize().y > 0) {
        sf::Vector2u texSz = cinemaLogoTexture.getSize();
        // Crop tightly around the central camera + curved CINESTAR logo
        int cropX = static_cast<int>(texSz.x * 0.24f);
        int cropY = static_cast<int>(texSz.y * 0.23f);
        int cropW = static_cast<int>(texSz.x * 0.52f);
        int cropH = static_cast<int>(texSz.y * 0.52f);
        cinemaLogoSprite.setTextureRect(sf::IntRect(cropX, cropY, cropW, cropH));
        float targetSize = 88.f;
        cinemaLogoSprite.setScale(
            targetSize / static_cast<float>(cropW),
            targetSize / static_cast<float>(cropH)
        );
        cinemaLogoSprite.setPosition(
            badgeX + (badgeSize - targetSize) / 2.0f,
            badgeY + (badgeSize - targetSize) / 2.0f
        );
        window.draw(cinemaLogoSprite);
    }

    // 2. Pill Badge: 🔑 CỔNG ĐIỀU HÀNH CỤM RẠP (perfectly centered icon + text)
    sf::Text pillText;
    pillText.setFont(Theme::FontPrimary);
    pillText.setCharacterSize(13);
    pillText.setStyle(sf::Text::Bold);
    pillText.setFillColor(sf::Color(3, 105, 161));
    pillText.setString(toSfString("CỔNG ĐIỀU HÀNH CỤM RẠP"));
    sf::FloatRect ptB = pillText.getLocalBounds();

    float iconW = 16.f;
    float iconGap = 9.f;
    float groupW = iconW + iconGap + ptB.width;
    float pillW = groupW + 38.f;
    float pillH = 34.f;
    float pillX = std::round(leftCenterX - pillW / 2.0f);
    float pillY = cardY + 206.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(pillX, pillY),
        sf::Vector2f(pillW, pillH),
        17.f,
        sf::Color(224, 242, 254)
    );

    float groupStartX = std::round(pillX + (pillW - groupW) / 2.0f);
    // Key icon inside pill
    sf::CircleShape keyPill(4.5f);
    keyPill.setPosition(groupStartX, pillY + (pillH - 9.f) / 2.0f);
    keyPill.setFillColor(sf::Color(2, 132, 199));
    window.draw(keyPill);

    sf::RectangleShape keyPillStem(sf::Vector2f(8.5f, 2.5f));
    keyPillStem.setPosition(groupStartX + 6.5f, pillY + (pillH - 2.5f) / 2.0f);
    keyPillStem.setFillColor(sf::Color(2, 132, 199));
    window.draw(keyPillStem);

    pillText.setPosition(
        std::round(groupStartX + iconW + iconGap - ptB.left),
        std::round(pillY + (pillH - ptB.height) / 2.0f - ptB.top)
    );
    window.draw(pillText);

    // 3. Brand Text: CINESTAR & Subtitle centered in the middle
    sf::Text brandText;
    brandText.setFont(Theme::FontCinema);
    brandText.setCharacterSize(54);
    brandText.setStyle(sf::Text::Bold);
    brandText.setFillColor(sf::Color(15, 23, 42));
    brandText.setString("CINESTAR");
    sf::FloatRect b1 = brandText.getLocalBounds();
    brandText.setPosition(
        std::round(leftCenterX - (b1.left + b1.width / 2.0f)),
        cardY + 262.f
    );
    window.draw(brandText);

    sf::Text brandSub;
    brandSub.setFont(Theme::FontPrimary);
    brandSub.setCharacterSize(14);
    brandSub.setStyle(sf::Text::Bold);
    brandSub.setFillColor(sf::Color(110, 124, 142));
    brandSub.setString(toSfString("HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM"));
    sf::FloatRect bs = brandSub.getLocalBounds();
    brandSub.setPosition(
        std::round(leftCenterX - (bs.left + bs.width / 2.0f)),
        cardY + 340.f
    );
    window.draw(brandSub);

    // 4. Bottom Security Shield Badge Card (matching reference image)
    float secW = 364.f;
    float secH = 72.f;
    float secX = std::round(leftCenterX - secW / 2.0f);
    float secY = cardY + 448.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(secX, secY),
        sf::Vector2f(secW, secH),
        16.f,
        sf::Color(255, 255, 255),
        1.5f,
        sf::Color(210, 236, 252)
    );

    // Soft ice-blue rounded square icon container on the left
    float iconBoxSize = 40.f;
    float iconBoxX = secX + 16.f;
    float iconBoxY = std::round(secY + (secH - iconBoxSize) / 2.0f);
    sf::Color iconBoxBg(235, 248, 255);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(iconBoxX, iconBoxY),
        sf::Vector2f(iconBoxSize, iconBoxSize),
        10.f,
        iconBoxBg,
        1.f,
        sf::Color(205, 234, 254)
    );

    // 16x Supersampled Smooth Shield + Checkmark Icon Texture
    static sf::Texture shieldTex;
    static bool shieldTexReady = false;
    if (!shieldTexReady) {
        const int texS = 40;
        sf::Image shImg;
        shImg.create(texS, texS, sf::Color(235, 248, 255, 0));

        auto inShield = [](float x, float y, float sc) -> bool {
            x /= sc;
            y /= sc;
            float yTop = -10.2f + 0.042f * x * x;
            if (y < yTop || y > 11.2f) return false;
            float maxW = 9.4f;
            if (y > -6.5f) {
                float t = (y + 6.5f) / (11.2f + 6.5f);
                if (t > 1.f) return false;
                maxW = 9.4f * std::pow(std::max(0.f, 1.f - t * t), 0.58f);
            }
            return std::fabs(x) <= maxW;
        };

        auto distSeg = [](float px, float py, float ax, float ay, float bx, float by) -> float {
            float vx = bx - ax, vy = by - ay;
            float wx = px - ax, wy = py - ay;
            float c1 = vx * wx + vy * wy;
            if (c1 <= 0.f) return std::sqrt(wx * wx + wy * wy);
            float c2 = vx * vx + vy * vy;
            if (c2 <= c1) return std::sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
            float b = c1 / c2;
            float pbx = ax + b * vx, pby = ay + b * vy;
            return std::sqrt((px - pbx) * (px - pbx) + (py - pby) * (py - pby));
        };

        for (int py = 0; py < texS; ++py) {
            for (int px = 0; px < texS; ++px) {
                int hits = 0;
                for (int sy = 0; sy < 4; ++sy) {
                    for (int sx = 0; sx < 4; ++sx) {
                        float fx = (px + (sx + 0.5f) * 0.25f) - texS * 0.5f;
                        float fy = (py + (sy + 0.5f) * 0.25f) - texS * 0.5f;
                        bool stroke = inShield(fx, fy, 1.0f) && !inShield(fx, fy, 0.78f);
                        float d1 = distSeg(fx, fy, -3.6f, 0.2f, -1.0f, 2.8f);
                        float d2 = distSeg(fx, fy, -1.0f, 2.8f,  4.0f, -2.4f);
                        bool check = (d1 <= 1.05f || d2 <= 1.05f);
                        if (stroke || check) hits++;
                    }
                }
                if (hits > 0) {
                    sf::Uint8 alpha = static_cast<sf::Uint8>((hits * 255) / 16);
                    shImg.setPixel(px, py, sf::Color(2, 132, 199, alpha));
                }
            }
        }
        shieldTex.loadFromImage(shImg);
        shieldTex.setSmooth(true);
        shieldTexReady = true;
    }

    sf::Sprite shieldSpr(shieldTex);
    shieldSpr.setPosition(iconBoxX, iconBoxY);
    window.draw(shieldSpr);

    float secTextX = iconBoxX + iconBoxSize + 14.f;
    float maxSecTextW = secW - (secTextX - secX) - 16.f;

    sf::Text secText1;
    secText1.setFont(Theme::FontPrimary);
    secText1.setCharacterSize(13);
    secText1.setFillColor(sf::Color(100, 116, 139));
    secText1.setString(toSfString("Hệ thống bảo mật nội bộ dành cho cán bộ &"));
    sf::FloatRect st1B = secText1.getLocalBounds();
    if (st1B.width > maxSecTextW && st1B.width > 0.f) {
        secText1.setScale(maxSecTextW / st1B.width, 1.f);
    }
    secText1.setPosition(std::round(secTextX), std::round(secY + 17.f));
    window.draw(secText1);

    sf::Text secText2;
    secText2.setFont(Theme::FontPrimary);
    secText2.setCharacterSize(13);
    secText2.setFillColor(sf::Color(100, 116, 139));
    secText2.setString(toSfString("nhân viên quản trị Cinestar"));
    secText2.setScale(secText1.getScale());
    secText2.setPosition(std::round(secTextX), std::round(secY + 38.f));
    window.draw(secText2);

    // ==========================================
    // RIGHT PANEL - FORM & INPUTS (CRISP WHITE)
    // ==========================================

    // 1. Heading
    sf::Text rTitle;
    rTitle.setFont(Theme::FontPrimary);
    rTitle.setCharacterSize(30);
    rTitle.setStyle(sf::Text::Bold);
    rTitle.setFillColor(sf::Color(15, 23, 42));
    rTitle.setString(toSfString("Đăng nhập hệ thống"));
    rTitle.setPosition(rightX, cardY + 70.f);
    window.draw(rTitle);

    sf::Text rSub;
    rSub.setFont(Theme::FontPrimary);
    rSub.setCharacterSize(15);
    rSub.setFillColor(sf::Color(100, 116, 139));
    rSub.setString(toSfString("Vui lòng xác thực tài khoản được cấp quyền quản trị cụm rạp"));
    rSub.setPosition(rightX, cardY + 114.f);
    window.draw(rSub);

    // 2. Field 1: TÀI KHOẢN NHÂN SỰ
    sf::RectangleShape idIconBg(sf::Vector2f(15.f, 13.f));
    idIconBg.setPosition(rightX, cardY + 170.f);
    idIconBg.setFillColor(sf::Color(2, 132, 199));
    window.draw(idIconBg);

    sf::Text idLabel;
    idLabel.setFont(Theme::FontPrimary);
    idLabel.setCharacterSize(14);
    idLabel.setStyle(sf::Text::Bold);
    idLabel.setFillColor(sf::Color(71, 85, 105));
    idLabel.setString(toSfString("TÀI KHOẢN NHÂN SỰ"));
    idLabel.setPosition(rightX + 22.f, cardY + 166.f);
    window.draw(idLabel);

    if (loginInputUser) {
        loginInputUser->draw(window);
        // Sharp inside user avatar icon 👤 (centered vertically inside 54px box)
        float uIconX = rightX + 18.f;
        float uIconY = cardY + 196.f + 19.f;

        sf::CircleShape uRing(9.f);
        uRing.setOrigin(9.f, 9.f);
        uRing.setPosition(uIconX + 8.f, uIconY + 8.f);
        uRing.setFillColor(sf::Color::Transparent);
        uRing.setOutlineThickness(1.5f);
        uRing.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(uRing);

        sf::CircleShape uHead(2.8f);
        uHead.setOrigin(2.8f, 2.8f);
        uHead.setPosition(uIconX + 8.f, uIconY + 5.f);
        uHead.setFillColor(sf::Color(148, 163, 184));
        window.draw(uHead);

        sf::CircleShape uBody(4.5f);
        uBody.setOrigin(4.5f, 4.5f);
        uBody.setScale(1.f, 0.5f);
        uBody.setPosition(uIconX + 8.f, uIconY + 12.f);
        uBody.setFillColor(sf::Color(148, 163, 184));
        window.draw(uBody);
    }

    // 3. Field 2: MẬT KHẨU TRUY CẬP
    sf::RectangleShape lockBody(sf::Vector2f(13.f, 10.f));
    lockBody.setPosition(rightX, cardY + 274.f);
    lockBody.setFillColor(sf::Color(2, 132, 199));
    window.draw(lockBody);

    sf::CircleShape lockShackle(4.f);
    lockShackle.setPosition(rightX + 2.5f, cardY + 269.f);
    lockShackle.setFillColor(sf::Color::Transparent);
    lockShackle.setOutlineThickness(1.5f);
    lockShackle.setOutlineColor(sf::Color(2, 132, 199));
    window.draw(lockShackle);

    sf::Text passLabel;
    passLabel.setFont(Theme::FontPrimary);
    passLabel.setCharacterSize(14);
    passLabel.setStyle(sf::Text::Bold);
    passLabel.setFillColor(sf::Color(71, 85, 105));
    passLabel.setString(toSfString("MẬT KHẨU TRUY CẬP"));
    passLabel.setPosition(rightX + 22.f, cardY + 270.f);
    window.draw(passLabel);

    if (loginInputPass) {
        loginInputPass->draw(window);
        // Sharp inside key icon 🔑 (centered vertically inside 54px box)
        float kIconX = rightX + 18.f;
        float kIconY = cardY + 300.f + 21.f;

        sf::CircleShape kHead(4.5f);
        kHead.setOrigin(4.5f, 4.5f);
        kHead.setPosition(kIconX + 6.f, kIconY + 6.f);
        kHead.setFillColor(sf::Color::Transparent);
        kHead.setOutlineThickness(1.5f);
        kHead.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(kHead);

        sf::RectangleShape kStem(sf::Vector2f(9.f, 2.f));
        kStem.setPosition(kIconX + 10.5f, kIconY + 5.f);
        kStem.setFillColor(sf::Color(148, 163, 184));
        window.draw(kStem);

        sf::RectangleShape kTooth(sf::Vector2f(2.f, 4.f));
        kTooth.setPosition(kIconX + 17.f, kIconY + 6.f);
        kTooth.setFillColor(sf::Color(148, 163, 184));
        window.draw(kTooth);
    }

    // 4. Remember Me Checkbox (without "Quên mật khẩu?")
    if (chkRememberMe) {
        chkRememberMe->draw(window);
    }

    // 5. Login Button: ĐĂNG NHẬP HỆ THỐNG ➔
    if (btnLogin) {
        btnLogin->draw(window);
    }

    // 6. Error message (if any)
    if (!loginErrorMsg.empty()) {
        sf::Text errText;
        errText.setFont(Theme::FontPrimary);
        errText.setCharacterSize(14);
        errText.setStyle(sf::Text::Bold);
        errText.setFillColor(sf::Color(220, 38, 38));
        errText.setString(toSfString(loginErrorMsg));
        errText.setPosition(rightX, cardY + 482.f);
        window.draw(errText);
    }
}

void CinemaUIApp::drawSidebarMenuIcon(sf::RenderTarget& window, int index, sf::Vector2f pos, sf::Color color) {
    switch (index) {
        case 0: { // 1. Bán vé (POS) - Ticket icon
            sf::RectangleShape ticket(sf::Vector2f(16.f, 11.f));
            ticket.setPosition(pos.x, pos.y + 2.f);
            ticket.setFillColor(sf::Color::Transparent);
            ticket.setOutlineThickness(1.3f);
            ticket.setOutlineColor(color);
            window.draw(ticket);

            sf::RectangleShape dash(sf::Vector2f(4.f, 1.5f));
            dash.setPosition(pos.x + 6.f, pos.y + 7.f);
            dash.setFillColor(color);
            window.draw(dash);
            break;
        }
        case 1: { // 2. Khách hàng (CRM) - User outline icon
            sf::CircleShape head(3.5f);
            head.setPosition(pos.x + 4.5f, pos.y + 0.5f);
            head.setFillColor(sf::Color::Transparent);
            head.setOutlineThickness(1.3f);
            head.setOutlineColor(color);
            window.draw(head);

            sf::CircleShape body(5.5f);
            body.setScale(1.f, 0.45f);
            body.setPosition(pos.x + 2.5f, pos.y + 8.5f);
            body.setFillColor(sf::Color::Transparent);
            body.setOutlineThickness(1.3f);
            body.setOutlineColor(color);
            window.draw(body);
            break;
        }
        case 2: { // 3. Quản lý Doanh thu - Wallet / Card
            sf::RectangleShape wallet(sf::Vector2f(16.f, 11.f));
            wallet.setPosition(pos.x, pos.y + 2.f);
            wallet.setFillColor(sf::Color::Transparent);
            wallet.setOutlineThickness(1.3f);
            wallet.setOutlineColor(color);
            window.draw(wallet);

            sf::CircleShape snap(1.5f);
            snap.setPosition(pos.x + 11.f, pos.y + 6.5f);
            snap.setFillColor(color);
            window.draw(snap);
            break;
        }
        case 3: { // 4. Quản lý phòng - Monitor / Cinema Screen
            sf::RectangleShape screen(sf::Vector2f(16.f, 10.f));
            screen.setPosition(pos.x, pos.y + 1.5f);
            screen.setFillColor(sf::Color::Transparent);
            screen.setOutlineThickness(1.3f);
            screen.setOutlineColor(color);
            window.draw(screen);

            sf::RectangleShape stand(sf::Vector2f(6.f, 1.5f));
            stand.setPosition(pos.x + 5.f, pos.y + 13.5f);
            stand.setFillColor(color);
            window.draw(stand);
            break;
        }
        case 4: { // 5. Quản lý ghế - 2x2 grid
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 2; ++c) {
                    sf::RectangleShape cell(sf::Vector2f(5.f, 5.f));
                    cell.setPosition(pos.x + c * 7.5f + 1.f, pos.y + r * 7.5f + 1.f);
                    cell.setFillColor(sf::Color::Transparent);
                    cell.setOutlineThickness(1.2f);
                    cell.setOutlineColor(color);
                    window.draw(cell);
                }
            }
            break;
        }
        case 5: { // 6. Quản lý suất chiếu - Clock
            sf::CircleShape clock(6.5f);
            clock.setPosition(pos.x + 1.5f, pos.y + 1.5f);
            clock.setFillColor(sf::Color::Transparent);
            clock.setOutlineThickness(1.3f);
            clock.setOutlineColor(color);
            window.draw(clock);

            sf::VertexArray hands(sf::Lines, 4);
            hands[0].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.f);
            hands[1].position = sf::Vector2f(pos.x + 8.f, pos.y + 4.5f);
            hands[2].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.f);
            hands[3].position = sf::Vector2f(pos.x + 11.f, pos.y + 8.f);
            for (int h = 0; h < 4; ++h) hands[h].color = color;
            window.draw(hands);
            break;
        }
        case 6: { // 7. Phim & Thể loại - Film strip
            sf::RectangleShape film(sf::Vector2f(16.f, 12.f));
            film.setPosition(pos.x, pos.y + 2.f);
            film.setFillColor(sf::Color::Transparent);
            film.setOutlineThickness(1.3f);
            film.setOutlineColor(color);
            window.draw(film);

            sf::RectangleShape hole1(sf::Vector2f(3.f, 2.f));
            hole1.setPosition(pos.x + 3.f, pos.y + 7.f);
            hole1.setFillColor(color);
            window.draw(hole1);

            sf::RectangleShape hole2(sf::Vector2f(3.f, 2.f));
            hole2.setPosition(pos.x + 10.f, pos.y + 7.f);
            hole2.setFillColor(color);
            window.draw(hole2);
            break;
        }
        case 7: { // 8. Quản lý F&B - Cup / Popcorn
            sf::ConvexShape cup(4);
            cup.setPoint(0, sf::Vector2f(pos.x + 2.f, pos.y + 2.5f));
            cup.setPoint(1, sf::Vector2f(pos.x + 14.f, pos.y + 2.5f));
            cup.setPoint(2, sf::Vector2f(pos.x + 11.5f, pos.y + 14.f));
            cup.setPoint(3, sf::Vector2f(pos.x + 4.5f, pos.y + 14.f));
            cup.setFillColor(sf::Color::Transparent);
            cup.setOutlineThickness(1.3f);
            cup.setOutlineColor(color);
            window.draw(cup);

            sf::RectangleShape stripe(sf::Vector2f(1.5f, 9.f));
            stripe.setPosition(pos.x + 7.25f, pos.y + 4.f);
            stripe.setFillColor(color);
            window.draw(stripe);
            break;
        }
        case 8: { // 9. Quản lý Báo cáo ca - Clipboard
            sf::RectangleShape clip(sf::Vector2f(13.f, 14.f));
            clip.setPosition(pos.x + 1.5f, pos.y + 2.f);
            clip.setFillColor(sf::Color::Transparent);
            clip.setOutlineThickness(1.3f);
            clip.setOutlineColor(color);
            window.draw(clip);

            sf::RectangleShape topTab(sf::Vector2f(5.f, 2.5f));
            topTab.setPosition(pos.x + 5.5f, pos.y + 0.5f);
            topTab.setFillColor(color);
            window.draw(topTab);

            sf::RectangleShape line1(sf::Vector2f(7.f, 1.3f));
            line1.setPosition(pos.x + 4.5f, pos.y + 6.5f);
            line1.setFillColor(color);
            window.draw(line1);

            sf::RectangleShape line2(sf::Vector2f(7.f, 1.3f));
            line2.setPosition(pos.x + 4.5f, pos.y + 9.5f);
            line2.setFillColor(color);
            window.draw(line2);
            break;
        }
        case 9: { // 10. Khuyến mãi SK - Gift box
            sf::RectangleShape gift(sf::Vector2f(14.f, 11.f));
            gift.setPosition(pos.x + 1.f, pos.y + 3.5f);
            gift.setFillColor(sf::Color::Transparent);
            gift.setOutlineThickness(1.3f);
            gift.setOutlineColor(color);
            window.draw(gift);

            sf::RectangleShape ribH(sf::Vector2f(14.f, 1.5f));
            ribH.setPosition(pos.x + 1.f, pos.y + 8.f);
            ribH.setFillColor(color);
            window.draw(ribH);

            sf::RectangleShape ribV(sf::Vector2f(1.5f, 11.f));
            ribV.setPosition(pos.x + 7.25f, pos.y + 3.5f);
            ribV.setFillColor(color);
            window.draw(ribV);
            break;
        }
        case 10: { // 11. Quản lý nhân sự - Group
            sf::CircleShape h2(3.f);
            h2.setPosition(pos.x + 8.5f, pos.y + 1.f);
            h2.setFillColor(color);
            window.draw(h2);

            sf::CircleShape h1(3.5f);
            h1.setPosition(pos.x + 2.f, pos.y + 2.5f);
            h1.setFillColor(sf::Color::Transparent);
            h1.setOutlineThickness(1.3f);
            h1.setOutlineColor(color);
            window.draw(h1);

            sf::CircleShape b1(5.f);
            b1.setScale(1.f, 0.45f);
            b1.setPosition(pos.x + 0.5f, pos.y + 9.5f);
            b1.setFillColor(sf::Color::Transparent);
            b1.setOutlineThickness(1.3f);
            b1.setOutlineColor(color);
            window.draw(b1);
            break;
        }
        case 11: { // 12. Quản lý tài khoản - Gear
            sf::CircleShape gear(6.f);
            gear.setPosition(pos.x + 2.f, pos.y + 2.5f);
            gear.setFillColor(sf::Color::Transparent);
            gear.setOutlineThickness(1.8f);
            gear.setOutlineColor(color);
            window.draw(gear);

            sf::CircleShape center(1.5f);
            center.setPosition(pos.x + 6.5f, pos.y + 7.f);
            center.setFillColor(color);
            window.draw(center);
            break;
        }
        case 12: { // 13. Nhật ký hoạt động - History / Clock
            sf::CircleShape logClock(6.5f);
            logClock.setPosition(pos.x + 1.5f, pos.y + 2.f);
            logClock.setFillColor(sf::Color::Transparent);
            logClock.setOutlineThickness(1.3f);
            logClock.setOutlineColor(color);
            window.draw(logClock);

            sf::VertexArray lHands(sf::Lines, 4);
            lHands[0].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.5f);
            lHands[1].position = sf::Vector2f(pos.x + 8.f, pos.y + 5.f);
            lHands[2].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.5f);
            lHands[3].position = sf::Vector2f(pos.x + 11.f, pos.y + 8.5f);
            for (int h = 0; h < 4; ++h) lHands[h].color = color;
            window.draw(lHands);
            break;
        }
    }
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
    logoText.setCharacterSize(24);
    logoText.setStyle(sf::Text::Bold);
    logoText.setFillColor(sf::Color(6, 182, 212)); // Cyan branding
    logoText.setString(toSfString("CINESTAR"));
    logoText.setPosition(22.f, 16.f);
    window.draw(logoText);

    sf::Text subLogo;
    subLogo.setFont(Theme::FontPrimary);
    subLogo.setCharacterSize(10);
    subLogo.setFillColor(Theme::TextSecondary);
    subLogo.setString(toSfString("HỆ THỐNG QUẢN LÝ RẠP"));
    subLogo.setPosition(22.f, 44.f);
    window.draw(subLogo);

    // Menu Items - Admin excludes SeatBooking (POS) and CRM as requested
    std::vector<std::pair<std::string, ActiveTab>> menuItems;
    if (currentUserRole == "Admin" || currentUserRole == "Khách") {
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
            {"Quản lý Doanh thu",    ActiveTab::Reports},
            {"Quản lý Báo cáo ca",   ActiveTab::ShiftReport},
            {"Quản lý tài khoản",    ActiveTab::Accounts},
            {"Nhật ký hoạt động",    ActiveTab::SystemLogs}
        };
    }

    float startY = 72.f;
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

void CinemaUIApp::renderDashboardView() {
    // 4 KPI Stat Cards
    for (auto& card : dashboardStats) {
        card->draw(window);
    }

    // Featured Movies Poster Slider
    if (dashboardSlider) {
        dashboardSlider->draw(window);
    }

    // Quick Stats Overview Panel
    float overviewX = sidebarWidth + 30.f;
    float overviewY = topbarHeight + 510.f;
    float overviewW = 1370.f;
    float overviewH = 360.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(overviewX, overviewY),
        sf::Vector2f(overviewW, overviewH),
        Theme::RadiusLarge,
        Theme::CardBg,
        1.0f,
        Theme::CardBorder
    );

    sf::Text panelTitle;
    panelTitle.setFont(Theme::FontTitle);
    panelTitle.setCharacterSize(Theme::SizeHeader);
    panelTitle.setFillColor(Theme::TextPrimary);
    panelTitle.setString(toSfString("HOẠT ĐỘNG PHÒNG VÉ & TRẠNG THÁI RẠP CHIẾU"));
    panelTitle.setPosition(overviewX + 24.f, overviewY + 20.f);
    window.draw(panelTitle);

    // Room status cards from DATA/quan_ly_phong_ghe.csv
    float roomCardW = 300.f;
    float roomCardH = 250.f;
    float gap = 24.f;

    for (size_t i = 0; i < roomList.size() && i < 4; ++i) {
        float rx = overviewX + 24.f + static_cast<float>(i) * (roomCardW + gap);
        float ry = overviewY + 65.f;

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rx, ry),
            sf::Vector2f(roomCardW, roomCardH),
            Theme::RadiusMedium,
            Theme::BgDark,
            1.0f,
            Theme::CardBorder
        );

        sf::Text rName;
        rName.setFont(Theme::FontPrimary);
        rName.setCharacterSize(Theme::SizeSubheader);
        rName.setStyle(sf::Text::Bold);
        rName.setFillColor(Theme::Cyan);
        rName.setString(toSfString(roomList[i].getTenPhong()));
        rName.setPosition(rx + 18.f, ry + 18.f);
        window.draw(rName);

        sf::Text rType;
        rType.setFont(Theme::FontPrimary);
        rType.setCharacterSize(Theme::SizeBody);
        rType.setFillColor(Theme::TextSecondary);
        rType.setString(toSfString("Loại phòng: " + roomList[i].getLoaiPhong() + " | Sức chứa: " + std::to_string(roomList[i].getSucChua()) + " chỗ"));
        rType.setPosition(rx + 18.f, ry + 50.f);
        window.draw(rType);

        sf::Text rStatus;
        rStatus.setFont(Theme::FontPrimary);
        rStatus.setCharacterSize(Theme::SizeBody);
        rStatus.setFillColor(Theme::Success);
        rStatus.setString(toSfString("Trạng thái: " + roomList[i].getTrangThaiPhong()));
        rStatus.setPosition(rx + 18.f, ry + 80.f);
        window.draw(rStatus);

        // Progress bar
        float percent = 0.65f + static_cast<float>(i) * 0.08f;
        RoundedRectRenderer::render(window, sf::Vector2f(rx + 18.f, ry + 130.f), sf::Vector2f(260.f, 10.f), 5.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(rx + 18.f, ry + 130.f), sf::Vector2f(260.f * percent, 10.f), 5.f, Theme::Primary);
    }
}

void CinemaUIApp::renderMoviesView() {
    float contentAreaW = 1728.f - sidebarWidth;

    // 1. Soft slate-white canvas background (#f8fafc) matching reference
    sf::RectangleShape mBg(sf::Vector2f(contentAreaW, 972.f));
    mBg.setPosition(sidebarWidth, 0.f);
    mBg.setFillColor(sf::Color(248, 250, 252));
    window.draw(mBg);

    // 2. Top Utility Strip (White bar with hub breadcrumb & server online badge)
    sf::RectangleShape topStrip(sf::Vector2f(contentAreaW, 44.f));
    topStrip.setPosition(sidebarWidth, 0.f);
    topStrip.setFillColor(sf::Color::White);
    window.draw(topStrip);

    sf::RectangleShape topStripLine(sf::Vector2f(contentAreaW, 1.f));
    topStripLine.setPosition(sidebarWidth, 44.f);
    topStripLine.setFillColor(sf::Color(226, 232, 240));
    window.draw(topStripLine);

    sf::Text hubTxt;
    hubTxt.setFont(Theme::FontPrimary);
    hubTxt.setCharacterSize(13);
    hubTxt.setStyle(sf::Text::Bold);
    hubTxt.setFillColor(sf::Color(30, 41, 59));
    hubTxt.setString(toSfString("Cinestar Quốc Thanh (Hub 01)  /  Cụm Rạp Trung Tâm"));
    hubTxt.setPosition(sidebarWidth + 24.f, 13.f);
    window.draw(hubTxt);

    // Server Online pill on top right
    RoundedRectRenderer::render(window, sf::Vector2f(1728.f - 185.f, 9.f), sf::Vector2f(125.f, 26.f), 13.f, sf::Color(240, 253, 244), 1.f, sf::Color(187, 247, 208));
    sf::CircleShape onlineDot(4.f);
    onlineDot.setFillColor(sf::Color(22, 163, 74));
    onlineDot.setPosition(1728.f - 173.f, 18.f);
    window.draw(onlineDot);

    sf::Text onlineTxt;
    onlineTxt.setFont(Theme::FontPrimary);
    onlineTxt.setCharacterSize(12);
    onlineTxt.setStyle(sf::Text::Bold);
    onlineTxt.setFillColor(sf::Color(21, 128, 61));
    onlineTxt.setString(toSfString("Máy chủ: Online"));
    onlineTxt.setPosition(1728.f - 160.f, 14.f);
    window.draw(onlineTxt);

    // User avatar circle on top right
    sf::CircleShape userAv(14.f);
    userAv.setFillColor(sf::Color(3, 105, 161));
    userAv.setPosition(1728.f - 48.f, 8.f);
    window.draw(userAv);

    // 3. Main Page Heading (Breadcrumb removed as requested)
    float leftColX = sidebarWidth + 24.f;
    float leftColW = 928.f;
    float rightColX = sidebarWidth + 968.f;
    float rightColW = 486.f;

    sf::Text hTitle;
    hTitle.setFont(Theme::FontPrimary);
    hTitle.setCharacterSize(24);
    hTitle.setStyle(sf::Text::Bold);
    hTitle.setFillColor(sf::Color(15, 23, 42));
    hTitle.setString(toSfString("QUẢN LÝ PHIM & THỂ LOẠI"));
    hTitle.setPosition(leftColX, 62.f);
    window.draw(hTitle);

    sf::Text hSub;
    hSub.setFont(Theme::FontPrimary);
    hSub.setCharacterSize(13);
    hSub.setFillColor(sf::Color(100, 116, 139));
    hSub.setString(toSfString("Quản lý danh sách phim, thông số kiểm duyệt, thông tin chi tiết và danh mục phân loại chiếu rạp"));
    hSub.setPosition(leftColX, 96.f);
    window.draw(hSub);

    // 4. Four Status Summary Pills
    int todayKey = 0;
    DateFormat::todayDateKey(todayKey);
    int nowShowingCount = 0;
    int comingSoonCount = 0;
    for (const auto& m : movieList) {
        int rKey = DateFormat::dateKey(DateFormat::normalizeDate(m.getNgayKhoiChieu()));
        if (todayKey > 0 && rKey > todayKey) comingSoonCount++;
        else nowShowingCount++;
    }

    auto drawStatPill = [&](float px, float py, float pw, const std::string& label, sf::Color bgCol, sf::Color borderCol, sf::Color txtCol, sf::Color dotCol, bool hasDot) {
        RoundedRectRenderer::render(window, sf::Vector2f(px, py), sf::Vector2f(pw, 30.f), 8.f, bgCol, 1.f, borderCol);
        float textOffsetX = px + 12.f;
        if (hasDot) {
            sf::CircleShape d(4.f);
            d.setFillColor(dotCol);
            d.setPosition(px + 12.f, py + 11.f);
            window.draw(d);
            textOffsetX = px + 24.f;
        }
        sf::Text pt;
        pt.setFont(Theme::FontPrimary);
        pt.setCharacterSize(13);
        pt.setStyle(sf::Text::Bold);
        pt.setFillColor(txtCol);
        pt.setString(toSfString(label));
        pt.setPosition(textOffsetX, py + 6.f);
        window.draw(pt);
    };

    float pillY = 134.f;
    drawStatPill(leftColX, pillY, 135.f, "Tổng số: " + std::to_string(movieList.size()) + " phim", sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(3, 105, 161), true);
    drawStatPill(leftColX + 145.f, pillY, 130.f, "Đang chiếu: " + std::to_string(nowShowingCount), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(3, 105, 161), true);
    drawStatPill(leftColX + 285.f, pillY, 125.f, "Sắp chiếu: " + std::to_string(comingSoonCount), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(15, 23, 42), sf::Color(71, 85, 105), true);
    drawStatPill(leftColX + 420.f, pillY, 145.f, "CMS: Trực tuyến", sf::Color(220, 252, 231), sf::Color(187, 247, 208), sf::Color(21, 128, 61), sf::Color(22, 163, 74), true);

    // 5. Sub-tabs Bar (y = 180.f, height = 40.f, matching reference image)
    float tabW = 205.f;
    float tabH = 40.f;
    float tabY = 180.f;
    float tab1X = leftColX;
    float tab2X = leftColX + 215.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab1X, tabY),
        sf::Vector2f(tabW, tabH),
        10.f,
        (movieSubTab == 0) ? sf::Color::White : sf::Color(248, 250, 252),
        1.5f,
        (movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(tab2X, tabY),
        sf::Vector2f(tabW + 15.f, tabH),
        10.f,
        (movieSubTab == 1) ? sf::Color::White : sf::Color(248, 250, 252),
        1.5f,
        (movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    if (btnSubTabMovies) btnSubTabMovies->draw(window);
    if (btnSubTabGenres) {
        btnSubTabGenres->setSize(sf::Vector2f(tabW + 15.f, tabH));
        btnSubTabGenres->draw(window);
    }

    // Tab 1: "Quản lý Phim" + [07] badge centered together
    sf::Text tab1Txt;
    tab1Txt.setFont(Theme::FontPrimary);
    tab1Txt.setCharacterSize(15);
    tab1Txt.setStyle(sf::Text::Bold);
    tab1Txt.setFillColor((movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tab1Txt.setString(toSfString("Quản lý Phim"));
    sf::FloatRect t1Bounds = tab1Txt.getLocalBounds();

    float badgeW = 30.f;
    float badgeH = 22.f;
    float groupGap = 8.f;
    float totalGroupW = t1Bounds.width + groupGap + badgeW;
    float groupStartX = tab1X + (tabW - totalGroupW) / 2.f;

    tab1Txt.setPosition(
        std::round(groupStartX - t1Bounds.left),
        std::round(tabY + (tabH - t1Bounds.height) / 2.f - t1Bounds.top)
    );
    window.draw(tab1Txt);

    float badgeX = std::round(groupStartX + t1Bounds.width + groupGap);
    float badgeY = std::round(tabY + (tabH - badgeH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX, badgeY),
        sf::Vector2f(badgeW, badgeH),
        11.f,
        (movieSubTab == 0) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    sf::Text cntBadge;
    cntBadge.setFont(Theme::FontPrimary);
    cntBadge.setCharacterSize(12);
    cntBadge.setStyle(sf::Text::Bold);
    cntBadge.setFillColor((movieSubTab == 0) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream cntOss;
    cntOss << std::setw(2) << std::setfill('0') << movieList.size();
    cntBadge.setString(cntOss.str());
    sf::FloatRect cbBounds = cntBadge.getLocalBounds();
    cntBadge.setPosition(
        std::round(badgeX + (badgeW - cbBounds.width) / 2.f - cbBounds.left),
        std::round(badgeY + (badgeH - cbBounds.height) / 2.f - cbBounds.top)
    );
    window.draw(cntBadge);

    // Tab 2: "Quản lý Thể loại" + [09] badge centered together
    sf::Text tab2Txt;
    tab2Txt.setFont(Theme::FontPrimary);
    tab2Txt.setCharacterSize(15);
    tab2Txt.setStyle(sf::Text::Bold);
    tab2Txt.setFillColor((movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(100, 116, 139));
    tab2Txt.setString(toSfString("Quản lý Thể loại"));
    sf::FloatRect t2Bounds = tab2Txt.getLocalBounds();

    float tab2ActualW = tabW + 15.f;
    float totalGroup2W = t2Bounds.width + groupGap + badgeW;
    float group2StartX = tab2X + (tab2ActualW - totalGroup2W) / 2.f;

    tab2Txt.setPosition(
        std::round(group2StartX - t2Bounds.left),
        std::round(tabY + (tabH - t2Bounds.height) / 2.f - t2Bounds.top)
    );
    window.draw(tab2Txt);

    float badge2X = std::round(group2StartX + t2Bounds.width + groupGap);
    float badge2Y = std::round(tabY + (tabH - badgeH) / 2.f);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badge2X, badge2Y),
        sf::Vector2f(badgeW, badgeH),
        11.f,
        (movieSubTab == 1) ? sf::Color(2, 132, 199) : sf::Color(226, 232, 240)
    );

    sf::Text cnt2Badge;
    cnt2Badge.setFont(Theme::FontPrimary);
    cnt2Badge.setCharacterSize(12);
    cnt2Badge.setStyle(sf::Text::Bold);
    cnt2Badge.setFillColor((movieSubTab == 1) ? sf::Color::White : sf::Color(71, 85, 105));
    std::ostringstream cnt2Oss;
    cnt2Oss << std::setw(2) << std::setfill('0') << genreList.size();
    cnt2Badge.setString(cnt2Oss.str());
    sf::FloatRect cb2Bounds = cnt2Badge.getLocalBounds();
    cnt2Badge.setPosition(
        std::round(badge2X + (badgeW - cb2Bounds.width) / 2.f - cb2Bounds.left),
        std::round(badge2Y + (badgeH - cb2Bounds.height) / 2.f - cb2Bounds.top)
    );
    window.draw(cnt2Badge);

    // Top-right Search & Export controls on Sub-tab bar when movieSubTab == 1
    if (movieSubTab == 1) {
        if (genreSearchInput) genreSearchInput->draw(window);
        // Magnifying glass icon inside genreSearchInput
        sf::CircleShape gLens(5.5f);
        gLens.setPosition(rightColX - 24.f, 193.f);
        gLens.setFillColor(sf::Color::Transparent);
        gLens.setOutlineThickness(1.6f);
        gLens.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(gLens);
        sf::RectangleShape gHandle(sf::Vector2f(5.f, 1.6f));
        gHandle.setPosition(rightColX - 15.f, 203.f);
        gHandle.setRotation(45.f);
        gHandle.setFillColor(sf::Color(148, 163, 184));
        window.draw(gHandle);

        if (btnExportGenres) btnExportGenres->draw(window);
        // Green spreadsheet/file icon inside btnExportGenres
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 298.f, 192.f), sf::Vector2f(13.f, 16.f), 2.f, sf::Color(220, 252, 231), 1.2f, sf::Color(22, 163, 74));
        sf::RectangleShape expLine1(sf::Vector2f(7.f, 1.5f));
        expLine1.setPosition(rightColX + 301.f, 197.f);
        expLine1.setFillColor(sf::Color(22, 163, 74));
        window.draw(expLine1);
        sf::RectangleShape expLine2(sf::Vector2f(7.f, 1.5f));
        expLine2.setPosition(rightColX + 301.f, 201.f);
        expLine2.setFillColor(sf::Color(22, 163, 74));
        window.draw(expLine2);
    }

    // Helper lambda to draw a Poster Thumbnail (either loaded sf::Texture or clean clapperboard placeholder)
    auto drawPosterBox = [&](const std::string& movieCode, const std::string& customPath, float bx, float by, float bw, float bh) {
        RoundedRectRenderer::render(window, sf::Vector2f(bx, by), sf::Vector2f(bw, bh), 6.f, sf::Color(224, 242, 254), 1.f, sf::Color(186, 230, 253));

        std::shared_ptr<sf::Texture> tex = nullptr;
        auto it = moviePosterTextures.find(movieCode);
        if (it != moviePosterTextures.end() && it->second && it->second->getSize().x > 0) {
            tex = it->second;
        } else if (!customPath.empty()) {
            std::ifstream f(customPath.c_str(), std::ios::binary);
            if (f.good()) {
                f.close();
                std::shared_ptr<sf::Texture> loaded(new sf::Texture());
                if (loaded->loadFromFile(customPath)) {
                    loaded->setSmooth(true);
                    moviePosterTextures[movieCode] = loaded;
                    tex = loaded;
                }
            }
        }

        if (tex && tex->getSize().x > 0 && tex->getSize().y > 0) {
            sf::Sprite sp(*tex);
            float sx = (bw - 2.f) / static_cast<float>(tex->getSize().x);
            float sy = (bh - 2.f) / static_cast<float>(tex->getSize().y);
            sp.setScale(sx, sy);
            sp.setPosition(bx + 1.f, by + 1.f);
            window.draw(sp);
        } else {
            // Vector film clapperboard icon inside placeholder slot
            float icW = std::min(bw * 0.52f, 20.f);
            float icH = std::min(bh * 0.36f, 16.f);
            float icX = bx + (bw - icW) / 2.f;
            float icY = by + (bh - icH) / 2.f;

            sf::RectangleShape clapBody(sf::Vector2f(icW, icH));
            clapBody.setPosition(icX, icY);
            clapBody.setFillColor(sf::Color::Transparent);
            clapBody.setOutlineThickness(1.5f);
            clapBody.setOutlineColor(sf::Color(2, 132, 199));
            window.draw(clapBody);

            sf::RectangleShape clapTop(sf::Vector2f(icW, 4.f));
            clapTop.setPosition(icX, icY);
            clapTop.setFillColor(sf::Color(2, 132, 199));
            window.draw(clapTop);
        }
    };

    // 6. LEFT CARD: White Table Card (cardY = 232.f, cardH = 716.f)
    float cardY = 232.f;
    float cardH = 716.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, cardY),
        sf::Vector2f(leftColW, cardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Filter Bar Row
        sf::Text lblSearch;
        lblSearch.setFont(Theme::FontPrimary);
        lblSearch.setCharacterSize(14);
        lblSearch.setStyle(sf::Text::Bold);
        lblSearch.setFillColor(sf::Color(30, 41, 59));
        lblSearch.setString(toSfString("Tìm nhanh:"));
        lblSearch.setPosition(leftColX + 20.f, cardY + 24.f);
        window.draw(lblSearch);

        if (movieSearchInput) movieSearchInput->draw(window);

        // Magnifying glass icon inside search box
        sf::CircleShape magGlass(5.5f);
        magGlass.setFillColor(sf::Color::Transparent);
        magGlass.setOutlineThickness(1.6f);
        magGlass.setOutlineColor(sf::Color(100, 116, 139));
        magGlass.setPosition(leftColX + 120.f, 259.f);
        window.draw(magGlass);

        // Age Rating Filter Bar (Tất cả | P | K | T13 | T18)
        float filterBarX = leftColX + 422.f;
        float filterBarY = 248.f;
        RoundedRectRenderer::render(window, sf::Vector2f(filterBarX, filterBarY), sf::Vector2f(266.f, 36.f), 8.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));

        struct AgePillItem { const char* label; const char* key; float x; float w; };
        AgePillItem filterItems[] = {
            {"Tất cả", "", 4.f, 56.f},
            {"P", "P", 64.f, 38.f},
            {"K", "K", 106.f, 38.f},
            {"T13", "13", 148.f, 52.f},
            {"T18", "18", 204.f, 58.f}
        };
        for (int f = 0; f < 5; ++f) {
            bool active = (movieAgeFilter == filterItems[f].key);
            if (active) {
                RoundedRectRenderer::render(window, sf::Vector2f(filterBarX + filterItems[f].x, filterBarY + 4.f), sf::Vector2f(filterItems[f].w, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(203, 213, 225));
            }
            sf::Text ft;
            ft.setFont(Theme::FontPrimary);
            ft.setCharacterSize(12);
            ft.setStyle(sf::Text::Bold);
            ft.setFillColor(active ? sf::Color(3, 105, 161) : sf::Color(71, 85, 105));
            ft.setString(toSfString(filterItems[f].label));
            sf::FloatRect fb = ft.getLocalBounds();
            ft.setPosition(
                std::round(filterBarX + filterItems[f].x + (filterItems[f].w - fb.width) / 2.f - fb.left),
                std::round(filterBarY + (36.f - fb.height) / 2.f - fb.top)
            );
            window.draw(ft);
        }

        if (btnQuickAddMovie) btnQuickAddMovie->draw(window);

        // Table Header (tblHeaderY = 296.f, height = 42.f)
        float tblHeaderY = cardY + 64.f;
        sf::RectangleShape thDivTop(sf::Vector2f(leftColW - 32.f, 1.f));
        thDivTop.setPosition(leftColX + 16.f, tblHeaderY);
        thDivTop.setFillColor(sf::Color(241, 245, 249));
        window.draw(thDivTop);

        sf::RectangleShape thDivBot(sf::Vector2f(leftColW - 32.f, 1.f));
        thDivBot.setPosition(leftColX + 16.f, tblHeaderY + 40.f);
        thDivBot.setFillColor(sf::Color(226, 232, 240));
        window.draw(thDivBot);

        std::vector<std::string> cols = {"MÃ PHIM", "TÊN PHIM", "THỂ LOẠI", "THỜI LƯỢNG", "NGÀY CHIẾU", "NHÃN TUỔI", "TRẠNG THÁI", "THAO TÁC"};
        std::vector<float> colX = {
            leftColX + 24.f,
            leftColX + 90.f,
            leftColX + 350.f,
            leftColX + 468.f,
            leftColX + 560.f,
            leftColX + 662.f,
            leftColX + 748.f,
            leftColX + 846.f
        };

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(12);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(71, 85, 105));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 12.f);
            window.draw(ch);
        }

        // Table Rows (rowStartY = 338.f, rowH = 66.f)
        float rowStartY = tblHeaderY + 42.f;
        float rowH = 66.f;
        std::string query = movieSearchInput ? movieSearchInput->getText() : "";
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        size_t displayCount = 0;
        for (size_t i = 0; i < movieList.size() && displayCount < 8; ++i) {
            std::string pName = movieList[i].getTenPhim();
            std::string pCode = movieList[i].getMaPhim();
            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
            if (pGenre.empty()) pGenre = "Tâm Lý, Hành Động";
            int pAge = movieList[i].getNhanDoTuoi();

            if (!movieAgeFilter.empty()) {
                if (movieAgeFilter == "P" && pAge > 0) continue;
                if (movieAgeFilter == "K" && pAge > 12) continue;
                if (movieAgeFilter == "13" && pAge != 13) continue;
                if (movieAgeFilter == "18" && pAge < 16) continue;
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

            float ry = rowStartY + static_cast<float>(displayCount) * rowH;
            bool isSelected = (selectedMovieRowIdx == static_cast<int>(i));

            if (isSelected) {
                RoundedRectRenderer::render(
                    window,
                    sf::Vector2f(leftColX + 12.f, ry + 2.f),
                    sf::Vector2f(leftColW - 24.f, rowH - 4.f),
                    8.f,
                    sf::Color(224, 242, 254)
                );
            }

            // 1. Mã Phim
            sf::Text cCode;
            cCode.setFont(Theme::FontPrimary);
            cCode.setCharacterSize(13);
            cCode.setStyle(sf::Text::Bold);
            cCode.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(71, 85, 105));
            cCode.setString(toSfString(pCode));
            cCode.setPosition(colX[0], ry + 23.f);
            window.draw(cCode);

            // 2. Poster Slot (36x50) + Tên Phim + Mô tả ngắn
            drawPosterBox(pCode, movieList[i].getPoster(), colX[1], ry + 8.f, 36.f, 50.f);

            sf::Text cName;
            cName.setFont(Theme::FontPrimary);
            cName.setCharacterSize(14);
            cName.setStyle(sf::Text::Bold);
            cName.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(15, 23, 42));
            std::string dispTitle = pName;
            if (dispTitle.length() > 26) dispTitle = dispTitle.substr(0, 23) + "...";
            cName.setString(toSfString(dispTitle));
            cName.setPosition(colX[1] + 46.f, ry + 13.f);
            window.draw(cName);

            sf::Text cStudio;
            cStudio.setFont(Theme::FontPrimary);
            cStudio.setCharacterSize(12);
            cStudio.setFillColor(sf::Color(100, 116, 139));
            std::string subDesc = movieList[i].getMoTa();
            if (subDesc.empty()) subDesc = "CineStar Distribution";
            if (subDesc.length() > 28) subDesc = subDesc.substr(0, 25) + "...";
            cStudio.setString(toSfString(subDesc));
            cStudio.setPosition(colX[1] + 46.f, ry + 34.f);
            window.draw(cStudio);

            // 3. Thể Loại
            sf::Text cGenre;
            cGenre.setFont(Theme::FontPrimary);
            cGenre.setCharacterSize(12);
            cGenre.setFillColor(sf::Color(71, 85, 105));
            std::string gDisp = pGenre;
            if (gDisp.length() > 16) gDisp = gDisp.substr(0, 14) + "...";
            cGenre.setString(toSfString(gDisp));
            cGenre.setPosition(colX[2], ry + 23.f);
            window.draw(cGenre);

            // 4. Thời lượng
            sf::Text cDur;
            cDur.setFont(Theme::FontPrimary);
            cDur.setCharacterSize(13);
            cDur.setStyle(sf::Text::Bold);
            cDur.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(30, 41, 59));
            cDur.setString(toSfString(std::to_string(movieList[i].getThoiLuong()) + " phút"));
            cDur.setPosition(colX[3], ry + 23.f);
            window.draw(cDur);

            // 5. Ngày chiếu
            sf::Text cDate;
            cDate.setFont(Theme::FontPrimary);
            cDate.setCharacterSize(13);
            cDate.setStyle(isSelected ? sf::Text::Bold : sf::Text::Regular);
            cDate.setFillColor(isSelected ? sf::Color(3, 105, 161) : sf::Color(30, 41, 59));
            cDate.setString(toSfString(movieList[i].getNgayKhoiChieu()));
            cDate.setPosition(colX[4], ry + 23.f);
            window.draw(cDate);

            // 6. Nhãn tuổi pill
            std::string ageBadgeStr = (pAge <= 0) ? "P" : ("T" + std::to_string(pAge));
            sf::Color ageBg = (pAge >= 16) ? sf::Color(254, 226, 226) : ((pAge >= 13) ? sf::Color(224, 231, 255) : sf::Color(220, 252, 231));
            sf::Color ageFg = (pAge >= 16) ? sf::Color(220, 38, 38) : ((pAge >= 13) ? sf::Color(67, 56, 202) : sf::Color(21, 128, 61));
            RoundedRectRenderer::render(window, sf::Vector2f(colX[5], ry + 19.f), sf::Vector2f(42.f, 26.f), 6.f, ageBg);

            sf::Text cAge;
            cAge.setFont(Theme::FontPrimary);
            cAge.setCharacterSize(12);
            cAge.setStyle(sf::Text::Bold);
            cAge.setFillColor(ageFg);
            cAge.setString(toSfString(ageBadgeStr));
            sf::FloatRect ab = cAge.getLocalBounds();
            cAge.setPosition(colX[5] + (42.f - ab.width) / 2.f, ry + 24.f);
            window.draw(cAge);

            // 7. Trạng thái pill (Đang chiếu / Sắp chiếu)
            int relKey = DateFormat::dateKey(DateFormat::normalizeDate(movieList[i].getNgayKhoiChieu()));
            bool isComingSoon = (todayKey > 0 && relKey > todayKey);
            sf::Color stBg = isComingSoon ? sf::Color(224, 231, 255) : sf::Color(187, 247, 208);
            sf::Color stFg = isComingSoon ? sf::Color(55, 48, 163) : sf::Color(21, 128, 61);
            RoundedRectRenderer::render(window, sf::Vector2f(colX[6] - 8.f, ry + 17.f), sf::Vector2f(82.f, 30.f), 15.f, stBg);

            sf::Text cSt;
            cSt.setFont(Theme::FontPrimary);
            cSt.setCharacterSize(11);
            cSt.setStyle(sf::Text::Bold);
            cSt.setFillColor(stFg);
            cSt.setString(toSfString(isComingSoon ? "Sắp chiếu" : "Đang chiếu"));
            sf::FloatRect sb = cSt.getLocalBounds();
            cSt.setPosition(colX[6] - 8.f + (82.f - sb.width) / 2.f, ry + 24.f);
            window.draw(cSt);

            // 8. Thao tác icons (16x supersampled Edit Pencil & Delete Trash Can)
            static sf::Texture pencilTex, trashTex;
            static bool actionIconsReady = false;
            if (!actionIconsReady) {
                const int icS = 26;
                sf::Image pImg, tImg;
                pImg.create(icS, icS, sf::Color(0, 0, 0, 0));
                tImg.create(icS, icS, sf::Color(0, 0, 0, 0));

                auto distSeg = [](float px, float py, float ax, float ay, float bx, float by) -> float {
                    float vx = bx - ax, vy = by - ay;
                    float wx = px - ax, wy = py - ay;
                    float c1 = vx * wx + vy * wy;
                    if (c1 <= 0.f) return std::sqrt(wx * wx + wy * wy);
                    float c2 = vx * vx + vy * vy;
                    if (c2 <= c1) return std::sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
                    float b = c1 / c2;
                    float pbx = ax + b * vx, pby = ay + b * vy;
                    return std::sqrt((px - pbx) * (px - pbx) + (py - pby) * (py - pby));
                };

                for (int py = 0; py < icS; ++py) {
                    for (int px = 0; px < icS; ++px) {
                        int pHits = 0, tHits = 0;
                        for (int sy = 0; sy < 4; ++sy) {
                            for (int sx = 0; sx < 4; ++sx) {
                                float fx = (px + (sx + 0.5f) * 0.25f) - icS * 0.5f;
                                float fy = (py + (sy + 0.5f) * 0.25f) - icS * 0.5f;

                                // Pencil: rotated 45 deg (top-right eraser to bottom-left tip)
                                float u = (fx - fy) * 0.70710678f;
                                float v = (fx + fy) * 0.70710678f;
                                bool inPencil = false;
                                // Shaft outline & eraser collar
                                if (u >= -3.2f && u <= 5.2f && std::fabs(v) <= 2.1f) {
                                    bool border = (std::fabs(v) >= 1.0f) || (u >= 4.1f) || (std::fabs(u - 2.4f) <= 0.55f) || (std::fabs(u + 3.0f) <= 0.5f);
                                    if (border) inPencil = true;
                                }
                                // Pointed nib tapering to (-6.2, 0)
                                if (u >= -6.2f && u < -3.2f) {
                                    float maxV = ((u + 6.2f) / 3.0f) * 2.1f;
                                    if (std::fabs(v) <= maxV && (std::fabs(v) >= maxV - 1.1f || u <= -4.8f)) {
                                        inPencil = true;
                                    }
                                }
                                if (inPencil) pHits++;

                                // Trash can: lid, top handle, tapered bin outline, and 2 vertical inner bars
                                bool inTrash = false;
                                // Lid bar
                                if (distSeg(fx, fy, -5.8f, -4.6f, 5.8f, -4.6f) <= 0.75f) inTrash = true;
                                // Top handle arch
                                if (fy >= -7.0f && fy <= -4.6f && std::fabs(fx) <= 2.4f) {
                                    if (fy <= -5.8f || std::fabs(fx) >= 1.25f) inTrash = true;
                                }
                                // Bin outline (left, right, bottom)
                                if (distSeg(fx, fy, -4.5f, -4.2f, -3.8f, 6.2f) <= 0.7f) inTrash = true;
                                if (distSeg(fx, fy,  4.5f, -4.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                                if (distSeg(fx, fy, -3.8f,  6.2f,  3.8f, 6.2f) <= 0.7f) inTrash = true;
                                // Inner vertical slats
                                if (distSeg(fx, fy, -1.5f, -1.8f, -1.3f, 3.8f) <= 0.6f) inTrash = true;
                                if (distSeg(fx, fy,  1.5f, -1.8f,  1.3f, 3.8f) <= 0.6f) inTrash = true;

                                if (inTrash) tHits++;
                            }
                        }
                        if (pHits > 0) {
                            pImg.setPixel(px, py, sf::Color(51, 65, 85, static_cast<sf::Uint8>((pHits * 255) / 16)));
                        }
                        if (tHits > 0) {
                            tImg.setPixel(px, py, sf::Color(220, 38, 38, static_cast<sf::Uint8>((tHits * 255) / 16)));
                        }
                    }
                }
                pencilTex.loadFromImage(pImg);
                pencilTex.setSmooth(true);
                trashTex.loadFromImage(tImg);
                trashTex.setSmooth(true);
                actionIconsReady = true;
            }

            RoundedRectRenderer::render(window, sf::Vector2f(colX[7], ry + 19.f), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(241, 245, 249));
            sf::Sprite editSpr(pencilTex);
            editSpr.setPosition(colX[7], ry + 19.f);
            window.draw(editSpr);

            RoundedRectRenderer::render(window, sf::Vector2f(colX[7] + 32.f, ry + 19.f), sf::Vector2f(26.f, 26.f), 6.f, sf::Color(254, 242, 242));
            sf::Sprite delSpr(trashTex);
            delSpr.setPosition(colX[7] + 32.f, ry + 19.f);
            window.draw(delSpr);

            // Row divider
            sf::RectangleShape rBorder(sf::Vector2f(leftColW - 32.f, 1.f));
            rBorder.setPosition(leftColX + 16.f, ry + rowH - 1.f);
            rBorder.setFillColor(sf::Color(241, 245, 249));
            window.draw(rBorder);

            displayCount++;
        }

        // Table Footer (Pagination & Record count)
        float footerY = cardY + cardH - 42.f;
        sf::Text footInfo;
        footInfo.setFont(Theme::FontPrimary);
        footInfo.setCharacterSize(13);
        footInfo.setFillColor(sf::Color(100, 116, 139));
        footInfo.setString(toSfString("Hiển thị 1 - " + std::to_string(displayCount) + " trên " + std::to_string(movieList.size()) + " bản ghi"));
        footInfo.setPosition(leftColX + 20.f, footerY + 6.f);
        window.draw(footInfo);

        // Pagination buttons [Trước] [1] [Sau]
        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + leftColW - 165.f, footerY), sf::Vector2f(54.f, 28.f), 6.f, sf::Color(241, 245, 249));
        sf::Text pPrev;
        pPrev.setFont(Theme::FontPrimary);
        pPrev.setCharacterSize(12);
        pPrev.setFillColor(sf::Color(148, 163, 184));
        pPrev.setString(toSfString("Trước"));
        pPrev.setPosition(leftColX + leftColW - 155.f, footerY + 6.f);
        window.draw(pPrev);

        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + leftColW - 105.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color(3, 105, 161));
        sf::Text pOne;
        pOne.setFont(Theme::FontPrimary);
        pOne.setCharacterSize(12);
        pOne.setStyle(sf::Text::Bold);
        pOne.setFillColor(sf::Color::White);
        pOne.setString("1");
        pOne.setPosition(leftColX + leftColW - 95.f, footerY + 6.f);
        window.draw(pOne);

        RoundedRectRenderer::render(window, sf::Vector2f(leftColX + leftColW - 71.f, footerY), sf::Vector2f(50.f, 28.f), 6.f, sf::Color(241, 245, 249));
        sf::Text pNext;
        pNext.setFont(Theme::FontPrimary);
        pNext.setCharacterSize(12);
        pNext.setFillColor(sf::Color(148, 163, 184));
        pNext.setString(toSfString("Sau"));
        pNext.setPosition(leftColX + leftColW - 57.f, footerY + 6.f);
        window.draw(pNext);
    } else {
        // Genre Table (Matching Reference Design 100%)
        // 1. Card Header: "Danh Sách Thể Loại Phim (Áp dụng trên toàn hệ thống rạp)" + "Sắp xếp theo: [Mã thể loại ↓↑]"
        sf::Text gHdrTitle;
        gHdrTitle.setFont(Theme::FontPrimary);
        gHdrTitle.setCharacterSize(17);
        gHdrTitle.setStyle(sf::Text::Bold);
        gHdrTitle.setFillColor(sf::Color(15, 23, 42));
        gHdrTitle.setString(toSfString("Danh Sách Thể Loại Phim"));
        gHdrTitle.setPosition(leftColX + 24.f, cardY + 18.f);
        window.draw(gHdrTitle);

        sf::FloatRect ghtB = gHdrTitle.getLocalBounds();
        sf::Text gHdrSub;
        gHdrSub.setFont(Theme::FontPrimary);
        gHdrSub.setCharacterSize(13);
        gHdrSub.setFillColor(sf::Color(100, 116, 139));
        gHdrSub.setString(toSfString("(Áp dụng trên toàn hệ thống rạp)"));
        gHdrSub.setPosition(leftColX + 34.f + ghtB.width, cardY + 21.f);
        window.draw(gHdrSub);

        sf::Text sortLbl;
        sortLbl.setFont(Theme::FontPrimary);
        sortLbl.setCharacterSize(13);
        sortLbl.setFillColor(sf::Color(100, 116, 139));
        sortLbl.setString(toSfString("Sắp xếp theo:"));
        sortLbl.setPosition(leftColX + leftColW - 228.f, cardY + 20.f);
        window.draw(sortLbl);

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(leftColX + leftColW - 138.f, cardY + 15.f),
            sf::Vector2f(114.f, 28.f),
            6.f,
            sf::Color(248, 250, 252),
            1.f,
            sf::Color(226, 232, 240)
        );
        sf::Text sortBtnTxt;
        sortBtnTxt.setFont(Theme::FontPrimary);
        sortBtnTxt.setCharacterSize(12);
        sortBtnTxt.setStyle(sf::Text::Bold);
        sortBtnTxt.setFillColor(sf::Color(30, 41, 59));
        sortBtnTxt.setString(toSfString(genreSortAsc ? "Mã thể loại ↓↑" : "Mã thể loại ↑↓"));
        sf::FloatRect sbB = sortBtnTxt.getLocalBounds();
        sortBtnTxt.setPosition(
            std::round(leftColX + leftColW - 138.f + (114.f - sbB.width) / 2.f - sbB.left),
            std::round(cardY + 15.f + (28.f - sbB.height) / 2.f - sbB.top)
        );
        window.draw(sortBtnTxt);

        // 2. Table Column Header Strip
        float tblHeaderY = cardY + 58.f;
        sf::RectangleShape thBg(sf::Vector2f(leftColW - 2.f, 40.f));
        thBg.setPosition(leftColX + 1.f, tblHeaderY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thTopLine(sf::Vector2f(leftColW - 2.f, 1.f));
        thTopLine.setPosition(leftColX + 1.f, tblHeaderY);
        thTopLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(thTopLine);

        sf::RectangleShape thBotLine(sf::Vector2f(leftColW - 2.f, 1.f));
        thBotLine.setPosition(leftColX + 1.f, tblHeaderY + 39.f);
        thBotLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(thBotLine);

        std::vector<std::string> cols = {"MÃ THỂ LOẠI", "TÊN THỂ LOẠI"};
        std::vector<float> colX = {leftColX + 36.f, leftColX + 210.f};

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(12);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(100, 116, 139));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 12.f);
            window.draw(ch);
        }

        // Palette of indicator dot colors matching reference screenshot
        const sf::Color dotColors[] = {
            sf::Color(239, 68, 68),   // 0: Red (Hành Động)
            sf::Color(139, 92, 246),  // 1: Purple (Kinh Dị)
            sf::Color(236, 72, 153),  // 2: Pink (Tình Cảm)
            sf::Color(245, 158, 11),  // 3: Amber (Hoạt Hình)
            sf::Color(59, 130, 246),  // 4: Blue (Tâm Lý)
            sf::Color(16, 185, 129),  // 5: Emerald (Gia Đình)
            sf::Color(6, 182, 212),   // 6: Cyan (Viễn Tưởng)
            sf::Color(249, 115, 22),  // 7: Orange (Kịch Tính)
            sf::Color(220, 38, 38)    // 8: Crimson (Gay Cấn)
        };

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

        float rowStartY = tblHeaderY + 40.f;
        float rowH = 56.f;
        int shownGenres = 0;
        for (size_t v = 0; v < visibleIndices.size() && v < 10; ++v) {
            int idx = visibleIndices[v];
            float ry = rowStartY + static_cast<float>(v) * rowH;
            bool isSelected = (selectedGenreRowIdx == idx);

            sf::RectangleShape rBg(sf::Vector2f(leftColW - 2.f, rowH - 1.f));
            rBg.setPosition(leftColX + 1.f, ry);
            rBg.setFillColor(isSelected ? sf::Color(240, 249, 255) : sf::Color::White);
            window.draw(rBg);

            if (isSelected) {
                sf::RectangleShape leftAccent(sf::Vector2f(4.f, rowH - 1.f));
                leftAccent.setPosition(leftColX + 1.f, ry);
                leftAccent.setFillColor(sf::Color(2, 132, 199));
                window.draw(leftAccent);
            }

            sf::RectangleShape rowDiv(sf::Vector2f(leftColW - 2.f, 1.f));
            rowDiv.setPosition(leftColX + 1.f, ry + rowH - 1.f);
            rowDiv.setFillColor(sf::Color(241, 245, 249));
            window.draw(rowDiv);

            // Code Pill Badge [TL01]
            sf::Color pillBg = isSelected ? sf::Color(224, 242, 254) : sf::Color(241, 245, 249);
            sf::Color pillFg = isSelected ? sf::Color(2, 132, 199) : sf::Color(71, 85, 105);
            RoundedRectRenderer::render(window, sf::Vector2f(colX[0], ry + 15.f), sf::Vector2f(54.f, 26.f), 6.f, pillBg);

            sf::Text tId;
            tId.setFont(Theme::FontPrimary);
            tId.setCharacterSize(12);
            tId.setStyle(sf::Text::Bold);
            tId.setFillColor(pillFg);
            tId.setString(genreList[idx].first);
            sf::FloatRect idB = tId.getLocalBounds();
            tId.setPosition(
                std::round(colX[0] + (54.f - idB.width) / 2.f - idB.left),
                std::round(ry + 15.f + (26.f - idB.height) / 2.f - idB.top)
            );
            window.draw(tId);

            // Colored Dot + Genre Name
            int cIdx = genreColorMap.count(genreList[idx].first) ? genreColorMap[genreList[idx].first] : idx;
            sf::CircleShape gDot(4.5f);
            gDot.setFillColor(dotColors[cIdx % 9]);
            gDot.setPosition(colX[1], ry + 23.5f);
            window.draw(gDot);

            sf::Text tName;
            tName.setFont(Theme::FontPrimary);
            tName.setCharacterSize(14);
            tName.setStyle(sf::Text::Bold);
            tName.setFillColor(isSelected ? sf::Color(15, 23, 42) : sf::Color(30, 41, 59));
            tName.setString(toSfString(genreList[idx].second));
            tName.setPosition(colX[1] + 18.f, ry + 18.f);
            window.draw(tName);

            shownGenres++;
        }

        // 3. Genre Table Footer ("Hiển thị 9 trên 9 thể loại phim" + [<] [1] [>])
        float footerY = cardY + cardH - 44.f;
        sf::RectangleShape footLine(sf::Vector2f(leftColW - 2.f, 1.f));
        footLine.setPosition(leftColX + 1.f, footerY - 10.f);
        footLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(footLine);

        sf::Text gFootTxt;
        gFootTxt.setFont(Theme::FontPrimary);
        gFootTxt.setCharacterSize(13);
        gFootTxt.setFillColor(sf::Color(100, 116, 139));
        gFootTxt.setString(toSfString("Hiển thị " + std::to_string(shownGenres) + " trên " + std::to_string(genreList.size()) + " thể loại phim"));
        gFootTxt.setPosition(leftColX + 24.f, footerY + 4.f);
        window.draw(gFootTxt);

        // Pagination [<] [1] [>]
        float pgX = leftColX + leftColW - 118.f;
        RoundedRectRenderer::render(window, sf::Vector2f(pgX, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::Text pgPrev;
        pgPrev.setFont(Theme::FontPrimary);
        pgPrev.setCharacterSize(13);
        pgPrev.setStyle(sf::Text::Bold);
        pgPrev.setFillColor(sf::Color(148, 163, 184));
        pgPrev.setString("<");
        pgPrev.setPosition(pgX + 10.f, footerY + 5.f);
        window.draw(pgPrev);

        RoundedRectRenderer::render(window, sf::Vector2f(pgX + 34.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color(2, 132, 199));
        sf::Text pgOne;
        pgOne.setFont(Theme::FontPrimary);
        pgOne.setCharacterSize(13);
        pgOne.setStyle(sf::Text::Bold);
        pgOne.setFillColor(sf::Color::White);
        pgOne.setString("1");
        pgOne.setPosition(pgX + 44.f, footerY + 5.f);
        window.draw(pgOne);

        RoundedRectRenderer::render(window, sf::Vector2f(pgX + 68.f, footerY), sf::Vector2f(28.f, 28.f), 6.f, sf::Color::White, 1.f, sf::Color(226, 232, 240));
        sf::Text pgNext;
        pgNext.setFont(Theme::FontPrimary);
        pgNext.setCharacterSize(13);
        pgNext.setStyle(sf::Text::Bold);
        pgNext.setFillColor(sf::Color(148, 163, 184));
        pgNext.setString(">");
        pgNext.setPosition(pgX + 78.f, footerY + 5.f);
        window.draw(pgNext);
    }

    // 7. RIGHT CARD: Form Card (White, cardY = 232.f, cardH = 716.f)
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, cardY),
        sf::Vector2f(rightColW, cardH),
        12.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Title "Thông tin Phim" + Status Badge "● Đang sửa: P01"
        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(18);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString("Thông tin Phim"));
        formTitle.setPosition(rightColX + 20.f, cardY + 18.f);
        window.draw(formTitle);

        std::string curIdBadge = (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size()))
                                 ? ("Đang sửa: " + movieList[selectedMovieRowIdx].getMaPhim())
                                 : "Tạo phim mới";
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 138.f, cardY + 16.f), sf::Vector2f(118.f, 26.f), 13.f, sf::Color(224, 242, 254));
        sf::CircleShape editDot(3.5f);
        editDot.setFillColor(sf::Color(3, 105, 161));
        editDot.setPosition(rightColX + rightColW - 128.f, cardY + 25.f);
        window.draw(editDot);

        sf::Text editBadgeTxt;
        editBadgeTxt.setFont(Theme::FontPrimary);
        editBadgeTxt.setCharacterSize(12);
        editBadgeTxt.setStyle(sf::Text::Bold);
        editBadgeTxt.setFillColor(sf::Color(3, 105, 161));
        editBadgeTxt.setString(toSfString(curIdBadge));
        editBadgeTxt.setPosition(rightColX + rightColW - 116.f, cardY + 21.f);
        window.draw(editBadgeTxt);

        // Labels (Bold 13px Segoe UI)
        std::vector<std::pair<std::string, float>> formLabels = {
            {"Mã Phim: *", 294.f},
            {"Tên Phim: *", 339.f},
            {"Thời lượng:", 384.f},
            {"Ngày chiếu:", 429.f},
            {"Nhãn tuổi (Độ):", 474.f},
            {"Thể loại:", 519.f},
            {"Poster Phim:", 566.f},
            {"Mô tả:", 626.f}
        };

        for (const auto& fl : formLabels) {
            sf::Text lText;
            lText.setFont(Theme::FontPrimary);
            lText.setCharacterSize(13);
            lText.setStyle(sf::Text::Bold);
            lText.setFillColor(sf::Color(51, 65, 85));
            lText.setString(toSfString(fl.first));
            lText.setPosition(rightColX + 20.f, fl.second);
            window.draw(lText);
        }

        // Draw Form Inputs
        if (inputMovieId) inputMovieId->draw(window);
        if (inputMovieTitle) inputMovieTitle->draw(window);
        if (inputMovieDuration) inputMovieDuration->draw(window);
        if (inputMovieReleaseDate) inputMovieReleaseDate->draw(window);
        if (inputMovieGenre) inputMovieGenre->draw(window);
        if (btnSelectGenre) btnSelectGenre->draw(window);
        if (inputMoviePoster) inputMoviePoster->draw(window);
        if (inputMovieDesc) inputMovieDesc->draw(window);

        // "phút" suffix inside Thời lượng input
        sf::Text unitTxt;
        unitTxt.setFont(Theme::FontPrimary);
        unitTxt.setCharacterSize(13);
        unitTxt.setStyle(sf::Text::Bold);
        unitTxt.setFillColor(sf::Color(100, 116, 139));
        unitTxt.setString(toSfString("phút"));
        unitTxt.setPosition(rightColX + rightColW - 60.f, 384.f);
        window.draw(unitTxt);

        // Segmented Age Rating Selector (P | K | T13 | T18) on y = 466.f
        float inputX = rightColX + 136.f;
        RoundedRectRenderer::render(window, sf::Vector2f(inputX, 466.f), sf::Vector2f(330.f, 35.f), 8.f, sf::Color(241, 245, 249), 1.f, sf::Color(226, 232, 240));
        std::string curAgeStr = inputMovieAgeRating ? inputMovieAgeRating->getText() : "T18";
        const char* ageOpts[] = {"P", "K", "T13", "T18"};
        for (int a = 0; a < 4; ++a) {
            float segW = 330.f / 4.f;
            float sx = inputX + a * segW;
            bool isSel = (curAgeStr == ageOpts[a]) ||
                         (a == 2 && (curAgeStr == "C13" || curAgeStr == "T13")) ||
                         (a == 3 && (curAgeStr == "C18" || curAgeStr == "T18" || curAgeStr == "C16"));
            if (isSel) {
                RoundedRectRenderer::render(window, sf::Vector2f(sx + 4.f, 470.f), sf::Vector2f(segW - 8.f, 27.f), 6.f, sf::Color(254, 226, 226));
            }
            sf::Text at;
            at.setFont(Theme::FontPrimary);
            at.setCharacterSize(12);
            at.setStyle(sf::Text::Bold);
            at.setFillColor(isSel ? sf::Color(220, 38, 38) : sf::Color(71, 85, 105));
            at.setString(ageOpts[a]);
            sf::FloatRect ab = at.getLocalBounds();
            at.setPosition(
                std::round(sx + (segW - ab.width) / 2.f - ab.left),
                std::round(466.f + (35.f - ab.height) / 2.f - ab.top)
            );
            window.draw(at);
        }

        // Dedicated Poster Preview Box (48x64) + Hint text for adding poster images
        std::string previewCode = inputMovieId ? inputMovieId->getText() : "P01";
        std::string previewPath = inputMoviePoster ? inputMoviePoster->getText() : "";
        drawPosterBox(previewCode, previewPath, inputX, 554.f, 48.f, 64.f);

        sf::Text posterHint;
        posterHint.setFont(Theme::FontPrimary);
        posterHint.setCharacterSize(11);
        posterHint.setFillColor(sf::Color(100, 116, 139));
        posterHint.setString(toSfString("Đặt ảnh vào thư mục assets/posters/<MãPhim>.jpg hoặc nhập đường dẫn"));
        posterHint.setPosition(inputX + 58.f, 604.f);
        window.draw(posterHint);

        // POS Schedule Activation Toggle Card (y = 736.f)
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + 20.f, 736.f), sf::Vector2f(rightColW - 40.f, 48.f), 8.f, sf::Color(240, 249, 255), 1.f, sf::Color(224, 242, 254));
        sf::Text posTitle;
        posTitle.setFont(Theme::FontPrimary);
        posTitle.setCharacterSize(13);
        posTitle.setStyle(sf::Text::Bold);
        posTitle.setFillColor(sf::Color(15, 23, 42));
        posTitle.setString(toSfString("Kích hoạt lịch chiếu POS"));
        posTitle.setPosition(rightColX + 34.f, 742.f);
        window.draw(posTitle);

        sf::Text posSub;
        posSub.setFont(Theme::FontPrimary);
        posSub.setCharacterSize(11);
        posSub.setFillColor(sf::Color(100, 116, 139));
        posSub.setString(toSfString("Cho phép mở bán vé trực tiếp tại quầy"));
        posSub.setPosition(rightColX + 34.f, 761.f);
        window.draw(posSub);

        // Toggle pill
        float togX = rightColX + rightColW - 76.f;
        float togY = 748.f;
        RoundedRectRenderer::render(window, sf::Vector2f(togX, togY), sf::Vector2f(44.f, 24.f), 12.f, moviePosActive ? sf::Color(22, 163, 74) : sf::Color(203, 213, 225));
        sf::CircleShape togKnob(9.f);
        togKnob.setFillColor(sf::Color::White);
        togKnob.setPosition(moviePosActive ? (togX + 23.f) : (togX + 3.f), togY + 3.f);
        window.draw(togKnob);

        // 4 Action Buttons
        if (btnMovieCreate) btnMovieCreate->draw(window);
        if (btnMovieUpdate) btnMovieUpdate->draw(window);
        if (btnMovieDelete) btnMovieDelete->draw(window);
        if (btnMovieClear) btnMovieClear->draw(window);
    } else {
        // Genre Form (Right Card: Matching Reference UI 100%)
        // 1. Header Icon Badge + Title & Subtitle + Mode Pill Badge
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightColX + 24.f, cardY + 20.f),
            sf::Vector2f(38.f, 38.f),
            10.f,
            sf::Color(224, 242, 254)
        );
        // Tag / label vector icon inside badge
        sf::ConvexShape tagIcon(5);
        tagIcon.setPoint(0, sf::Vector2f(rightColX + 35.f, cardY + 31.f));
        tagIcon.setPoint(1, sf::Vector2f(rightColX + 44.f, cardY + 31.f));
        tagIcon.setPoint(2, sf::Vector2f(rightColX + 51.f, cardY + 38.f));
        tagIcon.setPoint(3, sf::Vector2f(rightColX + 43.f, cardY + 46.f));
        tagIcon.setPoint(4, sf::Vector2f(rightColX + 35.f, cardY + 39.f));
        tagIcon.setFillColor(sf::Color(2, 132, 199));
        window.draw(tagIcon);
        sf::CircleShape tagHole(2.f);
        tagHole.setFillColor(sf::Color(224, 242, 254));
        tagHole.setPosition(rightColX + 37.5f, cardY + 33.5f);
        window.draw(tagHole);

        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(17);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString("Thông tin Thể loại"));
        formTitle.setPosition(rightColX + 74.f, cardY + 19.f);
        window.draw(formTitle);

        sf::Text formSub;
        formSub.setFont(Theme::FontPrimary);
        formSub.setCharacterSize(12);
        formSub.setFillColor(sf::Color(100, 116, 139));
        formSub.setString(toSfString("Xem hoặc cập nhật dữ liệu"));
        formSub.setPosition(rightColX + 74.f, cardY + 42.f);
        window.draw(formSub);

        bool isEditMode = (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size()));
        RoundedRectRenderer::render(
            window,
            sf::Vector2f(rightColX + rightColW - 148.f, cardY + 23.f),
            sf::Vector2f(124.f, 28.f),
            8.f,
            isEditMode ? sf::Color(240, 249, 255) : sf::Color(240, 253, 244),
            1.f,
            isEditMode ? sf::Color(186, 230, 253) : sf::Color(187, 247, 208)
        );
        sf::Text modeTxt;
        modeTxt.setFont(Theme::FontPrimary);
        modeTxt.setCharacterSize(11);
        modeTxt.setStyle(sf::Text::Bold);
        modeTxt.setFillColor(isEditMode ? sf::Color(2, 132, 199) : sf::Color(21, 128, 61));
        modeTxt.setString(toSfString(isEditMode ? "Chế độ: Xem / Sửa" : "Chế độ: Thêm mới"));
        sf::FloatRect mdB = modeTxt.getLocalBounds();
        modeTxt.setPosition(
            std::round(rightColX + rightColW - 148.f + (124.f - mdB.width) / 2.f - mdB.left),
            std::round(cardY + 23.f + (28.f - mdB.height) / 2.f - mdB.top)
        );
        window.draw(modeTxt);

        sf::RectangleShape hdrLine(sf::Vector2f(rightColW - 48.f, 1.f));
        hdrLine.setPosition(rightColX + 24.f, cardY + 74.f);
        hdrLine.setFillColor(sf::Color(241, 245, 249));
        window.draw(hdrLine);

        // 2. Field 1: MÃ THỂ LOẠI * + "🔒 Tự động khóa"
        sf::Text l1;
        l1.setFont(Theme::FontPrimary);
        l1.setCharacterSize(12);
        l1.setStyle(sf::Text::Bold);
        l1.setFillColor(sf::Color(51, 65, 85));
        l1.setString(toSfString("MÃ THỂ LOẠI"));
        l1.setPosition(rightColX + 24.f, 326.f);
        window.draw(l1);

        sf::Text star1;
        star1.setFont(Theme::FontPrimary);
        star1.setCharacterSize(13);
        star1.setStyle(sf::Text::Bold);
        star1.setFillColor(sf::Color(239, 68, 68));
        star1.setString("*");
        star1.setPosition(rightColX + 108.f, 325.f);
        window.draw(star1);

        // Lock icon + "Tự động khóa"
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 112.f, 331.f), sf::Vector2f(9.f, 8.f), 2.f, sf::Color(148, 163, 184));
        sf::CircleShape lockArch(2.5f);
        lockArch.setPosition(rightColX + rightColW - 110.f, 327.f);
        lockArch.setFillColor(sf::Color::Transparent);
        lockArch.setOutlineThickness(1.3f);
        lockArch.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(lockArch);

        sf::Text lockTxt;
        lockTxt.setFont(Theme::FontPrimary);
        lockTxt.setCharacterSize(11);
        lockTxt.setFillColor(sf::Color(148, 163, 184));
        lockTxt.setString(toSfString("Tự động khóa"));
        lockTxt.setPosition(rightColX + rightColW - 98.f, 326.f);
        window.draw(lockTxt);

        if (inputGenreId) inputGenreId->draw(window);

        // Shield icon inside right edge of inputGenreId
        sf::ConvexShape idShield(6);
        float shX = rightColX + rightColW - 46.f;
        float shY = 367.f;
        idShield.setPoint(0, sf::Vector2f(shX + 6.f, shY));
        idShield.setPoint(1, sf::Vector2f(shX + 12.f, shY + 2.f));
        idShield.setPoint(2, sf::Vector2f(shX + 11.f, shY + 9.f));
        idShield.setPoint(3, sf::Vector2f(shX + 6.f, shY + 14.f));
        idShield.setPoint(4, sf::Vector2f(shX + 1.f, shY + 9.f));
        idShield.setPoint(5, sf::Vector2f(shX, shY + 2.f));
        idShield.setFillColor(sf::Color(148, 163, 184));
        window.draw(idShield);

        // 3. Field 2: TÊN THỂ LOẠI * + helper text
        sf::Text l2;
        l2.setFont(Theme::FontPrimary);
        l2.setCharacterSize(12);
        l2.setStyle(sf::Text::Bold);
        l2.setFillColor(sf::Color(51, 65, 85));
        l2.setString(toSfString("TÊN THỂ LOẠI"));
        l2.setPosition(rightColX + 24.f, 416.f);
        window.draw(l2);

        sf::Text star2;
        star2.setFont(Theme::FontPrimary);
        star2.setCharacterSize(13);
        star2.setStyle(sf::Text::Bold);
        star2.setFillColor(sf::Color(239, 68, 68));
        star2.setString("*");
        star2.setPosition(rightColX + 112.f, 415.f);
        window.draw(star2);

        if (inputGenreName) inputGenreName->draw(window);

        // Keyboard icon inside right edge of inputGenreName
        RoundedRectRenderer::render(window, sf::Vector2f(rightColX + rightColW - 50.f, 458.f), sf::Vector2f(16.f, 11.f), 2.f, sf::Color::Transparent, 1.2f, sf::Color(148, 163, 184));
        for (int kx = 0; kx < 3; ++kx) {
            sf::RectangleShape kDot(sf::Vector2f(2.f, 2.f));
            kDot.setPosition(rightColX + rightColW - 47.f + kx * 4.f, 461.f);
            kDot.setFillColor(sf::Color(148, 163, 184));
            window.draw(kDot);
        }
        sf::RectangleShape kSpace(sf::Vector2f(8.f, 1.5f));
        kSpace.setPosition(rightColX + rightColW - 46.f, 465.f);
        kSpace.setFillColor(sf::Color(148, 163, 184));
        window.draw(kSpace);

        sf::Text helperTxt;
        helperTxt.setFont(Theme::FontPrimary);
        helperTxt.setCharacterSize(11);
        helperTxt.setFillColor(sf::Color(148, 163, 184));
        helperTxt.setString(toSfString("Ví dụ: Hành Động, Khoa Học Viễn Tưởng, Tâm Lý..."));
        helperTxt.setPosition(rightColX + 24.f, 494.f);
        window.draw(helperTxt);

        // 4. Field 3: MÀU SẮC BIỂU THỊ + 5 Swatch Circles
        sf::Text l3;
        l3.setFont(Theme::FontPrimary);
        l3.setCharacterSize(12);
        l3.setStyle(sf::Text::Bold);
        l3.setFillColor(sf::Color(51, 65, 85));
        l3.setString(toSfString("MÀU SẮC BIỂU THỊ"));
        l3.setPosition(rightColX + 24.f, 532.f);
        window.draw(l3);

        const sf::Color swatchColors[5] = {
            sf::Color(239, 68, 68),   // Red
            sf::Color(139, 92, 246),  // Purple
            sf::Color(236, 72, 153),  // Pink
            sf::Color(245, 158, 11),  // Amber
            sf::Color(2, 132, 199)    // Blue
        };
        const char* swatchNames[5] = {
            "Màu Đỏ (Hành Động)",
            "Màu Tím (Kinh Dị)",
            "Màu Hồng (Tình Cảm)",
            "Màu Cam (Hoạt Hình)",
            "Màu Xanh (Tâm Lý)"
        };

        int activeSwatch = (selectedGenreColorIdx >= 0 && selectedGenreColorIdx < 5) ? selectedGenreColorIdx : 0;
        for (int c = 0; c < 5; ++c) {
            float cx = rightColX + 42.f + c * 40.f;
            float cy = 574.f;
            if (c == activeSwatch) {
                sf::CircleShape ring(16.5f);
                ring.setPosition(cx - 16.5f, cy - 16.5f);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(1.8f);
                ring.setOutlineColor(swatchColors[c]);
                window.draw(ring);
            }
            sf::CircleShape sw(13.f);
            sw.setPosition(cx - 13.f, cy - 13.f);
            sw.setFillColor(swatchColors[c]);
            window.draw(sw);
        }

        sf::Text swLabel;
        swLabel.setFont(Theme::FontPrimary);
        swLabel.setCharacterSize(12);
        swLabel.setFillColor(sf::Color(100, 116, 139));
        swLabel.setString(toSfString(swatchNames[activeSwatch]));
        swLabel.setPosition(rightColX + 234.f, 566.f);
        window.draw(swLabel);

        // 5. Action Buttons
        if (btnGenreCreate) btnGenreCreate->draw(window);
        if (btnGenreDelete) {
            btnGenreDelete->draw(window);
            // Trash icon on btnGenreDelete
            float tx = rightColX + 24.f + (rightColW - 62.f) / 2.f + 14.f + 44.f;
            float ty = 631.f;
            RoundedRectRenderer::render(window, sf::Vector2f(tx, ty + 3.f), sf::Vector2f(10.f, 11.f), 1.5f, sf::Color::Transparent, 1.3f, sf::Color::White);
            sf::RectangleShape tLid(sf::Vector2f(12.f, 1.5f));
            tLid.setPosition(tx - 1.f, ty + 1.f);
            tLid.setFillColor(sf::Color::White);
            window.draw(tLid);
        }
        if (btnGenreUpdate) {
            btnGenreUpdate->draw(window);
            // Save floppy icon on btnGenreUpdate
            float sx = rightColX + 172.f;
            float sy = 687.f;
            RoundedRectRenderer::render(window, sf::Vector2f(sx, sy), sf::Vector2f(13.f, 13.f), 2.f, sf::Color::Transparent, 1.4f, sf::Color::White);
            sf::RectangleShape sTop(sf::Vector2f(7.f, 4.f));
            sTop.setPosition(sx + 3.f, sy + 1.f);
            sTop.setFillColor(sf::Color::White);
            window.draw(sTop);
        }
        if (btnGenreClear) {
            btnGenreClear->draw(window);
            // Refresh circle arrow icon on btnGenreClear
            sf::CircleShape refArc(5.5f);
            refArc.setPosition(rightColX + 164.f, 742.f);
            refArc.setFillColor(sf::Color::Transparent);
            refArc.setOutlineThickness(1.4f);
            refArc.setOutlineColor(sf::Color(71, 85, 105));
            window.draw(refArc);
        }
    }
}

void CinemaUIApp::renderSeatBookingView() {
    float contentAreaW = 1728.f - sidebarWidth;
    float contentAreaH = 972.f - topbarHeight;

    // 1. Soft grey canvas background matching reference
    sf::RectangleShape posBg(sf::Vector2f(contentAreaW, contentAreaH));
    posBg.setPosition(sidebarWidth, topbarHeight);
    posBg.setFillColor(sf::Color(241, 245, 249)); // #f1f5f9
    window.draw(posBg);

    // 2. Top Header Bar for POS Kiosk Mode
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, topbarHeight));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color(15, 23, 42)); // Dark Navy #0f172a
    window.draw(topBar);

    // Wallet / Ticket Vector Icon
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(sidebarWidth + 24.f, (topbarHeight - 20.f) / 2.0f),
        sf::Vector2f(24.f, 20.f),
        3.f,
        sf::Color::Transparent,
        2.f,
        sf::Color::White
    );
    sf::RectangleShape walletClasp(sf::Vector2f(8.f, 6.f));
    walletClasp.setPosition(sidebarWidth + 38.f, (topbarHeight - 6.f) / 2.0f);
    walletClasp.setFillColor(sf::Color::White);
    window.draw(walletClasp);

    // Header Title
    sf::Text title;
    title.setFont(Theme::FontPrimary);
    title.setCharacterSize(18);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::White);
    title.setString(toSfString("BÁN VÉ – POS (KIOSK MODE)"));
    title.setPosition(sidebarWidth + 56.f, (topbarHeight - 22.f) / 2.0f);
    window.draw(title);

    if (btnPosRefresh) btnPosRefresh->draw(window);

    // ==========================================
    // 3. LEFT COLUMN: FILTER CARD + SEAT MAP
    // ==========================================
    float leftColX = sidebarWidth + 20.f;
    float leftColW = 1040.f;

    // Top Filter Card (White)
    float filterCardY = topbarHeight + 14.f;
    float filterCardH = 138.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, filterCardY),
        sf::Vector2f(leftColW, filterCardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240) // #e2e8f0
    );

    // Labels
    sf::Text lblDate;
    lblDate.setFont(Theme::FontPrimary);
    lblDate.setCharacterSize(14);
    lblDate.setStyle(sf::Text::Bold);
    lblDate.setFillColor(sf::Color(15, 23, 42));
    lblDate.setString(toSfString("Ngày:"));
    lblDate.setPosition(leftColX + 24.f, filterCardY + 20.f);
    window.draw(lblDate);

    sf::Text lblMovie;
    lblMovie.setFont(Theme::FontPrimary);
    lblMovie.setCharacterSize(14);
    lblMovie.setStyle(sf::Text::Bold);
    lblMovie.setFillColor(sf::Color(15, 23, 42));
    lblMovie.setString(toSfString("Phim:"));
    lblMovie.setPosition(leftColX + 24.f, filterCardY + 60.f);
    window.draw(lblMovie);

    sf::Text lblShowtime;
    lblShowtime.setFont(Theme::FontPrimary);
    lblShowtime.setCharacterSize(14);
    lblShowtime.setStyle(sf::Text::Bold);
    lblShowtime.setFillColor(sf::Color(15, 23, 42));
    lblShowtime.setString(toSfString("Suất:"));
    lblShowtime.setPosition(leftColX + 24.f, filterCardY + 100.f);
    window.draw(lblShowtime);

    if (btnPosDate) btnPosDate->draw(window);
    if (btnPosMovie) btnPosMovie->draw(window);
    if (btnPosShowtime) btnPosShowtime->draw(window);

    // Bottom Seat Map Container Card (White)
    float seatCardY = topbarHeight + 162.f;
    float seatCardH = 718.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, seatCardY),
        sf::Vector2f(leftColW, seatCardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (!posShowSeatMap) {
        // Centered hint matching reference image
        sf::Text hintText;
        hintText.setFont(Theme::FontPrimary);
        hintText.setCharacterSize(15);
        hintText.setStyle(sf::Text::Italic);
        hintText.setFillColor(sf::Color(148, 163, 184)); // #94a3b8
        hintText.setString(toSfString("← Chọn suất chiếu ở trên để hiển thị sơ đồ ghế"));
        sf::FloatRect hb = hintText.getLocalBounds();
        hintText.setPosition(leftColX + (leftColW - hb.width) / 2.0f, seatCardY + 330.f);
        window.draw(hintText);
    } else {
        // Screen projection bar
        float screenW = 600.f;
        float screenX = leftColX + (leftColW - screenW) / 2.0f;
        float screenY = seatCardY + 28.f;

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(screenX, screenY),
            sf::Vector2f(screenW, 10.f),
            5.f,
            sf::Color(14, 165, 233)
        );

        sf::Text screenText;
        screenText.setFont(Theme::FontPrimary);
        screenText.setCharacterSize(12);
        screenText.setStyle(sf::Text::Bold);
        screenText.setFillColor(sf::Color(100, 116, 139));
        screenText.setString(toSfString("MÀN HÌNH CHIẾU (SCREEN)"));
        sf::FloatRect stb = screenText.getLocalBounds();
        screenText.setPosition(screenX + (screenW - stb.width) / 2.0f, screenY + 18.f);
        window.draw(screenText);

        // Seat Grid
        for (const auto& s : currentSeats) {
            sf::Color seatBg = sf::Color::White;
            sf::Color seatBorder = sf::Color(203, 213, 225);
            sf::Color seatTxt = sf::Color(51, 65, 85);

            if (s.trangThai == "DaDat") {
                seatBg = sf::Color(226, 232, 240); // Grey booked
                seatBorder = sf::Color(203, 213, 225);
                seatTxt = sf::Color(148, 163, 184);
            } else if (s.isSelected) {
                seatBg = sf::Color(34, 197, 94);   // Green selected
                seatBorder = sf::Color(22, 163, 74);
                seatTxt = sf::Color::White;
            } else if (s.loaiGhe == "VIP") {
                seatBg = sf::Color(254, 243, 199); // Yellow VIP
                seatBorder = sf::Color(245, 158, 11);
                seatTxt = sf::Color(180, 83, 9);
            } else if (s.loaiGhe == "Sweetbox") {
                seatBg = sf::Color(252, 231, 243); // Pink Sweetbox
                seatBorder = sf::Color(236, 72, 153);
                seatTxt = sf::Color(190, 24, 93);
            }

            RoundedRectRenderer::render(
                window,
                s.position,
                s.size,
                6.f,
                seatBg,
                1.0f,
                seatBorder
            );

            sf::Text codeText;
            codeText.setFont(Theme::FontPrimary);
            codeText.setCharacterSize(11);
            codeText.setStyle(sf::Text::Bold);
            codeText.setFillColor(seatTxt);
            codeText.setString(s.maGhe);
            sf::FloatRect cb = codeText.getLocalBounds();
            codeText.setPosition(
                s.position.x + (s.size.x - cb.width) / 2.f - cb.left,
                s.position.y + (s.size.y - cb.height) / 2.f - cb.top
            );
            window.draw(codeText);
        }

        // Legend at bottom of seat card
        float legY = seatCardY + seatCardH - 46.f;
        float legX = leftColX + 40.f;
        std::vector<std::tuple<std::string, sf::Color, sf::Color>> legends = {
            {"Ghế Thường", sf::Color::White, sf::Color(203, 213, 225)},
            {"Ghế VIP", sf::Color(254, 243, 199), sf::Color(245, 158, 11)},
            {"Sweetbox", sf::Color(252, 231, 243), sf::Color(236, 72, 153)},
            {"Đang Chọn", sf::Color(34, 197, 94), sf::Color(22, 163, 74)},
            {"Đã Đặt", sf::Color(226, 232, 240), sf::Color(203, 213, 225)}
        };

        for (const auto& leg : legends) {
            RoundedRectRenderer::render(
                window,
                sf::Vector2f(legX, legY),
                sf::Vector2f(20.f, 20.f),
                4.f,
                std::get<1>(leg),
                1.f,
                std::get<2>(leg)
            );

            sf::Text lText;
            lText.setFont(Theme::FontPrimary);
            lText.setCharacterSize(12);
            lText.setFillColor(sf::Color(71, 85, 105));
            lText.setString(toSfString(std::get<0>(leg)));
            lText.setPosition(legX + 28.f, legY + 2.f);
            window.draw(lText);

            legX += 160.f;
        }
    }

    // ==========================================
    // 4. RIGHT COLUMN: CART & CHECKOUT PANEL
    // ==========================================
    float rightColX = sidebarWidth + 1076.f;
    float rightColW = 394.f;

    // Card 1: Ghế đang chọn
    float c1Y = topbarHeight + 14.f;
    float c1H = 246.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c1Y),
        sf::Vector2f(rightColW, c1H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text t1;
    t1.setFont(Theme::FontPrimary);
    t1.setCharacterSize(14);
    t1.setStyle(sf::Text::Bold);
    t1.setFillColor(sf::Color(2, 132, 199)); // Sky blue #0284c7
    t1.setString(toSfString("Ghế đang chọn"));
    t1.setPosition(rightColX + 16.f, c1Y + 14.f);
    window.draw(t1);

    sf::RectangleShape div1(sf::Vector2f(rightColW - 32.f, 1.f));
    div1.setPosition(rightColX + 16.f, c1Y + 38.f);
    div1.setFillColor(sf::Color(241, 245, 249));
    window.draw(div1);

    for (size_t i = 0; i < selectedSeatCodes.size() && i < 6; ++i) {
        sf::Text sItem;
        sItem.setFont(Theme::FontPrimary);
        sItem.setCharacterSize(13);
        sItem.setFillColor(sf::Color(51, 65, 85));
        sItem.setString(toSfString("• Ghế " + selectedSeatCodes[i] + " (VIP / Standard)"));
        sItem.setPosition(rightColX + 16.f, c1Y + 48.f + static_cast<float>(i) * 24.f);
        window.draw(sItem);
    }

    sf::Text seatSubtotal;
    seatSubtotal.setFont(Theme::FontPrimary);
    seatSubtotal.setCharacterSize(14);
    seatSubtotal.setStyle(sf::Text::Bold);
    seatSubtotal.setFillColor(sf::Color(2, 132, 199));
    seatSubtotal.setString(toSfString("Tiền ghế: " + formatVND(posSeatTotal)));
    sf::FloatRect ssb = seatSubtotal.getLocalBounds();
    seatSubtotal.setPosition(rightColX + rightColW - 16.f - ssb.width, c1Y + c1H - 28.f);
    window.draw(seatSubtotal);

    // Card 2: F&B đã chọn
    float c2Y = topbarHeight + 270.f;
    float c2H = 236.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c2Y),
        sf::Vector2f(rightColW, c2H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text t2;
    t2.setFont(Theme::FontPrimary);
    t2.setCharacterSize(14);
    t2.setStyle(sf::Text::Bold);
    t2.setFillColor(sf::Color(217, 119, 6)); // Amber #d97706
    t2.setString(toSfString("F&B đã chọn"));
    t2.setPosition(rightColX + 16.f, c2Y + 14.f);
    window.draw(t2);

    sf::RectangleShape div2(sf::Vector2f(rightColW - 32.f, 1.f));
    div2.setPosition(rightColX + 16.f, c2Y + 38.f);
    div2.setFillColor(sf::Color(241, 245, 249));
    window.draw(div2);

    for (size_t i = 0; i < posSelectedFnb.size() && i < 4; ++i) {
        sf::Text fItem;
        fItem.setFont(Theme::FontPrimary);
        fItem.setCharacterSize(13);
        fItem.setFillColor(sf::Color(51, 65, 85));
        fItem.setString(toSfString("• " + posSelectedFnb[i].first));
        fItem.setPosition(rightColX + 16.f, c2Y + 48.f + static_cast<float>(i) * 24.f);
        window.draw(fItem);
    }

    if (btnPosAddFnb) btnPosAddFnb->draw(window);

    sf::Text fnbSubtotal;
    fnbSubtotal.setFont(Theme::FontPrimary);
    fnbSubtotal.setCharacterSize(14);
    fnbSubtotal.setStyle(sf::Text::Bold);
    fnbSubtotal.setFillColor(sf::Color(217, 119, 6));
    fnbSubtotal.setString(toSfString("Tiền F&B: " + formatVND(posFnbTotal)));
    sf::FloatRect fsb = fnbSubtotal.getLocalBounds();
    fnbSubtotal.setPosition(rightColX + rightColW - 16.f - fsb.width, c2Y + c2H - 28.f);
    window.draw(fnbSubtotal);

    // Card 3: TỔNG CỘNG & Actions
    float c3Y = topbarHeight + 516.f;
    float c3H = 200.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, c3Y),
        sf::Vector2f(rightColW, c3H),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    sf::Text totTitle;
    totTitle.setFont(Theme::FontPrimary);
    totTitle.setCharacterSize(16);
    totTitle.setStyle(sf::Text::Bold);
    totTitle.setFillColor(sf::Color(15, 23, 42));
    totTitle.setString(toSfString("TỔNG CỘNG"));
    totTitle.setPosition(rightColX + 16.f, c3Y + 16.f);
    window.draw(totTitle);

    sf::Text totVal;
    totVal.setFont(Theme::FontPrimary);
    totVal.setCharacterSize(22);
    totVal.setStyle(sf::Text::Bold);
    totVal.setFillColor(sf::Color(220, 38, 38)); // Red #dc2626
    totVal.setString(toSfString(formatVND(posGrandTotal)));
    sf::FloatRect tvb = totVal.getLocalBounds();
    totVal.setPosition(rightColX + rightColW - 16.f - tvb.width, c3Y + 12.f);
    window.draw(totVal);

    if (btnPosCheckout) btnPosCheckout->draw(window);
    if (btnPosCancel) btnPosCancel->draw(window);
}

void CinemaUIApp::renderConcessionsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(Theme::SizeTitle);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẦY BẮP NƯỚC (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddFnb) btnAddFnb->draw(window);

    float startX = contentX;
    float startY = contentY + 60.f;
    float cardW = 420.f;
    float cardH = 110.f;
    float gap = 20.f;

    double fnbTotal = 0.0;

    for (size_t i = 0; i < fnbList.size(); ++i) {
        int row = static_cast<int>(i) / 2;
        int col = static_cast<int>(i) % 2;
        float cx = startX + static_cast<float>(col) * (cardW + gap);
        float cy = startY + static_cast<float>(row) * (cardH + gap);

        RoundedRectRenderer::render(
            window,
            sf::Vector2f(cx, cy),
            sf::Vector2f(cardW, cardH),
            Theme::RadiusMedium,
            Theme::CardBg,
            1.0f,
            Theme::CardBorder
        );

        sf::Text itemName;
        itemName.setFont(Theme::FontPrimary);
        itemName.setCharacterSize(Theme::SizeBody);
        itemName.setStyle(sf::Text::Bold);
        itemName.setFillColor(Theme::TextPrimary);
        itemName.setString(toSfString(fnbList[i].getTenMatHang()));
        itemName.setPosition(cx + 20.f, cy + 18.f);
        window.draw(itemName);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << fnbList[i].getGiaTien() << " đ";
        sf::Text priceText;
        priceText.setFont(Theme::FontPrimary);
        priceText.setCharacterSize(Theme::SizeSubheader);
        priceText.setFillColor(Theme::Cyan);
        priceText.setString(oss.str());
        priceText.setPosition(cx + 20.f, cy + 55.f);
        window.draw(priceText);

        // Counter: [-] [qty] [+]
        float minusX = cx + cardW - 90.f;
        float btnY = cy + 55.f;

        // Minus button
        RoundedRectRenderer::render(window, sf::Vector2f(minusX, btnY), sf::Vector2f(28.f, 28.f), 4.f, Theme::CardBorder);
        sf::Text minText;
        minText.setFont(Theme::FontPrimary);
        minText.setCharacterSize(16);
        minText.setFillColor(Theme::TextPrimary);
        minText.setString("-");
        minText.setPosition(minusX + 10.f, btnY + 2.f);
        window.draw(minText);

        // Quantity
        int qty = (i < fnbQuantities.size()) ? fnbQuantities[i] : 0;
        fnbTotal += static_cast<double>(qty) * fnbList[i].getGiaTien();

        sf::Text qtyText;
        qtyText.setFont(Theme::FontPrimary);
        qtyText.setCharacterSize(16);
        qtyText.setStyle(sf::Text::Bold);
        qtyText.setFillColor(Theme::TextPrimary);
        qtyText.setString(std::to_string(qty));
        qtyText.setPosition(minusX + 36.f, btnY + 2.f);
        window.draw(qtyText);

        // Plus button
        float plusX = cx + cardW - 38.f;
        RoundedRectRenderer::render(window, sf::Vector2f(plusX, btnY), sf::Vector2f(28.f, 28.f), 4.f, Theme::Primary);
        sf::Text plusText;
        plusText.setFont(Theme::FontPrimary);
        plusText.setCharacterSize(16);
        plusText.setFillColor(Theme::TextPrimary);
        plusText.setString("+");
        plusText.setPosition(plusX + 8.f, btnY + 2.f);
        window.draw(plusText);
    }

    // Right Summary Panel
    float panelX = sidebarWidth + 930.f;
    float panelY = contentY + 60.f;
    float panelW = 440.f;
    float panelH = 700.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(panelX, panelY),
        sf::Vector2f(panelW, panelH),
        Theme::RadiusLarge,
        Theme::CardBg,
        1.5f,
        Theme::CardBorder
    );

    sf::Text sumTitle;
    sumTitle.setFont(Theme::FontTitle);
    sumTitle.setCharacterSize(Theme::SizeHeader);
    sumTitle.setFillColor(Theme::TextPrimary);
    sumTitle.setString(toSfString("HÓA ĐƠN BẮP NƯỚC"));
    sumTitle.setPosition(panelX + 24.f, panelY + 24.f);
    window.draw(sumTitle);

    std::ostringstream totalOss;
    totalOss << std::fixed << std::setprecision(0) << fnbTotal << " đ";

    sf::Text totalLabel;
    totalLabel.setFont(Theme::FontPrimary);
    totalLabel.setCharacterSize(Theme::SizeSubheader);
    totalLabel.setFillColor(Theme::TextSecondary);
    totalLabel.setString(toSfString("TỔNG TIỀN F&B:"));
    totalLabel.setPosition(panelX + 24.f, panelY + 430.f);
    window.draw(totalLabel);

    sf::Text totalVal;
    totalVal.setFont(Theme::FontTitle);
    totalVal.setCharacterSize(36);
    totalVal.setStyle(sf::Text::Bold);
    totalVal.setFillColor(Theme::Success);
    totalVal.setString(totalOss.str());
    totalVal.setPosition(panelX + 24.f, panelY + 460.f);
    window.draw(totalVal);

    if (btnCheckoutFnb) btnCheckoutFnb->draw(window);
}

void CinemaUIApp::renderReportsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(Theme::SizeTitle);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("BÁO CÁO THỐNG KÊ DOANH THU"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnReportAll) btnReportAll->draw(window);
    if (btnReportCash) btnReportCash->draw(window);
    if (btnReportBank) btnReportBank->draw(window);

    // Report chart container
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(contentX, contentY + 60.f),
        sf::Vector2f(1370.f, 740.f),
        Theme::RadiusLarge,
        Theme::CardBg,
        1.0f,
        Theme::CardBorder
    );

    // Visual Bar Chart for Movies
    sf::Text chartTitle;
    chartTitle.setFont(Theme::FontTitle);
    chartTitle.setCharacterSize(Theme::SizeHeader);
    chartTitle.setFillColor(Theme::Cyan);
    chartTitle.setString(toSfString("DOANH THU THEO TỪNG PHIM (DỮ LIỆU THỰC TẾ)"));
    chartTitle.setPosition(contentX + 30.f, contentY + 90.f);
    window.draw(chartTitle);

    float barStartY = contentY + 160.f;
    float barMaxW = 750.f;

    std::vector<std::pair<std::string, double>> chartData;
    for (const auto& m : movieList) {
        chartData.push_back({m.getTenPhim(), static_cast<double>(m.getThoiLuong() * 350000)});
    }
    if (chartData.empty()) {
        chartData.push_back({"Lat Mat 7", 85000000.0});
        chartData.push_back({"Mai", 62000000.0});
    }

    double maxVal = 100000000.0;
    for (const auto& cd : chartData) {
        if (cd.second > maxVal) maxVal = cd.second;
    }

    for (size_t i = 0; i < chartData.size() && i < 6; ++i) {
        float by = barStartY + static_cast<float>(i) * 65.f;

        sf::Text mText;
        mText.setFont(Theme::FontPrimary);
        mText.setCharacterSize(Theme::SizeBody);
        mText.setFillColor(Theme::TextPrimary);
        mText.setString(toSfString(chartData[i].first));
        mText.setPosition(contentX + 30.f, by);
        window.draw(mText);

        // Bar background
        float bw = static_cast<float>((chartData[i].second / maxVal) * barMaxW);
        RoundedRectRenderer::render(window, sf::Vector2f(contentX + 30.f, by + 26.f), sf::Vector2f(barMaxW, 14.f), 7.f, Theme::BgDark);
        RoundedRectRenderer::render(window, sf::Vector2f(contentX + 30.f, by + 26.f), sf::Vector2f(bw, 14.f), 7.f, (i == 0) ? Theme::Primary : Theme::Cyan);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << chartData[i].second / 1000000.0 << " Triệu đ";
        sf::Text vText;
        vText.setFont(Theme::FontPrimary);
        vText.setCharacterSize(Theme::SizeSmall);
        vText.setFillColor(Theme::Success);
        vText.setString(oss.str());
        vText.setPosition(contentX + 30.f + bw + 15.f, by + 24.f);
        window.draw(vText);
    }
}

void CinemaUIApp::renderLogsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(Theme::SizeTitle);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("NHẬT KÝ HOẠT ĐỘNG VÀ VẬN HÀNH HỆ THỐNG (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnRefreshLogs) btnRefreshLogs->draw(window);

    if (logsTable) logsTable->draw(window);
}

void CinemaUIApp::renderCRMView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ KHÁCH HÀNG & HỘI VIÊN (CRM - SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddCustomer) btnAddCustomer->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Dữ liệu hội viên đọc trực tiếp từ bảng users & customers trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    int countStandard = 0, countVip = 0, countDiamond = 0;
    for (const auto& kh : customerList) {
        std::string tier = kh.getHangThanhVien();
        if (tier.find("Kim") != std::string::npos || tier.find("Diamond") != std::string::npos || tier.find("Vang") != std::string::npos) {
            countDiamond++;
        } else if (tier.find("VIP") != std::string::npos || tier.find("Bac") != std::string::npos) {
            countVip++;
        } else {
            countStandard++;
        }
    }

    // 3 Tier Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> tiers = {
        {"HỘI VIÊN TIÊU CHUẨN / ĐỒNG", std::to_string(countStandard) + " Hội Viên", "Tích lũy điểm thưởng trực tiếp trên cinema.db", sf::Color(6, 182, 212)},
        {"HỘI VIÊN BẠC / VIP", std::to_string(countVip) + " Hội Viên", "Tích lũy điểm & ưu đãi giảm giá F&B", sf::Color(245, 158, 11)},
        {"HỘI VIÊN VÀNG / KIM CƯƠNG", std::to_string(countDiamond) + " Hội Viên", "Tổng số khách hàng hệ thống: " + std::to_string(customerList.size()), sf::Color(168, 85, 247)}
    };

    float cardW = 440.f;
    float cardH = 130.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < tiers.size(); ++i) {
        float cx = contentX + i * (cardW + gap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(tiers[i]));

        sf::Text tName;
        tName.setFont(Theme::FontTitle);
        tName.setCharacterSize(16);
        tName.setFillColor(std::get<3>(tiers[i]));
        tName.setString(toSfString(std::get<0>(tiers[i])));
        tName.setPosition(cx + 20.f, startY + 16.f);
        window.draw(tName);

        sf::Text tCount;
        tCount.setFont(Theme::FontTitle);
        tCount.setCharacterSize(26);
        tCount.setFillColor(Theme::TextPrimary);
        tCount.setString(toSfString(std::get<1>(tiers[i])));
        tCount.setPosition(cx + 20.f, startY + 44.f);
        window.draw(tCount);

        sf::Text tDesc;
        tDesc.setFont(Theme::FontSecondary);
        tDesc.setCharacterSize(12);
        tDesc.setFillColor(Theme::TextSecondary);
        tDesc.setString(toSfString(std::get<2>(tiers[i])));
        tDesc.setPosition(cx + 20.f, startY + 90.f);
        window.draw(tDesc);
    }

    // Recent Customers Table Card
    float tblY = startY + cardH + 28.f;
    float tblW = 1360.f;
    float tblH = 430.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    sf::Text tblTitle;
    tblTitle.setFont(Theme::FontTitle);
    tblTitle.setCharacterSize(16);
    tblTitle.setFillColor(Theme::TextPrimary);
    tblTitle.setString(toSfString("DANH SÁCH KHÁCH HÀNG TRONG CƠ SỞ DỮ LIỆU SQLITE"));
    tblTitle.setPosition(contentX + 24.f, tblY + 18.f);
    window.draw(tblTitle);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 54.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ KH / SĐT", "TÊN KHÁCH HÀNG", "HẠNG THẺ", "ĐIỂM TÍCH LŨY", "TỔNG CHI TIÊU", "EMAIL"};
    std::vector<float> colX = {contentX + 40.f, contentX + 240.f, contentX + 520.f, contentX + 720.f, contentX + 920.f, contentX + 1120.f};

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 64.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < customerList.size() && r < 7; ++r) {
        float ry = tblY + 102.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& kh = customerList[r];
        std::vector<std::string> vals = {
            kh.getMaKH() + " - " + kh.getSoDienThoai(),
            kh.getHoTen(),
            kh.getHangThanhVien(),
            std::to_string(kh.getDiemTichLuy()) + " Điểm",
            std::to_string(kh.getTongChiTieu()) + " đ",
            kh.getEmail()
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 2) ? Theme::Cyan : ((c == 4) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

void CinemaUIApp::renderRoomsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ PHÒNG CHIẾU CỤM RẠP"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddRoom) btnAddRoom->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Danh sách phòng chiếu phim kỹ thuật số, công nghệ âm thanh và màn chiếu CineStar"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float cardW = 325.f;
    float cardH = 180.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < roomList.size() && i < 8; ++i) {
        int row = static_cast<int>(i / 4);
        int col = static_cast<int>(i % 4);
        float cx = contentX + col * (cardW + gap);
        float cy = startY + row * (cardH + gap);

        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, 4.f), 2.f, Theme::Cyan);

        sf::Text rName;
        rName.setFont(Theme::FontTitle);
        rName.setCharacterSize(18);
        rName.setFillColor(Theme::Cyan);
        rName.setString(toSfString(roomList[i].getTenPhong()));
        rName.setPosition(cx + 20.f, cy + 18.f);
        window.draw(rName);

        sf::Text rCode;
        rCode.setFont(Theme::FontPrimary);
        rCode.setCharacterSize(12);
        rCode.setFillColor(Theme::TextSecondary);
        rCode.setString(toSfString("Mã: " + roomList[i].getMaPhong() + " | Định Dạng: 2D/3D Digital"));
        rCode.setPosition(cx + 20.f, cy + 50.f);
        window.draw(rCode);

        sf::Text rCap;
        rCap.setFont(Theme::FontPrimary);
        rCap.setCharacterSize(12);
        rCap.setFillColor(Theme::TextPrimary);
        rCap.setString(toSfString("Sức chứa: " + std::to_string(roomList[i].getSucChua()) + " Ghế"));
        rCap.setPosition(cx + 20.f, cy + 76.f);
        window.draw(rCap);

        sf::Text rTech;
        rTech.setFont(Theme::FontSecondary);
        rTech.setCharacterSize(11);
        rTech.setFillColor(Theme::TextSecondary);
        rTech.setString(toSfString("Máy chiếu: 4K Laser | Âm thanh: Dolby 7.1"));
        rTech.setPosition(cx + 20.f, cy + 104.f);
        window.draw(rTech);

        // Status badge
        RoundedRectRenderer::render(window, sf::Vector2f(cx + 20.f, cy + 134.f), sf::Vector2f(110.f, 24.f), 12.f, sf::Color(16, 185, 129, 30));
        sf::Text sBadge;
        sBadge.setFont(Theme::FontPrimary);
        sBadge.setCharacterSize(10);
        sBadge.setStyle(sf::Text::Bold);
        sBadge.setFillColor(Theme::Success);
        sBadge.setString(toSfString("HOẠT ĐỘNG TỐT"));
        sBadge.setPosition(cx + 30.f, cy + 138.f);
        window.draw(sBadge);
    }
}

void CinemaUIApp::renderSeatsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ SƠ ĐỒ VÀ PHÂN LOẠI GHẾ"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAutoGenerateSeats) btnAutoGenerateSeats->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Cấu hình sơ đồ chỗ ngồi, quy chuẩn ghế và biểu giá vé rạp CineStar"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    // 4 Stat Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> seatStats = {
        {"TỔNG SỐ GHẾ QUẢN LÝ", std::to_string(allSeats.size()) + " Ghế", "Phân bố 7 phòng chiếu", Theme::Primary},
        {"GHẾ TIÊU CHUẨN (STANDARD)", "680 Ghế", "Giá niêm yết: 75.000 VNĐ", Theme::SeatStandard},
        {"GHẾ VIP CAO CẤP", "280 Ghế", "Giá niêm yết: 95.000 VNĐ", Theme::SeatVIP},
        {"GHẾ ĐÔI (SWEETBOX)", "60 Ghế", "Giá niêm yết: 180.000 VNĐ", Theme::SeatSweetbox}
    };

    float cardW = 325.f;
    float cardH = 130.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < seatStats.size(); ++i) {
        float cx = contentX + i * (cardW + gap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(seatStats[i]));

        sf::Text tName;
        tName.setFont(Theme::FontTitle);
        tName.setCharacterSize(14);
        tName.setFillColor(std::get<3>(seatStats[i]));
        tName.setString(toSfString(std::get<0>(seatStats[i])));
        tName.setPosition(cx + 20.f, startY + 16.f);
        window.draw(tName);

        sf::Text tCount;
        tCount.setFont(Theme::FontTitle);
        tCount.setCharacterSize(26);
        tCount.setFillColor(Theme::TextPrimary);
        tCount.setString(toSfString(std::get<1>(seatStats[i])));
        tCount.setPosition(cx + 20.f, startY + 44.f);
        window.draw(tCount);

        sf::Text tDesc;
        tDesc.setFont(Theme::FontSecondary);
        tDesc.setCharacterSize(12);
        tDesc.setFillColor(Theme::TextSecondary);
        tDesc.setString(toSfString(std::get<2>(seatStats[i])));
        tDesc.setPosition(cx + 20.f, startY + 90.f);
        window.draw(tDesc);
    }

    // Preview Layout Box
    float prevY = startY + cardH + 28.f;
    float prevW = 1360.f;
    float prevH = 430.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, prevY), sf::Vector2f(prevW, prevH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    sf::Text prevTitle;
    prevTitle.setFont(Theme::FontTitle);
    prevTitle.setCharacterSize(16);
    prevTitle.setFillColor(Theme::TextPrimary);
    prevTitle.setString(toSfString("MÔ PHỎNG SƠ ĐỒ CHỖ NGỒI RẠP CHIẾU"));
    prevTitle.setPosition(contentX + 24.f, prevY + 18.f);
    window.draw(prevTitle);

    // Screen Banner
    RoundedRectRenderer::render(window, sf::Vector2f(contentX + 380.f, prevY + 55.f), sf::Vector2f(600.f, 24.f), 6.f, sf::Color(51, 65, 85));
    sf::Text scrText;
    scrText.setFont(Theme::FontPrimary);
    scrText.setCharacterSize(11);
    scrText.setStyle(sf::Text::Bold);
    scrText.setFillColor(sf::Color(148, 163, 184));
    scrText.setString(toSfString("MÀN CHIẾU CHÍNH (SCREEN)"));
    scrText.setPosition(contentX + 590.f, prevY + 59.f);
    window.draw(scrText);

    // Mini preview seats
    int rows = 6;
    int cols = 14;
    float seatSz = 26.f;
    float sGap = 8.f;
    float sStartX = contentX + (prevW - (cols * (seatSz + sGap))) / 2.f;
    float sStartY = prevY + 110.f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float sx = sStartX + c * (seatSz + sGap);
            float sy = sStartY + r * (seatSz + sGap);
            sf::Color sc = (r >= 4) ? Theme::SeatSweetbox : ((r >= 2) ? Theme::SeatVIP : Theme::SeatStandard);
            RoundedRectRenderer::render(window, sf::Vector2f(sx, sy), sf::Vector2f(seatSz, seatSz), 4.f, sc);
        }
    }

    // Legend at bottom
    float legY = prevY + 360.f;
    std::vector<std::pair<std::string, sf::Color>> leg = {
        {"Ghế Thường (Standard)", Theme::SeatStandard},
        {"Ghế VIP Trung Tâm", Theme::SeatVIP},
        {"Ghế Đôi Sweetbox", Theme::SeatSweetbox}
    };
    for (size_t l = 0; l < leg.size(); ++l) {
        float lx = contentX + 400.f + l * 220.f;
        RoundedRectRenderer::render(window, sf::Vector2f(lx, legY), sf::Vector2f(16.f, 16.f), 3.f, leg[l].second);
        sf::Text lt;
        lt.setFont(Theme::FontPrimary);
        lt.setCharacterSize(12);
        lt.setFillColor(Theme::TextPrimary);
        lt.setString(toSfString(leg[l].first));
        lt.setPosition(lx + 24.f, legY);
        window.draw(lt);
    }
}

void CinemaUIApp::renderShowtimesView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ LỊCH VÀ SUẤT CHIẾU"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddShowtime) btnAddShowtime->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Khung giờ chiếu các phim theo từng phòng chiếu tại cụm rạp CineStar"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float tblY = contentY + 70.f;
    float tblW = 1360.f;
    float tblH = 620.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 20.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ SUẤT", "TÊN PHIM CHIẾU", "PHÒNG", "GIỜ CHIẾU", "NGÀY CHIẾU", "GIÁ VÉ", "TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 160.f, contentX + 540.f, contentX + 680.f, contentX + 850.f, contentX + 1020.f, contentX + 1180.f};

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 30.f);
        window.draw(ch);
    }

    // Rows
    for (size_t i = 0; i < showtimeList.size() && i < 11; ++i) {
        float ry = tblY + 68.f + i * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        std::string mName = showtimeList[i].getMaPhim();
        for (const auto& m : movieList) {
            if (m.getMaPhim() == showtimeList[i].getMaPhim()) {
                mName = m.getTenPhim();
                break;
            }
        }

        std::vector<std::string> vals = {
            showtimeList[i].getMaSuatChieu(),
            mName,
            showtimeList[i].getMaPhong(),
            showtimeList[i].getGioBatDau() + " - " + showtimeList[i].getGioKetThuc(),
            showtimeList[i].getNgayChieu(),
            "85.000 đ",
            "Đang Mở Bán"
        };

        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 0) ? Theme::Cyan : ((c == 6) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

void CinemaUIApp::renderShiftReportView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("BÁO CÁO DOANH THU & KẾT TOÁN CA (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnApproveShift) btnApproveShift->draw(window);
    if (btnCloseShift) btnCloseShift->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Dữ liệu ca trực đọc trực tiếp từ bảng shift_managements trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    long long totalSysRev = 0, totalActualCash = 0, totalDiff = 0;
    for (const auto& s : shiftList) {
        totalSysRev += s.getDoanhThuHeThong();
        totalActualCash += s.getTienThucTe();
        totalDiff += s.getChenhLech();
    }

    // 3 Summary Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> shifts = {
        {"TỔNG DOANH THU HỆ THỐNG", std::to_string(totalSysRev) + " VNĐ", "Tổng số ca ghi nhận: " + std::to_string(shiftList.size()) + " ca", Theme::Cyan},
        {"TỔNG TIỀN MẶT THỰC TẾ", std::to_string(totalActualCash) + " VNĐ", "Tiền mặt kiểm kê trong ngăn kéo", Theme::Warning},
        {"CHÊNH LỆCH KẾT TOÁN", std::to_string(totalDiff) + " VNĐ", (totalDiff == 0 ? "Khớp 100% két tiền mặt" : "Có chênh lệch cần đối soát"), Theme::Success}
    };

    float cardW = 440.f;
    float cardH = 130.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < shifts.size(); ++i) {
        float cx = contentX + i * (cardW + gap);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, startY), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(shifts[i]));

        sf::Text tName;
        tName.setFont(Theme::FontTitle);
        tName.setCharacterSize(15);
        tName.setFillColor(std::get<3>(shifts[i]));
        tName.setString(toSfString(std::get<0>(shifts[i])));
        tName.setPosition(cx + 20.f, startY + 16.f);
        window.draw(tName);

        sf::Text tCount;
        tCount.setFont(Theme::FontTitle);
        tCount.setCharacterSize(26);
        tCount.setFillColor(Theme::TextPrimary);
        tCount.setString(toSfString(std::get<1>(shifts[i])));
        tCount.setPosition(cx + 20.f, startY + 44.f);
        window.draw(tCount);

        sf::Text tDesc;
        tDesc.setFont(Theme::FontSecondary);
        tDesc.setCharacterSize(12);
        tDesc.setFillColor(Theme::TextSecondary);
        tDesc.setString(toSfString(std::get<2>(shifts[i])));
        tDesc.setPosition(cx + 20.f, startY + 90.f);
        window.draw(tDesc);
    }

    // Shift table from SQLite
    float tblY = startY + cardH + 28.f;
    float tblW = 1360.f;
    float tblH = 420.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 20.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ CA", "NHÂN VIÊN", "NGÀY LÀM", "KHUNG GIỜ", "DOANH THU HT", "TIỀN THỰC TẾ", "TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 160.f, contentX + 420.f, contentX + 580.f, contentX + 760.f, contentX + 980.f, contentX + 1180.f};
    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 30.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < shiftList.size() && r < 7; ++r) {
        float ry = tblY + 68.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& s = shiftList[r];
        std::vector<std::string> vals = {
            s.getMaCa(),
            s.getMaNhanVien() + " - " + s.getTenNhanVien(),
            s.getNgayLam(),
            s.getGioBatDau() + " - " + s.getGioKetThuc(),
            std::to_string(s.getDoanhThuHeThong()) + " đ",
            std::to_string(s.getTienThucTe()) + " đ",
            s.getTrangThai()
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 0) ? Theme::Cyan : ((c == 6) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

void CinemaUIApp::renderPromotionsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("CHƯƠNG TRÌNH KHUYẾN MÃI & VOUCHER (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddPromo) btnAddPromo->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Danh sách khuyến mãi đọc trực tiếp từ bảng promotions trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    std::vector<sf::Color> palette = {
        sf::Color(6, 182, 212),
        sf::Color(16, 185, 129),
        sf::Color(245, 158, 11),
        sf::Color(236, 72, 153)
    };

    float cardW = 660.f;
    float cardH = 150.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < promoList.size() && i < 8; ++i) {
        int r = static_cast<int>(i / 2);
        int c = static_cast<int>(i % 2);
        float cx = contentX + c * (cardW + gap);
        float cy = startY + r * (cardH + gap);
        sf::Color accent = palette[i % palette.size()];

        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, 4.f), 2.f, accent);

        const auto& pr = promoList[i];
        sf::Text pName;
        pName.setFont(Theme::FontTitle);
        pName.setCharacterSize(18);
        pName.setFillColor(accent);
        pName.setString(toSfString(pr.getMaKhuyenMai() + " - " + pr.getTenKhuyenMai()));
        pName.setPosition(cx + 24.f, cy + 20.f);
        window.draw(pName);

        std::ostringstream descOss;
        descOss << "Giảm " << static_cast<int>(pr.getPhanTramGiam()) << "% (Tối đa "
                << static_cast<long long>(pr.getMucGiamToiDa()) << " đ) | Phạm vi: " << pr.getPhamViApDung();
        sf::Text pDesc;
        pDesc.setFont(Theme::FontPrimary);
        pDesc.setCharacterSize(13);
        pDesc.setFillColor(Theme::TextPrimary);
        pDesc.setString(toSfString(descOss.str()));
        pDesc.setPosition(cx + 24.f, cy + 54.f);
        window.draw(pDesc);

        sf::Text pCode;
        pCode.setFont(Theme::FontSecondary);
        pCode.setCharacterSize(12);
        pCode.setFillColor(Theme::TextSecondary);
        pCode.setString(toSfString("Mã Code: " + pr.getMaCode() + " | Thời hạn: " + pr.getNgayBatDau() + " -> " + pr.getNgayKetThuc()));
        pCode.setPosition(cx + 24.f, cy + 96.f);
        window.draw(pCode);
    }
}

void CinemaUIApp::renderStaffView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ NHÂN SỰ & CÁN BỘ ĐIỀU HÀNH (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddStaff) btnAddStaff->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Danh sách nhân sự đọc trực tiếp từ bảng users trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float tblY = contentY + 70.f;
    float tblW = 1360.f;
    float tblH = 500.f;
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, tblY), sf::Vector2f(tblW, tblH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 20.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"MÃ NV", "HỌ VÀ TÊN", "VAI TRÒ", "SỐ ĐIỆN THOẠI", "EMAIL LIÊN HỆ", "TÀI KHOẢN / TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 160.f, contentX + 420.f, contentX + 580.f, contentX + 780.f, contentX + 1060.f};

    for (size_t c = 0; c < cols.size(); ++c) {
        sf::Text ch;
        ch.setFont(Theme::FontPrimary);
        ch.setCharacterSize(11);
        ch.setStyle(sf::Text::Bold);
        ch.setFillColor(Theme::TextSecondary);
        ch.setString(toSfString(cols[c]));
        ch.setPosition(colX[c], tblY + 30.f);
        window.draw(ch);
    }

    for (size_t r = 0; r < staffUserList.size() && r < 9; ++r) {
        float ry = tblY + 68.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        const auto& st = staffUserList[r];
        std::vector<std::string> vals = {
            st.maNguoiDung,
            st.hoTen,
            st.vaiTro,
            st.soDienThoai,
            st.email,
            st.taiKhoan + " (" + st.trangThaiLamViec + ")"
        };
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 0) ? Theme::Cyan : ((c == 5) ? Theme::Success : Theme::TextPrimary));
            cv.setString(toSfString(vals[c]));
            cv.setPosition(colX[c], ry + 8.f);
            window.draw(cv);
        }
    }
}

void CinemaUIApp::renderAccountsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("QUẢN LÝ TÀI KHOẢN & PHÂN QUYỀN (SQLite cinema.db)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnChangePassword) btnChangePassword->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Tài khoản đăng nhập đồng bộ trực tiếp từ bảng users trong DATA/cinema.db"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    float cardW = 660.f;
    float cardH = 160.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < staffUserList.size() && i < 6; ++i) {
        int r = static_cast<int>(i / 2);
        int c = static_cast<int>(i % 2);
        float cx = contentX + c * (cardW + gap);
        float cy = startY + r * (cardH + gap);
        const auto& acc = staffUserList[i];
        bool isAdmin = (acc.vaiTro == "Admin");
        sf::Color accent = isAdmin ? Theme::Primary : Theme::Cyan;

        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, 4.f), 2.f, accent);

        sf::Text aTitle;
        aTitle.setFont(Theme::FontTitle);
        aTitle.setCharacterSize(17);
        aTitle.setFillColor(accent);
        aTitle.setString(toSfString(acc.maNguoiDung + " - " + acc.hoTen + " (" + acc.vaiTro + ")"));
        aTitle.setPosition(cx + 24.f, cy + 18.f);
        window.draw(aTitle);

        sf::Text aDesc;
        aDesc.setFont(Theme::FontPrimary);
        aDesc.setCharacterSize(13);
        aDesc.setFillColor(Theme::TextPrimary);
        aDesc.setString(toSfString("Tài khoản: " + acc.taiKhoan + " | Email: " + acc.email));
        aDesc.setPosition(cx + 24.f, cy + 54.f);
        window.draw(aDesc);

        sf::Text aPerm;
        aPerm.setFont(Theme::FontSecondary);
        aPerm.setCharacterSize(12);
        aPerm.setFillColor(Theme::TextSecondary);
        aPerm.setString(toSfString("Trạng thái làm việc: " + acc.trangThaiLamViec + " | Trạng thái TK: " + acc.trangThaiTaiKhoan));
        aPerm.setPosition(cx + 24.f, cy + 92.f);
        window.draw(aPerm);
    }
}

} // namespace UI
