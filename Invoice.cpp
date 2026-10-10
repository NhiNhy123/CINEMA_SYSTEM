#include "Invoice.h"
#include "Database.h"
#include "Ticket.h"
#include "FAndB.h"
#include "Promotion.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <ctime>

using namespace std;

// --- CLASS INVOICE ---

Invoice::Invoice() {
    this->tongTien = 0.0;
    this->trangThaiThanhToan = "ChuaTra";
}

Invoice::Invoice(string maHD, string maKH, string maNV, string thoiGian, double tongT, string hinhThuc, string maGD, string trangThai) {
    this->maHoaDon = maHD;
    this->maKhachHang = maKH;
    this->maNhanVien = maNV;
    this->thoiGianLap = thoiGian;
    this->tongTien = tongT;
    this->hinhThucThanhToan = hinhThuc;
    this->maGiaoDich = maGD;
    this->trangThaiThanhToan = trangThai;
}

// Getters
string Invoice::getMaHoaDon() const { return maHoaDon; }
string Invoice::getMaKhachHang() const { return maKhachHang; }
string Invoice::getMaNhanVien() const { return maNhanVien; }
string Invoice::getThoiGianLap() const { return thoiGianLap; }
double Invoice::getTongTien() const { return tongTien; }
string Invoice::getHinhThucThanhToan() const { return hinhThucThanhToan; }
string Invoice::getMaGiaoDich() const { return maGiaoDich; }
string Invoice::getTrangThaiThanhToan() const { return trangThaiThanhToan; }
const vector<InvoiceFBItem>& Invoice::getChiTietFB() const { return chiTietFB; }

void Invoice::themChiTietFB(const InvoiceFBItem& item) {
    chiTietFB.push_back(item);
}

void Invoice::setTrangThaiThanhToan(const string& trangThai) {
    trangThaiThanhToan = trangThai;
}

string Invoice::toCSV() const {
    stringstream ss;
    ss << maHoaDon << "," << maKhachHang << "," << maNhanVien << "," << thoiGianLap << "," 
       << fixed << setprecision(0) << tongTien << "," << hinhThucThanhToan << "," 
       << maGiaoDich << "," << trangThaiThanhToan;
    return ss.str();
}

Invoice Invoice::fromCSV(const string& line) {
    stringstream ss(line);
    string maHD, maKH, maNV, thoiGian, strTongTien, hinhThuc, maGD, trangThai;

    getline(ss, maHD, ',');
    getline(ss, maKH, ',');
    getline(ss, maNV, ',');
    getline(ss, thoiGian, ',');
    getline(ss, strTongTien, ',');
    getline(ss, hinhThuc, ',');
    getline(ss, maGD, ',');
    getline(ss, trangThai, ',');

    double tongT = 0.0;
    if (!strTongTien.empty()) {
        stringstream ssTien(strTongTien);
        ssTien >> tongT;
    }

    return Invoice(maHD, maKH, maNV, thoiGian, tongT, hinhThuc, maGD, trangThai);
}

void Invoice::hienThi() const {
    cout << left << setw(10) << maHoaDon 
         << setw(12) << (maKhachHang.empty() ? "KhachLe" : maKhachHang)
         << setw(10) << maNhanVien 
         << setw(16) << thoiGianLap 
         << setw(15) << fixed << setprecision(0) << tongTien 
         << setw(15) << hinhThucThanhToan 
         << setw(12) << trangThaiThanhToan << endl;
}


// --- CLASS INVOICE MANAGER ---

InvoiceManager::InvoiceManager() {}

void InvoiceManager::taiDuLieu(Database& db) {
    db.docDanhSachHoaDon(danhSachHoaDon);
}

