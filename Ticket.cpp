#include "Ticket.h"
#include "Database.h"
#include "Showtime.h"
#include "Seat.h"
#include "Movie.h"
#include "Customer.h"
#include "Invoice.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cstdlib>

// --- IMPLEMENTATION OF TICKET CLASS ---

Ticket::Ticket() {
    this->giaVeThucTe = 0.0;
    this->trangThaiSuDung = "ChuaCheckIn";
}

Ticket::Ticket(string maVe, string maHD, string maSC, string maGhe, double gia, string qr, string trangThai) {
    this->maVe = maVe;
    this->maHoaDon = maHD;
    this->maSuatChieu = maSC;
    this->maGhe = maGhe;
    this->giaVeThucTe = gia;
    this->maQRCode = qr;
    this->trangThaiSuDung = trangThai;
}

string Ticket::getMaVe() const { return maVe; }
string Ticket::getMaHoaDon() const { return maHoaDon; }
string Ticket::getMaSuatChieu() const { return maSuatChieu; }
string Ticket::getMaGhe() const { return maGhe; }
double Ticket::getGiaVeThucTe() const { return giaVeThucTe; }
string Ticket::getMaQRCode() const { return maQRCode; }
string Ticket::getTrangThaiSuDung() const { return trangThaiSuDung; }

void Ticket::setTrangThaiSuDung(const string& trangThai) {
    this->trangThaiSuDung = trangThai;
}

string Ticket::toCSV() const {
    stringstream ssGia;
    ssGia << fixed << setprecision(0) << giaVeThucTe;
    return maVe + "," + maHoaDon + "," + maSuatChieu + "," + maGhe + "," + ssGia.str() + "," + maQRCode + "," + trangThaiSuDung;
}

Ticket Ticket::fromCSV(const string& line) {
    stringstream ss(line);
    string maVe, maHD, maSC, maGhe, strGia, maQR, trangThai;

    getline(ss, maVe, ',');
    getline(ss, maHD, ',');
    getline(ss, maSC, ',');
    getline(ss, maGhe, ',');
    getline(ss, strGia, ',');
    getline(ss, maQR, ',');
    getline(ss, trangThai, ',');

    double gia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> gia;
    }

    return Ticket(maVe, maHD, maSC, maGhe, gia, maQR, trangThai);
}

void Ticket::hienThi() const {
    // 2. DÙNG STRINGSTREAM THAY CHO TO_STRING Ð? AN TOÀN TRÊN M?I CHU?N C++
    stringstream ssGia;
    ssGia << (int)giaVeThucTe << "d";

    cout << left << setw(10) << maVe 
         << setw(15) << maHoaDon 
         << setw(12) << maSuatChieu 
         << setw(8)  << maGhe 
         << setw(12) << ssGia.str() 
         << setw(15) << maQRCode 
         << setw(15) << trangThaiSuDung << endl;
}

// --- IMPLEMENTATION OF TICKET MANAGER (QUY TRÌNH BÁN VÉ 6 BU?C) ---

