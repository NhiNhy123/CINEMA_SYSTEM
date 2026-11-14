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

struct SeatTypeItem {
    std::string maLoai;
    std::string tenLoai;
    double donGia;
    std::string moTa;
    sf::Color mauSac;
    bool kichHoatBan;
};

struct SeatMapBox {
    std::string maGhe;
    std::string label;
    char rowChar;
    int colStart;
    int colEnd;
    std::string loaiGhe;
    bool isBooked;
    sf::FloatRect bounds;
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
    int movieCurrentPage;
    bool showMovieSpotlight;
    int spotlightMovieIdx;
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
    std::unique_ptr<DropdownBox> movieFormAgeDropdown;
    std::unique_ptr<DropdownBox> movieAgeFilterDropdown;
    std::vector<std::string> customAgeRatings;
    bool showAgeInputDialog;
    std::unique_ptr<UIInputBox> inputNewAgeRating;
    std::unique_ptr<RoundRectButton> btnSaveNewAge;
    std::unique_ptr<RoundRectButton> btnCancelNewAge;
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
    std::unique_ptr<RoundRectButton> btnQuickAddGenre;
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
    int fnbSubTab;
    int selectedFnbRowIdx;
    bool fnbSortAsc;
    std::string fnbFormLoai;
    bool fnbFormActive;
    std::unique_ptr<RoundRectButton> btnFnbTabAll;
    std::unique_ptr<RoundRectButton> btnFnbTabCombo;
    std::unique_ptr<UIInputBox> fnbSearchInput;
    std::unique_ptr<RoundRectButton> btnFnbExport;
    std::unique_ptr<RoundRectButton> btnFnbQuickAdd;
    std::unique_ptr<UIInputBox> inputFnbId;
    std::unique_ptr<UIInputBox> inputFnbName;
    std::unique_ptr<UIInputBox> inputFnbPrice;
    std::unique_ptr<UIInputBox> inputFnbNote;
    std::unique_ptr<UIInputBox> inputFnbImage;
    std::map<std::string, std::shared_ptr<sf::Texture>> fnbImageTextures;
    std::map<std::string, std::string> fnbImagePaths;
    std::unique_ptr<RoundRectButton> btnFnbCreate;
    std::unique_ptr<RoundRectButton> btnFnbDelete;
    std::unique_ptr<RoundRectButton> btnFnbUpdate;
    std::unique_ptr<RoundRectButton> btnFnbClear;

    // Combo Recipe items inside F&B Form
    int comboItem1Idx; // Index in retail list
    int comboItem1Qty; // Quantity of item 1
    int comboItem2Idx; // Index in retail list
    int comboItem2Qty; // Quantity of item 2
    std::vector<int> getRetailFnbIndices() const;

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

    // Rooms Management UI Components (CineOS v3.2 Pro)
    int selectedRoomRowIdx;
    int roomCurrentPage;
    std::string roomSearchQuery;
    int roomFilterType;  // 0: Tất cả, 1: Standard, 2: Premium, 3: Đặc biệt
    int roomFormTechIdx; // 0: Standard, 1: VIP, 2: IMAX, 3: 4DX, 4: 10D VR
    int roomFormCapacity;
    int roomFormStatusIdx; // 0: HoatDong (Sẵn sàng đón khách), 1: TamNgung (Tạm ngưng / Bảo trì)

    std::unique_ptr<UIInputBox> roomSearchInput;
    std::unique_ptr<RoundRectButton> btnRoomFilterAll;
    std::unique_ptr<RoundRectButton> btnRoomFilterStandard;
    std::unique_ptr<RoundRectButton> btnRoomFilterPremium;
    std::unique_ptr<RoundRectButton> btnRoomFilterSpecial;

    std::unique_ptr<RoundRectButton> btnRoomExportPDF;
    std::unique_ptr<RoundRectButton> btnRoomAddNew;

    std::unique_ptr<UIInputBox> inputRoomUUID;
    std::unique_ptr<UIInputBox> inputRoomName;
    std::unique_ptr<UIInputBox> inputRoomCapacity;
    std::unique_ptr<RoundRectButton> btnRoomCapMinus;
    std::unique_ptr<RoundRectButton> btnRoomCapPlus;
    std::vector<std::unique_ptr<RoundRectButton>> btnRoomTechOptions;
    std::unique_ptr<DropdownBox> roomStatusDropdown;

    std::unique_ptr<RoundRectButton> btnRoomCreate;
    std::unique_ptr<RoundRectButton> btnRoomUpdate;
    std::unique_ptr<RoundRectButton> btnRoomDelete;
    std::unique_ptr<RoundRectButton> btnRoomReset;

    std::unique_ptr<RoundRectButton> btnRoomPrevPage;
    std::unique_ptr<RoundRectButton> btnRoomNextPage;

    // Seats Management UI Components (CineOS Enterprise / v3.2 Pro)
    int seatSubTab; // 0: Thiết lập sơ đồ ghế, 1: Loại ghế & đơn giá
    std::string seatSelectedRoomId;
    int seatViewMode; // 0: 3D, 1: 2D Phẳng, 2: POV Ghế
    float seatZoomLevel;
    std::vector<std::string> seatSelectedCodes;
    int seatListPage;
    std::string seatListSearchQuery;
    int selectedSeatRowIdx;

