#include "Promotion.h"
#include "Database.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

// --- CLASS PROMOTION ---

Promotion::Promotion() {
    this->phanTramGiam = 0.0;
    this->mucGiamToiDa = 0.0;
}

Promotion::Promotion(string maKM, string tenKM, string code, double ptGiam, double maxGiam, string start, string end, string phamVi) {
    this->maKhuyenMai = maKM;
    this->tenKhuyenMai = tenKM;
    this->maCode = code;
    this->phanTramGiam = ptGiam;
    this->mucGiamToiDa = maxGiam;
    this->ngayBatDau = start;
    this->ngayKetThuc = end;
    this->phamViApDung = phamVi;
}

// Getters
string Promotion::getMaKhuyenMai() const { return maKhuyenMai; }
string Promotion::getTenKhuyenMai() const { return tenKhuyenMai; }
string Promotion::getMaCode() const { return maCode; }
double Promotion::getPhanTramGiam() const { return phanTramGiam; }
double Promotion::getMucGiamToiDa() const { return mucGiamToiDa; }
string Promotion::getNgayBatDau() const { return ngayBatDau; }
string Promotion::getNgayKetThuc() const { return ngayKetThuc; }
string Promotion::getPhamViApDung() const { return phamViApDung; }

// Setters
void Promotion::setTenKhuyenMai(const string& ten) { tenKhuyenMai = ten; }
void Promotion::setMaCode(const string& code) { maCode = code; }
void Promotion::setPhanTramGiam(double pt) { if (pt >= 0 && pt <= 100) phanTramGiam = pt; }
void Promotion::setMucGiamToiDa(double maxGia) { if (maxGia >= 0) mucGiamToiDa = maxGia; }
void Promotion::setNgayBatDau(const string& start) { ngayBatDau = start; }
void Promotion::setNgayKetThuc(const string& end) { ngayKetThuc = end; }
void Promotion::setPhamViApDung(const string& pv) { phamViApDung = pv; }

string Promotion::toCSV() const {
    stringstream ssPT, ssMax;
    ssPT << fixed << setprecision(1) << phanTramGiam;
    ssMax << fixed << setprecision(0) << mucGiamToiDa;
    return maKhuyenMai + "," + tenKhuyenMai + "," + maCode + "," + ssPT.str() + "," + ssMax.str() + "," + ngayBatDau + "," + ngayKetThuc + "," + phamViApDung;
}

Promotion Promotion::fromCSV(const string& line) {
    stringstream ss(line);
    string maKM, tenKM, code, strPT, strMax, start, end, phamVi;

    getline(ss, maKM, ',');
    getline(ss, tenKM, ',');
    getline(ss, code, ',');
    getline(ss, strPT, ',');
    getline(ss, strMax, ',');
    getline(ss, start, ',');
    getline(ss, end, ',');
    getline(ss, phamVi, ',');

    double pt = 0.0, maxG = 0.0;
    if (!strPT.empty()) { stringstream(strPT) >> pt; }
    if (!strMax.empty()) { stringstream(strMax) >> maxG; }

    return Promotion(maKM, tenKM, code, pt, maxG, start, end, phamVi);
}

void Promotion::hienThi() const {
    cout << left << setw(10) << maKhuyenMai 
         << setw(20) << tenKhuyenMai 
         << setw(12) << maCode 
         << setw(10) << fixed << setprecision(1) << phanTramGiam << "%"
         << setw(15) << setprecision(0) << mucGiamToiDa 
         << setw(12) << ngayBatDau 
         << setw(12) << ngayKetThuc 
         << setw(15) << phamViApDung << endl;
}


// --- CLASS PROMOTION MANAGER ---

PromotionManager::PromotionManager() {}

void PromotionManager::taiDuLieu(Database& db) {
    db.docDanhSachKhuyenMai(danhSachKhuyenMai);
}