Customer xuLyThongTinKhachHangCRM(Database& db, double tongTienHoaDon) {
    cout << "\n==================================================================-\n";
    cout << "                    XU LY THONG TIN KHACH HANG (CRM)                 \n";
    cout << "===================================================================\n";
    
    string sdt;
    cout << "Nhap so dien thoai khach hang: ";
    cin >> sdt;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // 1. Ð?c danh sách khách hàng t? Database ([USERS] và [CUSTOMERS])
    vector<Customer> danhSachKH;
    db.docDanhSachUsers(danhSachKH);

    Customer* khachHangTimThay = NULL;
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        if (danhSachKH[i].getSoDienThoai() == sdt) {
            khachHangTimThay = &danhSachKH[i];
            break;
        }
    }

    // 2. N?u tìm th?y thành viên cu
    if (khachHangTimThay != NULL) {
        cout << "-> He thong tim thay thanh vien: " << khachHangTimThay->getHoTen() 
             << " (Hang " << khachHangTimThay->getHangThanhVien() 
             << " - Diem tich luy: " << khachHangTimThay->getDiemTichLuy() << ")\n";
        
        // Ti?n hành tích di?m t? hóa don hi?n t?i
        int soDiemThuong = static_cast<int>(tongTienHoaDon / 10000); // Quy u?c: 10.000d = 1 di?m
        string thoiGian = "6/5/2026 18:30"; // Ho?c l?y th?i gian h? th?ng th?c t?
        string noiDung = "Tich diem tu hoa don ban ve";

        khachHangTimThay->tichDiem(soDiemThuong, noiDung, thoiGian);
        khachHangTimThay->themChiTieu(static_cast<long long>(tongTienHoaDon));

        // Ghi l?i l?ch s? giao d?ch di?m vào b?ng [CUSTOMER_POINTS_HISTORY]
        vector<GiaoDichDiem> danhSachGD;
        db.docLichSuGiaoDichDiem(danhSachGD);

        stringstream ss;
        ss << "GD0" << (danhSachGD.size() + 1);
        string maGD = ss.str();

        GiaoDichDiem gdMoi(maGD, khachHangTimThay->getMaKH(), "TichDiem", soDiemThuong, noiDung, thoiGian);
        danhSachGD.push_back(gdMoi);

        // Luu toàn b? thay d?i xu?ng Database
        db.ghiDanhSachUsers(danhSachKH);
        db.ghiLichSuGiaoDichDiem(danhSachGD);

        cout << "-> Da tich them " << soDiemThuong << " diem cho khach hang!\n";
        return *khachHangTimThay;
    } 
    // 3. N?u chua có -> Yêu c?u nhân viên nh?p thông tin d? dang ký m?i
    else {
        cout << "-> Khong tim thay khach hang voi SÐT: " << sdt << ". Vui long dang ky thong tin moi!\n";
        
        string hoTen, email, ngaySinh, gioiTinh;
        cout << "Nhap ho ten khach hang: ";
        getline(cin, hoTen);
        cout << "Nhap email: ";
        getline(cin, email);
        cout << "Nhap ngay sinh (M/D/YYYY): ";
        getline(cin, ngaySinh);
        cout << "Nhap gioi tinh (Nam/Nu): ";
        getline(cin, gioiTinh);

        // T? d?ng sinh mã khách hàng m?i tang d?n
        stringstream ssMa;
        ssMa << "U0" << (danhSachKH.size() + 1);
        string maKHMoi = ssMa.str();

        // Kh?i t?o d?i tu?ng khách hàng m?i (di?m = 0, chi tiêu = 0)
        Customer khachMoi(maKHMoi, hoTen, sdt, email, 0, "Thuong", 0);
        
        // Tích di?m cho hóa don d?u tiên luôn
        int soDiemThuong = static_cast<int>(tongTienHoaDon / 10000);
        string thoiGian = "6/5/2026 18:30";
        string noiDung = "Tich diem hoa don dau tien";
        
        khachMoi.tichDiem(soDiemThuong, noiDung, thoiGian);
        khachMoi.themChiTieu(static_cast<long long>(tongTienHoaDon));

        danhSachKH.push_back(khachMoi);

        // Ghi nh?n l?ch s? giao d?ch di?m
        vector<GiaoDichDiem> danhSachGD;
        db.docLichSuGiaoDichDiem(danhSachGD);
        
        stringstream ssGD;
        ssGD << "GD0" << (danhSachGD.size() + 1);
        string maGD = ssGD.str();

        GiaoDichDiem gdMoi(maGD, maKHMoi, "TichDiem", soDiemThuong, noiDung, thoiGian);
	danhSachGD.push_back(gdMoi);

        // Luu xu?ng file h? th?ng (`[USERS]`, `[CUSTOMERS]`, `[CUSTOMER_POINTS_HISTORY]`)
        db.ghiDanhSachUsers(danhSachKH);
        db.ghiLichSuGiaoDichDiem(danhSachGD);

        cout << "-> Dang ky thanh vien moi va tich " << soDiemThuong << " diem thanh cong! Ma KH: " << maKHMoi << "\n";
        return khachMoi;
    }
}