    // Tab 1 UI controls
    std::unique_ptr<DropdownBox> seatRoomDropdown;
    std::unique_ptr<RoundRectButton> btnSeatCopyLayout;
    std::unique_ptr<RoundRectButton> btnSeatExportMap;
    std::unique_ptr<RoundRectButton> btnSeatAddRowQuick;
    std::unique_ptr<RoundRectButton> btnSeatSubTabMatrix;
    std::unique_ptr<RoundRectButton> btnSeatSubTabTypes;

    std::unique_ptr<RoundRectButton> btnSeatView3D;
    std::unique_ptr<RoundRectButton> btnSeatView2D;
    std::unique_ptr<RoundRectButton> btnSeatViewPOV;
    std::unique_ptr<RoundRectButton> btnSeatZoomIn;
    std::unique_ptr<RoundRectButton> btnSeatZoomOut;
    std::unique_ptr<RoundRectButton> btnSeatZoomReset;

    std::unique_ptr<UIInputBox> seatSearchInput;
    std::unique_ptr<RoundRectButton> btnSeatListPrev;
    std::unique_ptr<RoundRectButton> btnSeatListNext;

    std::unique_ptr<UIInputBox> inputSeatBatchRows;
    std::unique_ptr<UIInputBox> inputSeatBatchCols;
    std::unique_ptr<DropdownBox> seatBatchTypeDropdown;
    std::unique_ptr<RoundRectButton> btnSeatBatchAdd;
    std::unique_ptr<RoundRectButton> btnSeatUpdateType;
    std::unique_ptr<RoundRectButton> btnSeatDeleteBatch;
    std::unique_ptr<RoundRectButton> btnSeatResetForm;

    // Tab 2 UI controls
    std::vector<SeatTypeItem> seatTypeList;
    int selectedSeatTypeRowIdx;
    int seatTypeFilterIdx; // 0: Tất cả, 1: Ghế Đơn, 2: Ghế Đôi / Premium, 3: Công Nghệ Đặc Biệt
    std::unique_ptr<RoundRectButton> btnSeatExportPricing;
    std::unique_ptr<RoundRectButton> btnSeatAddNewType;
    std::unique_ptr<RoundRectButton> btnSeatPriceHistory;
    std::unique_ptr<UIInputBox> seatTypeSearchInput;
    std::unique_ptr<RoundRectButton> btnSeatTypeFilterAll;
    std::unique_ptr<RoundRectButton> btnSeatTypeFilterSingle;
    std::unique_ptr<RoundRectButton> btnSeatTypeFilterCouple;
    std::unique_ptr<RoundRectButton> btnSeatTypeFilterSpecial;

    std::string seatFormTypeCode;
    std::unique_ptr<UIInputBox> inputSeatTypeName;
    std::unique_ptr<UIInputBox> inputSeatTypePrice;
    std::unique_ptr<UIInputBox> inputSeatTypeDesc;
    int seatTypeColorIdx; // 0..4
    bool seatTypeSaleActive;

    std::unique_ptr<RoundRectButton> btnSeatTypeAdd;
    std::unique_ptr<RoundRectButton> btnSeatTypeUpdate;
    std::unique_ptr<RoundRectButton> btnSeatTypeDelete;
    std::unique_ptr<RoundRectButton> btnSeatTypeReset;

    std::unique_ptr<RoundRectButton> btnAddShowtime;
    std::unique_ptr<RoundRectButton> btnManageGenres;
    std::unique_ptr<RoundRectButton> btnAddFnb;
    std::unique_ptr<RoundRectButton> btnApproveShift;
    std::unique_ptr<RoundRectButton> btnCloseShift;
    std::unique_ptr<RoundRectButton> btnAddPromo;
    std::unique_ptr<RoundRectButton> btnAddStaff;
    std::unique_ptr<RoundRectButton> btnChangePassword;
    std::unique_ptr<RoundRectButton> btnRefreshLogs;

    // Showtime Management UI Components (CineOS v3.2 Design)
    int showtimeSubTab; // 0: Bảng danh sách, 1: Timeline lịch theo phòng, 2: Lịch chiếu theo phim
    int selectedShowtimeRowIdx;
    int showtimeCurrentPage;
    int stLastValidMinutes;
    std::string showtimeTimelineDate;
    std::unique_ptr<RoundRectButton> btnStTabList;
    std::unique_ptr<RoundRectButton> btnStTabTimeline;
    std::unique_ptr<RoundRectButton> btnStTabByMovie;
    std::unique_ptr<RoundRectButton> btnStExport;
    std::unique_ptr<RoundRectButton> btnStNew;

