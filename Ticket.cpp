#include "Ticket.h"
#include "Database.h"
#include "Showtime.h"
#include "Seat.h"
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

void TicketManager::quyTrinhBanVe(Database& db) {
    // BUOC 1: DOC DANH SACH SUAT CHIUE TU DATABASE
    vector<Showtime> dsShowtime;
    db.docDanhSachSuatChieu(dsShowtime);
    if (dsShowtime.empty()) {
        cout << "\n[LOI]: Hien tai khong co suat chieu nao!\n";
        return;
    }

    cout << "\n===================================================================\n";
    cout << "                    CHON SUAT CHIEU CHO HOA DON\n";
    cout << "===================================================================\n";
    cout << left << setw(6) << "STT" 
         << setw(10) << "Ma SC" 
         << setw(15) << "Ma Phim" 
         << setw(15) << "Ngay chieu" 
         << setw(13) << "Gio chieu" 
         << setw(10) << "Ma Phong" << endl;
    cout << "-------------------------------------------------------------------\n";
    
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        cout << left << setw(6) << (i + 1)
             << setw(10) << dsShowtime[i].getMaSuatChieu()
             << setw(15) << dsShowtime[i].getMaPhim()
             << setw(15) << dsShowtime[i].getNgayChieu()
             << setw(13) << dsShowtime[i].getGioBatDau()
             << setw(10) << dsShowtime[i].getMaPhong() << endl;
    }
    cout << "-------------------------------------------------------------------\n";
    
    int chonSTT;
    cout << "Nhap STT suat chieu can ban ve (Nhap 0 de huy): ";
    cin >> chonSTT;
    if (chonSTT <= 0 || chonSTT > (int)dsShowtime.size()) {
        return;
    }

    Showtime selectedShowtime = dsShowtime[chonSTT - 1];
    string selectedSC = selectedShowtime.getMaSuatChieu();
    string selectedMovie = selectedShowtime.getMaPhim();
    string maPhongChieu = selectedShowtime.getMaPhong();

    // BUOC 2: DOC DANH SACH GHE THUOC PHONG CHIEU TU DATABASE (Dung lop Seat)
    vector<Seat> dsGhePhong;
    SeatManager seatMgr;
    seatMgr.taiDuLieu(db); 
    // Hoac neu Database ho tro doc truc tiep:
    db.docDanhSachGhe(dsGhePhong); // Dieu chinh ten ham nay neu trong Database.h cua ban khac

    vector<string> dsGheChon;
    double tongTienGhe = 0.0;
    char tiepTucChon = 'y';