// Hien thi danh sach khuyen mai
void PromotionManager::hienThiDanhSach() const {
    cout << "\n========================================= DANH SACH KHUYEN MAI & VOUCHER =========================================\n";
    cout << left << setw(10) << "Ma KM" 
         << setw(20) << "Ten Chuong Trinh" 
         << setw(12) << "Ma Code" 
         << setw(10) << "Giam (%)"
         << setw(15) << "Giam Toi Da" 
         << setw(12) << "Bat Dau" 
         << setw(12) << "Ket Thuc" 
         << setw(15) << "Pham Vi" << endl;
    cout << "-----------------------------------------------------------------------------------------------------------------\n";
    if (danhSachKhuyenMai.empty()) {
        cout << "Chua co chuong trinh khuyen mai nao duoc cau hinh!\n";
    } else {
        for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
            danhSachKhuyenMai[i].hienThi();
        }
    }
    cout << "=================================================================================================================\n";
}

// --- PHAN HE 1: QUAN TRI VIEN (ADMIN) ---

void PromotionManager::themKhuyenMai(Database& db) {
    string maKM, tenKM, code, start, end, phamVi;
    double ptGiam, maxGiam;

    cout << "\n--- THEM CHUONG TRINH KHUYEN MAI / VOUCHER MOI ---\n";
    cout << "Nhap ma khuyen mai (VD: KM01): ";
    cin >> maKM;

    // Kiem tra trung ma ID hoac ma code
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        if (danhSachKhuyenMai[i].getMaKhuyenMai() == maKM) {
            cout << "Loi: Ma khuyen mai da ton tai!\n";
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten chuong trinh: ";
    getline(cin, tenKM);

    cout << "Nhap ma code dung khi thanh toan (VD: TET2026): ";
    cin >> code;

    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        if (danhSachKhuyenMai[i].getMaCode() == code) {
            cout << "Loi: Ma code nay da duoc su dung cho chuong trinh khac!\n";
            return;
        }
    }

    cout << "Nhap phan tram giam gia (% tu 0 den 100): ";
    cin >> ptGiam;

    cout << "Nhap muc giam tien toi da (VND): ";
    cin >> maxGiam;

    cout << "Nhap ngay bat dau (YYYY-MM-DD): ";
    cin >> start;
    cout << "Nhap ngay ket thuc (YYYY-MM-DD): ";
    cin >> end;

    // Rang buoc logic: Ngay bat dau khong duoc sau ngay ket thuc
    if (start > end) {
        cout << "Loi: Ngay bat dau khong the sau ngay ket thuc! Huy thao tac them.\n";
        return;
    }

    cout << "Nhap pham vi ap dung ('All' hoac ma phim cu the nhu 'P01'): ";
    cin >> phamVi;

    Promotion newPromo(maKM, tenKM, code, ptGiam, maxGiam, start, end, phamVi);
    danhSachKhuyenMai.push_back(newPromo);

    // Dong bo luu xuong file thong qua Database
    db.ghiDanhSachKhuyenMai(danhSachKhuyenMai);
    cout << "Them chuong trinh khuyen mai thanh cong!\n";
}