    std::unique_ptr<UIInputBox> stSearchInput;
    std::unique_ptr<DropdownBox> stMovieFilterDropdown;
    std::unique_ptr<DropdownBox> stDateDropdown;
    std::unique_ptr<RoundRectButton> btnStTodayQuick;
    std::unique_ptr<DropdownBox> stRoomDropdown;
    std::unique_ptr<RoundRectButton> btnStConfig;
    std::string stMovieFilterVal;
    std::string stDateFilterVal;
    std::string stRoomFilterVal;

    // Right Card Form Components
    int stFormMovieIdx;
    int stFormRoomIdx;
    std::unique_ptr<DropdownBox> stMovieDropdown;
    std::unique_ptr<DropdownBox> stRoomFormDropdown;
    std::unique_ptr<UIInputBox> inputStDate;
    std::unique_ptr<DropdownBox> stDatePickerDropdown;
    std::unique_ptr<UIInputBox> inputStTime;
    std::unique_ptr<DropdownBox> stTimePickerDropdown;
    int stCleaningMinutes; // 10, 15, 20
    std::unique_ptr<RoundRectButton> btnStCleanMinus;
    std::unique_ptr<RoundRectButton> btnStCleanPlus;
    std::unique_ptr<RoundRectButton> btnStPreset10;
    std::unique_ptr<RoundRectButton> btnStPreset15;
    std::unique_ptr<RoundRectButton> btnStPreset20;
    std::unique_ptr<RoundRectButton> btnStSave;
    std::unique_ptr<RoundRectButton> btnStCreate;
    std::unique_ptr<RoundRectButton> btnStDelete;
    std::unique_ptr<RoundRectButton> btnStReset;

    // Promotions Management UI Components (CineOS v3.2 Pro)
    int selectedPromoRowIdx;
    int promoCurrentPage;
    int promoFilterTab; // 0: Tất cả, 1: Đang chạy, 2: Sắp tới, 3: Hết hạn
    std::string promoScopeVal; // "All" or movie ID e.g. "P01"

    std::unique_ptr<RoundRectButton> btnPromoExport;
    std::unique_ptr<RoundRectButton> btnPromoNew;
    std::unique_ptr<UIInputBox> promoSearchInput;
    std::unique_ptr<RoundRectButton> btnPromoFilterAll;
    std::unique_ptr<RoundRectButton> btnPromoFilterActive;
    std::unique_ptr<RoundRectButton> btnPromoFilterUpcoming;
    std::unique_ptr<RoundRectButton> btnPromoFilterExpired;

    std::unique_ptr<UIInputBox> inputPromoId;
    std::unique_ptr<UIInputBox> inputPromoName;
    std::unique_ptr<UIInputBox> inputPromoCode;
    std::unique_ptr<RoundRectButton> btnPromoCopyCode;
    std::unique_ptr<UIInputBox> inputPromoPercent;
    std::unique_ptr<UIInputBox> inputPromoMaxDiscount;
    std::unique_ptr<UIInputBox> inputPromoStartDate;
    std::unique_ptr<UIInputBox> inputPromoEndDate;
    std::unique_ptr<DropdownBox> promoScopeDropdown;
    std::unique_ptr<RoundRectButton> btnPromoSelectMovie;

    std::unique_ptr<RoundRectButton> btnPromoCreate;
    std::unique_ptr<RoundRectButton> btnPromoUpdate;
    std::unique_ptr<RoundRectButton> btnPromoDelete;
    std::unique_ptr<RoundRectButton> btnPromoClear;

public:
    CinemaUIApp();
    ~CinemaUIApp() = default;

    bool init();
    void run();

private:
    void loadDataFromDatabase();
    void reloadMoviePosterTextures();
    void reloadFnbImageTextures();
    void setupUIComponents();
    void setupLoginView();
    bool authenticateUser(const std::string& u, const std::string& p);

    void setupNavigation();
    void setupDashboardView();
    void setupMoviesView();
    void setupRoomsView();
    void setupSeatsView();
    void setupSeatBookingView();
    void setupConcessionsView();
    void setupShowtimesView();
    void setupPromotionsView();
    void setupLogsView();

    void syncPromoSelectionToForm(int idx);
    void refreshPromoScopeDropdown();
    std::string generateNextPromoId() const;
    std::vector<int> getFilteredPromoIndices() const;

    void refreshSeatTypeList();
    void syncSeatTypeSelectionToForm(int idx);
    void applyBatchSeatChanges(bool isUpdate);
    void deleteSelectedSeats();
    void refreshSeatsRoomDropdown();
    bool isSeatBookedInRoom(const std::string& maPhong, const std::string& maGhe) const;
    std::vector<SeatMapBox> buildCurrentRoomSeatMapBoxes() const;
    std::vector<int> getFilteredRoomSeatIndices() const;

    void refreshSeatsForShowtime(const std::string& maSuatChieu);
    void syncShowtimeSelectionToForm(int idx);
    void syncRoomSelectionToForm(int idx);
    void generateSeatsForRoom(const std::string& roomId, int capacity, const std::string& techType);
    void refreshShowtimesDropdowns();
    void refreshAgeRatingDropdowns();
    void syncMovieFormAgeDropdown(const std::string& ageCode);
    void addNewAgeRating(const std::string& newAge);
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
