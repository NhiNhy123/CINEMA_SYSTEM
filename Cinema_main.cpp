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
    shiftMgr.taiDuLieu(db);

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

        switch (choice) {
            case 1: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY PHIM ---\n";
                    cout << "1. Hien thi danh sach phim\n";
                    cout << "2. Them phim\n";
                    cout << "3. Sua phim\n";
                    cout << "4. Xoa phim\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) Movie::hienThiDanhSach(db);
                    else if (subChoice == 2) Movie::themPhim(db);
                    else if (subChoice == 3) Movie::suaPhim(db);
                    else if (subChoice == 4) Movie::xoaPhim(db);
                    else if (subChoice == 0) break;
                    else cout << "Lua chon khong hop le!\n";

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 2: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY PHONG ---\n";
                    cout << "1. Hien thi danh sach phong\n";
                    cout << "2. Them phong\n";
                    cout << "3. Sua phong\n";
                    cout << "4. Xoa phong\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) roomMgr.hienThiDanhSach();
                    else if (subChoice == 2) roomMgr.themPhong(db);
                    else if (subChoice == 3) roomMgr.suaPhong(db);
                    else if (subChoice == 4) roomMgr.xoaPhong(db);
                    else if (subChoice == 0) break;
                    else cout << "Lua chon khong hop le!\n";

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 3: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY GHE ---\n";
                    cout << "1. Xem so do ghe phong\n";
                    cout << "2. Khoi tao ghe hang loat cho phong\n";
                    cout << "3. Sao chep ghe giua cac phong\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) {
                        string maP;
                        cout << "Nhap ma phong: "; cin >> maP;
                        seatMgr.hienThiSoDoPhong(maP);
                    } else if (subChoice == 2) {
                        string maP;
                        cout << "Nhap ma phong: "; cin >> maP;
                        seatMgr.taoGheTuDongChoPhong(maP, db);
                    } else if (subChoice == 3) {
                        string pDich, pNguon;
                        cout << "Nhap phong dich (nhan): "; cin >> pDich;
                        cout << "Nhap phong nguon (mau): "; cin >> pNguon;
                        seatMgr.saoChepBucLuc(pDich, pNguon, db);
                    } else if (subChoice == 0) {
                        break;
                    } else {
                        cout << "Lua chon khong hop le!\n";
                    }

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 4: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY SUAT CHIEU ---\n";
                    cout << "1. Hien thi tat ca suat chieu\n";
                    cout << "2. Them suat chieu moi\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) Showtime::hienThiTatCa(db);
                    else if (subChoice == 2) Showtime::themSuatChieu(db);
                    else if (subChoice == 0) break;
                    else cout << "Lua chon khong hop le!\n";

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 5: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY F&B ---\n";
                    cout << "1. Hien thi danh muc F&B\n";
                    cout << "2. Them mon moi\n";
                    cout << "3. Sua thong tin mon\n";
                    cout << "4. Xoa mon\n";
                    cout << "5. Cap nhat trang thai kinh doanh\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) fandbMgr.hienThiDanhSach();
                    else if (subChoice == 2) fandbMgr.themMatHang(db);
                    else if (subChoice == 3) fandbMgr.suaMatHang(db);
                    else if (subChoice == 4) fandbMgr.xoaMatHang(db);
                    else if (subChoice == 5) fandbMgr.capNhatTrangThaiKinhDoanh(db);
                    else if (subChoice == 0) break;
                    else cout << "Lua chon khong hop le!\n";

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 6: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY KHUYEN MAI ---\n";
                    cout << "1. Hien thi danh sach KM\n";
                    cout << "2. Them chuong trinh KM\n";
                    cout << "3. Sua KM\n";
                    cout << "4. Xoa KM\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) promoMgr.hienThiDanhSach();
                    else if (subChoice == 2) promoMgr.themKhuyenMai(db);
                    else if (subChoice == 3) promoMgr.suaKhuyenMai(db);
                    else if (subChoice == 4) promoMgr.xoaKhuyenMai(db);
                    else if (subChoice == 0) break;
                    else cout << "Lua chon khong hop le!\n";

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 7:
                system("cls"); // Ðã thêm clear màn hình
                invoiceMgr.hienThiLichSuGiaoDich();
                break;
            case 8:
                system("cls"); // Ðã thêm clear màn hình
                invoiceMgr.thongKeTongQuanDoanhThu();
                break;
            case 9: {
                int subShift;
                do {
                    system("cls");
                    cout << "\n--- QUAN LY BAO CAO CA ---\n"
                         << "1. Xem ca cho duyet\n"
                         << "2. Duyet bao cao ca\n"
                         << "3. Khoa bao cao ca\n"
                         << "4. Xem tong hop tat ca ca\n"
                         << "0. Quay lai menu chinh\n"
                         << "Chon: ";
                    cin >> subShift;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subShift == 1) {
                        shiftMgr.hienThiCaChoDuyet();
                    } 
                    else if (subShift == 2) {
                        string maCaDuyet;
                        cout << "Nhap ma ca can duyet: ";
                        cin >> maCaDuyet;
                        shiftMgr.duyetBaoCao(maCaDuyet, db);
                    }     
                    else if (subShift == 3) {
                        string maCaKhoa;
                        cout << "Nhap ma ca can khoa bao cao: ";
                        cin >> maCaKhoa;
                        shiftMgr.khoaBaoCao(maCaKhoa, db);
                    } 
                    else if (subShift == 4) {
                    	shiftMgr.taiDuLieu(db);
                        shiftMgr.hienThiTongHop();
                        shiftMgr.hienThiTatCa();
                    }
                    else if (subShift == 0) {
                        break;
                    }
                    else {
                        cout << "Lua chon khong hop le!\n";
                    }

                    if (subShift != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subShift != 0);
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
    
    ticketMgr.taiDuLieu(db);
    invoiceMgr.taiDuLieu(db);
    shiftMgr.taiDuLieu(db);

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

        switch (choice) {
            case 1: {
                int subChoice;
                do {
                    system("cls");
                    cout << "\n--- BAN VE & THANH TOAN ---\n";
                    cout << "1. Thuc hien quy trinh ban ve moi\n";
                    cout << "0. Quay lai menu chinh\n";
                    cout << "Chon: ";
                    cin >> subChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Loi: Vui long nhap so hop le!\n";
                        system("pause");
                        continue;
                    }

                    if (subChoice == 1) {
                        ticketMgr.quyTrinhBanVe(db, maNV);
                    } else if (subChoice == 0) {
                        break;
                    } else {
                        cout << "Lua chon khong hop le!\n";
                    }

                    if (subChoice != 0) {
                        cout << "\n";
                        system("pause");
                    }
                } while (subChoice != 0);
                break;
            }
            case 2:
                system("cls"); // Ðã thêm clear màn hình
                ticketMgr.hienThiDanhSachVe(db);
                break;
            case 3:
                system("cls"); // Ðã thêm clear màn hình
                ticketMgr.checkInVe(db);
                break;
            case 4:
                system("cls"); // Ðã thêm clear màn hình
                invoiceMgr.taiDuLieu(db);
                invoiceMgr.hienThiLichSuGiaoDich();
                break;
            case 5: {
                system("cls"); // Ðã thêm clear màn hình
                string maCa, ngayLam, gioBD, gioKT;
                long long doanhThuHT, tienTT;
                cout << "\n--- CHOT CA LAM VIEC ---\n";
                cout << "Nhap ma ca (VD: CA_01): "; cin >> maCa;
                cout << "Nhap ngay lam (DD/MM/YYYY): "; cin >> ngayLam;
                cout << "Nhap gio bat dau (HH:MM): "; cin >> gioBD;
                cout << "Nhap gio ket thuc (HH:MM): "; cin >> gioKT;
                cout << "Nhap doanh thu he thong (VND): "; cin >> doanhThuHT;
                cout << "Nhap tien thuc te kiem ke trong ngan (VND): "; cin >> tienTT;

                shiftMgr.chotCa(maCa, maNV, "Nhan Vien Thuc Te", ngayLam, gioBD, gioKT, doanhThuHT, tienTT, db);
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
    SysLog sysLog("DATA/quan_ly_nhan_su.csv");

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