while (tiepTucChon == 'y' || tiepTucChon == 'Y') {
        cout << "\n===================================================================\n";
        cout << "            SO DO GHE PHONG CHIEU: " << maPhongChieu << " \n";
        cout << "===================================================================\n";
        cout << "                      Man hinh chieu phim (SCREEN)\n";
        cout << "-------------------------------------------------------------------\n";
        
        // Hi?n th? tr?c quan các gh? thu?c phòng chi?u này t? dsGhePhong
        for (size_t k = 0; k < dsGhePhong.size(); ++k) {
            if (dsGhePhong[k].getMaPhong() == maPhongChieu) {
                string maGheHienTai = dsGhePhong[k].getMaGhe();
                string trangThaiGhe = dsGhePhong[k].getTrangThai();
                
                // Ki?m tra xem gh? này có dang du?c ch?n t?m trong phiên giao d?ch hi?n t?i không
                for (size_t idx = 0; idx < dsGheChon.size(); ++idx) {
                    if (dsGheChon[idx] == maGheHienTai) {
                        trangThaiGhe = "DaDat"; // Ho?c hi?n th? là dang ch?n
                        break;
                    }
                }

                if (trangThaiGhe == "DaDat" || trangThaiGhe == "DangChon") {
                    cout << "[X] " << maGheHienTai << " ";
                } else {
                    cout << "[O] " << maGheHienTai << " ";
                }
            }
        }
        cout << "\n-------------------------------------------------------------------\n";
        
        string ghe;
        cout << "Nhap ma ghe muon chon (VD: A1, B1, VIP1...): ";
        cin >> ghe;

        // Ki?m tra xem gh? có t?n t?i và h?p l? không
        bool hopLe = false;
        double giaGheHienTai = 70000.0; // M?c d?nh
        
        for (size_t k = 0; k < dsGhePhong.size(); ++k) {
            if (dsGhePhong[k].getMaPhong() == maPhongChieu && dsGhePhong[k].getMaGhe() == ghe) {
                if (dsGhePhong[k].getTrangThai() == "Trong") {
                    hopLe = true;
                    giaGheHienTai = dsGhePhong[k].getDonGiaGhe(); // L?y tr?c ti?p don giá t? file d? li?u c?a gh?!
                }
                break;
            }
        }

        // Ki?m tra xem gh? dã b? ch?n trong danh sách t?m chua
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

        cout << "Da chon ghe [" << ghe << "] thanh công. Tam tinh tien ghe: " << (int)tongTienGhe << " VND\n";
        cout << "Ban co muon chon them ghe nua khong? (y/n): ";
        cin >> tiepTucChon;
    }

    if (dsGheChon.empty()) {
        cout << "[THONG BAO]: Ban chua chon ghe nao. Huy giao dich ban ve!\n";
        return;
    }

    // BUOC 3: THEM F&B (TUY CHON)
    cout << "\n==================================================================-\n";
    cout << "                        CHON DO AN / THUC UONG\n";
    cout << "===================================================================\n";
    cout << "1. Combo Bap + 2 Nuoc ngot (60,000d)\n";
    cout << "2. Bap rang bo lon (40,000d)\n";
    cout << "3. Nuoc suoi (15,000d)\n";
    cout << "0. Khong dung them F&B\n";
    int chonFB, slFB;
    cout << "Chon ma san pham (0-3): ";
    cin >> chonFB;
    double tienFB = 0.0;
    string tenFB = "Khong";
    if (chonFB > 0) {
        cout << "Nhap so luong: ";
        cin >> slFB;
        
        stringstream ssFB;
        if (chonFB == 1) { 
            tienFB = 60000.0 * slFB; 
            ssFB << "Combo Bap + 2 Nuoc ngot x" << slFB; 
        }
        else if (chonFB == 2) { 
            tienFB = 40000.0 * slFB; 
            ssFB << "Bap rang bo lon x" << slFB; 
        }
        else if (chonFB == 3) { 
            tienFB = 15000.0 * slFB; 
            ssFB << "Nuoc suoi x" << slFB; 
        }
        tenFB = ssFB.str();
    }

    // BUOC 4: XU LY THONG TIN KHACH HANG (CRM)
    cout << "\n==================================================================-\n";
    cout << "                    XU LY THONG TIN KHACH HANG (CRM)\n";
    cout << "===================================================================\n";
    string sdt;
    cout << "Nhap so dien thoai khach hang: ";
    cin >> sdt;
    string tenKH = "Nguyen Van A";
    cout << "-> He thong tim thay thanh vien: " << tenKH << " (Hang Bac - Diem tich luy: 120)\n";

    // BUOC 5: AP DUNG MA KHUYEN MAI (VOUCHER)
    cout << "\n==================================================================-\n";
    cout << "                        AP DUNG MA KHUYEN MAI\n";
    cout << "===================================================================\n";
    cin.ignore(numeric_limits<int>::max(), '\n');
    string voucher;
    cout << "Nhap ma Voucher (Bam Enter de bo qua): ";
    getline(cin, voucher);
    
    double tongTienTamTinh = tongTienGhe + tienFB;
    double giamGia = 0.0;
    if (!voucher.empty()) {
        if (voucher == "MAVIP2026") {
            giamGia = tongTienTamTinh * 0.1;
            cout << "-> [Thong bao]: Ma hop le! Giam 10% tong hoa don.\n";
        } else {
            cout << "-> [Canh bao]: Ma khong hop le! Bo qua giam gia.\n";
        }
    }
    double tongThanhToan = tongTienTamTinh - giamGia;

    // BUOC 6: THANH TOAN & CAP NHAT TRANG THAI GHE XUONG DATABASE
    cout << "\n==================================================================-\n";
    cout << "                         THANH TOAN HOA DON\n";
    cout << "===================================================================\n";
    cout << "Tong tien thanh toan: " << (int)tongThanhToan << " VND\n";
    cout << "Chon phuong thuc thanh toan:\n1. Tien mat\n2. The ngan hang\n3. Quet ma QR Code\nLua chon (1-3): ";
    int pttt;
    cin >> pttt;

    if (pttt == 3) {
        cout << "\n  +----------------------------------+\n";
        cout << "  |   [QR CODE MOPHON THANH TOAN]    |\n";
        cout << "  |   So tien: " << (int)tongThanhToan << " VND             |\n";
        cout << "  +----------------------------------+\n";
        cout << "Dang cho xac nhan thanh toan... [THANH TOAN THANH CONG]\n";
    } else {
        cout << "Thanh toan truc tiep thanh cong!\n";
    }

    // CAP NHAT TRANG THAI GHE THÀNH "DaDat" TRONG DANH SACH GHE VA GHI LAI VAO DATABASE
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        for (size_t k = 0; k < dsGhePhong.size(); ++k) {
            if (dsGhePhong[k].getMaPhong() == maPhongChieu && dsGhePhong[k].getMaGhe() == dsGheChon[i]) {
                dsGhePhong[k].setTrangThai("DaDat"); // Goi phuong th?c setter trong Seat.h
                break;
            }
        }
    }
    
    // Goi ham ghi danh sach ghe xuong Database (su dung ham ghi ghe co san cua ban)
    db.ghiDanhSachGhe(dsGhePhong);

    // Tao ma hoa don
    int randomNum = rand() % 900 + 100;
    stringstream ssHD;
    ssHD << "HD_" << selectedSC << "_" << randomNum;
    string maHD = ssHD.str();
