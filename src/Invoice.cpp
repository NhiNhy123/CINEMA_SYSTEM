#include "../include/Invoice.h"
#include "../include/Database.h"
#include "../include/Ticket.h"
#include "../include/FAndB.h"
#include "../include/Promotion.h"
#include "../include/DateFormat.h"
#include "../include/ActivityLog.h"
#include "../include/ShilftManagement.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <ctime>
#include <windows.h>

using namespace std;

namespace {
bool parseTimeOfDay(const string& value, int& seconds) {
    stringstream stream(value);
    int hour, minute, second = 0;
    char separator;
    if (!(stream >> hour >> separator) || separator != ':' || !(stream >> minute)) return false;
    if (stream.peek() == ':') {
        stream.get();
        if (!(stream >> second)) return false;
    }
    stream >> ws;
    if (!stream.eof() || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }
    seconds = hour * 3600 + minute * 60 + second;
    return true;
}

bool nextDateKey(const string& date, int& nextKey) {
    int day, month, year;
    if (!DateFormat::parseDate(date, day, month, year)) return false;
    const bool leapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    const int daysInMonth[] = {31, leapYear ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (++day > daysInMonth[month - 1]) {
        day = 1;
        if (++month > 12) {
            month = 1;
            ++year;
        }
    }
    nextKey = year * 10000 + month * 100 + day;
    return true;
}

bool invoiceBelongsToShift(const Invoice& invoice, const ShiftReport& shift) {
    if (invoice.getMaNhanVien() != shift.getMaNhanVien()) return false;

    const string& timestamp = invoice.getThoiGianLap();
    const size_t separator = timestamp.find(' ');
    if (separator == string::npos) return false;

    const string invoiceDate = timestamp.substr(0, separator);
    const string invoiceTime = timestamp.substr(separator + 1);
    const int invoiceDateKey = DateFormat::dateKey(invoiceDate);
    const int shiftDateKey = DateFormat::dateKey(shift.getNgayLam());
    int invoiceSeconds, shiftStart, shiftEnd;
    if (invoiceDateKey == 0 || shiftDateKey == 0 ||
        !parseTimeOfDay(invoiceTime, invoiceSeconds) ||
        !parseTimeOfDay(shift.getGioBatDau(), shiftStart) ||
        !parseTimeOfDay(shift.getGioKetThuc(), shiftEnd)) {
        return false;
    }

    if (shiftEnd >= shiftStart) {
        return invoiceDateKey == shiftDateKey &&
               invoiceSeconds >= shiftStart && invoiceSeconds <= shiftEnd;
    }

    int nextShiftDateKey;
    if (!nextDateKey(shift.getNgayLam(), nextShiftDateKey)) return false;
    return (invoiceDateKey == shiftDateKey && invoiceSeconds >= shiftStart) ||
           (invoiceDateKey == nextShiftDateKey && invoiceSeconds <= shiftEnd);
}

void hienThiKetQuaDoanhThu(const vector<Invoice>& invoices,
                           const ShiftReport* shift,
                           const string& maThuNgan,
                           const string& phuongThuc) {
    double total = 0.0;
    size_t count = 0;
    cout << "\nMa HD     Ma KH       Thu Ngan  Thoi Gian              Tong Tien      Hinh Thuc      Trang Thai\n";
    cout << "---------------------------------------------------------------------------------------------\n";

    for (size_t i = 0; i < invoices.size(); ++i) {
        const Invoice& invoice = invoices[i];
        if (invoice.getTrangThaiThanhToan() != "DaTra" ||
            (!maThuNgan.empty() && invoice.getMaNhanVien() != maThuNgan) ||
            (!phuongThuc.empty() && invoice.getHinhThucThanhToan() != phuongThuc) ||
            (shift != NULL && !invoiceBelongsToShift(invoice, *shift))) {
            continue;
        }
        invoice.hienThi();
        total += invoice.getTongTien();
        ++count;
    }

    cout << "---------------------------------------------------------------------------------------------\n";
    cout << "So hoa don da thanh toan: " << count << "\n";
    cout << "Tong doanh thu: " << fixed << setprecision(0) << total << " VND\n";
    if (count == 0) cout << "Khong co hoa don phu hop voi dieu kien loc.\n";
}
}

Invoice::Invoice() {
    this->tongTien = 0.0;
    this->trangThaiThanhToan = "ChuaTra";
}

Invoice::Invoice(string maHD, string maKH, string maNV, string thoiGian, double tongT, string hinhThuc, string maGD, string trangThai) {
    this->maHoaDon = maHD;
    this->maKhachHang = maKH;
    this->maNhanVien = maNV;
    this->thoiGianLap = DateFormat::normalizeDateTime(thoiGian);
    this->tongTien = tongT;
    this->hinhThucThanhToan = hinhThuc;
    this->maGiaoDich = maGD;
    this->trangThaiThanhToan = trangThai;
}

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
    ss << maHoaDon << "," << maKhachHang << "," << maNhanVien << ","
       << DateFormat::normalizeDateTime(thoiGianLap) << ","
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
         << setw(23) << DateFormat::normalizeDateTime(thoiGianLap)
         << setw(15) << fixed << setprecision(0) << tongTien
         << setw(15) << hinhThucThanhToan
         << setw(12) << trangThaiThanhToan << endl;
}

