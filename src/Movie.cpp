#include "../include/Movie.h"
#include "../include/Database.h"
#include "../include/Showtime.h"
#include "../include/DateFormat.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

// Constructor m?c d?nh
Movie::Movie() {
    thoiLuong = 0;
    nhanDoTuoi = 0;
}

// Constructor d?y d? tham s?
Movie::Movie(string ma, string ten, int thoiLuong, int nhanTuoi, string ngayKC, string ngayKT, string moTa, string poster) {
    this->maPhim = ma;
    this->tenPhim = ten;
    this->thoiLuong = thoiLuong;
    this->nhanDoTuoi = nhanTuoi;
    this->ngayKhoiChieu = ngayKC;
    this->ngayKetThuc = ngayKT;
    this->moTa = moTa;
    this->poster = poster;
}

// Getters
string Movie::getMaPhim() const { return maPhim; }
string Movie::getTenPhim() const { return tenPhim; }
int Movie::getThoiLuong() const { return thoiLuong; }
int Movie::getNhanDoTuoi() const { return nhanDoTuoi; }
string Movie::getNgayKhoiChieu() const { return ngayKhoiChieu; }
string Movie::getNgayKetThuc() const { return ngayKetThuc; }
string Movie::getMoTa() const { return moTa; }
string Movie::getPoster() const { return poster; }

// Setters
void Movie::setTenPhim(const string& ten) { tenPhim = ten; }
void Movie::setThoiLuong(int tl) { thoiLuong = tl; }
void Movie::setNhanDoTuoi(int tuoi) { nhanDoTuoi = tuoi; }
void Movie::setNgayKhoiChieu(const string& ngay) { ngayKhoiChieu = ngay; }
void Movie::setNgayKetThuc(const string& ngay) { ngayKetThuc = ngay; }
void Movie::setMoTa(const string& desc) { moTa = desc; }
void Movie::setPoster(const string& p) { poster = p; }

// Chuy?n d?i d?i tu?ng Movie thành dòng CSV
string Movie::toCSV() const {
    stringstream ssTL, ssTuoi;
    ssTL << thoiLuong;
    ssTuoi << nhanDoTuoi;
    return maPhim + "," + tenPhim + "," + ssTL.str() + "," + ssTuoi.str() + "," + ngayKhoiChieu + "," + ngayKetThuc + "," + moTa + "," + poster;
}

// Ð?c m?t dòng t? file Database
Movie Movie::fromCSV(const string& line) {
    stringstream ss(line);
    string ma, ten, strThoiLuong, strNhanTuoi, ngayKC, ngayKT, moTa, poster;

    getline(ss, ma, ',');
    getline(ss, ten, ',');
    getline(ss, strThoiLuong, ',');
    getline(ss, strNhanTuoi, ',');
    getline(ss, ngayKC, ',');
    getline(ss, ngayKT, ',');
    getline(ss, moTa, ',');
    getline(ss, poster, ',');

    int thoiLuong = 0;
    if (!strThoiLuong.empty()) {
        stringstream ssTL(strThoiLuong);
        ssTL >> thoiLuong;
    }

    int nhanDoTuoi = 0;
    if (!strNhanTuoi.empty()) {
        stringstream ssTuoi(strNhanTuoi);
        ssTuoi >> nhanDoTuoi;
    }

    return Movie(ma, ten, thoiLuong, nhanDoTuoi, ngayKC, ngayKT, moTa, poster);
}

// Hi?n th? m?t dòng thông tin phim
void Movie::hienThi(Database& db) const {
    stringstream temp1, temp2;
    temp1 << thoiLuong << "p";
    temp2 << nhanDoTuoi << "+";

    string ssTL = temp1.str();
    string ssTuoi = temp2.str();
    string tenTheLoai = db.layTheLoaiCuaPhim(maPhim); // L?y th? lo?i t? Database

    cout << left << setw(10) << maPhim 
         << setw(25) << tenPhim 
         << setw(23) << tenTheLoai
         << setw(12) << ssTL
         << setw(10) << ssTuoi
         << setw(15) << ngayKhoiChieu 
         << setw(15) << ngayKetThuc << endl;
}

