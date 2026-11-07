#ifndef CINEMA_UI_APP_H
#define CINEMA_UI_APP_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <map>

#include "UITheme.h"
#include "RoundedRectRenderer.h"
#include "Button.h"
#include "RoundRectButton.h"
#include "TextButton.h"
#include "UIInputBox.h"
#include "TextBox.h"
#include "PasswordField.h"
#include "DropdownBox.h"
#include "UICheckboxItem.h"
#include "UIScrollableList.h"
#include "EditableTable.h"
#include "StatCard.h"
#include "GlobalSearchBar.h"
#include "UserSelectPopup.h"
#include "PosterSlider.h"

// Backend system headers
#include "../Database.h"
#include "../Movie.h"
#include "../Room.h"
#include "../Seat.h"
#include "../Showtime.h"
#include "../Ticket.h"
#include "../FAndB.h"
#include "../Customer.h"
#include "../Invoice.h"
#include "../Promotion.h"
#include "../ShilftManagement.h"
#include "../SysLog.h"
#include "../ActivityLog.h"
#include "../DateFormat.h"

namespace UI {

enum class ActiveTab {
    SeatBooking = 0,    // Bán vé (POS)
    CRM = 1,            // Khách hàng (CRM)
    Reports = 2,        // Quản lý Doanh thu
    Rooms = 3,          // Quản lý phòng
    Seats = 4,          // Quản lý ghế
    Showtimes = 5,      // Quản lý suất chiếu
    Movies = 6,         // Phim & Thể loại
    Concessions = 7,    // Quản lý F&B
    ShiftReport = 8,    // Quản lý Báo cáo ca
    Promotions = 9,     // Khuyến mãi SK
    Staff = 10,         // Quản lý nhân sự
    Accounts = 11,      // Quản lý tài khoản
    SystemLogs = 12,    // Nhật ký hoạt động
    Dashboard = 13      // Tổng quan (KPI)
};

struct VisualSeat {
    std::string maGhe;
    std::string loaiGhe;
    double donGia;
    std::string trangThai; // "Trong", "DaDat", "DangChon"
    sf::Vector2f position;
    sf::Vector2f size;
    bool isSelected;
};

struct StaffAccountInfo {
    std::string maNguoiDung;
    std::string hoTen;
    std::string soDienThoai;
    std::string email;
    std::string ngaySinh;
    std::string gioiTinh;
    std::string cccd;
    std::string taiKhoan;
    std::string matKhau;
    std::string vaiTro;
    std::string trangThaiLamViec;
    std::string trangThaiTaiKhoan;
};

class CinemaUIApp {
private:
    sf::RenderWindow window;
    Database db;

    // Authentication State
    bool isLoggedIn;
    std::string currentUserId;
    std::string currentUserName;
    std::string currentUserRole;
    std::string loginErrorMsg;

    // Login UI Components
    std::unique_ptr<UIInputBox> loginInputUser;
    std::unique_ptr<PasswordField> loginInputPass;
    std::unique_ptr<UICheckboxItem> chkRememberMe;
    std::unique_ptr<RoundRectButton> btnLogin;
    std::unique_ptr<RoundRectButton> btnQuickAdmin;
    std::unique_ptr<RoundRectButton> btnQuickStaff;
    std::unique_ptr<RoundRectButton> btnLogout;
    sf::Texture loginBackgroundTexture;
    sf::Sprite loginBackgroundSprite;
    float loginBackgroundScale;
    sf::Vector2f loginBackgroundOffset;
    sf::Texture cinemaLogoTexture;
    sf::Sprite cinemaLogoSprite;

    // Active View Tab
    ActiveTab currentTab;

    // Cached Backend Data (Synchronized 100% with SQLite DATA/cinema.db)
    std::vector<Movie> movieList;
    std::vector<Room> roomList;
    std::vector<Seat> allSeats;
    std::vector<Showtime> showtimeList;
    std::vector<FoodAndBeverage> fnbList;
    std::vector<std::pair<std::string, string>> genreList;
    std::vector<Customer> customerList;
    std::vector<Promotion> promoList;
    std::vector<ShiftReport> shiftList;
    std::vector<Invoice> invoiceList;
    std::vector<Ticket> ticketList;
    std::vector<StaffAccountInfo> staffUserList;

    // Navigation & Layout
    float sidebarWidth;
    float topbarHeight;
    std::vector<std::unique_ptr<RoundRectButton>> navButtons;

    // Top Bar
    std::unique_ptr<GlobalSearchBar> searchBar;

    // View 0: Dashboard Components
    std::vector<std::unique_ptr<StatCard>> dashboardStats;
    std::unique_ptr<PosterSlider> dashboardSlider;

    // View 1: Movies & Genres Components (White UI matching reference)
    int movieSubTab; // 0 = Quản lý Phim, 1 = Quản lý Thể loại
    int selectedMovieRowIdx;
    int selectedGenreRowIdx;
    int selectedGenreChoiceIdx;
    std::string movieAgeFilter; // "", "P", "T13", "T18", "C18"
    bool moviePosActive;
    std::map<std::string, std::shared_ptr<sf::Texture>> moviePosterTextures;

    std::unique_ptr<RoundRectButton> btnSubTabMovies;
    std::unique_ptr<RoundRectButton> btnSubTabGenres;
    std::unique_ptr<RoundRectButton> btnQuickAddMovie;
    std::unique_ptr<UIInputBox> movieSearchInput;