InvoiceManager::InvoiceManager() {}

void InvoiceManager::taiDuLieu(Database& db) {
    db.docDanhSachHoaDon(danhSachHoaDon);
}

bool InvoiceManager::taoHoaDonMoi(Database& db, const string& maNV, const string& maKH, double tongTienHang,
                                   const vector<InvoiceFBItem>& dsFB, const string& phimChieuCode,
                                   double soTienDoiDiem, const string& maSuatChieu,
                                   const vector<string>& dsGheGiu, const string& maPhienGiu) {

    double tongTienFB = 0.0;
    for (size_t i = 0; i < dsFB.size(); ++i) {
        tongTienFB += dsFB[i].thanhTien;
    }
    const double tongTienVe = tongTienHang - tongTienFB;

    string maCodeVoucher;
    cout << "Nhap ma voucher khuyen mai (Bam Enter de bo qua): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, maCodeVoucher);

    double soTienGiamGia = 0.0;
    double tongTienSauDoiDiem = tongTienHang - soTienDoiDiem;
    if (tongTienSauDoiDiem < 0) tongTienSauDoiDiem = 0;
    if (!maCodeVoucher.empty()) {
        vector<Promotion> dsKM;
        db.docDanhSachKhuyenMai(dsKM);
        for (size_t i = 0; i < dsKM.size(); ++i) {
            if (dsKM[i].getMaCode() == maCodeVoucher) {
                int todayKey;
                int startKey = DateFormat::dateKey(dsKM[i].getNgayBatDau());
                int endKey = DateFormat::dateKey(dsKM[i].getNgayKetThuc());
                if (!DateFormat::todayDateKey(todayKey)) {
                    cout << "-> [Canh bao]: Khong the xac dinh ngay hien tai de kiem tra voucher!\n";
                    break;
                }
                if (startKey == 0 || endKey == 0 || todayKey < startKey || todayKey > endKey) {
                    cout << "-> [Canh bao]: Voucher chua den ngay ap dung hoac da het han!\n";
                    break;
                }

                string phamVi = dsKM[i].getPhamViApDung();
                if (phamVi == "All" || phamVi == phimChieuCode) {
                    double giam = tongTienSauDoiDiem * (dsKM[i].getPhanTramGiam() / 100.0);
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

    double tongThanhToan = tongTienSauDoiDiem - soTienGiamGia;
    if (tongThanhToan < 0) tongThanhToan = 0;

    stringstream ssHD;
    ssHD << "HD_" << (danhSachHoaDon.size() + 1);
    string maHD = ssHD.str();

    const time_t now = time(0);
    const tm* localTime = localtime(&now);
    if (localTime == NULL) {
        cout << "[Loi]: Khong the xac dinh thoi gian lap hoa don.\n";
        return false;
    }
    char timestamp[20];
    if (strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", localTime) == 0) {
        cout << "[Loi]: Khong the dinh dang thoi gian lap hoa don.\n";
        return false;
    }
    string thoiGianLap = timestamp;

    cout << "\n==================== XU LY THANH TOAN HOA DON ====================\n";
    cout << "Ma hoa don tam tao: " << maHD << "\n";
    cout << "Tong tien ve (gia ban day du theo loai ghe): "
         << fixed << setprecision(0) << tongTienVe << " VND\n";
    cout << "Tong tien F&B: " << tongTienFB << " VND\n";
    cout << "Tong gia tri don hang: " << tongTienHang << " VND\n";
    if (soTienDoiDiem > 0) {
        cout << "Giam gia tu diem: -" << soTienDoiDiem << " VND\n";
    }
    if (soTienGiamGia > 0) {
        cout << "Giam gia tu Promotion: -" << soTienGiamGia << " VND\n";
    }
    cout << "TONG TIEN THUC THU: " << fixed << setprecision(0) << tongThanhToan << " VND\n";
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
	    cout << "-> Dang ket noi the ngan hang";

    	for (int i = 0; i < 3; ++i) {
        	cout << "." << flush;
        	Sleep(1000);
    	}
        cout << "\n-> Giao dich thanh cong!\n";
    } else if (chonThanhToan == 3) {
        hinhThuc = "QueQR";
        maGD = "TXN_QR_" + maHD;
        cout << "\n[MA QR THANH TOAN MO PHONG]\n";
        cout << "||||||||||||||||||||||||||||||||\n";
        cout << "|    QUET MA DE THANH TOAN NAY   |\n";
        cout << "||||||||||||||||||||||||||||||||\n";
        cout << "-> Vui long doi khach hang quét ma";
        for (int i = 0; i < 3; ++i) {
        	cout << "." << flush;
        	Sleep(1000);
    	}
        cout << "\n-> Khach da quet ma thanh toan thanh cong!\n";
    } else {
        hinhThuc = "TienMat";
        double tienKhachDua = 0.0;

        while (true) {
            cout << "-> Nhap so tien khach dua (VND): ";
            cin >> tienKhachDua;

            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "[Loi]: Vui long nhap mot so hop le!\n";
            } else if (tienKhachDua < tongThanhToan) {
                cout << "[Canh bao]: So tien khach dua (" << (long long)tienKhachDua
                     << " VND) thieu so voi tong thanh toan (" << (long long)tongThanhToan << " VND). Vui long nhap lai!\n";
            } else {
                double tienThua = tienKhachDua - tongThanhToan;
                cout << "-> Nhan tien mat thanh cong.\n";
                cout << "-> **TIEN THUA TRA LAI CHO KHACH**: " << fixed << setprecision(0) << tienThua << " VND\n";
                break;
            }
        }

        Sleep(500);
    }

    if (!db.chotGheSuatChieu(maSuatChieu, dsGheGiu, maPhienGiu)) {
        cout << "\n[CANH BAO]: Thoi gian giu ghe da het hoac ghe khong con thuoc giao dich nay. Khong tao hoa don.\n";
        return false;
    }

    Invoice newInvoice(maHD, maKH, maNV, thoiGianLap, tongThanhToan, hinhThuc, maGD, "DaTra");

    for (size_t i = 0; i < dsFB.size(); ++i) {
        InvoiceFBItem itemNho = dsFB[i];
        itemNho.maHoaDon = maHD;
        newInvoice.themChiTietFB(itemNho);
    }

    danhSachHoaDon.push_back(newInvoice);

    db.ghiDanhSachHoaDon(danhSachHoaDon);
    std::ostringstream chiTietNhatKy;
    chiTietNhatKy << "Ma hoa don: " << maHD
                  << ", ma khach hang: " << (maKH.empty() ? "KhachLe" : maKH)
                  << ", tong tien: " << fixed << setprecision(0) << tongThanhToan
                  << " VND, phuong thuc: " << hinhThuc;
    ActivityLog::ghiNhan(maNV, "Staff", "THANH_TOAN_HOA_DON", chiTietNhatKy.str());

    cout << "\n==================================================================\n";
    cout << "                    CINEMA TICKET INVOICE                         \n";
    cout << "==================================================================\n";
    cout << "Ma HD: " << maHD << " | Thu ngan: " << maNV << "\n";
    cout << "Thoi gian: " << thoiGianLap << "\n";
    cout << "Khach hang: " << (maKH.empty() ? "Khach vang lai" : maKH) << "\n";
    cout << "------------------------------------------------------------------\n";
    cout << "Tong tien ve: " << fixed << setprecision(0) << tongTienVe << " VND\n";
    cout << "Tong tien F&B: " << tongTienFB << " VND\n";
    cout << "Tong gia tri don hang: " << tongTienHang << " VND\n";
    if (soTienDoiDiem > 0) {
        cout << "Giam gia tu diem: -" << soTienDoiDiem << " VND\n";
    }
    if (soTienGiamGia > 0) {
        cout << "Giam gia voucher: -" << soTienGiamGia << " VND\n";
    }
    cout << "Phuong thuc: " << hinhThuc << " | Ma GD: " << maGD << "\n";
    cout << "Trang thai: DA THANH TOAN (DaTra)\n";
    cout << "TONG THANH TOAN SAU GIAM GIA: " << fixed << setprecision(0) << tongThanhToan << " VND\n";
    cout << "==================================================================\n";
    cout << "          Cam on quy khach va hen gap lai lan sau!                \n";
    cout << "==================================================================\n";
    return true;
}

void InvoiceManager::hienThiLichSuGiaoDich() const {
    cout << "\n================================= LICH SU GIAO DICH HOA DON ==================================\n";
    cout << left << setw(10) << "Ma HD"
         << setw(12) << "Ma KH"
         << setw(10) << "Thu Ngan"
         << setw(23) << "Thoi Gian"
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
    cout << "=============================================================================================\n";
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

void InvoiceManager::thongKeDoanhThuTheoCa(const ShiftReport& ca) const {
    cout << "\nDOANH THU CA " << ca.getMaCa()
         << " | Thu ngan: " << ca.getMaNhanVien()
         << " | Ngay: " << ca.getNgayLam()
         << " | Gio: " << ca.getGioBatDau() << " - " << ca.getGioKetThuc() << "\n";
    hienThiKetQuaDoanhThu(danhSachHoaDon, &ca, "", "");
}

void InvoiceManager::thongKeDoanhThuTheoThuNgan(const string& maThuNgan) const {
    cout << "\nDOANH THU THEO THU NGAN: " << maThuNgan << "\n";
    hienThiKetQuaDoanhThu(danhSachHoaDon, NULL, maThuNgan, "");
}

void InvoiceManager::thongKeDoanhThuTheoPhuongThuc(const string& phuongThuc) const {
    cout << "\nDOANH THU THEO PHUONG THUC: " << phuongThuc << "\n";
    hienThiKetQuaDoanhThu(danhSachHoaDon, NULL, "", phuongThuc);
}
