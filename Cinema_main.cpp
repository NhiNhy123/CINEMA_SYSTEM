#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include "Database.h"
#include "User.h"
#include "Customer.h"
#include "Movie.h"
#include "Room.h"
#include "Seat.h"
#include "Showtime.h"
#include "Ticket.h"
#include "FAndB.h"
#include "Promotion.h"
#include "Invoice.h"
#include "ShilftManagement.h"
#include "SysLog.h"

using namespace std;

void menuAdmin(Database& db) {
    RoomManager roomMgr;
    SeatManager seatMgr;
    FAndBManager fandbMgr;
    PromotionManager promoMgr;
    InvoiceManager invoiceMgr;
    ShiftManagement shiftMgr;
    
    roomMgr.taiDuLieu(db);
    seatMgr.taiDuLieu(db);
    fandbMgr.taiDuLieu(db);
    promoMgr.taiDuLieu(db);
    invoiceMgr.taiDuLieu(db);

    int choice = 0;
    do {
        system("cls"); // Xóa màn hình tru?c khi hi?n menu Admin
        cout << "\n============================== MENU QUAN TRI VIEN (ADMIN) ==============================\n";
        cout << "1. Quan ly Phim & The loai\n";
        cout << "2. Quan ly Phong chieu\n";
        cout << "3. Quan ly So do ghe\n";
        cout << "4. Quan ly Suat chieu\n";
        cout << "5. Quan ly Do an & Thuc uong (F&B)\n";
        cout << "6. Quan ly Khuyen mai & Voucher\n";
        cout << "7. Xem lich su giao dich hoa don\n";
        cout << "8. Thong ke tong quan doanh thu\n";
        cout << "9. Quan ly & Duyet bao cao ca lam viec (Shift Management)\n";
        cout << "0. Dang xuat (Quay lai man hinh dang nhap)\n";
        cout << "========================================================================================\n";
        cout << "Nhap lua chon cua ban: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Loi: Vui long nhap so hop le!\n";
            system("pause");
            continue;
        }

        system("cls"); // Xóa màn hình tru?c khi vào tác v? con c?a Admin
        switch (choice) {
            case 1: {
                int subChoice;
                cout << "\n--- QUAN LY PHIM ---\n1. Hien thi danh sach phim\n2. Them phim\n3. Sua phim\n4. Xoa phim\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) Movie::hienThiDanhSach(db);
                else if (subChoice == 2) Movie::themPhim(db);
                else if (subChoice == 3) Movie::suaPhim(db);
                else if (subChoice == 4) Movie::xoaPhim(db);
                break;
            }
            case 2: {
                int subChoice;
                cout << "\n--- QUAN LY PHONG ---\n1. Hien thi danh sach phong\n2. Them phong\n3. Sua phong\n4. Xoa phong\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) roomMgr.hienThiDanhSach();
                else if (subChoice == 2) roomMgr.themPhong(db);
                else if (subChoice == 3) roomMgr.suaPhong(db);
                else if (subChoice == 4) roomMgr.xoaPhong(db);
                break;
            }
            case 3: {
                int subChoice;
                string maP;
                cout << "\n--- QUAN LY GHE ---\n1. Xem so do ghe phong\n2. Khoi tao ghe hang loat cho phong\n3. Sao chep ghe giua cac phong\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) {
                    cout << "Nhap ma phong: "; cin >> maP;
                    seatMgr.hienThiSoDoPhong(maP);
                } else if (subChoice == 2) {
                    cout << "Nhap ma phong: "; cin >> maP;
                    seatMgr.taoGheHangLoat(maP, db);
                } else if (subChoice == 3) {
                    string pDich, pNguon;
                    cout << "Nhap phong dich (nhan): "; cin >> pDich;
                    cout << "Nhap phong nguon (mau): "; cin >> pNguon;
                    seatMgr.saoChepBucLuc(pDich, pNguon, db);
                }
                break;
            }
            case 4: {
                int subChoice;
                cout << "\n--- QUAN LY SUAT CHIEU ---\n1. Hien thi tat ca suat chieu\n2. Them suat chieu moi\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) Showtime::hienThiTatCa(db);
                else if (subChoice == 2) Showtime::themSuatChieu(db);
                break;
            }
            case 5: {
                int subChoice;
                cout << "\n--- QUAN LY F&B ---\n1. Hien thi danh muc F&B\n2. Them mon moi\n3. Sua thong tin mon\n4. Xoa mon\n5. Cap nhat trang thai kinh doanh\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) fandbMgr.hienThiDanhSach();
                else if (subChoice == 2) fandbMgr.themMatHang(db);
                else if (subChoice == 3) fandbMgr.suaMatHang(db);
                else if (subChoice == 4) fandbMgr.xoaMatHang(db);
                else if (subChoice == 5) fandbMgr.capNhatTrangThaiKinhDoanh(db);
                break;
            }
            case 6: {
                int subChoice;
                cout << "\n--- QUAN LY KHUYEN MAI ---\n1. Hien thi danh sach KM\n2. Them chuong trinh KM\n3. Sua KM\n4. Xoa KM\nChon: ";
                cin >> subChoice;
                if (subChoice == 1) promoMgr.hienThiDanhSach();
                else if (subChoice == 2) promoMgr.themKhuyenMai(db);
                else if (subChoice == 3) promoMgr.suaKhuyenMai(db);
                else if (subChoice == 4) promoMgr.xoaKhuyenMai(db);
                break;
            }
            case 7:
                invoiceMgr.hienThiLichSuGiaoDich();
                break;
            case 8:
                invoiceMgr.thongKeTongQuanDoanhThu();
                break;
            case 9: {
                int subShift;
                cout << "\n--- QUAN LY BAO CAO CA ---\n1. Xem ca cho duyet\n2. Duyet bao cao ca\n3. Xem tong hop tat ca ca\nChon: ";
                cin >> subShift;
                if (subShift == 1) {
                    shiftMgr.hienThiCaChoDuyet();
                } else if (subShift == 2) {
                    string maCaDuyet;
                    cout << "Nhap ma ca can duyet: ";
                    cin >> maCaDuyet;
                    shiftMgr.duyetBaoCao(maCaDuyet);
                } else if (subShift == 3) {
                    shiftMgr.hienThiTongHop();
                    shiftMgr.hienThiTatCa();
                }
                break;
            }
            case 0:
                cout << "Dang xuat khoi tai khoan Admin...\n";
                break;
            default:
                cout << "Lua chon khong hop le!\n";
        }
        if (choice != 0) {
            cout << "\n";
            system("pause");
        }
    } while (choice != 0);
}