void PromotionManager::suaKhuyenMai(Database& db) {
    cout << "\n--- SUA THONG TIN KHUYEN MAI ---\n";
    
    // 1. Hi?n th? danh sách khuy?n mãi d? ngu?i dùng xem mã
    hienThiDanhSach();

    string maKM;
    cout << "\nNhap ma khuyen mai can sua (nhap 0 de quay lai): ";
    cin >> maKM;
    if (maKM == "0") return;

    // T?i l?i d? li?u m?i nh?t t? database
    taiDuLieu(db);
    bool found = false;

    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        if (danhSachKhuyenMai[i].getMaKhuyenMai() == maKM) {
            found = true;
            int choice;
            do {
                // 2. In ra chi ti?t toàn b? thông tin hi?n t?i c?a mã khuy?n mãi
                cout << "\n==================================================\n";
                cout << "     THONG TIN CHI TIET KHUYEN MAI DANG SUA       \n";
                cout << "==================================================\n";
                cout << "Ma khuyen mai : " << danhSachKhuyenMai[i].getMaKhuyenMai() << "\n";
                cout << "Ten khuyen mai: " << danhSachKhuyenMai[i].getTenKhuyenMai() << "\n";
                cout << "Phan tram giam: " << danhSachKhuyenMai[i].getPhanTramGiam() << "%\n";
                cout << "Giam toi da   : " << fixed << setprecision(0) << danhSachKhuyenMai[i].getMucGiamToiDa() << " VND\n";
                cout << "Ngay bat dau  : " << danhSachKhuyenMai[i].getNgayBatDau() << "\n";
                cout << "Ngay ket thuc : " << danhSachKhuyenMai[i].getNgayKetThuc() << "\n";
                cout << "Pham vi ap dung: " << danhSachKhuyenMai[i].getPhamViApDung() << "\n";
                cout << "==================================================\n";

                // Menu tùy ch?n s?a t?ng thu?c tính
                cout << "1. Sua ten khuyen mai\n";
                cout << "2. Sua phan tram giam (%)\n";
                cout << "3. Sua muc giam toi da\n";
                cout << "4. Sua thoi gian (Ngay bat dau & Ngay ket thuc)\n";
                cout << "5. Sua pham vi ap dung ('All' hoac ma phim)\n";
                cout << "0. Hoan tat va luu thay doi\n";
                cout << "Chon thuoc tinh can sua (0-5): ";
                cin >> choice;

                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Xóa b? d?m sau khi nh?p s?

                switch (choice) {
                    case 1: {
                        string tenMoi;
                        cout << "Nhap ten moi (" << danhSachKhuyenMai[i].getTenKhuyenMai() << "): ";
                        getline(cin, tenMoi);
                        if (!tenMoi.empty()) {
                            danhSachKhuyenMai[i].setTenKhuyenMai(tenMoi);
                            cout << "-> Da cap nhat ten khuyen mai!\n";
                        }
                        break;
                    }
                    case 2: {
                        double ptMoi;
                        cout << "Nhap phan tram giam moi (%) (" << danhSachKhuyenMai[i].getPhanTramGiam() << "): ";
                        cin >> ptMoi;
                        if (ptMoi >= 0 && ptMoi <= 100) {
                            danhSachKhuyenMai[i].setPhanTramGiam(ptMoi);
                            cout << "-> Da cap nhat phan tram giam!\n";
                        } else {
                            cout << "Loi: Phan tram giam phai tu 0 den 100!\n";
                        }
                        break;
                    }
                    case 3: {
                        double maxMoi;
                        cout << "Nhap muc giam toi da moi (" << danhSachKhuyenMai[i].getMucGiamToiDa() << "): ";
                        cin >> maxMoi;
                        if (maxMoi >= 0) {
                            danhSachKhuyenMai[i].setMucGiamToiDa(maxMoi);
                            cout << "-> Da cap nhat muc giam toi da!\n";
                        } else {
                            cout << "Loi: Muc giam toi da khong duoc am!\n";
                        }
                        break;
                    }
                    case 4: {
                        string startMoi, endMoi;
                        cout << "Nhap ngay bat dau moi (YYYY-MM-DD) (" << danhSachKhuyenMai[i].getNgayBatDau() << "): ";
                        getline(cin, startMoi);
                        cout << "Nhap ngay ket thuc moi (YYYY-MM-DD) (" << danhSachKhuyenMai[i].getNgayKetThuc() << "): ";
                        getline(cin, endMoi);

                        if (!startMoi.empty() && !endMoi.empty()) {
                            if (startMoi > endMoi) {
                                cout << "Loi: Ngay bat dau sau ngay ket thuc! Khong luu thay doi thoi gian.\n";
                            } else {
                                danhSachKhuyenMai[i].setNgayBatDau(startMoi);
                                danhSachKhuyenMai[i].setNgayKetThuc(endMoi);
                                cout << "-> Da cap nhat thoi gian ap dung!\n";
                            }
                        }
                        break;
                    }
                    case 5: {
                        string phamViMoi;
                        cout << "Nhap pham vi ap dung moi ('All' hoac ma phim) (" << danhSachKhuyenMai[i].getPhamViApDung() << "): ";
                        getline(cin, phamViMoi);
                        if (!phamViMoi.empty()) {
                            danhSachKhuyenMai[i].setPhamViApDung(phamViMoi);
                            cout << "-> Da cap nhat pham vi ap dung!\n";
                        }
                        break;
                    }
                    case 0: {
                        // Luu toàn b? danh sách khuy?n mãi xu?ng Database khi hoàn t?t
                        db.ghiDanhSachKhuyenMai(danhSachKhuyenMai);
                        cout << "\nLuu thay doi thong tin khuyen mai thanh cong!\n";
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
        cout << "Khong tim thay ma khuyen mai can sua!\n";
    }
}

void PromotionManager::xoaKhuyenMai(Database& db) {
    string maKM;
    cout << "\n--- XOA CHUONG TRINH KHUYEN MAI ---\n";
    cout << "Nhap ma khuyen mai can xoa: ";
    cin >> maKM;

    bool deleted = false;
    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        if (danhSachKhuyenMai[i].getMaKhuyenMai() == maKM) {
            danhSachKhuyenMai.erase(danhSachKhuyenMai.begin() + i);
            deleted = true;
            break;
        }
    }

    if (deleted) {
        db.ghiDanhSachKhuyenMai(danhSachKhuyenMai);
        cout << "Da xoa chuong trinh khuyen mai thanh cong!\n";
    } else {
        cout << "Khong tim thay ma khuyen mai tuong ung!\n";
    }
}


// --- PHAN HE 2: NHAN VIEN (STAFF) ---

// Kiem tra va ap dung ma voucher cho hoa don thanh toan
bool PromotionManager::apDungVoucher(const string& maCodeNhap, const string& maPhimChieu, double tongTienDonHang, double& soTienDuocGiam) const {
    const Promotion* matchedPromo = NULL;

    for (size_t i = 0; i < danhSachKhuyenMai.size(); ++i) {
        if (danhSachKhuyenMai[i].getMaCode() == maCodeNhap) {
            matchedPromo = &danhSachKhuyenMai[i];
            break;
        }
    }

    if (!matchedPromo) {
        cout << "Canh bao: Ma code voucher khong ton tai trong he thong!\n";
        soTienDuocGiam = 0.0;
        return false;
    }

    // Kiem tra pham vi ap dung (Doc quyen 1 phim hay All)
    string phamVi = matchedPromo->getPhamViApDung();
    if (phamVi != "All" && phamVi != maPhimChieu) {
        cout << "Thanh toan that bai: Ma doc quyen [" << maCodeNhap << "] chi ap dung cho phim [" << phamVi 
             << "], khong khop voi phim khach chon (" << maPhimChieu << ")!\n";
        soTienDuocGiam = 0.0;
        return false;
    }

    // Tinh toan so tien duoc giam dua tren phan tram va gioi han toi da
    double giamTheoPhanTram = tongTienDonHang * (matchedPromo->getPhanTramGiam() / 100.0);
    if (matchedPromo->getMucGiamToiDa() > 0 && giamTheoPhanTram > matchedPromo->getMucGiamToiDa()) {
        soTienDuocGiam = matchedPromo->getMucGiamToiDa();
    } else {
        soTienDuocGiam = giamTheoPhanTram;
    }

    cout << "Ap dung voucher thanh cong! Ten uu dai: " << matchedPromo->getTenKhuyenMai() 
         << " | Duoc giam: " << fixed << setprecision(0) << soTienDuocGiam << " VND\n";
    return true;
}