// --- PHAN HE 2: NHAN VIEN - THANH TOAN & XUAT HOA DON (T?NG H?P T? TICKET, F&B, PROMOTION) ---
void InvoiceManager::taoHoaDonMoi(Database& db, const string& maNV, const string& maKH, double tongTienHang, 
                                 const vector<InvoiceFBItem>& dsFB, const string& phimChieuCode){
    // Có th? m? r?ng nh?n thêm mã voucher ho?c t? d?ng d?c t? input n?u c?n
    string maCodeVoucher;
    cout << "Nhap ma voucher khuyen mai (Bam Enter de bo qua): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, maCodeVoucher);

    // 1. T? d?ng ki?m tra và áp d?ng Promotion (Mã gi?m giá) n?u có
    double soTienGiamGia = 0.0;
    if (!maCodeVoucher.empty()) {
        vector<Promotion> dsKM;
        db.docDanhSachKhuyenMai(dsKM);
        for (size_t i = 0; i < dsKM.size(); ++i) {
            if (dsKM[i].getMaCode() == maCodeVoucher) {
                string phamVi = dsKM[i].getPhamViApDung();
                if (phamVi == "All" || phamVi == phimChieuCode) {
                    double giam = tongTienHang * (dsKM[i].getPhanTramGiam() / 100.0);
                    if (dsKM[i].getMucGiamToiDa() > 0 && giam > dsKM[i].getMucGiamToiDa()) {
                        soTienGiamGia = dsKM[i].getMucGiamToiDa();
                    } else {
                        soTienGiamGia = giam;
                    }
                    cout << "-> Ap dung voucher [" << dsKM[i].getTenKhuyenMai() << "] thanh cong! Giam: " << (int)soTienGiamGia << " VND\n";
                } else {
                    cout << "-> [Canh bao]: Ma khuyen mai khong ap dung cho phim nay!\n";
                }
                break;
            }
        }
    }

    double tongThanhToan = tongTienHang - soTienGiamGia;
    if (tongThanhToan < 0) tongThanhToan = 0;

    stringstream ssHD;
    ssHD << "HD_" << (danhSachHoaDon.size() + 1);
    string maHD = ssHD.str();
    
    // L?y th?i gian hi?n t?i làm th?i gian l?p hóa don
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    string thoiGianLap = string(buf);

    cout << "\n==================== XU LY THANH TOAN HOA DON ====================\n";
    cout << "Ma hoa don tam tao: " << maHD << "\n";
    cout << "Tong gia tri don hang (Da gom Ve + F&B): " << fixed << setprecision(0) << tongTienHang << " VND\n";
    if (soTienGiamGia > 0) {
        cout << "Giam gia tu Promotion: -" << soTienGiamGia << " VND\n";
    }
    cout << "TONG TIEN THUC THU: " << tongThanhToan << " VND\n";
    cout << "------------------------------------------------------------------\n";
    cout << "Chon phuong thuc thanh toan:\n";
    cout << "1. Tien mat (TienMat)\n";
    cout << "2. The ngan hang (TheNganHang)\n";
    cout << "3. Quet ma QR (QueQR)\n";
    cout << "Lua chon cua ban (1-3): ";
    
    int chonThanhToan;
    cin >> chonThanhToan;

    string hinhThuc = "TienMat";
    string maGD = "N/A";

    if (chonThanhToan == 2) {
        hinhThuc = "TheNganHang";
        maGD = "TXN_CARD_" + maHD;
        cout << "-> Dang quet the ngan hang... Giao dich thanh cong!\n";
    } else if (chonThanhToan == 3) {
        hinhThuc = "QueQR";
        maGD = "TXN_QR_" + maHD;
        cout << "\n[MA QR THANH TOAN MOH PHONG]\n";
        cout << "||||||||||||||||||||||||||||||||\n";
        cout << "|    QUET MA DE THANH TOAN NAY    |\n";
        cout << "||||||||||||||||||||||||||||||||\n";
        cout << "-> Khach da quet ma thanh toan thanh cong!\n";
    } else {
        hinhThuc = "TienMat";
        cout << "-> Thanh toan bang tien mat truc tiep tai quay.\n";
    }

    // Kh?i t?o hóa don m?i v?i t?ng ti?n th?c t? sau khi áp d?ng Promotion
    Invoice newInvoice(maHD, maKH, maNV, thoiGianLap, tongThanhToan, hinhThuc, maGD, "DaTra");

    // Thêm các s?n ph?m F&B vào chi ti?t hóa don
    for (size_t i = 0; i < dsFB.size(); ++i) {
        InvoiceFBItem itemNho = dsFB[i];
        itemNho.maHoaDon = maHD;
        newInvoice.themChiTietFB(itemNho);
    }

    danhSachHoaDon.push_back(newInvoice);

    // Ð?ng b? luu xu?ng file thông qua Database
    db.ghiDanhSachHoaDon(danhSachHoaDon);

    // Xu?t hóa don chi ti?t ra màn hình cho khách hàng
    cout << "\n==================================================================\n";
    cout << "                    CINEMA TICKET INVOICE                         \n";
    cout << "==================================================================\n";
    cout << "Ma HD: " << maHD << " | Thu ngan: " << maNV << "\n";
    cout << "Thoi gian: " << thoiGianLap << "\n";
    cout << "Khach hang: " << (maKH.empty() ? "Khach vang lai" : maKH) << "\n";
    cout << "------------------------------------------------------------------\n";
    cout << "Phuong thuc: " << hinhThuc << " | Ma GD: " << maGD << "\n";
    cout << "Trang thai: DA THANH TOAN (DaTra)\n";
    cout << "TONG THANH TOAN SAU GIAM GIA: " << fixed << setprecision(0) << tongThanhToan << " VND\n";
    cout << "==================================================================\n";
    cout << "          Cam on quy khach va hen gap lai lan sau!                \n";
    cout << "==================================================================\n";
}