// >>> B? SUNG: T?o và luu vé vào Database sau khi thanh toán thành công <<<
    vector<Ticket> dsVeHienTai;
    db.docDanhSachVe(dsVeHienTai);

    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        int rNum = rand() % 9000 + 1000;
        stringstream ssMaVe, ssQR;
        ssMaVe << "V_" << selectedSC << "_" << dsGheChon[i];
        ssQR << "QR_" << maHD << "_" << dsGheChon[i];
        
        // Tính giá vé th?c t? cho t?ng gh?
        double giaVeGhe = 70000.0;
        if (dsGheChon[i][0] == 'B' || dsGheChon[i][0] == 'b' || dsGheChon[i][0] == 'C' || dsGheChon[i][0] == 'c') {
            giaVeGhe += 20000.0;
        }

        Ticket veMoi(ssMaVe.str(), maHD, selectedSC, dsGheChon[i], giaVeGhe, ssQR.str(), "ChuaCheckIn");
        dsVeHienTai.push_back(veMoi);
    }
    db.ghiDanhSachVe(dsVeHienTai);
    // IN HOA DON HOAN TAT
    cout << "\n==================================================================-\n";
    cout << "                        XUAT HOA DON & VE XEM PHIM\n";
    cout << "===================================================================\n";
    cout << "Ma Hoa Don: " << maHD << "\n";
    cout << "Khach hang: " << tenKH << " (SDT: " << sdt << ")\n";
    cout << "Suat chieu: Phim [" << selectedMovie << "] luc " << selectedShowtime.getGioBatDau()
         << " ngay " << selectedShowtime.getNgayChieu() << " tai " << maPhongChieu << "\n\n";
    cout << "--- CHI TIET GHE DA DAT ---\n";
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        cout << (i + 1) << ". Ghe: " << dsGheChon[i] << " | Trang thai: DaDat\n";
    }
    if (chonFB > 0) {
        cout << "F&B: " << tenFB << " = " << (int)tienFB << "d\n";
    }
    if (giamGia > 0) {
        cout << "Giam gia Voucher: -" << (int)giamGia << "d\n";
    }
    cout << "\nTONG TIEN THUC THU: " << (int)tongThanhToan << " VND\n";
    cout << "Diem tich luy moi nhan: +" << (int)(tongThanhToan / 10000) << " diem.\n";
    cout << "===================================================================\n";
    cout << "In ve thanh cong! Trang thai ghe da duoc cap nhat vao Database.\n";
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
	db.docDanhSachVe(danhSach);
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
