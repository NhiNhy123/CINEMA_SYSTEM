#include "FAndB.h"
#include "Database.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

// --- CLASS FOOD AND BEVERAGE ---

FoodAndBeverage::FoodAndBeverage() {
    this->giaTien = 0.0;
    this->trangThaiKinhDoanh = "DangBan";
}

FoodAndBeverage::FoodAndBeverage(string maMatHang, string tenMatHang, string loai, double giaTien, string trangThaiKinhDoanh) {
    this->maMatHang = maMatHang;
    this->tenMatHang = tenMatHang;
    this->loai = loai;
    this->giaTien = giaTien;
    this->trangThaiKinhDoanh = trangThaiKinhDoanh;
}

// Getters
string FoodAndBeverage::getMaMatHang() const { return maMatHang; }
string FoodAndBeverage::getTenMatHang() const { return tenMatHang; }
string FoodAndBeverage::getLoai() const { return loai; }
double FoodAndBeverage::getGiaTien() const { return giaTien; }
string FoodAndBeverage::getTrangThaiKinhDoanh() const { return trangThaiKinhDoanh; }

// Setters
void FoodAndBeverage::setTenMatHang(const string& ten) { tenMatHang = ten; }
void FoodAndBeverage::setLoai(const string& l) { loai = l; }
void FoodAndBeverage::setGiaTien(double gia) { if (gia > 0) giaTien = gia; }
void FoodAndBeverage::setTrangThaiKinhDoanh(const string& trangThai) { trangThaiKinhDoanh = trangThai; }
// --- CHUY?N Ð?I D? LI?U CSV CHO F&B ---

string FoodAndBeverage::toCSV() const {
    stringstream ss;
    ss << fixed << setprecision(0) << giaTien;
    return maMatHang + "," + tenMatHang + "," + loai + "," + ss.str() + "," + trangThaiKinhDoanh;
}

FoodAndBeverage FoodAndBeverage::fromCSV(const string& line) {
    stringstream ss(line);
    string ma, ten, loai, strGia, trangThai;

    getline(ss, ma, ',');
    getline(ss, ten, ',');
    getline(ss, loai, ',');
    getline(ss, strGia, ',');
    getline(ss, trangThai, ',');

    double gia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> gia;
    }

    return FoodAndBeverage(ma, ten, loai, gia, trangThai);
}

void FoodAndBeverage::hienThi() const {
    cout << left << setw(10) << maMatHang 
         << setw(30) << tenMatHang 
         << setw(12) << loai 
         << setw(15) << fixed << setprecision(0) << giaTien 
         << setw(15) << trangThaiKinhDoanh << endl;
}


// --- CLASS F&B MANAGER ---

FAndBManager::FAndBManager() {}

void FAndBManager::taiDuLieu(Database& db) {
    db.docDanhSachFAndB(danhSachFAndB);
}

// Hien thi toan bo danh muc F&B
void FAndBManager::hienThiDanhSach() const {
    cout << "\n============================== DANH MUC DO AN & THUC UONG (F&B) ==============================\n";
    cout << left << setw(10) << "Ma SP" 
         << setw(30) << "Ten San Pham" 
         << setw(12) << "Loai" 
         << setw(15) << "Gia Ban (VND)" 
         << setw(15) << "Trang Thai" << endl;
    cout << "-------------------------------------------------------------------------------------------\n";
    if (danhSachFAndB.empty()) {
        cout << "Danh muc F&B hien dang trong!\n";
    } else {
        for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
            danhSachFAndB[i].hienThi();
        }
    }
    cout << "===========================================================================================\n";
}

// --- PHAN HE 1: QUAN TRI VIEN (ADMIN) ---