void Movie::hienThiDanhSach(Database& db) {
    vector<Movie> danhSach;
    db.docDanhSachPhim(danhSach);
    if (danhSach.empty()) {
        cout << "Danh sach phim hien dang trong!\n";
        return;
    }
    
    // M? r?ng khung nhìn b?ng d? ch?a thêm c?t Th? lo?i
    cout << "\n=================================================================================================================\n";
    cout << left << setw(10) << "Ma Phim"  
         << setw(25) << "Ten Phim"   
         << setw(23) << "The Loai"   
         << setw(12) << "Thoi luong"  
         << setw(10) << "Do tuoi"  
         << setw(15) << "Khoi chieu"  
         << setw(15) << "Ket thuc" << endl;
    cout << "-----------------------------------------------------------------------------------------------------------------\n";
    
    for (size_t i = 0; i < danhSach.size(); ++i) {
        danhSach[i].hienThi(db); // Truy?n db vào hàm hienThi d? l?y tên th? lo?i
    }
    cout << "=================================================================================================================\n";
}

void Movie::themPhim(Database& db) {
    string ma, ten, ngayKC, ngayKT, moTa, poster, tenTheLoai;
    int thoiLuong, nhanDoTuoi;

    cout << "\n--- THEM PHIM MOI ---\n";
    cout << "Nhap ma phim (VD: P01): ";
    cin >> ma;

    vector<Movie> danhSach;
    db.docDanhSachPhim(danhSach);
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            cout << "Loi: Ma phim nay da ton tai!\n";
            return;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten phim: ";
    getline(cin, ten);

    cout << "Nhap the loai phim (VD: Hanh Dong, Kinh Di, Hoat Hinh...): ";
    getline(cin, tenTheLoai);

    cout << "Nhap thoi luong (phut): ";
    cin >> thoiLuong;
    cout << "Nhap nhan do tuoi (13, 16, 18...): ";
    cin >> nhanDoTuoi;
    
    int todayKey;
    if (!DateFormat::todayDateKey(todayKey)) {
        cout << "Loi: Khong the xac dinh ngay hien tai!\n";
        return;
    }

    while (true) {
        cout << "Nhap ngay khoi chieu (DD/MM/YYYY, VD: 06/05/2026): ";
        cin >> ngayKC;
        if (!DateFormat::isValidDate(ngayKC)) {
            cout << "Loi: Ngay khoi chieu khong hop le!\n";
            continue;
        }
        ngayKC = DateFormat::normalizeDate(ngayKC);
        if (DateFormat::dateKey(ngayKC) < todayKey) {
            cout << "Loi: Ngay khoi chieu khong duoc nam trong qua khu!\n";
            continue;
        }
        break;
    }

    while (true) {
        cout << "Nhap ngay ket thuc (DD/MM/YYYY, VD: 06/06/2026): ";
        cin >> ngayKT;
        if (!DateFormat::isValidDate(ngayKT)) {
            cout << "Loi: Ngay ket thuc khong hop le!\n";
            continue;
        }
        ngayKT = DateFormat::normalizeDate(ngayKT);
        if (DateFormat::dateKey(ngayKT) < todayKey) {
            cout << "Loi: Ngay ket thuc khong duoc nam trong qua khu!\n";
        } else if (DateFormat::dateKey(ngayKT) < DateFormat::dateKey(ngayKC)) {
            cout << "Loi: Ngay ket thuc khong the truoc ngay khoi chieu!\n";
        } else {
            break;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap mo ta phim: ";
    getline(cin, moTa);
    cout << "Nhap duong dan poster/trailer: ";
    getline(cin, poster);

    // 1. Thêm phim vào danh sách và ghi xu?ng database [MOVIES]
    Movie moi(ma, ten, thoiLuong, nhanDoTuoi, ngayKC, ngayKT, moTa, poster);
    danhSach.push_back(moi);
    db.ghiDanhSachPhim(danhSach);

    // 2. T? d?ng g?n th? lo?i cho phim v?a t?o thông qua Database
    if (!tenTheLoai.empty()) {
        db.themTheLoaiChoPhim(ma, tenTheLoai);
    }

    cout << "Them phim moi va gan the loai thanh cong va da luu vao he thong!\n";
}

void Movie::suaPhim(Database& db) {
    cout << "\n--- SUA THONG TIN PHIM ---\n";
    
    hienThiDanhSach(db);

    string ma;
    cout << "\nNhap ma phim can sua (nhap 0 de quay lai): ";
    cin >> ma;
    if (ma == "0") return;

    vector<Movie> danhSach;
    db.docDanhSachPhim(danhSach);
    bool found = false;

    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            found = true;
            int choice;
            do {
                cout << "\n==================================================\n";
                cout << "         THONG TIN CHI TIET PHIM DANG SUA         \n";
                cout << "==================================================\n";
                cout << "Ma phim     : " << danhSach[i].getMaPhim() << "\n";
                cout << "Ten phim    : " << danhSach[i].getTenPhim() << "\n";
                cout << "The loai    : " << db.layTheLoaiCuaPhim(danhSach[i].getMaPhim()) << "\n";
                cout << "Thoi luong  : " << danhSach[i].getThoiLuong() << " phut\n";
                cout << "Do tuoi     : " << danhSach[i].getNhanDoTuoi() << "+\n";
                cout << "Khoi chieu  : " << danhSach[i].getNgayKhoiChieu() << "\n";
                cout << "Ket thuc    : " << danhSach[i].getNgayKetThuc() << "\n";
                cout << "Mo ta       : " << danhSach[i].getMoTa() << "\n";
                cout << "Poster      : " << danhSach[i].getPoster() << "\n";
                cout << "==================================================\n";
                
                cout << "1. Sua ten phim\n";
                cout << "2. Sua the loai phim\n";
                cout << "3. Sua thoi luong\n";
                cout << "4. Sua nhan do tuoi\n";
                cout << "5. Sua ngay khoi chieu\n";
                cout << "6. Sua ngay ket thuc\n";
                cout << "7. Sua mo ta\n";
                cout << "8. Sua poster\n";
                cout << "0. Hoan tat va luu thay doi\n";
                cout << "Chon thuoc tinh can sua (0-8): ";
                cin >> choice;

                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                switch (choice) {
                    case 1: {
                        string tenMoi;
                        cout << "Nhap ten phim moi (" << danhSach[i].getTenPhim() << "): ";
                        getline(cin, tenMoi);
                        if (!tenMoi.empty()) danhSach[i].setTenPhim(tenMoi);
                        cout << "-> Da cap nhat ten phim!\n";
                        break;
                    }
                    case 2: {
                        int subChoice;
                        do {
                            vector<pair<string, string> > dsTheLoai;
                            db.docDanhSachTheLoai(dsTheLoai);

                            cout << "\n--------------------------------------------------\n";
                            cout << "QUAN LY THE LOAI CHO PHIM: " << danhSach[i].getTenPhim() << "\n";
                            cout << "The loai hien tai: " << db.layTheLoaiCuaPhim(danhSach[i].getMaPhim()) << "\n";
                            cout << "--------------------------------------------------\n";
                            cout << "Danh sach tat ca the loai trong he thong:\n";
                            for (size_t k = 0; k < dsTheLoai.size(); ++k) {
                                cout << "  - " << dsTheLoai[k].second << " (Ma: " << dsTheLoai[k].first << ")\n";
                            }
                            cout << "--------------------------------------------------\n";
                            cout << "1. Them the loai moi (Giung nguyen the loai cu, gan them)\n";
                            cout << "2. Thay the toan bo (Xoa the loai cu, gan the loai moi)\n";
                            cout << "0. Quay lai menu truoc\n";
                            cout << "Chon thao tac (0-2): ";
                            cin >> subChoice;

                            cin.ignore(numeric_limits<streamsize>::max(), '\n');

                            if (subChoice == 1) {
                                string theLoaiThem;
                                cout << "Nhap ten the loai muon them: ";
                                getline(cin, theLoaiThem);
                                if (!theLoaiThem.empty()) {
                                    db.themTheLoaiChoPhim(danhSach[i].getMaPhim(), theLoaiThem);
                                    cout << "-> Da them the loai thanh cong!\n";
                                }
                            } 
                            else if (subChoice == 2) {
                                string theLoaiMoi;
                                cout << "Nhap ten the loai thay the moi: ";
                                getline(cin, theLoaiMoi);
                                if (!theLoaiMoi.empty()) {
                                    db.capNhatTheLoaiChoPhim(danhSach[i].getMaPhim(), theLoaiMoi);
                                    cout << "-> Da thay the toan bo the loai thanh cong!\n";
                                }
                            } 
                            else if (subChoice != 0) {
                                cout << "Lua chon khong hop le!\n";
                            }
                        } while (subChoice != 0);
                        
                        break;
                    }
                    case 3: {
                        int thoiLuongMoi;
                        cout << "Nhap thoi luong moi (phut) (" << danhSach[i].getThoiLuong() << "): ";
                        cin >> thoiLuongMoi;
                        danhSach[i].setThoiLuong(thoiLuongMoi);
                        cout << "-> Da cap nhat thoi luong!\n";
                        break;
                    }
                    case 4: {
                        int doTuoiMoi;
                        cout << "Nhap nhan do tuoi moi (" << danhSach[i].getNhanDoTuoi() << "): ";
                        cin >> doTuoiMoi;
                        danhSach[i].setNhanDoTuoi(doTuoiMoi);
                        cout << "-> Da cap nhat nhan do tuoi!\n";
                        break;
                    }
                    case 5: {
                        string ngayKCMoi;
                        cout << "Nhap ngay khoi chieu moi (DD/MM/YYYY) (" << danhSach[i].getNgayKhoiChieu() << "): ";
                        getline(cin, ngayKCMoi);
                        if (ngayKCMoi.empty()) {
                            cout << "-> Giu nguyen ngay khoi chieu.\n";
                        } else {
                            int todayKey;
                            const string ngayChuanHoa = DateFormat::normalizeDate(ngayKCMoi);
                            if (!DateFormat::isValidDate(ngayKCMoi) ||
                                !DateFormat::todayDateKey(todayKey) ||
                                DateFormat::dateKey(ngayChuanHoa) < todayKey ||
                                DateFormat::dateKey(ngayChuanHoa) > DateFormat::dateKey(danhSach[i].getNgayKetThuc())) {
                                cout << "Loi: Ngay khoi chieu phai hop le, tu hom nay tro di va khong sau ngay ket thuc!\n";
                            } else {
                                danhSach[i].setNgayKhoiChieu(ngayChuanHoa);
                                cout << "-> Da cap nhat ngay khoi chieu!\n";
                            }
                        }
                        break;
                    }
                    case 6: {
                        string ngayKTMoi;
                        cout << "Nhap ngay ket thuc moi (DD/MM/YYYY) (" << danhSach[i].getNgayKetThuc() << "): ";
                        getline(cin, ngayKTMoi);
                        if (ngayKTMoi.empty()) {
                            cout << "-> Giu nguyen ngay ket thuc.\n";
                        } else {
                            int todayKey;
                            const string ngayChuanHoa = DateFormat::normalizeDate(ngayKTMoi);
                            if (!DateFormat::isValidDate(ngayKTMoi) ||
                                !DateFormat::todayDateKey(todayKey) ||
                                DateFormat::dateKey(ngayChuanHoa) < todayKey ||
                                DateFormat::dateKey(ngayChuanHoa) < DateFormat::dateKey(danhSach[i].getNgayKhoiChieu())) {
                                cout << "Loi: Ngay ket thuc phai hop le, tu hom nay tro di va khong truoc ngay khoi chieu!\n";
                            } else {
                                danhSach[i].setNgayKetThuc(ngayChuanHoa);
                                cout << "-> Da cap nhat ngay ket thuc!\n";
                            }
                        }
                        break;
                    }
                    case 7: {
                        string moTaMoi;
                        cout << "Nhap mo ta moi: ";
                        getline(cin, moTaMoi);
                        if (!moTaMoi.empty()) danhSach[i].setMoTa(moTaMoi);
                        cout << "-> Da cap nhat mo ta!\n";
                        break;
                    }
                    case 8: {
                        string posterMoi;
                        cout << "Nhap poster moi: ";
                        getline(cin, posterMoi);
                        if (!posterMoi.empty()) danhSach[i].setPoster(posterMoi);
                        cout << "-> Da cap nhat poster!\n";
                        break;
                    }
                    case 0:
                        db.ghiDanhSachPhim(danhSach);
                        cout << "\nLuu thay doi thong tin phim thanh cong!\n";
                        break;
                    default:
                        cout << "Lua chon khong hop le! Vui long chon lai.\n";
                }
            } while (choice != 0);

            break;
        }
    }

    if (!found) {
        cout << "Khong tim thay phim co ma: " << ma << endl;
    }
}

bool Movie::kiemTraPhimDangChieu(const string& maPhim, Database& db) {
    vector<Showtime> dsShowtime;
    db.docDanhSachSuatChieu(dsShowtime);
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhim() == maPhim) {
            return true;
        }
    }
    return false;
}

void Movie::xoaPhim(Database& db) {
    string ma;
    cout << "\n--- XOA PHIM ---\n";
    cout << "Nhap ma phim can xoa: ";
    cin >> ma;

    if (kiemTraPhimDangChieu(ma, db)) {
        cout << "Loi: Khong the xoa phim nay vi dang co suat chieu hoac dang trong dot chieu hoat dong!\n";
        return;
    }

    vector<Movie> danhSach;
    db.docDanhSachPhim(danhSach);
    
    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaPhim() == ma) {
            danhSach.erase(danhSach.begin() + i);
            db.ghiDanhSachPhim(danhSach);
            cout << "Xoa phim thanh cong!\n";
            return;
        }
    }
    cout << "Khong tim thay phim co ma: " << ma << endl;
}