void TicketManager::quyTrinhBanVe(Database& db, const string& maNV) {
    // BU?C 1.1: Ð?C VÀ HI?N TH? DANH SÁCH PHIM ÐANG CHI?U
    vector<Showtime> dsShowtime;
    db.docDanhSachSuatChieu(dsShowtime);
    if (dsShowtime.empty()) {
        cout << "\n[LOI]: Hien tai khong co suat chieu nao!\n";
        return;
    }

    // L?c ra danh sách các mã phim duy nh?t t? các su?t chi?u hi?n có
    vector<string> dsMaPhim;
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        bool exist = false;
        for (size_t j = 0; j < dsMaPhim.size(); ++j) {
            if (dsMaPhim[j] == dsShowtime[i].getMaPhim()) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            dsMaPhim.push_back(dsShowtime[i].getMaPhim());
        }
    }

    Movie movie;
    movie.hienThiDanhSach(db);
    int chonPhimSTT;
    cout << "Nhap STT phim can xem (Nhap 0 de huy): ";
    cin >> chonPhimSTT;
    if (chonPhimSTT <= 0 || chonPhimSTT > (int)dsMaPhim.size()) {
        return;
    }
    string selectedMovie = dsMaPhim[chonPhimSTT - 1];

    // BU?C 1.2: L?C VÀ HI?N TH? CÁC SU?T CHIEU THU?C V? PHIM ÐÃ CH?N
    vector<Showtime> dsSuatChieuCuaPhim;
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhim() == selectedMovie) {
            dsSuatChieuCuaPhim.push_back(dsShowtime[i]);
        }
    }

    cout << "\n===================================================================\n";
    cout << "            SUAT CHIEU CHO PHIM: " << selectedMovie << "\n";
    cout << "===================================================================\n";
    cout << left << setw(6) << "STT" 
         << setw(10) << "Ma SC" 
         << setw(15) << "Ngay chieu" 
         << setw(13) << "Gio chieu" 
         << setw(10) << "Ma Phong" << endl;
    cout << "-------------------------------------------------------------------\n";

    for (size_t i = 0; i < dsSuatChieuCuaPhim.size(); ++i) {
        cout << left << setw(6) << (i + 1)
             << setw(10) << dsSuatChieuCuaPhim[i].getMaSuatChieu()
             << setw(15) << dsSuatChieuCuaPhim[i].getNgayChieu()
             << setw(13) << dsSuatChieuCuaPhim[i].getGioBatDau()
             << setw(10) << dsSuatChieuCuaPhim[i].getMaPhong() << endl;
    }
    cout << "-------------------------------------------------------------------\n";

    int chonSTT;
    cout << "Nhap STT suat chieu (Nhap 0 de huy): ";
    cin >> chonSTT;
    if (chonSTT <= 0 || chonSTT > (int)dsSuatChieuCuaPhim.size()) {
        return;
    }

    Showtime selectedShowtime = dsSuatChieuCuaPhim[chonSTT - 1];
    string selectedSC = selectedShowtime.getMaSuatChieu();
    string maPhongChieu = selectedShowtime.getMaPhong();

    // Ð?C DANH SÁCH GH? V?T LÝ T? DATABASE
    vector<Seat> dsGhePhong;
    SeatManager seatMgr;
    seatMgr.taiDuLieu(db); 
    db.docDanhSachGhe(dsGhePhong);

    vector<string> dsGheChon;
    double tongTienGhe = 0.0;
    char tiepTucChon = 'y';

    while (tiepTucChon == 'y' || tiepTucChon == 'Y') {
        seatMgr.hienThiSoDoPhong(maPhongChieu, selectedSC, db);
        
        string ghe;
        cout << "Nhap ma ghe muon chon (VD: A1, B1, VIP1...): ";
        cin >> ghe;

        bool hopLe = false;
        double giaGheHienTai = 70000.0;
        
        vector<pair<string, string> > trangThaiGheList;
        db.docTrangThaiGheSuatChieu(selectedSC, trangThaiGheList);

        for (size_t k = 0; k < dsGhePhong.size(); ++k) {
            if (dsGhePhong[k].getMaPhong() == maPhongChieu && dsGhePhong[k].getMaGhe() == ghe) {
                string trangThaiGhe = "Trong";
                for (size_t i = 0; i < trangThaiGheList.size(); ++i) {
                    if (trangThaiGheList[i].first == ghe) {
                        trangThaiGhe = trangThaiGheList[i].second;
                        break;
                    }
                }

                if (trangThaiGhe == "Trong") {
                    hopLe = true;
                    giaGheHienTai = dsGhePhong[k].getDonGiaGhe();
                }
                break;
            }
        }

        for (size_t k = 0; k < dsGheChon.size(); ++k) {
            if (dsGheChon[k] == ghe) {
                hopLe = false;
                break;
            }
        }

        if (!hopLe) {
            cout << "[CANH BAO]: Ghe [" << ghe << "] khong ton tai, da co nguoi dat hoac khong kha dung!\n";
            continue;
        }

        dsGheChon.push_back(ghe);
        tongTienGhe += giaGheHienTai;

        cout << "Da chon ghe [" << ghe << "] thanh cong. Tam tinh tien ghe: " << (int)tongTienGhe << " VND\n";
        cout << "Ban co muon chon them ghe nua khong? (y/n): ";
        cin >> tiepTucChon;
    }

    if (dsGheChon.empty()) {
        cout << "[THONG BAO]: Ban chua chon ghe nao. Huy giao dich ban ve!\n";
        return;
    }

    // BU?C 3: THÊM F&B (TU? CH?N) & CHU?N B? DANH SÁCH CHI TI?T
    cout << "\n===================================================================\n";
    cout << "                    CHON DO AN / THUC UONG                         \n";
    cout << "===================================================================\n";
    cout << "1. Combo Bap + 2 Nuoc ngot (60,000d)\n";
    cout << "2. Bap rang bo lon (40,000d)\n";
    cout << "3. Nuoc suoi (15,000d)\n";
    cout << "0. Khong dung them F&B\n";
    int chonFB, slFB;
    cout << "Chon ma san pham (0-3): ";
    cin >> chonFB;
    
    vector<InvoiceFBItem> dsFBItems;
    double tienFB = 0.0;
    string tenFB = "Khong";
    
    if (chonFB > 0) {
        cout << "Nhap so luong: ";
        cin >> slFB;
        
        string maMH = "";
        string tenMH = "";
        double giaTienMH = 0.0;

        if (chonFB == 1) { 
            maMH = "FB01"; tenMH = "Combo Bap + 2 Nuoc ngot"; giaTienMH = 60000.0; 
        } else if (chonFB == 2) { 
            maMH = "FB02"; tenMH = "Bap rang bo lon"; giaTienMH = 40000.0; 
        } else if (chonFB == 3) { 
            maMH = "FB03"; tenMH = "Nuoc suoi"; giaTienMH = 15000.0; 
        }

        if (!maMH.empty()) {
            tienFB = giaTienMH * slFB;
            stringstream ss;
			ss << slFB;
			tenFB = tenMH + " x" + ss.str();
            // Ðua vào danh sách s?n ph?m F&B c?a hóa don
            InvoiceFBItem item;
item.maHoaDon = "";
item.maMatHang = maMH;
item.soLuong = slFB;
item.thanhTien = tienFB;

dsFBItems.push_back(item);
        }
    }

    // T?o gi? l?p ti?n hàng t?ng c?ng g?m Ti?n Gh? + Ti?n F&B (ho?c b?n có th? quy d?i ti?n gh? thành item F&B tùy c?u trúc InvoiceFBItem c?a h? th?ng)
    double tongTienHang = tongTienGhe + tienFB;

    // BU?C 4: G?I HÀM X? LÝ THÔNG TIN KHÁCH HÀNG (CRM)
    Customer khachHangThanhToan = xuLyThongTinKhachHangCRM(db, tongTienHang);
    string maKH = khachHangThanhToan.getMaKH();
    string tenKH = khachHangThanhToan.getHoTen();
    string sdt = khachHangThanhToan.getSoDienThoai();

    // BU?C 5 & 6: G?I TR?C TI?P HÀM T?O HÓA ÐON CHU?N T? INVOICEMANAGER Ð? X? LÝ VOUCHER & THANH TOÁN
    InvoiceManager invoiceMgr;
    invoiceMgr.taiDuLieu(db);
    
    // G?i hàm t?o hóa don chu?n dã tích h?p s?n ch?n phuong th?c thanh toán, mã gi?m giá theo ph?m vi phim và ghi file giao_dich_fnb.csv
    invoiceMgr.taoHoaDonMoi(db, maNV, maKH, tongTienHang, dsFBItems, selectedMovie);

    // L?y mã hóa don v?a phát sinh (mã hóa don m?i n?m ? cu?i danh sách hóa don)
    vector<Invoice> danhSachHD;
    db.docDanhSachHoaDon(danhSachHD);
    string maHD = danhSachHD.empty() ? "HD_UNKNOWN" : danhSachHD.back().getMaHoaDon();
    double tongThanhToan = danhSachHD.empty() ? tongTienHang : danhSachHD.back().getTongTien();

    // C?P NH?T TR?NG THÁI GH? THEO SU?T CHIEU VÀO B?NG [SEAT_STATUS] TRONG lich_va_ve.csv
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        db.capNhatTrangThaiGheSuatChieu(selectedSC, dsGheChon[i], "DaDat");
    }
    cout << "[THONG BAO]: Da luu trang thai dat ghe cho suat chieu " << selectedSC << " thanh cong!\n";

    // T?O VÀ LUU VÉ VÀO DATABASE SAU KHI THANH TOÁN THÀNH CÔNG
    vector<Ticket> dsVeHienTai;
    db.docDanhSachVe(dsVeHienTai);

    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        stringstream ssMaVe, ssQR;
        ssMaVe << "V_" << selectedSC << "_" << dsGheChon[i];
        ssQR << "QR_" << maHD << "_" << dsGheChon[i];
        
        double giaVeGhe = 70000.0;
        if (dsGheChon[i][0] == 'B' || dsGheChon[i][0] == 'b' || dsGheChon[i][0] == 'C' || dsGheChon[i][0] == 'c') {
            giaVeGhe += 20000.0;
        }

        Ticket veMoi(ssMaVe.str(), maHD, selectedSC, dsGheChon[i], giaVeGhe, ssQR.str(), "ChuaCheckIn");
        dsVeHienTai.push_back(veMoi);
    }
    db.ghiDanhSachVe(dsVeHienTai);

    // IN THÔNG TIN VÉ XEM PHIM HOÀN T?T
    cout << "\n===================================================================\n";
    cout << "                    XUAT THONG TIN VE XEM PHIM                     \n";
    cout << "===================================================================\n";
    cout << "Suat chieu: Phim [" << selectedMovie << "] luc " << selectedShowtime.getGioBatDau()
         << " ngay " << selectedShowtime.getNgayChieu() << " tai " << maPhongChieu << "\n\n";
    cout << "--- CHI TIET GHE DA DAT ---\n";
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        cout << (i + 1) << ". Ghe: " << dsGheChon[i] << " | Trang thai: DaDat\n";
    }
    if (chonFB > 0) {
        cout << "F&B: " << tenFB << " = " << (int)tienFB << "d\n";
    }
    cout << "\nDiem tich luy khach hang moi nhan: +" << (int)(tongTienHang / 10000) << " diem.\n";
    cout << "===================================================================\n";
    cout << "In ve thanh cong! Trang thai ghe, hoa don va thong tin CRM da duoc cap nhat.\n";
}

