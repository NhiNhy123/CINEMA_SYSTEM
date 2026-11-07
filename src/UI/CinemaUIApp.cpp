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
      selectedGenreChoiceIdx(0)
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

    // Read real data directly from CSV files via Database class
    db.docDanhSachPhim(movieList);
    db.docDanhSachPhong(roomList);
    db.docDanhSachGhe(allSeats);
    db.docDanhSachSuatChieu(showtimeList);
    db.docDanhSachFAndB(fnbList);
    db.docDanhSachTheLoai(genreList);

    fnbQuantities.assign(fnbList.size(), 0);
}

void CinemaUIApp::setupLoginView() {
    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f
    float cardY = (972.f - cardH) / 2.0f;  // 206.f
    float leftW = 420.f;
    float rightX = cardX + leftW + 45.f;   // 819.f
    float rightW = cardW - leftW - 90.f;   // 510.f

    loginInputUser = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightX, cardY + 170.f),
        sf::Vector2f(rightW, 46.f),
        "Nhập mã nhân viên hoặc email...",
        Theme::SizeBody
    ));
    loginInputUser->setTextInset(44.f);
    loginInputUser->setCornerRadius(8.f);
    loginInputUser->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(234, 88, 12),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    loginInputPass = std::unique_ptr<PasswordField>(new PasswordField(
        sf::Vector2f(rightX, cardY + 262.f),
        sf::Vector2f(rightW, 46.f),
        "••••••••••••",
        Theme::SizeBody
    ));
    loginInputPass->setTextInset(44.f);
    loginInputPass->setCornerRadius(8.f);
    loginInputPass->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(234, 88, 12),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    chkRememberMe = std::unique_ptr<UICheckboxItem>(new UICheckboxItem(
        "Ghi nhớ đăng nhập",
        sf::Vector2f(rightX, cardY + 326.f),
        true,
        13
    ));
    chkRememberMe->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(234, 88, 12),
        sf::Color(100, 116, 139)
    );

    btnLogin = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "ĐĂNG NHẬP HỆ THỐNG  →",
        sf::Vector2f(rightX, cardY + 368.f),
        sf::Vector2f(rightW, 48.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnLogin->setCustomColors(
        sf::Color(234, 88, 12),
        sf::Color(217, 80, 8),
        sf::Color(194, 65, 12),
        sf::Color(203, 213, 225)
    );
    btnLogin->setFont(Theme::FontPrimary);
    btnLogin->setFontSize(15);

    btnQuickAdmin = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Mẫu Admin: admin01 / 123456",
        sf::Vector2f(rightX, cardY + 434.f),
        sf::Vector2f(250.f, 32.f),
        ButtonStyle::Dark,
        6.f
    ));
    btnQuickAdmin->setCustomColors(
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(203, 213, 225)
    );
    btnQuickAdmin->setOutline(1.f, sf::Color(203, 213, 225));
    btnQuickAdmin->setFontSize(12);

    btnQuickStaff = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Mẫu Staff: staff01 / 123456",
        sf::Vector2f(rightX + 265.f, cardY + 434.f),
        sf::Vector2f(250.f, 32.f),
        ButtonStyle::Dark,
        6.f
    ));
    btnQuickStaff->setCustomColors(
        sf::Color(248, 250, 252),
        sf::Color(241, 245, 249),
        sf::Color(226, 232, 240),
        sf::Color(203, 213, 225)
    );
    btnQuickStaff->setOutline(1.f, sf::Color(203, 213, 225));
    btnQuickStaff->setFontSize(12);

    // Handle button actions
    btnLogin->setOnClick([this]() {
        std::string u = loginInputUser->getText();
        std::string p = loginInputPass->getText();
        authenticateUser(u, p);
    });

    btnQuickAdmin->setOnClick([this]() {
        loginInputUser->setText("admin01");
        loginInputPass->setText("123456");
        loginErrorMsg = "";
    });

    btnQuickStaff->setOnClick([this]() {
        loginInputUser->setText("staff01");
        loginInputPass->setText("123456");
        loginErrorMsg = "";
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

    // Read real users from DATA/quan_ly_nhan_su.csv
    std::ifstream file("DATA/quan_ly_nhan_su.csv");
    if (!file.is_open()) {
        loginErrorMsg = "Không thể mở file DATA/quan_ly_nhan_su.csv!";
        return false;
    }

    std::string line;
    bool inUserSection = false;
    bool found = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.find("[USERS]") != std::string::npos) {
            inUserSection = true;
            continue;
        }
        if (inUserSection && line[0] == '[') {
            break;
        }

        if (inUserSection && line.find("maNguoiDung") == std::string::npos) {
            std::stringstream ss(line);
            std::string item;
            std::vector<std::string> parts;
            while (std::getline(ss, item, ',')) {
                parts.push_back(item);
            }

            // CSV format: maNguoiDung,hoTen,soDienThoai,email,ngaySinh,gioiTinh,cccd,taiKhoan,matKhau,vaiTro,...
            if (parts.size() >= 10) {
                std::string csvEmail = parts[3];
                std::string csvAcc = parts[7];
                std::string csvPass = parts[8];
                bool identityMatches =
                    parts[0] == inputUser || csvEmail == inputUser || csvAcc == inputUser;

                if (identityMatches && csvPass == inputPass) {
                    currentUserId = parts[0];
                    currentUserName = parts[1];
                    currentUserRole = parts[9];
                    found = true;
                    break;
                }
            }
        }
    }

    if (found) {
        isLoggedIn = true;
        loginErrorMsg = "";

        if (currentUserRole == "Admin") {
            currentTab = ActiveTab::Reports;
        } else {
            currentTab = ActiveTab::SeatBooking;
        }
        setupNavigation();

        // Record real activity log into CSV
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_NHAP", "Dang nhap thanh cong qua giao dien UI");
        setupLogsView(); // refresh log table
        return true;
    } else {
        loginErrorMsg = "Tài khoản hoặc mật khẩu không chính xác!";
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
                "Đã tải lại toàn bộ nhật ký kiểm toán mới nhất từ nhat_ky_hoat_dong.csv!",
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
    float leftColX = sidebarWidth + 20.f;
    float rightColX = sidebarWidth + 960.f;
    float rightColW = 494.f;

    // Sub-tab buttons
    btnSubTabMovies = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Quản lý Phim",
        sf::Vector2f(leftColX + 4.f, 74.f),
        sf::Vector2f(130.f, 34.f),
        ButtonStyle::Ghost,
        4.f
    ));
    btnSubTabMovies->setFontSize(15);
    btnSubTabMovies->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 60),
        sf::Color(203, 213, 225),
        sf::Color::Transparent
    );
    btnSubTabMovies->setTextColor(sf::Color(15, 23, 42));
    btnSubTabMovies->setOnClick([this]() {
        movieSubTab = 0;
    });

    btnSubTabGenres = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Quản lý Thể loại",
        sf::Vector2f(leftColX + 145.f, 74.f),
        sf::Vector2f(150.f, 34.f),
        ButtonStyle::Ghost,
        4.f
    ));
    btnSubTabGenres->setFontSize(15);
    btnSubTabGenres->setCustomColors(
        sf::Color::Transparent,
        sf::Color(226, 232, 240, 60),
        sf::Color(203, 213, 225),
        sf::Color::Transparent
    );
    btnSubTabGenres->setTextColor(sf::Color(100, 116, 139));
    btnSubTabGenres->setOnClick([this]() {
        movieSubTab = 1;
    });

    // Quick search input
    movieSearchInput = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(leftColX + 115.f, 132.f),
        sf::Vector2f(320.f, 36.f),
        "Tìm theo Mã, Tên, Thể loại...",
        Theme::SizeBody
    ));
    movieSearchInput->setCornerRadius(6.f);
    movieSearchInput->setColors(
        sf::Color::White,
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    // Movie Form Inputs
    float inputX = rightColX + 140.f;
    float inputW = 320.f;
    float inputH = 36.f;

    inputMovieId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 175.f),
        sf::Vector2f(inputW, inputH),
        "Mã phim tự sinh...",
        Theme::SizeBody
    ));
    inputMovieId->setColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(226, 232, 240), sf::Color(71, 85, 105), sf::Color(148, 163, 184));
    inputMovieId->setCornerRadius(6.f);
    inputMovieId->setText("P0" + std::to_string(movieList.size() + 1));

    inputMovieTitle = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 224.f),
        sf::Vector2f(inputW, inputH),
        "Nhập tên phim...",
        Theme::SizeBody
    ));
    inputMovieTitle->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieTitle->setCornerRadius(6.f);

    inputMovieDuration = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 273.f),
        sf::Vector2f(inputW, inputH),
        "120",
        Theme::SizeBody
    ));
    inputMovieDuration->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieDuration->setCornerRadius(6.f);

    inputMovieReleaseDate = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 322.f),
        sf::Vector2f(inputW, inputH),
        "dd/MM/yyyy",
        Theme::SizeBody
    ));
    inputMovieReleaseDate->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieReleaseDate->setCornerRadius(6.f);
    inputMovieReleaseDate->setText("01/04/2026");

    inputMovieAgeRating = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 371.f),
        sf::Vector2f(inputW, inputH),
        "P, C13, C16, C18",
        Theme::SizeBody
    ));
    inputMovieAgeRating->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieAgeRating->setCornerRadius(6.f);
    inputMovieAgeRating->setText("C18");

    inputMovieGenre = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 420.f),
        sf::Vector2f(inputW - 110.f, inputH),
        "Hành Động, Tâm Lý",
        Theme::SizeBody
    ));
    inputMovieGenre->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieGenre->setCornerRadius(6.f);

    btnSelectGenre = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Chọn thể lo...",
        sf::Vector2f(inputX + inputW - 100.f, 420.f),
        sf::Vector2f(100.f, inputH),
        ButtonStyle::Ghost,
        6.f
    ));
    btnSelectGenre->setFontSize(13);
    btnSelectGenre->setCustomColors(sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240), sf::Color(248, 250, 252));
    btnSelectGenre->setOutline(1.f, sf::Color(203, 213, 225));
    btnSelectGenre->setTextColor(sf::Color(51, 65, 85));
    btnSelectGenre->setOnClick([this]() {
        if (!genreList.empty()) {
            selectedGenreChoiceIdx = (selectedGenreChoiceIdx + 1) % genreList.size();
            inputMovieGenre->setText(genreList[selectedGenreChoiceIdx].second);
        }
    });

    inputMovieDesc = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 472.f),
        sf::Vector2f(inputW, 76.f),
        "Nhập nội dung tóm tắt phim...",
        Theme::SizeBody
    ));
    inputMovieDesc->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputMovieDesc->setCornerRadius(6.f);

    // Form Action Buttons
    float btnW = (rightColW - 48.f - 14.f) / 2.0f; // ~215.f
    float btnRow1Y = 720.f;
    float btnRow2Y = 772.f;

    btnMovieCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thêm mới",
        sf::Vector2f(rightColX + 24.f, btnRow1Y),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Success,
        6.f
    ));
    btnMovieCreate->setFontSize(15);
    btnMovieCreate->setCustomColors(sf::Color(34, 197, 94), sf::Color(22, 163, 74), sf::Color(21, 128, 61), sf::Color(203, 213, 225));
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
        if (ageStr == "P" || ageStr == "p") age = 0;
        else {
            std::string numOnly = "";
            for (char ch : ageStr) {
                if (ch >= '0' && ch <= '9') numOnly += ch;
            }
            try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
        }

        std::string gName = inputMovieGenre->getText();
        std::string desc = inputMovieDesc->getText();

        int day = 1, month = 1, year = 2026;
        DateFormat::parseDate(normRel, day, month, year);
        month += 1;
        if (month > 12) { month = 1; year += 1; }
        std::ostringstream ssEnd;
        ssEnd << std::setfill('0') << std::setw(2) << day << "/"
              << std::setw(2) << month << "/" << std::setw(4) << year;
        std::string endDate = ssEnd.str();

        Movie m(id, title, dur, age, normRel, endDate, desc, "assets/posters/poster_default.jpg");
        movieList.push_back(m);
        db.ghiDanhSachPhim(movieList);
        if (!gName.empty()) {
            db.capNhatTheLoaiChoPhim(id, gName);
            db.docDanhSachTheLoai(genreList);
        }
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "THEM_PHIM", "Them phim moi: " + title + " (" + id + ")");
        confirmPopup->show("Thành công", "Đã thêm phim '" + title + "' vào cơ sở dữ liệu quan_ly_phim.csv!", ButtonStyle::Success, "Hoàn tất", "Đóng");
    });

    btnMovieUpdate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Cập nhật",
        sf::Vector2f(rightColX + 24.f + btnW + 14.f, btnRow1Y),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnMovieUpdate->setFontSize(15);
    btnMovieUpdate->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240));
    btnMovieUpdate->setOutline(1.f, sf::Color(203, 213, 225));
    btnMovieUpdate->setTextColor(sf::Color(15, 23, 42));
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
            if (!rel.empty()) {
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
                rel = normRel;
            }

            std::string ageStr = inputMovieAgeRating->getText();
            int age = 0;
            if (ageStr == "P" || ageStr == "p") age = 0;
            else {
                std::string numOnly = "";
                for (char ch : ageStr) {
                    if (ch >= '0' && ch <= '9') numOnly += ch;
                }
                try { age = numOnly.empty() ? 0 : std::stoi(numOnly); } catch (...) { age = 0; }
            }

            std::string gName = inputMovieGenre->getText();
            std::string mId = movieList[selectedMovieRowIdx].getMaPhim();

            movieList[selectedMovieRowIdx].setTenPhim(title);
            movieList[selectedMovieRowIdx].setThoiLuong(dur);
            movieList[selectedMovieRowIdx].setNhanDoTuoi(age);
            if (!rel.empty()) {
                movieList[selectedMovieRowIdx].setNgayKhoiChieu(rel);
            }
            movieList[selectedMovieRowIdx].setMoTa(inputMovieDesc->getText());

            db.ghiDanhSachPhim(movieList);
            if (!gName.empty()) {
                db.capNhatTheLoaiChoPhim(mId, gName);
                db.docDanhSachTheLoai(genreList);
            }

            ActivityLog::ghiNhan(currentUserId, currentUserRole, "SUA_PHIM", "Cap nhat thong tin phim: " + title);
            confirmPopup->show("Thành công", "Cập nhật thông tin phim thành công!", ButtonStyle::Success, "Đóng", "Quay lại");
        } else {
            confirmPopup->show("Thông báo", "Vui lòng chọn 1 phim từ danh sách để cập nhật!", ButtonStyle::Primary, "Đóng", "Hủy");
        }
    });

    btnMovieDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa",
        sf::Vector2f(rightColX + 24.f, btnRow2Y),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnMovieDelete->setFontSize(15);
    btnMovieDelete->setCustomColors(sf::Color::White, sf::Color(254, 242, 242), sf::Color(254, 226, 226), sf::Color(226, 232, 240));
    btnMovieDelete->setOutline(1.f, sf::Color(252, 165, 165));
    btnMovieDelete->setTextColor(sf::Color(220, 38, 38));
    btnMovieDelete->setOnClick([this]() {
        if (selectedMovieRowIdx >= 0 && selectedMovieRowIdx < static_cast<int>(movieList.size())) {
            std::string pName = movieList[selectedMovieRowIdx].getTenPhim();
            confirmPopup->show("XÁC NHẬN XÓA", "Bạn có chắc chắn muốn xóa phim '" + pName + "' khỏi hệ thống?", ButtonStyle::Danger, "Xác nhận xóa", "Hủy");
            confirmPopup->setOnConfirm([this, pName]() {
                movieList.erase(movieList.begin() + selectedMovieRowIdx);
                db.ghiDanhSachPhim(movieList);
                ActivityLog::ghiNhan(currentUserId, currentUserRole, "XOA_PHIM", "Xoa phim: " + pName);
                selectedMovieRowIdx = -1;
            });
        }
    });

    btnMovieClear = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Clear Form",
        sf::Vector2f(rightColX + 24.f + btnW + 14.f, btnRow2Y),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Ghost,
        6.f
    ));
    btnMovieClear->setFontSize(15);
    btnMovieClear->setCustomColors(sf::Color::White, sf::Color(248, 250, 252), sf::Color(241, 245, 249), sf::Color(226, 232, 240));
    btnMovieClear->setOutline(1.f, sf::Color(203, 213, 225));
    btnMovieClear->setTextColor(sf::Color(15, 23, 42));
    btnMovieClear->setOnClick([this]() {
        selectedMovieRowIdx = -1;
        inputMovieId->setText("P0" + std::to_string(movieList.size() + 1));
        inputMovieTitle->setText("");
        inputMovieDuration->setText("120");
        inputMovieReleaseDate->setText("01/04/2026");
        inputMovieAgeRating->setText("C18");
        inputMovieGenre->setText("");
        inputMovieDesc->setText("");
    });

    // Populate initial form with 1st movie
    if (!movieList.empty()) {
        selectedMovieRowIdx = 0;
        inputMovieId->setText(movieList[0].getMaPhim());
        inputMovieTitle->setText(movieList[0].getTenPhim());
        inputMovieDuration->setText(std::to_string(movieList[0].getThoiLuong()));
        inputMovieReleaseDate->setText(movieList[0].getNgayKhoiChieu());
        inputMovieAgeRating->setText("C" + std::to_string(movieList[0].getNhanDoTuoi()));
        inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[0].getMaPhim()));
        inputMovieDesc->setText(movieList[0].getMoTa());
    }

    // Genre Form Setup
    inputGenreId = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 175.f),
        sf::Vector2f(inputW, inputH),
        "TL01",
        Theme::SizeBody
    ));
    inputGenreId->setColors(sf::Color(248, 250, 252), sf::Color(226, 232, 240), sf::Color(226, 232, 240), sf::Color(71, 85, 105), sf::Color(148, 163, 184));
    inputGenreId->setCornerRadius(6.f);

    inputGenreName = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(inputX, 225.f),
        sf::Vector2f(inputW, inputH),
        "Tên thể loại mới...",
        Theme::SizeBody
    ));
    inputGenreName->setColors(sf::Color::White, sf::Color(203, 213, 225), sf::Color(2, 132, 199), sf::Color(15, 23, 42), sf::Color(148, 163, 184));
    inputGenreName->setCornerRadius(6.f);

    btnGenreCreate = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Thêm thể loại",
        sf::Vector2f(rightColX + 24.f, 290.f),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Success,
        6.f
    ));
    btnGenreCreate->setFontSize(15);
    btnGenreCreate->setCustomColors(sf::Color(34, 197, 94), sf::Color(22, 163, 74), sf::Color(21, 128, 61), sf::Color(203, 213, 225));
    btnGenreCreate->setOnClick([this]() {
        std::string gName = inputGenreName->getText();
        if (!gName.empty()) {
            db.themLoaiTheLoai(gName);
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
            confirmPopup->show("Thành công", "Đã thêm thể loại: " + gName, ButtonStyle::Success, "Đóng", "Hủy");
        }
    });

    btnGenreDelete = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "Xóa thể loại",
        sf::Vector2f(rightColX + 24.f + btnW + 14.f, 290.f),
        sf::Vector2f(btnW, 42.f),
        ButtonStyle::Danger,
        6.f
    ));
    btnGenreDelete->setFontSize(15);
    btnGenreDelete->setOnClick([this]() {
        if (selectedGenreRowIdx >= 0 && selectedGenreRowIdx < static_cast<int>(genreList.size())) {
            std::string gId = genreList[selectedGenreRowIdx].first;
            db.xoaLoaiTheLoai(gId);
            genreList.clear();
            db.docDanhSachTheLoai(genreList);
            selectedGenreRowIdx = -1;
        }
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
        {"Tài Khoản", 180.f, false},
        {"Hành Động", 280.f, false},
        {"Mã Đối Tượng", 180.f, false},
        {"Chi Tiết Nhật Ký (nhat_ky_hoat_dong.csv)", 510.f, false}
    };
    logsTable->setColumns(cols);

    // Read activity logs directly from DATA/nhat_ky_hoat_dong.csv
    std::vector<std::vector<std::string>> tRows;
    std::ifstream file("DATA/nhat_ky_hoat_dong.csv");
    if (file.is_open()) {
        std::string line;
        bool isHeader = true;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (isHeader) { isHeader = false; continue; }
            std::stringstream ss(line);
            std::string item;
            std::vector<std::string> row;
            while (std::getline(ss, item, ',')) {
                row.push_back(item);
            }
            if (!row.empty()) tRows.push_back(row);
        }
    }

    if (tRows.empty()) {
        tRows.push_back({"2026-10-31 09:00:00", "U001", "Admin", "DANG_NHAP", "Dang nhap he thong thanh cong"});
        tRows.push_back({"2026-10-31 09:15:30", "U002", "Staff", "BAN_VE", "Xuat ve ghe A1, A2 suat SC01"});
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
            if (btnQuickAdmin) {
                if (event.type == sf::Event::MouseMoved) btnQuickAdmin->handleMouseMove(mousePos);
                else if (event.type == sf::Event::MouseButtonPressed) btnQuickAdmin->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                else if (event.type == sf::Event::MouseButtonReleased) btnQuickAdmin->handleMouseButtonReleased(event.mouseButton.button, mousePos);
            }
            if (btnQuickStaff) {
                if (event.type == sf::Event::MouseMoved) btnQuickStaff->handleMouseMove(mousePos);
                else if (event.type == sf::Event::MouseButtonPressed) btnQuickStaff->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                else if (event.type == sf::Event::MouseButtonReleased) btnQuickStaff->handleMouseButtonReleased(event.mouseButton.button, mousePos);
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
                    if (inputMovieDesc) {
                        if (event.type == sf::Event::MouseButtonPressed) inputMovieDesc->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputMovieDesc->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnSelectGenre);
                    handleBtn(btnMovieCreate);
                    handleBtn(btnMovieUpdate);
                    handleBtn(btnMovieDelete);
                    handleBtn(btnMovieClear);

                    // Row selection click in movie table
                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 20.f;
                        float leftColW = 920.f;
                        float rowStartY = 120.f + 60.f + 44.f;
                        float rowH = 50.f;
                        if (mousePos.x >= leftColX + 20.f && mousePos.x <= leftColX + leftColW - 20.f &&
                            mousePos.y >= rowStartY && mousePos.y <= rowStartY + 14.f * rowH) {
                            int clickedVisualRow = static_cast<int>((mousePos.y - rowStartY) / rowH);

                            std::string query = movieSearchInput ? movieSearchInput->getText() : "";
                            std::transform(query.begin(), query.end(), query.begin(), ::tolower);

                            int matchCount = 0;
                            int targetIdx = -1;
                            for (size_t i = 0; i < movieList.size(); ++i) {
                                std::string pName = movieList[i].getTenPhim();
                                std::string pCode = movieList[i].getMaPhim();
                                if (!query.empty()) {
                                    std::string pNameLower = pName;
                                    std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                                    if (pNameLower.find(query) == std::string::npos && pCode.find(query) == std::string::npos) {
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
                                selectedMovieRowIdx = targetIdx;
                                inputMovieId->setText(movieList[targetIdx].getMaPhim());
                                inputMovieTitle->setText(movieList[targetIdx].getTenPhim());
                                inputMovieDuration->setText(std::to_string(movieList[targetIdx].getThoiLuong()));
                                inputMovieReleaseDate->setText(movieList[targetIdx].getNgayKhoiChieu());
                                inputMovieAgeRating->setText("C" + std::to_string(movieList[targetIdx].getNhanDoTuoi()));
                                inputMovieGenre->setText(db.layTheLoaiCuaPhim(movieList[targetIdx].getMaPhim()));
                                inputMovieDesc->setText(movieList[targetIdx].getMoTa());
                            }
                        }
                    }
                } else {
                    if (inputGenreId) {
                        if (event.type == sf::Event::MouseButtonPressed) inputGenreId->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputGenreId->handleTextEntered(event.text.unicode);
                    }
                    if (inputGenreName) {
                        if (event.type == sf::Event::MouseButtonPressed) inputGenreName->handleMouseButtonPressed(event.mouseButton.button, mousePos);
                        else if (event.type == sf::Event::TextEntered) inputGenreName->handleTextEntered(event.text.unicode);
                    }
                    handleBtn(btnGenreCreate);
                    handleBtn(btnGenreDelete);

                    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                        float leftColX = sidebarWidth + 20.f;
                        float leftColW = 920.f;
                        float rowStartY = 120.f + 20.f + 38.f;
                        float rowH = 46.f;
                        if (mousePos.x >= leftColX + 20.f && mousePos.x <= leftColX + leftColW - 20.f &&
                            mousePos.y >= rowStartY && mousePos.y <= rowStartY + genreList.size() * rowH) {
                            int clickedIdx = static_cast<int>((mousePos.y - rowStartY) / rowH);
                            if (clickedIdx >= 0 && clickedIdx < static_cast<int>(genreList.size())) {
                                selectedGenreRowIdx = clickedIdx;
                                inputGenreId->setText(genreList[clickedIdx].first);
                                inputGenreName->setText(genreList[clickedIdx].second);
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
    // 1. Draw warm cinematic golden bokeh background (Full Screen 1728x972)
    if (loginBackgroundTexture.getSize().x > 0) {
        window.draw(loginBackgroundSprite);
    } else {
        sf::RectangleShape bg(sf::Vector2f(1728.f, 972.f));
        bg.setFillColor(sf::Color(30, 20, 15));
        window.draw(bg);
    }

    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f (Centered)
    float cardY = (972.f - cardH) / 2.0f;  // 206.f (Centered)
    float leftW = 420.f;
    float rightX = cardX + leftW + 45.f;   // 819.f
    float rightW = cardW - leftW - 90.f;   // 510.f

    // 2. Drop shadow behind the card (smooth, soft black)
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX - 4.f, cardY + 6.f),
        sf::Vector2f(cardW + 8.f, cardH + 8.f),
        22.f,
        sf::Color(0, 0, 0, 95)
    );

    // 3. Main Crisp White Card Background
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, cardH),
        16.f,
        sf::Color(255, 255, 255)
    );

    // 4. Left Panel Background: Warm Ivory / Cream
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(leftW, cardH),
        16.f,
        sf::Color(255, 253, 244)
    );
    // Square off the right side of the left panel
    sf::RectangleShape leftFiller(sf::Vector2f(16.f, cardH));
    leftFiller.setPosition(cardX + leftW - 16.f, cardY);
    leftFiller.setFillColor(sf::Color(255, 253, 244));
    window.draw(leftFiller);

    // 5. Top Orange Accent Strip
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, 4.f),
        2.f,
        sf::Color(234, 88, 12)
    );

    // ==========================================
    // LEFT PANEL - BRANDING & SECURITY (CRISP)
    // ==========================================
    float leftCenterX = cardX + leftW / 2.0f;

    // 1. Logo Badge (Rounded Square with User's Photo & Video Film Reel Logo)
    float badgeSize = 98.f;
    float badgeX = leftCenterX - badgeSize / 2.0f;
    float badgeY = cardY + 88.f;

    // Drop shadow behind logo badge
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX - 1.f, badgeY + 2.f),
        sf::Vector2f(badgeSize + 2.f, badgeSize + 2.f),
        22.f,
        sf::Color(0, 0, 0, 18)
    );

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX, badgeY),
        sf::Vector2f(badgeSize, badgeSize),
        20.f,
        sf::Color(255, 255, 255),
        1.5f,
        sf::Color(253, 230, 138)
    );

    // Draw uploaded cinema logo image
    if (cinemaLogoTexture.getSize().x > 0) {
        float logoTargetSize = 80.f;
        float lScale = logoTargetSize / static_cast<float>(cinemaLogoTexture.getSize().x);
        cinemaLogoSprite.setScale(lScale, lScale);
        cinemaLogoSprite.setPosition(
            badgeX + (badgeSize - static_cast<float>(cinemaLogoTexture.getSize().x) * lScale) / 2.0f,
            badgeY + (badgeSize - static_cast<float>(cinemaLogoTexture.getSize().y) * lScale) / 2.0f
        );
        window.draw(cinemaLogoSprite);
    } else {
        // Fallback lens illustration
        sf::CircleShape lensCircle(18.f);
        lensCircle.setOrigin(18.f, 18.f);
        lensCircle.setPosition(leftCenterX, badgeY + badgeSize / 2.0f);
        lensCircle.setFillColor(sf::Color(255, 255, 255));
        lensCircle.setOutlineThickness(2.f);
        lensCircle.setOutlineColor(sf::Color(234, 88, 12));
        window.draw(lensCircle);
    }

    // 2. Pill Badge: 🔑 CỔNG ĐIỀU HÀNH CỤM RẠP
    float pillW = 224.f;
    float pillH = 30.f;
    float pillX = leftCenterX - pillW / 2.0f;
    float pillY = cardY + 204.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(pillX, pillY),
        sf::Vector2f(pillW, pillH),
        15.f,
        sf::Color(254, 243, 199)
    );

    // Key icon inside pill
    sf::CircleShape keyPill(4.f);
    keyPill.setPosition(pillX + 16.f, pillY + 9.5f);
    keyPill.setFillColor(sf::Color(217, 119, 6));
    window.draw(keyPill);

    sf::RectangleShape keyPillStem(sf::Vector2f(8.f, 2.5f));
    keyPillStem.setPosition(pillX + 22.f, pillY + 12.5f);
    keyPillStem.setFillColor(sf::Color(217, 119, 6));
    window.draw(keyPillStem);

    sf::Text pillText;
    pillText.setFont(Theme::FontSecondary);
    pillText.setCharacterSize(12);
    pillText.setStyle(sf::Text::Bold);
    pillText.setFillColor(sf::Color(180, 83, 9));
    pillText.setString(toSfString("CỔNG ĐIỀU HÀNH CỤM RẠP"));
    pillText.setPosition(pillX + 38.f, pillY + 6.f);
    window.draw(pillText);

    // 3. Brand Text: CINESTAR CINEMA (Bebas Neue display title)
    sf::Text brandText1;
    brandText1.setFont(Theme::FontCinema);
    brandText1.setCharacterSize(46);
    brandText1.setStyle(sf::Text::Bold);
    brandText1.setFillColor(sf::Color(15, 23, 42));
    brandText1.setString("CINESTAR");
    sf::FloatRect b1 = brandText1.getLocalBounds();
    brandText1.setPosition(leftCenterX - b1.width / 2.0f - b1.left, cardY + 250.f);
    window.draw(brandText1);

    sf::Text brandText2;
    brandText2.setFont(Theme::FontCinema);
    brandText2.setCharacterSize(46);
    brandText2.setStyle(sf::Text::Bold);
    brandText2.setFillColor(sf::Color(15, 23, 42));
    brandText2.setString("CINEMA");
    sf::FloatRect b2 = brandText2.getLocalBounds();
    brandText2.setPosition(leftCenterX - b2.width / 2.0f - b2.left, cardY + 296.f);
    window.draw(brandText2);

    sf::Text brandSub;
    brandSub.setFont(Theme::FontSecondary);
    brandSub.setCharacterSize(13);
    brandSub.setStyle(sf::Text::Bold);
    brandSub.setFillColor(sf::Color(71, 85, 105));
    brandSub.setString(toSfString("HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM"));
    sf::FloatRect bs = brandSub.getLocalBounds();
    brandSub.setPosition(leftCenterX - bs.width / 2.0f - bs.left, cardY + 352.f);
    window.draw(brandSub);

    // 4. Bottom Security Shield Badge Card
    float secW = 340.f;
    float secH = 50.f;
    float secX = leftCenterX - secW / 2.0f;
    float secY = cardY + 472.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(secX, secY),
        sf::Vector2f(secW, secH),
        10.f,
        sf::Color(255, 255, 255),
        1.f,
        sf::Color(254, 236, 185)
    );

    // Shield Icon
    sf::ConvexShape shield(5);
    shield.setPoint(0, sf::Vector2f(secX + 22.f, secY + 14.f));
    shield.setPoint(1, sf::Vector2f(secX + 32.f, secY + 14.f));
    shield.setPoint(2, sf::Vector2f(secX + 32.f, secY + 26.f));
    shield.setPoint(3, sf::Vector2f(secX + 27.f, secY + 34.f));
    shield.setPoint(4, sf::Vector2f(secX + 22.f, secY + 26.f));
    shield.setFillColor(sf::Color(234, 88, 12));
    window.draw(shield);

    // Inner checkmark for shield
    sf::VertexArray sTick(sf::LinesStrip, 3);
    sTick[0].position = sf::Vector2f(secX + 24.5f, secY + 22.f);
    sTick[1].position = sf::Vector2f(secX + 26.5f, secY + 25.f);
    sTick[2].position = sf::Vector2f(secX + 30.f, secY + 19.f);
    sTick[0].color = sf::Color(255, 255, 255);
    sTick[1].color = sf::Color(255, 255, 255);
    sTick[2].color = sf::Color(255, 255, 255);
    window.draw(sTick);

    sf::Text secText1;
    secText1.setFont(Theme::FontSecondary);
    secText1.setCharacterSize(11);
    secText1.setFillColor(sf::Color(100, 116, 139));
    secText1.setString(toSfString("Hệ thống bảo mật nội bộ dành cho cán"));
    secText1.setPosition(secX + 42.f, secY + 9.f);
    window.draw(secText1);

    sf::Text secText2;
    secText2.setFont(Theme::FontSecondary);
    secText2.setCharacterSize(11);
    secText2.setFillColor(sf::Color(100, 116, 139));
    secText2.setString(toSfString("bộ & nhân viên CineStar"));
    secText2.setPosition(secX + 42.f, secY + 25.f);
    window.draw(secText2);

    // ==========================================
    // RIGHT PANEL - FORM & INPUTS (CRISP WHITE)
    // ==========================================

    // 1. Heading
    sf::Text rTitle;
    rTitle.setFont(Theme::FontPrimary);
    rTitle.setCharacterSize(26);
    rTitle.setStyle(sf::Text::Bold);
    rTitle.setFillColor(sf::Color(15, 23, 42));
    rTitle.setString(toSfString("Đăng nhập hệ thống"));
    rTitle.setPosition(rightX, cardY + 52.f);
    window.draw(rTitle);

    sf::Text rSub;
    rSub.setFont(Theme::FontSecondary);
    rSub.setCharacterSize(13);
    rSub.setFillColor(sf::Color(100, 116, 139));
    rSub.setString(toSfString("Vui lòng xác thực tài khoản được cấp quyền quản trị cụm rạp"));
    rSub.setPosition(rightX, cardY + 94.f);
    window.draw(rSub);

    // 2. Field 1: TÀI KHOẢN NHÂN SỰ
    sf::RectangleShape idIconBg(sf::Vector2f(14.f, 12.f));
    idIconBg.setPosition(rightX, cardY + 148.f);
    idIconBg.setFillColor(sf::Color(234, 88, 12));
    window.draw(idIconBg);

    sf::Text idLabel;
    idLabel.setFont(Theme::FontPrimary);
    idLabel.setCharacterSize(12);
    idLabel.setStyle(sf::Text::Bold);
    idLabel.setFillColor(sf::Color(71, 85, 105));
    idLabel.setString(toSfString("TÀI KHOẢN NHÂN SỰ"));
    idLabel.setPosition(rightX + 20.f, cardY + 146.f);
    window.draw(idLabel);

    if (loginInputUser) {
        loginInputUser->draw(window);
        // Sharp inside user avatar icon 👤
        float uIconX = rightX + 16.f;
        float uIconY = cardY + 170.f + 15.f;

        sf::CircleShape uRing(8.f);
        uRing.setOrigin(8.f, 8.f);
        uRing.setPosition(uIconX + 8.f, uIconY + 8.f);
        uRing.setFillColor(sf::Color::Transparent);
        uRing.setOutlineThickness(1.5f);
        uRing.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(uRing);

        sf::CircleShape uHead(2.5f);
        uHead.setOrigin(2.5f, 2.5f);
        uHead.setPosition(uIconX + 8.f, uIconY + 5.f);
        uHead.setFillColor(sf::Color(148, 163, 184));
        window.draw(uHead);

        sf::CircleShape uBody(4.f);
        uBody.setOrigin(4.f, 4.f);
        uBody.setScale(1.f, 0.5f);
        uBody.setPosition(uIconX + 8.f, uIconY + 11.5f);
        uBody.setFillColor(sf::Color(148, 163, 184));
        window.draw(uBody);
    }

    // 3. Field 2: MẬT KHẨU TRUY CẬP
    sf::RectangleShape lockBody(sf::Vector2f(12.f, 9.f));
    lockBody.setPosition(rightX, cardY + 240.f);
    lockBody.setFillColor(sf::Color(234, 88, 12));
    window.draw(lockBody);

    sf::CircleShape lockShackle(3.5f);
    lockShackle.setPosition(rightX + 2.5f, cardY + 236.f);
    lockShackle.setFillColor(sf::Color::Transparent);
    lockShackle.setOutlineThickness(1.5f);
    lockShackle.setOutlineColor(sf::Color(234, 88, 12));
    window.draw(lockShackle);

    sf::Text passLabel;
    passLabel.setFont(Theme::FontPrimary);
    passLabel.setCharacterSize(12);
    passLabel.setStyle(sf::Text::Bold);
    passLabel.setFillColor(sf::Color(71, 85, 105));
    passLabel.setString(toSfString("MẬT KHẨU TRUY CẬP"));
    passLabel.setPosition(rightX + 20.f, cardY + 238.f);
    window.draw(passLabel);

    if (loginInputPass) {
        loginInputPass->draw(window);
        // Sharp inside key icon 🔑
        float kIconX = rightX + 16.f;
        float kIconY = cardY + 262.f + 15.f;

        sf::CircleShape kHead(4.f);
        kHead.setOrigin(4.f, 4.f);
        kHead.setPosition(kIconX + 6.f, kIconY + 6.f);
        kHead.setFillColor(sf::Color::Transparent);
        kHead.setOutlineThickness(1.5f);
        kHead.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(kHead);

        sf::RectangleShape kStem(sf::Vector2f(8.f, 2.f));
        kStem.setPosition(kIconX + 10.f, kIconY + 5.f);
        kStem.setFillColor(sf::Color(148, 163, 184));
        window.draw(kStem);

        sf::RectangleShape kTooth(sf::Vector2f(2.f, 3.5f));
        kTooth.setPosition(kIconX + 16.f, kIconY + 6.f);
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
        errText.setCharacterSize(13);
        errText.setStyle(sf::Text::Bold);
        errText.setFillColor(sf::Color(220, 38, 38));
        errText.setString(toSfString(loginErrorMsg));
        errText.setPosition(rightX, cardY + 430.f);
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
    float contentAreaH = 972.f - topbarHeight;

    // 1. Soft grey canvas background matching reference
    sf::RectangleShape mBg(sf::Vector2f(contentAreaW, contentAreaH));
    mBg.setPosition(sidebarWidth, topbarHeight);
    mBg.setFillColor(sf::Color(241, 245, 249)); // #f1f5f9
    window.draw(mBg);

    // 2. Top Header Bar (Dark Navy matching target image)
    sf::RectangleShape topBar(sf::Vector2f(contentAreaW, topbarHeight));
    topBar.setPosition(sidebarWidth, 0.f);
    topBar.setFillColor(sf::Color(15, 23, 42)); // Dark Navy #0f172a
    window.draw(topBar);

    sf::Text hTitle;
    hTitle.setFont(Theme::FontPrimary);
    hTitle.setCharacterSize(20);
    hTitle.setStyle(sf::Text::Bold);
    hTitle.setFillColor(sf::Color::White);
    hTitle.setString(toSfString("QUẢN LÝ PHIM & THỂ LOẠI"));
    hTitle.setPosition(sidebarWidth + 24.f, 13.f);
    window.draw(hTitle);

    sf::Text hSub;
    hSub.setFont(Theme::FontPrimary);
    hSub.setCharacterSize(13);
    hSub.setFillColor(sf::Color(148, 163, 184)); // #94a3b8
    hSub.setString(toSfString("Quản lý danh sách phim, thông tin chi tiết và danh mục thể loại"));
    hSub.setPosition(sidebarWidth + 24.f, 38.f);
    window.draw(hSub);

    // 3. Sub-tabs Bar
    float leftColX = sidebarWidth + 20.f;
    float leftColW = 920.f;
    float rightColX = sidebarWidth + 960.f;
    float rightColW = 494.f;

    if (btnSubTabMovies) btnSubTabMovies->draw(window);
    if (btnSubTabGenres) btnSubTabGenres->draw(window);

    // Active Tab Underline Indicator
    sf::RectangleShape activeBar;
    if (movieSubTab == 0) {
        activeBar.setPosition(leftColX + 4.f, 108.f);
        activeBar.setSize(sf::Vector2f(115.f, 3.f));
    } else {
        activeBar.setPosition(leftColX + 145.f, 108.f);
        activeBar.setSize(sf::Vector2f(135.f, 3.f));
    }
    activeBar.setFillColor(sf::Color(2, 132, 199)); // Cyan/Blue #0284c7
    window.draw(activeBar);

    // 4. LEFT CARD: White Table Card
    float cardY = 120.f;
    float cardH = 824.f;
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(leftColX, cardY),
        sf::Vector2f(leftColW, cardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Search Row
        sf::Text lblSearch;
        lblSearch.setFont(Theme::FontPrimary);
        lblSearch.setCharacterSize(15);
        lblSearch.setStyle(sf::Text::Bold);
        lblSearch.setFillColor(sf::Color(15, 23, 42));
        lblSearch.setString(toSfString("Tìm nhanh:"));
        lblSearch.setPosition(leftColX + 22.f, cardY + 22.f);
        window.draw(lblSearch);

        if (movieSearchInput) movieSearchInput->draw(window);

        // Table Header
        float tblHeaderY = cardY + 60.f;
        sf::RectangleShape thBg(sf::Vector2f(leftColW - 40.f, 42.f));
        thBg.setPosition(leftColX + 20.f, tblHeaderY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        sf::RectangleShape thDiv(sf::Vector2f(leftColW - 40.f, 1.f));
        thDiv.setPosition(leftColX + 20.f, tblHeaderY + 42.f);
        thDiv.setFillColor(sf::Color(226, 232, 240));
        window.draw(thDiv);

        std::vector<std::string> cols = {"Mã phim", "Tên phim", "Thể loại", "Thời lượng", "Ngày chiếu", "Nhãn tuổi"};
        std::vector<float> colX = {leftColX + 32.f, leftColX + 130.f, leftColX + 355.f, leftColX + 565.f, leftColX + 685.f, leftColX + 805.f};

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(15);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(15, 23, 42));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 11.f);
            window.draw(ch);
        }

        // Table Rows
        float rowStartY = tblHeaderY + 44.f;
        float rowH = 50.f;
        std::string query = movieSearchInput ? movieSearchInput->getText() : "";
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        size_t displayCount = 0;
        for (size_t i = 0; i < movieList.size() && displayCount < 14; ++i) {
            std::string pName = movieList[i].getTenPhim();
            std::string pCode = movieList[i].getMaPhim();
            std::string pGenre = db.layTheLoaiCuaPhim(pCode);
            if (pGenre.empty()) pGenre = "Hành Động, Khoa...";

            // Filter check
            if (!query.empty()) {
                std::string pNameLower = pName;
                std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                if (pNameLower.find(query) == std::string::npos && pCode.find(query) == std::string::npos) {
                    continue;
                }
            }

            float ry = rowStartY + static_cast<float>(displayCount) * rowH;
            bool isSelected = (selectedMovieRowIdx == static_cast<int>(i));

            sf::RectangleShape rBg(sf::Vector2f(leftColW - 40.f, rowH - 2.f));
            rBg.setPosition(leftColX + 20.f, ry);
            if (isSelected) {
                rBg.setFillColor(sf::Color(224, 242, 254)); // Light Cyan #e0f2fe
            } else if (displayCount % 2 == 1) {
                rBg.setFillColor(sf::Color(248, 250, 252));
            } else {
                rBg.setFillColor(sf::Color::White);
            }
            window.draw(rBg);

            // Row text - Crisp, large 14px Segoe UI
            sf::Color rowTxtColor = isSelected ? sf::Color(3, 105, 161) : sf::Color(15, 23, 42);

            auto drawCell = [&](const std::string& val, float x) {
                sf::Text cText;
                cText.setFont(Theme::FontPrimary);
                cText.setCharacterSize(14);
                cText.setFillColor(rowTxtColor);
                std::string disp = val;
                if (disp.length() > 30) disp = disp.substr(0, 27) + "...";
                cText.setString(toSfString(disp));
                cText.setPosition(x, ry + 15.f);
                window.draw(cText);
            };

            drawCell(pCode, colX[0]);
            drawCell(pName, colX[1]);
            drawCell(pGenre, colX[2]);
            drawCell(std::to_string(movieList[i].getThoiLuong()) + " phút", colX[3]);
            drawCell(movieList[i].getNgayKhoiChieu(), colX[4]);
            drawCell("T" + std::to_string(movieList[i].getNhanDoTuoi()), colX[5]);

            // Bottom border
            sf::RectangleShape rBorder(sf::Vector2f(leftColW - 40.f, 1.f));
            rBorder.setPosition(leftColX + 20.f, ry + rowH - 2.f);
            rBorder.setFillColor(sf::Color(241, 245, 249));
            window.draw(rBorder);

            displayCount++;
        }
    } else {
        // Genre Table
        float tblHeaderY = cardY + 20.f;
        sf::RectangleShape thBg(sf::Vector2f(leftColW - 40.f, 42.f));
        thBg.setPosition(leftColX + 20.f, tblHeaderY);
        thBg.setFillColor(sf::Color(248, 250, 252));
        window.draw(thBg);

        std::vector<std::string> cols = {"Mã thể loại", "Tên thể loại", "Số lượng phim sử dụng"};
        std::vector<float> colX = {leftColX + 40.f, leftColX + 240.f, leftColX + 560.f};

        for (size_t c = 0; c < cols.size(); ++c) {
            sf::Text ch;
            ch.setFont(Theme::FontPrimary);
            ch.setCharacterSize(15);
            ch.setStyle(sf::Text::Bold);
            ch.setFillColor(sf::Color(15, 23, 42));
            ch.setString(toSfString(cols[c]));
            ch.setPosition(colX[c], tblHeaderY + 11.f);
            window.draw(ch);
        }

        float rowStartY = tblHeaderY + 44.f;
        float rowH = 50.f;
        for (size_t i = 0; i < genreList.size() && i < 15; ++i) {
            float ry = rowStartY + static_cast<float>(i) * rowH;
            bool isSelected = (selectedGenreRowIdx == static_cast<int>(i));

            sf::RectangleShape rBg(sf::Vector2f(leftColW - 40.f, rowH - 2.f));
            rBg.setPosition(leftColX + 20.f, ry);
            rBg.setFillColor(isSelected ? sf::Color(224, 242, 254) : (i % 2 == 1 ? sf::Color(248, 250, 252) : sf::Color::White));
            window.draw(rBg);

            sf::Color rowTxtColor = isSelected ? sf::Color(3, 105, 161) : sf::Color(15, 23, 42);

            sf::Text tId;
            tId.setFont(Theme::FontPrimary);
            tId.setCharacterSize(14);
            tId.setFillColor(rowTxtColor);
            tId.setString(genreList[i].first);
            tId.setPosition(colX[0], ry + 15.f);
            window.draw(tId);

            sf::Text tName;
            tName.setFont(Theme::FontPrimary);
            tName.setCharacterSize(14);
            tName.setFillColor(rowTxtColor);
            tName.setString(toSfString(genreList[i].second));
            tName.setPosition(colX[1], ry + 15.f);
            window.draw(tName);

            sf::Text tCnt;
            tCnt.setFont(Theme::FontPrimary);
            tCnt.setCharacterSize(14);
            tCnt.setFillColor(rowTxtColor);
            tCnt.setString(toSfString(std::to_string(i * 2 + 1) + " phim"));
            tCnt.setPosition(colX[2], ry + 15.f);
            window.draw(tCnt);
        }
    }

    // 5. RIGHT CARD: Form Card (White)
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(rightColX, cardY),
        sf::Vector2f(rightColW, cardH),
        8.f,
        sf::Color::White,
        1.f,
        sf::Color(226, 232, 240)
    );

    if (movieSubTab == 0) {
        // Title
        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(17);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString("Thông tin Phim"));
        formTitle.setPosition(rightColX + 20.f, cardY + 18.f);
        window.draw(formTitle);

        sf::RectangleShape fDiv(sf::Vector2f(rightColW - 40.f, 1.f));
        fDiv.setPosition(rightColX + 20.f, cardY + 46.f);
        fDiv.setFillColor(sf::Color(226, 232, 240));
        window.draw(fDiv);

        // Labels (Bold 14px Segoe UI, aligned with inputs)
        std::vector<std::pair<std::string, float>> formLabels = {
            {"Mã Phim:", 183.f},
            {"Tên Phim:", 232.f},
            {"Thời lượng:", 281.f},
            {"Ngày chiếu:", 330.f},
            {"Nhãn tuổi (Độ):", 379.f},
            {"Thể loại:", 428.f},
            {"Mô tả:", 480.f}
        };

        for (const auto& fl : formLabels) {
            sf::Text lText;
            lText.setFont(Theme::FontPrimary);
            lText.setCharacterSize(14);
            lText.setStyle(sf::Text::Bold);
            lText.setFillColor(sf::Color(51, 65, 85));
            lText.setString(toSfString(fl.first));
            lText.setPosition(rightColX + 20.f, fl.second);
            window.draw(lText);
        }

        // Inputs
        if (inputMovieId) inputMovieId->draw(window);
        if (inputMovieTitle) inputMovieTitle->draw(window);
        if (inputMovieDuration) inputMovieDuration->draw(window);
        if (inputMovieReleaseDate) inputMovieReleaseDate->draw(window);
        if (inputMovieAgeRating) inputMovieAgeRating->draw(window);
        if (inputMovieGenre) inputMovieGenre->draw(window);
        if (btnSelectGenre) btnSelectGenre->draw(window);
        if (inputMovieDesc) inputMovieDesc->draw(window);

        // Buttons
        if (btnMovieCreate) btnMovieCreate->draw(window);
        if (btnMovieUpdate) btnMovieUpdate->draw(window);
        if (btnMovieDelete) btnMovieDelete->draw(window);
        if (btnMovieClear) btnMovieClear->draw(window);
    } else {
        // Genre Form
        sf::Text formTitle;
        formTitle.setFont(Theme::FontPrimary);
        formTitle.setCharacterSize(17);
        formTitle.setStyle(sf::Text::Bold);
        formTitle.setFillColor(sf::Color(15, 23, 42));
        formTitle.setString(toSfString("Thông tin Thể loại"));
        formTitle.setPosition(rightColX + 20.f, cardY + 18.f);
        window.draw(formTitle);

        sf::Text l1;
        l1.setFont(Theme::FontPrimary);
        l1.setCharacterSize(14);
        l1.setStyle(sf::Text::Bold);
        l1.setFillColor(sf::Color(51, 65, 85));
        l1.setString(toSfString("Mã thể loại:"));
        l1.setPosition(rightColX + 20.f, 183.f);
        window.draw(l1);

        sf::Text l2;
        l2.setFont(Theme::FontPrimary);
        l2.setCharacterSize(14);
        l2.setStyle(sf::Text::Bold);
        l2.setFillColor(sf::Color(51, 65, 85));
        l2.setString(toSfString("Tên thể loại:"));
        l2.setPosition(rightColX + 20.f, 233.f);
        window.draw(l2);

        if (inputGenreId) inputGenreId->draw(window);
        if (inputGenreName) inputGenreName->draw(window);
        if (btnGenreCreate) btnGenreCreate->draw(window);
        if (btnGenreDelete) btnGenreDelete->draw(window);
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
    title.setString(toSfString("QUẦY BẮP NƯỚC (giao_dich_fnb.csv)"));
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
    title.setString(toSfString("NHẬT KÝ HOẠT ĐỘNG VÀ VẬN HÀNH HỆ THỐNG (nhat_ky_hoat_dong.csv)"));
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
    title.setString(toSfString("QUẢN LÝ KHÁCH HÀNG & HỘI VIÊN (CRM)"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddCustomer) btnAddCustomer->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Chương trình hội viên C-Friend, tích lũy điểm thưởng và dữ liệu khách hàng thân thiết"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    // 3 Tier Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> tiers = {
        {"C-FRIEND TIÊU CHUẨN", "1,240 Hội Viên", "Tích lũy 5% điểm chi tiêu - Tặng bắp sinh nhật", sf::Color(6, 182, 212)},
        {"C-VIP THÂN THIẾT", "480 Hội Viên", "Tích lũy 10% điểm - Giảm 10% F&B - 02 Vé mời 2D", sf::Color(245, 158, 11)},
        {"C-DIAMOND THƯỢNG KHÁCH", "125 Hội Viên", "Tích lũy 15% điểm - Vé mời Premiere - Phòng VIP", sf::Color(168, 85, 247)}
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
    tblTitle.setString(toSfString("DANH SÁCH GIAO DỊCH KHÁCH HÀNG GẦN ĐÂY"));
    tblTitle.setPosition(contentX + 24.f, tblY + 18.f);
    window.draw(tblTitle);

    // Table Header
    sf::RectangleShape thBg(sf::Vector2f(tblW - 48.f, 36.f));
    thBg.setPosition(contentX + 24.f, tblY + 54.f);
    thBg.setFillColor(sf::Color(15, 23, 42));
    window.draw(thBg);

    std::vector<std::string> cols = {"SỐ ĐIỆN THOẠI", "TÊN KHÁCH HÀNG", "HẠNG THẺ", "ĐIỂM TÍCH LŨY", "TỔNG CHI TIÊU", "TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 220.f, contentX + 480.f, contentX + 720.f, contentX + 960.f, contentX + 1180.f};

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

    // Sample data rows
    struct CrmRow {
        std::string phone, name, tier, pts, spend, status;
    };
    std::vector<CrmRow> rows = {
        {"0905123456", "Khách Hàng Thành Viên #01", "C-Diamond", "1,850 Pts", "8.450.000 đ", "Đang Hoạt Động"},
        {"0912345678", "Khách Hàng Thành Viên #02", "C-VIP", "920 Pts", "4.200.000 đ", "Đang Hoạt Động"},
        {"0987654321", "Khách Hàng Thành Viên #03", "C-Friend", "340 Pts", "1.650.000 đ", "Đang Hoạt Động"},
        {"0933221100", "Khách Hàng Thành Viên #04", "C-VIP", "780 Pts", "3.900.000 đ", "Đang Hoạt Động"},
        {"0977889900", "Khách Hàng Thành Viên #05", "C-Friend", "150 Pts", "850.000 đ", "Đang Hoạt Động"}
    };

    for (size_t r = 0; r < rows.size(); ++r) {
        float ry = tblY + 102.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        std::vector<std::string> vals = {rows[r].phone, rows[r].name, rows[r].tier, rows[r].pts, rows[r].spend, rows[r].status};
        for (size_t c = 0; c < vals.size(); ++c) {
            sf::Text cv;
            cv.setFont(Theme::FontPrimary);
            cv.setCharacterSize(12);
            cv.setFillColor((c == 2) ? Theme::Cyan : ((c == 5) ? Theme::Success : Theme::TextPrimary));
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
    title.setString(toSfString("BÁO CÁO DOANH THU & KẾT TOÁN CA"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnApproveShift) btnApproveShift->draw(window);
    if (btnCloseShift) btnCloseShift->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Tổng hợp doanh thu vé, bắp nước F&B và tiền mặt thực tế trong ca trực"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    // 3 Summary Cards
    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> shifts = {
        {"DOANH THU VÉ TRONG CA", "42.500.000 VNĐ", "Tổng số vé bán ra: 560 vé (POS & Online)", Theme::Cyan},
        {"DOANH THU QUẦY BẮP NƯỚC", "18.250.000 VNĐ", "Tổng số đơn hàng: 210 combo F&B", Theme::Warning},
        {"TỔNG TIỀN MẶT KẾT TOÁN", "60.750.000 VNĐ", "Khớp 100% két tiền mặt ca trực - Đã bàn giao", Theme::Success}
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
}

void CinemaUIApp::renderPromotionsView() {
    float contentX = sidebarWidth + 30.f;
    float contentY = topbarHeight + 25.f;

    sf::Text title;
    title.setFont(Theme::FontTitle);
    title.setCharacterSize(24);
    title.setFillColor(Theme::TextPrimary);
    title.setString(toSfString("CHƯƠNG TRÌNH KHUYẾN MÃI & SỰ KIỆN"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddPromo) btnAddPromo->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Các ưu đãi giá vé học sinh - sinh viên, ngày vàng và voucher giảm giá tại rạp"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    std::vector<std::tuple<std::string, std::string, std::string, sf::Color>> promos = {
        {"C-STUDENT 55.000 Đ", "Đồng giá vé 55.000đ cho Học sinh - Sinh viên từ Thứ 2 đến Thứ 6", "Mã áp dụng: CSTUDENT | Áp dụng mọi phim 2D", sf::Color(6, 182, 212)},
        {"HAPPY WEDNESDAY", "Thứ Tư Vui Vẻ - Giá vé chỉ 50.000đ cho tất cả khách hàng", "Mã áp dụng: HAPPYWED | Áp dụng cả ngày Thứ Tư", sf::Color(16, 185, 129)},
        {"COMBO BẮP NƯỚC VIP", "Giảm ngay 25% khi mua vé online kèm Combo bắp nước lớn", "Mã áp dụng: COMBODIAL | Áp dụng tại quầy F&B", sf::Color(245, 158, 11)},
        {"ƯU ĐÃI SINH NHẬT", "Tặng 01 vé xem phim 2D + 01 Bắp ngọt trong tuần sinh nhật", "Mã áp dụng: BIRTHDAY | Dành riêng cho hội viên", sf::Color(236, 72, 153)}
    };

    float cardW = 660.f;
    float cardH = 150.f;
    float gap = 20.f;
    float startY = contentY + 70.f;

    for (size_t i = 0; i < promos.size(); ++i) {
        int r = static_cast<int>(i / 2);
        int c = static_cast<int>(i % 2);
        float cx = contentX + c * (cardW + gap);
        float cy = startY + r * (cardH + gap);

        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
        RoundedRectRenderer::render(window, sf::Vector2f(cx, cy), sf::Vector2f(cardW, 4.f), 2.f, std::get<3>(promos[i]));

        sf::Text pName;
        pName.setFont(Theme::FontTitle);
        pName.setCharacterSize(18);
        pName.setFillColor(std::get<3>(promos[i]));
        pName.setString(toSfString(std::get<0>(promos[i])));
        pName.setPosition(cx + 24.f, cy + 20.f);
        window.draw(pName);

        sf::Text pDesc;
        pDesc.setFont(Theme::FontPrimary);
        pDesc.setCharacterSize(13);
        pDesc.setFillColor(Theme::TextPrimary);
        pDesc.setString(toSfString(std::get<1>(promos[i])));
        pDesc.setPosition(cx + 24.f, cy + 54.f);
        window.draw(pDesc);

        sf::Text pCode;
        pCode.setFont(Theme::FontSecondary);
        pCode.setCharacterSize(12);
        pCode.setFillColor(Theme::TextSecondary);
        pCode.setString(toSfString(std::get<2>(promos[i])));
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
    title.setString(toSfString("QUẢN LÝ NHÂN SỰ & CÁN BỘ ĐIỀU HÀNH"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnAddStaff) btnAddStaff->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Danh sách cán bộ quản trị và nhân sự vận hành cụm rạp CineStar Cinema"));
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

    std::vector<std::string> cols = {"MÃ NHÂN VIÊN", "VAI TRÒ / CHỨC VỤ", "SỐ ĐIỆN THOẠI", "EMAIL LIÊN HỆ", "CA TRỰC", "TRẠNG THÁI"};
    std::vector<float> colX = {contentX + 40.f, contentX + 220.f, contentX + 500.f, contentX + 750.f, contentX + 1020.f, contentX + 1180.f};

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

    struct StaffRow {
        std::string code, role, phone, email, shift, status;
    };
    std::vector<StaffRow> staff = {
        {"U001", "Quản Trị Viên (Admin)", "0905123456", "admin01@cinestar.vn", "Toàn Thời Gian", "Đang Làm Việc"},
        {"U002", "Nhân Viên Thu Ngân POS", "0905234567", "staff01@cinestar.vn", "Ca Sáng (08:00 - 16:00)", "Đang Làm Việc"},
        {"U003", "Nhân Viên Quầy Bắp Nước", "0905345678", "fnb01@cinestar.vn", "Ca Chiều (16:00 - 23:00)", "Đang Làm Việc"},
        {"U004", "Kỹ Thuật Viên Máy Chiếu", "0905456789", "tech01@cinestar.vn", "Ca Sáng (08:00 - 16:00)", "Đang Làm Việc"}
    };

    for (size_t r = 0; r < staff.size(); ++r) {
        float ry = tblY + 68.f + r * 44.f;
        sf::RectangleShape rLine(sf::Vector2f(tblW - 48.f, 1.f));
        rLine.setPosition(contentX + 24.f, ry + 36.f);
        rLine.setFillColor(sf::Color(51, 65, 85, 80));
        window.draw(rLine);

        std::vector<std::string> vals = {staff[r].code, staff[r].role, staff[r].phone, staff[r].email, staff[r].shift, staff[r].status};
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
    title.setString(toSfString("QUẢN LÝ TÀI KHOẢN & PHÂN QUYỀN"));
    title.setPosition(contentX, contentY);
    window.draw(title);

    if (btnChangePassword) btnChangePassword->draw(window);

    sf::Text sub;
    sub.setFont(Theme::FontSecondary);
    sub.setCharacterSize(13);
    sub.setFillColor(Theme::TextSecondary);
    sub.setString(toSfString("Quản trị tài khoản đăng nhập, bảo mật nội bộ và cấp quyền quản trị cụm rạp"));
    sub.setPosition(contentX, contentY + 34.f);
    window.draw(sub);

    // 2 Big Account Cards
    float cardW = 660.f;
    float cardH = 220.f;
    float startY = contentY + 70.f;

    // Card 1: Admin
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
    RoundedRectRenderer::render(window, sf::Vector2f(contentX, startY), sf::Vector2f(cardW, 4.f), 2.f, Theme::Primary);

    sf::Text a1Title;
    a1Title.setFont(Theme::FontTitle);
    a1Title.setCharacterSize(18);
    a1Title.setFillColor(Theme::Primary);
    a1Title.setString(toSfString("TÀI KHOẢN QUẢN TRỊ VIÊN (ADMIN)"));
    a1Title.setPosition(contentX + 24.f, startY + 20.f);
    window.draw(a1Title);

    sf::Text a1Desc;
    a1Desc.setFont(Theme::FontPrimary);
    a1Desc.setCharacterSize(13);
    a1Desc.setFillColor(Theme::TextPrimary);
    a1Desc.setString(toSfString("Tài khoản: admin01 | Trạng thái: Đang kích hoạt"));
    a1Desc.setPosition(contentX + 24.f, startY + 56.f);
    window.draw(a1Desc);

    sf::Text a1Perm;
    a1Perm.setFont(Theme::FontSecondary);
    a1Perm.setCharacterSize(12);
    a1Perm.setFillColor(Theme::TextSecondary);
    a1Perm.setString(toSfString("Quyền hạn: Toàn quyền quản trị hệ thống, quản lý doanh thu, xem nhật ký, thêm/sửa/xóa phim"));
    a1Perm.setPosition(contentX + 24.f, startY + 90.f);
    window.draw(a1Perm);

    // Card 2: Staff
    RoundedRectRenderer::render(window, sf::Vector2f(contentX + cardW + 20.f, startY), sf::Vector2f(cardW, cardH), Theme::RadiusMedium, Theme::CardBg, 1.f, Theme::CardBorder);
    RoundedRectRenderer::render(window, sf::Vector2f(contentX + cardW + 20.f, startY), sf::Vector2f(cardW, 4.f), 2.f, Theme::Cyan);

    sf::Text a2Title;
    a2Title.setFont(Theme::FontTitle);
    a2Title.setCharacterSize(18);
    a2Title.setFillColor(Theme::Cyan);
    a2Title.setString(toSfString("TÀI KHOẢN NHÂN VIÊN VẬN HÀNH (STAFF)"));
    a2Title.setPosition(contentX + cardW + 44.f, startY + 20.f);
    window.draw(a2Title);

    sf::Text a2Desc;
    a2Desc.setFont(Theme::FontPrimary);
    a2Desc.setCharacterSize(13);
    a2Desc.setFillColor(Theme::TextPrimary);
    a2Desc.setString(toSfString("Tài khoản: staff01 | Trạng thái: Đang kích hoạt"));
    a2Desc.setPosition(contentX + cardW + 44.f, startY + 56.f);
    window.draw(a2Desc);

    sf::Text a2Perm;
    a2Perm.setFont(Theme::FontSecondary);
    a2Perm.setCharacterSize(12);
    a2Perm.setFillColor(Theme::TextSecondary);
    a2Perm.setString(toSfString("Quyền hạn: Bán vé POS tại quầy, bán bắp nước F&B, tra cứu lịch chiếu và in hóa đơn khách hàng"));
    a2Perm.setPosition(contentX + cardW + 44.f, startY + 90.f);
    window.draw(a2Perm);
}

} // namespace UI