void menuStaff(Database& db, const string& maNV) {
    TicketManager ticketMgr;
    InvoiceManager invoiceMgr;
    ShiftManagement shiftMgr;
    
    invoiceMgr.taiDuLieu(db);

    int choice = 0;
    do {
        system("cls"); // Xóa màn hình tru?c khi hi?n menu Staff
        cout << "\n============================== MENU NHAN VIEN (STAFF) ==============================\n";
        cout << "1. Thuc hien quy trinh ban ve & thanh toan (Chon ghe, F&B, Voucher, Hoa don)\n";
        cout << "2. Hien thi danh sach ve da ban\n";
        cout << "3. Soat ve / Check-in tai cua phong chieu\n";
        cout << "4. Xem lich su giao dich hoa don\n";
        cout << "5. Chot ca lam viec & nop tien doanh thu\n";
        cout << "0. Dang xuat (Quay lai man hinh dang nhap)\n";
        cout << "====================================================================================\n";
        cout << "Nhap lua chon cua ban: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Loi: Vui long nhap so hop le!\n";
            system("pause");
            continue;
        }

        system("cls"); // Xóa màn hình tru?c khi vào tác v? con c?a Staff
        switch (choice) {
            case 1: {
                ticketMgr.quyTrinhBanVe(db);
                break;
            }
            case 2:
                ticketMgr.hienThiDanhSachVe(db);
                break;
            case 3:
                ticketMgr.checkInVe(db);
                break;
            case 4:
                invoiceMgr.hienThiLichSuGiaoDich();
                break;
            case 5: {
                string maCa, ngayLam, gioBD, gioKT;
                long long doanhThuHT, tienTT;
                cout << "\n--- CHOT CA LAM VIEC ---\n";
                cout << "Nhap ma ca (VD: CA_01): "; cin >> maCa;
                cout << "Nhap ngay lam (DD/MM/YYYY): "; cin >> ngayLam;
                cout << "Nhap gio bat dau (HH:MM): "; cin >> gioBD;
                cout << "Nhap gio ket thuc (HH:MM): "; cin >> gioKT;
                cout << "Nhap doanh thu he thong (VND): "; cin >> doanhThuHT;
                cout << "Nhap tien thuc te kiem ke trong ngan (VND): "; cin >> tienTT;

                shiftMgr.chotCa(maCa, maNV, "Nhan Vien Thuc Te", ngayLam, gioBD, gioKT, doanhThuHT, tienTT);
                break;
            }
            case 0:
                cout << "Dang xuat khoi tai khoan Staff...\n";
                break;
            default:
                cout << "Lua chon khong hop le!\n";
        }
        if (choice != 0) {
            cout << "\n";
            system("pause");
        }
    } while (choice != 0);
}

int main() {
    Database db;
    SysLog sysLog("quan_ly_nhan_su.csv");

    while (true) {
        system("cls"); // Xóa màn hình tru?c khi hi?n th? giao di?n dang nh?p
        User* currentUser = sysLog.dangNhap();

        if (currentUser != NULL) {
            string vaiTro = currentUser->getVaiTro();
            string maNV = currentUser->getMaNguoiDung();

            system("cls"); // Xóa màn hình dang nh?p thành công tru?c khi vào Menu chính
            if (vaiTro == "Admin") {
                menuAdmin(db);
            } else if (vaiTro == "Staff") {
                menuStaff(db, maNV);
            }

            delete currentUser; 
        } else {
            char thoat;
            cout << "\nBan co muon thoat chuong trinh khong? (y/n): ";
            cin >> thoat;
            if (thoat == 'y' || thoat == 'Y') {
                break;
            }
        }
    }

    system("cls");
    cout << "\nCam on ban da su dung he thong quan ly rap chieu phim. Tam biet!\n";
    return 0;
}