// Them mot hang moi
void FAndBManager::themMatHang(Database& db) {
    string ma, ten, loai;
    double gia;

    cout << "\n--- THEM MOT HANG F&B MOI ---\n";
    cout << "Nhap ma mat hang (VD: FB01): ";
    cin >> ma;
    
    // Kiem tra trung ma
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        if (danhSachFAndB[i].getMaMatHang() == ma) {
            cout << "Loi: Ma mat hang nay da ton tai trong he thong!\n";
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten mat hang: ";
    getline(cin, ten);
    
    cout << "Nhap loai san pham (Bap / Nuoc / Combo): ";
    cin >> loai;

    // Rang buoc gia ban > 0, bat loi khi nhap ky tu chu
    while (true) {
        cout << "Nhap gia ban (VND, phai > 0): ";
        cin >> gia;
        if (cin.fail()) {
            cin.clear(); // Xoa co loi cua cin
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Loi: Gia ban khong hop le (vui long nhap so)!\n";
        } else if (gia <= 0) {
            cout << "Loi: Gia ban phai lon hon 0!\n";
        } else {
              cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }

    // Mac dinh trang thai khi them moi la "DangBan"
    FoodAndBeverage newItem(ma, ten, loai, gia, "DangBan");
    danhSachFAndB.push_back(newItem);

    // Dong bo luu xuong file thong qua Database
    db.ghiDanhSachFAndB(danhSachFAndB);
    cout << "Them mat hang F&B thanh cong!\n";
}

// Sua thong tin mat hang
void FAndBManager::suaMatHang(Database& db) {
    cout << "\n--- SUA THONG TIN MAT HANG F&B ---\n";
    
    // 1. Hi?n th? danh sách m?t hàng d? ngu?i dùng xem mã
    hienThiDanhSach();

    string ma;
    cout << "\nNhap ma mat hang can sua (nhap 0 de quay lai): ";
    cin >> ma;
    if (ma == "0") return;

    // T?i l?i d? li?u m?i nh?t t? database
    taiDuLieu(db);
    bool found = false;

    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        if (danhSachFAndB[i].getMaMatHang() == ma) {
            found = true;
            int choice;
            do {
                // 2. In ra chi ti?t toàn b? thông tin hi?n t?i c?a m?t hàng
                cout << "\n==================================================\n";
                cout << "       THONG TIN CHI TIET MAT HANG DANG SUA       \n";
                cout << "==================================================\n";
                cout << "Ma mat hang : " << danhSachFAndB[i].getMaMatHang() << "\n";
                cout << "Ten mat hang: " << danhSachFAndB[i].getTenMatHang() << "\n";
                cout << "Loai hang   : " << danhSachFAndB[i].getLoai() << "\n";
                cout << "Gia ban     : " << fixed << setprecision(0) << danhSachFAndB[i].getGiaTien() << " VND\n";
                cout << "==================================================\n";

                // Menu tùy ch?n s?a t?ng thu?c tính
                cout << "1. Sua ten mat hang\n";
                cout << "2. Sua loai mat hang (Nuoc, DoAn...)\n";
                cout << "3. Sua gia ban\n";
                cout << "0. Hoan tat va luu thay doi\n";
                cout << "Chon thuoc tinh can sua (0-3): ";
                cin >> choice;

                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Xóa b? d?m sau khi nh?p s?

                switch (choice) {
                    case 1: {
                        string tenMoi;
                        cout << "Nhap ten moi (" << danhSachFAndB[i].getTenMatHang() << "): ";
                        getline(cin, tenMoi);
                        if (!tenMoi.empty()) {
                            danhSachFAndB[i].setTenMatHang(tenMoi);
                            cout << "-> Da cap nhat ten mat hang!\n";
                        }
                        break;
                    }
                    case 2: {
                        string loaiMoi;
                        cout << "Nhap loai moi (" << danhSachFAndB[i].getLoai() << "): ";
                        getline(cin, loaiMoi);
                        if (!loaiMoi.empty()) {
                            danhSachFAndB[i].setLoai(loaiMoi);
                            cout << "-> Da cap nhat loai mat hang!\n";
                        }
                        break;
                    }
                    case 3: {
                        double giaMoi;
                        while (true) {
                            cout << "Nhap gia ban moi (Hien tai: " << danhSachFAndB[i].getGiaTien() << "): ";
                            cin >> giaMoi;
                            if (cin.fail()) {
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                cout << "Loi: Gia ban khong hop le (vui long nhap so)!\n";
                            } else if (giaMoi <= 0) {
                                cout << "Loi: Gia ban phai lon hon 0!\n";
                            } else {
                                danhSachFAndB[i].setGiaTien(giaMoi);
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                cout << "-> Da cap nhat gia ban moi!\n";
                                break;
                            }
                        }
                        break;
                    }
                    case 0: {
                        // Luu toàn b? danh sách m?t hàng xu?ng Database khi b?m hoàn t?t
                        db.ghiDanhSachFAndB(danhSachFAndB);
                        cout << "\nLuu thay doi thong tin mat hang F&B thanh cong!\n";
                        break;
                    }
                    default:
                        cout << "Lua chon khong hop le! Vui long chon lai.\n";
                }
            } while (choice != 0);

            break;
        }
    }

    if (!found) {
        cout << "Khong tim thay ma mat hang can sua!\n";
    }
}

// Xoa mat hang
void FAndBManager::xoaMatHang(Database& db) {
    string ma;
    cout << "\n--- XOA MAT HANG F&B ---\n";
    cout << "Nhap ma mat hang can xoa: ";
    cin >> ma;

    bool deleted = false;
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        if (danhSachFAndB[i].getMaMatHang() == ma) {
            danhSachFAndB.erase(danhSachFAndB.begin() + i);
            deleted = true;
            break;
        }
    }

    if (deleted) {
        db.ghiDanhSachFAndB(danhSachFAndB);
        cout << "Da xoa mat hang thanh cong!\n";
    } else {
        cout << "Khong tim thay mat hang voi ma tuong ung de xoa!\n";
    }
}