// --- PHAN HE 1: QUAN TRI VIEN - QUAN LY DOANH THU & THONG KE ---
void InvoiceManager::hienThiLichSuGiaoDich() const {
    cout << "\n================================= LICH SU GIAO DICH HOA DON =================================\n";
    cout << left << setw(10) << "Ma HD" 
         << setw(12) << "Ma KH" 
         << setw(10) << "Thu Ngan" 
         << setw(16) << "Thoi Gian" 
         << setw(15) << "Tong Tien" 
         << setw(15) << "Hinh Thuc" 
         << setw(12) << "Trang Thai" << endl;
    cout << "---------------------------------------------------------------------------------------------\n";
    if (danhSachHoaDon.empty()) {
        cout << "Chua co giao dich hoa don nao trong he thong!\n";
    } else {
        for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
            danhSachHoaDon[i].hienThi();
        }
    }
    cout << "============================================================================================-\n";
}

void InvoiceManager::thongKeTongQuanDoanhThu() const {
    double tongDoanhThu = 0.0;
    double tienMat = 0.0;
    double chuyenKhoanOrQR = 0.0;

    for (size_t i = 0; i < danhSachHoaDon.size(); ++i) {
        if (danhSachHoaDon[i].getTrangThaiThanhToan() == "DaTra") {
            tongDoanhThu += danhSachHoaDon[i].getTongTien();
            if (danhSachHoaDon[i].getHinhThucThanhToan() == "TienMat") {
                tienMat += danhSachHoaDon[i].getTongTien();
            } else {
                chuyenKhoanOrQR += danhSachHoaDon[i].getTongTien();
            }
        }
    }

    cout << "\n==================== TONG QUAN DOANH THU HE THONG ===================\n";
    cout << " Tong doanh thu thuc te: " << fixed << setprecision(0) << tongDoanhThu << " VND\n";
    cout << " - Tien mat             : " << tienMat << " VND\n";
    cout << " - Chuyen khoan / QR    : " << chuyenKhoanOrQR << " VND\n";
    cout << " Tong so hoa don da xuat: " << danhSachHoaDon.size() << "\n";
    cout << "=======================================================================\n";
}