void TicketManager::hienThiDanhSachVe(Database& db) {
    vector<Ticket> danhSach;
    db.docDanhSachVe(danhSach);
    if (danhSach.empty()) {
        cout << "Chua co ve nao duoc ban ra trong he thong!\n";
        return;
    }

    cout << "\n============================== DANH SACH VE XEM PHIM THUC TE ==============================\n";
    cout << left << setw(10) << "Ma Ve" 
         << setw(15) << "Ma Hoa Don" 
         << setw(12) << "Ma Suat" 
         << setw(8)  << "Ghe" 
         << setw(12) << "Gia Ve" 
         << setw(15) << "Ma QR Code" 
         << setw(15) << "Trang Thai" << endl;
    cout << "-------------------------------------------------------------------------------------------\n";
    for (size_t i = 0; i < danhSach.size(); ++i) {
        danhSach[i].hienThi();
    }
    cout << "===========================================================================================\n";
}

void TicketManager::checkInVe(Database& db) {
    string maCheckIn;
    cout << "\n--- SOAT VE / CHECK-IN TAI RAP ---\n";
    cout << "Nhap Ma ve hoac quet Ma QR Code cua khach: ";
    cin >> maCheckIn;

    vector<Ticket> danhSach;
    db.docDanhSachVe(danhSach); // Ð?c danh sách vé lên RAM
    bool found = false;

    for (size_t i = 0; i < danhSach.size(); ++i) {
        if (danhSach[i].getMaVe() == maCheckIn || danhSach[i].getMaQRCode() == maCheckIn) {
            found = true;
            if (danhSach[i].getTrangThaiSuDung() == "DaCheckIn") {
                cout << "[CANH BAO]: Ve nay DA DUOC SU DUNG truoc do! Tu choi vao phong chieu.\n";
            } else {
                danhSach[i].setTrangThaiSuDung("DaCheckIn");
                db.ghiDanhSachVe(danhSach);
                cout << "[THANH CONG]: Check-in hop le! Moi khach vao phong chieu ghe [" << danhSach[i].getMaGhe() << "].\n";
            }
            break;
        }
    }

    if (!found) {
        cout << "[LOI]: Khong tim thay ma ve hoac ma QR hop le trong he thong!\n";
    }
}