// Cap nhat trang thai kinh doanh (Dang ban / Ngung ban)
void FAndBManager::capNhatTrangThaiKinhDoanh(Database& db) {
    string ma;
    cout << "\n--- CAP NHAT TRANG THAI KINH DOANH F&B ---\n";
    cout << "Nhap ma mat hang: ";
    cin >> ma;

    bool found = false;
    for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
        if (danhSachFAndB[i].getMaMatHang() == ma) {
            found = true;
            cout << "Trang thai hien tai cua [" << danhSachFAndB[i].getTenMatHang() << "]: " << danhSachFAndB[i].getTrangThaiKinhDoanh() << "\n";
            cout << "Chon trang thai moi:\n";
            cout << "1. Dang ban (DangBan)\n";
            cout << "2. Ngung ban (NgungBan)\n";
            cout << "Lua chon cua ban (1/2): ";
            int choice;
            cin >> choice;

            if (choice == 1) {
                danhSachFAndB[i].setTrangThaiKinhDoanh("DangBan");
                cout << "Da chuyen sang trang thai: Dang ban.\n";
            } else if (choice == 2) {
                danhSachFAndB[i].setTrangThaiKinhDoanh("NgungBan");
                cout << "Da chuyen sang trang thai: Ngung ban.\n";
            } else {
                cout << "Lua chon khong hop le, giu nguyen trang thai cu!\n";
                return;
            }

            db.ghiDanhSachFAndB(danhSachFAndB);
            break;
        }
    }
    if (!found) {
        cout << "Khong tim thay mat hang tuong ung!\n";
    }
}


// --- PHAN HE 2: NHAN VIEN (STAFF) ---

// Chon san pham F&B => nhap so luong => he thong tu dong tinh gia tien
void FAndBManager::chonMuaFAndB() const {
    if (danhSachFAndB.empty()) {
        cout << "\nKhong co san pham F&B nao trong he thong!\n";
        return;
    }

    cout << "\n--- GOI MON DO AN & THUC UONG (F&B) CHO KHACH ---\n";
    hienThiDanhSach();

    char tiepTuc = 'y';
    double tongTienFAndB = 0.0;

    while (tiepTuc == 'y' || tiepTuc == 'Y') {
        string maChon;
        int soLuong;

        cout << "\nNhap ma san pham muon mua: ";
        cin >> maChon;

        const FoodAndBeverage* selectedItem = NULL;
        for (size_t i = 0; i < danhSachFAndB.size(); ++i) {
            if (danhSachFAndB[i].getMaMatHang() == maChon) {
                selectedItem = &danhSachFAndB[i];
                break;
            }
        }

        if (!selectedItem) {
            cout << "Loi: Khong tim thay san pham co ma nay!\n";
        } else if (selectedItem->getTrangThaiKinhDoanh() == "NgungBan") {
            cout << "Rot tiec, san pham [" << selectedItem->getTenMatHang() << "] hien dang NGUNG BAN!\n";
        } else {
            cout << "Nhap so luong cho [" << selectedItem->getTenMatHang() << "]: ";
            cin >> soLuong;

            if (soLuong <= 0) {
                cout << "So luong phai lon hon 0!\n";
            } else {
                double thanhTien = selectedItem->getGiaTien() * soLuong;
                tongTienFAndB += thanhTien;
                cout << "-> Da them: " << soLuong << " x " << selectedItem->getTenMatHang() 
                     << " = " << fixed << setprecision(0) << thanhTien << " VND\n";
            }
        }

        cout << "Ban co muon chon them mon F&B khac khong? (y/n): ";
        cin >> tiepTuc;
    }

    cout << "\n----------------------------------------------------\n";
    cout << "TONG TIEN F&B TAM TINH CHO HOA DON: " << fixed << setprecision(0) << tongTienFAndB << " VND\n";
    cout << "----------------------------------------------------\n";
}