    // Movie Form Inputs
    std::unique_ptr<UIInputBox> inputMovieId;
    std::unique_ptr<UIInputBox> inputMovieTitle;
    std::unique_ptr<UIInputBox> inputMovieDuration;
    std::unique_ptr<UIInputBox> inputMovieReleaseDate;
    std::unique_ptr<UIInputBox> inputMovieAgeRating;
    std::unique_ptr<UIInputBox> inputMovieGenre;
    std::unique_ptr<UIInputBox> inputMoviePoster;
    std::unique_ptr<UIInputBox> inputMovieDesc;

    // Movie Form Buttons
    std::unique_ptr<RoundRectButton> btnSelectGenre;
    std::unique_ptr<RoundRectButton> btnMovieCreate;
    std::unique_ptr<RoundRectButton> btnMovieUpdate;
    std::unique_ptr<RoundRectButton> btnMovieDelete;
    std::unique_ptr<RoundRectButton> btnMovieClear;

    // Genre Form Inputs & Buttons
    std::unique_ptr<UIInputBox> inputGenreId;
    std::unique_ptr<UIInputBox> inputGenreName;
    std::unique_ptr<RoundRectButton> btnGenreCreate;
    std::unique_ptr<RoundRectButton> btnGenreUpdate;
    std::unique_ptr<RoundRectButton> btnGenreDelete;
    std::unique_ptr<RoundRectButton> btnGenreClear;
    std::unique_ptr<UIInputBox> genreSearchInput;
    std::unique_ptr<RoundRectButton> btnExportGenres;
    int selectedGenreColorIdx;
    std::map<std::string, int> genreColorMap;
    bool genreSortAsc;

    // View 2: Seat Booking Components (Bán Vé - POS Kiosk Mode)
    std::string currentShowtimeId;
    std::unique_ptr<DropdownBox> showtimeDropdown;
    std::vector<VisualSeat> currentSeats;
    std::vector<std::string> selectedSeatCodes;
    double bookingTotalPrice;
    std::unique_ptr<UIInputBox> inputCustomerPhone;
    std::unique_ptr<RoundRectButton> btnCheckoutTickets;

    // POS Kiosk UI Elements (Matching Image Reference)
    std::unique_ptr<RoundRectButton> btnPosRefresh;       // "Làm mới lịch chiếu"
    std::unique_ptr<RoundRectButton> btnPosDate;          // "Hôm nay, 06/04"
    std::unique_ptr<RoundRectButton> btnPosMovie;         // "Thỏ Ơi"
    std::unique_ptr<RoundRectButton> btnPosShowtime;      // "17:00 - Phòng 1 - Standard"
    std::unique_ptr<RoundRectButton> btnPosAddFnb;        // "+ Thêm bắp/nước"
    std::unique_ptr<RoundRectButton> btnPosCheckout;      // "Thanh toán (F5)"
    std::unique_ptr<RoundRectButton> btnPosCancel;        // "Hủy đơn"

    std::vector<std::pair<std::string, double>> posSelectedFnb;
    double posSeatTotal;
    double posFnbTotal;
    double posGrandTotal;
    int posSelectedMovieIdx;
    int posSelectedShowtimeIdx;
    bool posShowSeatMap;

    // View 3: F&B Concessions Components
    std::vector<int> fnbQuantities;
    std::unique_ptr<RoundRectButton> btnCheckoutFnb;

    // View 5: Logs Components
    std::unique_ptr<EditableTable> logsTable;

    // Modal Confirmation Dialog
    std::unique_ptr<UserSelectPopup> confirmPopup;

    // Action Buttons for Modules (linked from Cinema_main.cpp)
    std::unique_ptr<RoundRectButton> btnCheckInTicket;
    std::unique_ptr<RoundRectButton> btnAddCustomer;
    std::unique_ptr<RoundRectButton> btnReportAll;
    std::unique_ptr<RoundRectButton> btnReportCash;
    std::unique_ptr<RoundRectButton> btnReportBank;
    std::unique_ptr<RoundRectButton> btnAddRoom;
    std::unique_ptr<RoundRectButton> btnAutoGenerateSeats;
    std::unique_ptr<RoundRectButton> btnAddShowtime;
    std::unique_ptr<RoundRectButton> btnManageGenres;
    std::unique_ptr<RoundRectButton> btnAddFnb;
    std::unique_ptr<RoundRectButton> btnApproveShift;
    std::unique_ptr<RoundRectButton> btnCloseShift;
    std::unique_ptr<RoundRectButton> btnAddPromo;
    std::unique_ptr<RoundRectButton> btnAddStaff;
    std::unique_ptr<RoundRectButton> btnChangePassword;
    std::unique_ptr<RoundRectButton> btnRefreshLogs;

public:
    CinemaUIApp();
    ~CinemaUIApp() = default;

    bool init();
    void run();

private:
    void loadDataFromDatabase();
    void reloadMoviePosterTextures();
    void setupUIComponents();
    void setupLoginView();
    bool authenticateUser(const std::string& u, const std::string& p);

    void setupNavigation();
    void setupDashboardView();
    void setupMoviesView();
    void setupSeatBookingView();
    void setupConcessionsView();
    void setupLogsView();

    void refreshSeatsForShowtime(const std::string& maSuatChieu);
    void handleEvents();
    void update(float dt);
    void render();

    // View Renderers
    void renderLoginView();
    void renderSidebar();
    void renderTopBar();
    void renderDashboardView();
    void renderMoviesView();
    void renderSeatBookingView();
    void renderConcessionsView();
    void renderReportsView();
    void renderLogsView();
    void renderCRMView();
    void renderRoomsView();
    void renderSeatsView();
    void renderShowtimesView();
    void renderShiftReportView();
    void renderPromotionsView();
    void renderStaffView();
    void renderAccountsView();
    void drawSidebarMenuIcon(sf::RenderTarget& window, int index, sf::Vector2f pos, sf::Color color);
};

} // namespace UI

#endif // CINEMA_UI_APP_H
