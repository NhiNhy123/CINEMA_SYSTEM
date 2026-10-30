#include "../include/Ticket.h"
#include "../include/Database.h"
#include "../include/Showtime.h"
#include "../include/Seat.h"
#include "../include/Movie.h"
#include "../include/Customer.h"
#include "../include/Invoice.h"
#include "../include/FAndB.h"
#include "../include/Room.h"
#include "../include/DateFormat.h"
#include "../include/ActivityLog.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <ctime>

static bool laChuoiChuSo(const string& giaTri) {
    if (giaTri.empty()) return false;
    for (size_t i = 0; i < giaTri.size(); ++i) {
        if (giaTri[i] < '0' || giaTri[i] > '9') return false;
    }
    return true;
}

static bool sdtHopLe(const string& sdt) {
    return sdt.length() == 10 && sdt[0] == '0' && laChuoiChuSo(sdt);
}

static bool sdtTrungKhop(const string& sdtNhap, const string& sdtDaLuu) {
    if (sdtNhap == sdtDaLuu) return true;
    return sdtDaLuu.length() == 9 && sdtNhap.length() == 10 &&
           sdtNhap[0] == '0' && sdtNhap.substr(1) == sdtDaLuu;
}

static bool cccdHopLe(const string& cccd) {
    return cccd.length() == 12 && laChuoiChuSo(cccd);
}

static bool laGiuBoiPhien(const string& trangThai, const string& maPhienGiu, long long* thoiGianConLai = NULL) {
    const string tienTo = "DangChon|";
    if (trangThai.find(tienTo) != 0) return false;
    const size_t dauPhanCach = trangThai.find('|', tienTo.length());
    if (dauPhanCach == string::npos) return false;

    const long long hetHan = atoll(trangThai.substr(tienTo.length(), dauPhanCach - tienTo.length()).c_str());
    const string phienDangGiu = trangThai.substr(dauPhanCach + 1);
    const long long conLai = hetHan - static_cast<long long>(time(NULL));
    if (conLai <= 0 || phienDangGiu != maPhienGiu) return false;
    if (thoiGianConLai != NULL) *thoiGianConLai = conLai;
    return true;
}

static void giaiPhongGheCuaPhien(Database& db, const string& maSC,
                                  const vector<string>& danhSachGhe, const string& maPhienGiu) {
    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        db.giaiPhongGheSuatChieu(maSC, danhSachGhe[i], maPhienGiu);
    }
}

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

    stringstream ssGia;
    ssGia << (int)giaVeThucTe << "d";

    cout << left << setw(13) << maVe
         << setw(15) << maHoaDon
         << setw(12) << maSuatChieu
         << setw(8)  << maGhe
         << setw(12) << ssGia.str()
         << setw(15) << maQRCode
         << setw(15) << trangThaiSuDung << endl;
}

Customer xuLyThongTinKhachHangCRM(Database& db, double tongTienHoaDon, double& soTienDoiDiem,
                                  vector<Customer>& danhSachKH,
                                  vector<GiaoDichDiem>& danhSachGDCapNhat,
                                  bool& khachHangMoi) {
    cout << "\n===================================================================\n";
    cout << "                    XU LY THONG TIN KHACH HANG (CRM)                 \n";
    cout << "===================================================================\n";
    soTienDoiDiem = 0.0;
    khachHangMoi = false;

    string sdt;
    do {
        cout << "Nhap so dien thoai khach hang (10 so, bat dau bang 0): ";
        cin >> sdt;
        if (!sdtHopLe(sdt)) {
            cout << "Loi: So dien thoai phai gom dung 10 chu so va bat dau bang 0!\n";
        }
    } while (!sdtHopLe(sdt));
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    time_t now = time(0);
    char bufTime[80];
    strftime(bufTime, sizeof(bufTime), "%d/%m/%Y %H:%M:%S", localtime(&now));
    string thoiGian = string(bufTime);

    db.docDanhSachUsers(danhSachKH);

    Customer* khachHangTimThay = NULL;
    for (size_t i = 0; i < danhSachKH.size(); ++i) {
        if (sdtTrungKhop(sdt, danhSachKH[i].getSoDienThoai())) {
            khachHangTimThay = &danhSachKH[i];
            break;
        }
    }

    string maKHThaoTac = "";
    bool daDoiDiem = false;
    int soDiemDaDoi = 0;

    if (khachHangTimThay != NULL) {
        cout << "-> He thong tim thay thanh vien: " << khachHangTimThay->getHoTen()
             << " (Hang " << khachHangTimThay->getHangThanhVien()
             << " - Diem tich luy: " << khachHangTimThay->getDiemTichLuy() << ")\n";

        const int diemToiDaTheoDon = static_cast<int>(tongTienHoaDon / 100.0);
        const int diemToiDaDoi = khachHangTimThay->getDiemTichLuy() < diemToiDaTheoDon
                               ? khachHangTimThay->getDiemTichLuy()
                               : diemToiDaTheoDon;
        if (diemToiDaDoi > 0) {
            int diemMuonDoi;
            while (true) {
                cout << "Nhap so diem muon doi (0-" << diemToiDaDoi
                     << ", 0 de bo qua; 1 diem = 100 VND): ";
                if (cin >> diemMuonDoi && diemMuonDoi >= 0 && diemMuonDoi <= diemToiDaDoi) {
                    break;
                }
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Loi: So diem doi phai nam trong khoang cho phep!\n";
            }

            if (diemMuonDoi > 0) {
                if (khachHangTimThay->doiDiem(diemMuonDoi, "Doi diem khi thanh toan", thoiGian)) {
                    daDoiDiem = true;
                    soDiemDaDoi = diemMuonDoi;
                    soTienDoiDiem = diemMuonDoi * 100.0;
                    cout << "-> Du kien doi " << diemMuonDoi << " diem, giam "
                         << fixed << setprecision(0) << soTienDoiDiem
                         << " VND sau khi thanh toan thanh cong.\n";
                } else {
                    cout << "-> Khong the doi diem; khong ap dung giam gia diem.\n";
                }
            }
        } else {
            cout << "-> Don hang khong du dieu kien toi thieu de doi diem.\n";
        }

        maKHThaoTac = khachHangTimThay->getMaKH();
    }

    else {
        khachHangMoi = true;
        cout << "-> Khong tim thay khach hang voi SÐT: " << sdt << ". Vui long dang ky thong tin moi!\n";

        string hoTen, email, ngaySinh, gioiTinh, cccd;
        cout << "Nhap ho ten khach hang: ";
        getline(cin, hoTen);
        do {
            cout << "Nhap CCCD (12 chu so): ";
            getline(cin, cccd);
            if (!cccdHopLe(cccd)) {
                cout << "Loi: CCCD phai gom dung 12 chu so!\n";
            }
        } while (!cccdHopLe(cccd));
        cout << "Nhap email: ";
        getline(cin, email);
        do {
            cout << "Nhap ngay sinh (DD/MM/YYYY): ";
            getline(cin, ngaySinh);
            if (!DateFormat::isValidDate(ngaySinh)) cout << "Loi: Ngay sinh khong hop le, vui long nhap lai!\n";
        } while (!DateFormat::isValidDate(ngaySinh));
        cout << "Nhap gioi tinh (Nam/Nu): ";
        getline(cin, gioiTinh);

        stringstream ssMa;
        ssMa << "U0" << (danhSachKH.size() + 1);
        string maKHMoi = ssMa.str();
        maKHThaoTac = maKHMoi;

        Customer khachMoi(maKHMoi, hoTen, sdt, email, 0, "Thuong", 0);
        khachMoi.setCCCD(cccd);

        danhSachKH.push_back(khachMoi);
        cout << "-> Da nhap thong tin thanh vien moi, ma KH: " << maKHMoi << "\n";
        cout << "-> Diem se duoc cong sau khi thanh toan thanh cong.\n";
    }

    db.docLichSuGiaoDichDiem(danhSachGDCapNhat);

    if (daDoiDiem) {
        stringstream ssGD;
        ssGD << "GD0" << (danhSachGDCapNhat.size() + 1);
        danhSachGDCapNhat.push_back(GiaoDichDiem(ssGD.str(), maKHThaoTac, "DoiDiem",
                                                soDiemDaDoi, "Doi diem khi thanh toan", thoiGian));
    }
    if (khachHangTimThay != NULL) {
        return *khachHangTimThay;
    } else {
        return danhSachKH.back();
    }
}

void TicketManager::quyTrinhBanVe(Database& db, const string& maNV) {

    vector<Showtime> dsShowtimeGoc;
    db.docDanhSachSuatChieu(dsShowtimeGoc);
    if (dsShowtimeGoc.empty()) {
        cout << "\n[LOI]: Hien tai khong co suat chieu nao!\n";
        return;
    }

    vector<Room> dsPhong;
    db.docDanhSachPhong(dsPhong);

    vector<Showtime> dsShowtime;
    for (size_t i = 0; i < dsShowtimeGoc.size(); ++i) {
        bool phongBiKhoa = false;
        for (size_t j = 0; j < dsPhong.size(); ++j) {

            if (dsPhong[j].getMaPhong() == dsShowtimeGoc[i].getMaPhong() && dsPhong[j].getTrangThaiPhong() == "TamNgung") {
                phongBiKhoa = true;
                break;
            }
        }

        if (!phongBiKhoa &&
            DateFormat::isFutureDateTime(dsShowtimeGoc[i].getNgayChieu(),
                                        dsShowtimeGoc[i].getGioBatDau())) {
            dsShowtime.push_back(dsShowtimeGoc[i]);
        }
    }

    if (dsShowtime.empty()) {
        cout << "\n[LOI]: Khong co suat chieu tuong lai nao trong cac phong dang hoat dong!\n";
        return;
    }

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

    vector<Movie> dsPhim;
    db.docDanhSachPhim(dsPhim);
    cout << "\n--- DANH SACH PHIM CO SUAT CHIEU TUONG LAI ---\n";
    for (size_t i = 0; i < dsMaPhim.size(); ++i) {
        string tenPhim = dsMaPhim[i];
        for (size_t j = 0; j < dsPhim.size(); ++j) {
            if (dsPhim[j].getMaPhim() == dsMaPhim[i]) {
                tenPhim = dsPhim[j].getTenPhim();
                break;
            }
        }
        cout << (i + 1) << ". " << tenPhim << " (" << dsMaPhim[i] << ")\n";
    }

    int chonPhimSTT;
    cout << "Nhap STT phim can xem (Nhap 0 de huy): ";
    cin >> chonPhimSTT;
    if (chonPhimSTT <= 0 || chonPhimSTT > (int)dsMaPhim.size()) {
        return;
    }
    string selectedMovie = dsMaPhim[chonPhimSTT - 1];

    vector<Showtime> dsSuatChieuCuaPhim;
    for (size_t i = 0; i < dsShowtime.size(); ++i) {
        if (dsShowtime[i].getMaPhim() == selectedMovie) {
            dsSuatChieuCuaPhim.push_back(dsShowtime[i]);
        }
    }

    cout << "\n===================================================================\n";
    cout << "            SUAT CHI?U CHO PHIM: " << selectedMovie << " (Da loc phong kha dung)\n";
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
    if (!DateFormat::isFutureDateTime(selectedShowtime.getNgayChieu(),
                                      selectedShowtime.getGioBatDau())) {
        cout << "[CANH BAO]: Suat chieu da bat dau hoac da qua. Vui long chon suat khac.\n";
        return;
    }
    string selectedSC = selectedShowtime.getMaSuatChieu();
    string maPhongChieu = selectedShowtime.getMaPhong();

    vector<Seat> dsGhePhong;
    db.docDanhSachGhe(dsGhePhong);

    vector<string> dsGheChon;
    vector<double> dsGiaVeGheChon;
    double tongTienGhe = 0.0;
    static unsigned long long soPhienGiu = 0;
    ++soPhienGiu;
    stringstream ssPhienGiu;
    ssPhienGiu << static_cast<long long>(time(NULL)) << "_" << clock() << "_" << rand() << "_" << soPhienGiu;
    const string maPhienGiu = ssPhienGiu.str();
    char tiepTucChon = 'y';

    while (tiepTucChon == 'y' || tiepTucChon == 'Y') {
        vector<pair<string, string> > trangThaiGheList;
        db.docTrangThaiGheSuatChieu(selectedSC, trangThaiGheList);

        for (size_t i = 0; i < dsGheChon.size();) {
            bool conGiu = false;
            for (size_t j = 0; j < trangThaiGheList.size(); ++j) {
                if (trangThaiGheList[j].first == dsGheChon[i] &&
                    laGiuBoiPhien(trangThaiGheList[j].second, maPhienGiu)) {
                    conGiu = true;
                    break;
                }
            }
            if (conGiu) {
                ++i;
            } else {
                cout << "[CANH BAO]: Thoi gian giu ghe " << dsGheChon[i]
                     << " da het; ghe duoc tra lai va xoa khoi giao dich.\n";
                tongTienGhe -= dsGiaVeGheChon[i];
                dsGheChon.erase(dsGheChon.begin() + i);
                dsGiaVeGheChon.erase(dsGiaVeGheChon.begin() + i);
            }
        }

        cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhongChieu << " ====================\n";
        cout << " [MAN HINH CHIEU PHIM]\n";
        cout << "-----------------------------------------------------------------------\n";
        bool coGheTrongPhong = false;
        for (size_t k = 0; k < dsGhePhong.size(); ++k) {
            if (dsGhePhong[k].getMaPhong() != maPhongChieu) {
                continue;
            }

            string trangThaiGhe = "Trong";
            for (size_t i = 0; i < trangThaiGheList.size(); ++i) {
                if (trangThaiGheList[i].first == dsGhePhong[k].getMaGhe()) {
                    trangThaiGhe = trangThaiGheList[i].second;
                    break;
                }
            }

            Seat gheHienThi = dsGhePhong[k];
            gheHienThi.setTrangThai(trangThaiGhe);
            gheHienThi.hienThi();
            coGheTrongPhong = true;
        }
        if (!coGheTrongPhong) {
            cout << "Phong nay chua duoc thiet lap so do ghe!\n";
        }
        cout << "\n-----------------------------------------------------------------------\n";
        cout << "Chu thich: [ ] Trong | [?] Dang duoc giu | [X] Da dat\n";
        if (!dsGheChon.empty()) {
            cout << "Ghe dang giu: ";
            for (size_t i = 0; i < dsGheChon.size(); ++i) {
                long long conLai = 0;
                for (size_t j = 0; j < trangThaiGheList.size(); ++j) {
                    if (trangThaiGheList[j].first == dsGheChon[i]) {
                        laGiuBoiPhien(trangThaiGheList[j].second, maPhienGiu, &conLai);
                        break;
                    }
                }
                cout << dsGheChon[i] << " (";
                if (conLai > 0) {
                    cout << setfill('0') << setw(2) << (conLai / 60) << ":"
                         << setw(2) << (conLai % 60) << setfill(' ');
                } else {
                    cout << "00:00";
                }
                cout << ")";
                if (i + 1 < dsGheChon.size()) cout << ", ";
            }
            cout << "\n";
        }
        cout << "=======================================================================\n";

        string ghe;
        cout << "Nhap ma ghe muon chon (VD: A1, B1, VIP1...; nhap HUY de bo tat ca ghe): ";
        cin >> ghe;
        if (ghe == "HUY" || ghe == "huy") {
            giaiPhongGheCuaPhien(db, selectedSC, dsGheChon, maPhienGiu);
            dsGheChon.clear();
            dsGiaVeGheChon.clear();
            tongTienGhe = 0.0;
            cout << "[THONG BAO]: Da huy tat ca ghe dang chon va huy giao dich ban ve!\n";
            return;
        }

        bool hopLe = false;
        double giaGheHienTai = 0.0;

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
                    giaGheHienTai = dsGhePhong[k].getDonGiaGhe();
                    hopLe = giaGheHienTai > 0;
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

        if (!db.giuGheSuatChieu(selectedSC, ghe, maPhienGiu)) {
            cout << "[CANH BAO]: Ghe [" << ghe << "] vua duoc giao dich khac giu; vui long chon ghe khac.\n";
            continue;
        }

        dsGheChon.push_back(ghe);
        dsGiaVeGheChon.push_back(giaGheHienTai);
        tongTienGhe += giaGheHienTai;

        cout << "Da chon ghe [" << ghe << "] thanh cong (giu toi da 15 phut). Gia ghe: "
             << fixed << setprecision(0) << giaGheHienTai
             << " VND. Tam tinh tien ve: " << tongTienGhe << " VND\n";
        cout << "Ban co muon chon them ghe nua khong? (y/n): ";
        cin >> tiepTucChon;
    }

    if (dsGheChon.empty()) {
        cout << "[THONG BAO]: Ban chua chon ghe nao. Huy giao dich ban ve!\n";
        return;
    }

    cout << "\n===================================================================\n";
    cout << "                    CHON DO AN / THUC UONG (F&B)                   \n";
    cout << "===================================================================\n";

    vector<FoodAndBeverage> dsTam;
    db.docDanhSachFAndB(dsTam);

    vector<FoodAndBeverage> dsDangBan;
    for (size_t i = 0; i < dsTam.size(); ++i) {
        if (dsTam[i].getTrangThaiKinhDoanh() != "NgungBan") {
            dsDangBan.push_back(dsTam[i]);
        }
    }

    cout << left << setw(6) << "STT" << setw(40) << "Ten San Pham" << setw(15) << "Gia Ban (VND)" << endl;
    cout << "-------------------------------------------------------------------\n";

    for (size_t i = 0; i < dsDangBan.size(); ++i) {
        cout << left << setw(6) << (i + 1)
             << setw(40) << dsDangBan[i].getTenMatHang()
             << setw(15) << fixed << setprecision(0) << dsDangBan[i].getGiaTien() << endl;
    }
    cout << "-------------------------------------------------------------------\n";

    if (dsDangBan.empty()) {
        cout << "Hien tai khong có san pham F&B nao dang kinh doanh!\n";
    }

    vector<InvoiceFBItem> dsFBItems;
    double tienFB = 0.0;
    char tiepTuc = 'y';

    while (!dsDangBan.empty() && (tiepTuc == 'y' || tiepTuc == 'Y')) {
        int chonSTT;
        int soLuong;

        cout << "\nNhap so thu tu (STT) mon an muon mua (1-" << dsDangBan.size() <<", nhap 0 de bo qua): ";
        cin >> chonSTT;

        if (chonSTT == 0) {
            break;
        }

        if (chonSTT < 1 || chonSTT > (int)dsDangBan.size()) {
            cout << "[Loi]: Lua chon STT khong hop le!\n";
        } else {
            FoodAndBeverage selectedItem = dsDangBan[chonSTT - 1];

            cout << "Nhap so luong cho [" << selectedItem.getTenMatHang() << "]: ";
            cin >> soLuong;

            if (soLuong <= 0) {
                cout << "[Canh bao]: So luong phai lon hon 0!\n";
            } else {
                double thanhTien = selectedItem.getGiaTien() * soLuong;
                tienFB += thanhTien;

                cout << "-> Da them: " << soLuong << " x " << selectedItem.getTenMatHang()
                     << " = " << fixed << setprecision(0) << thanhTien << " VND\n";

                InvoiceFBItem item;
                item.maHoaDon = "";
                item.maMatHang = selectedItem.getMaMatHang();
                item.soLuong = soLuong;
                item.thanhTien = thanhTien;

                dsFBItems.push_back(item);
            }
        }

        cout << "Ban co muon chon them mon F&B khac khong? (y/n): ";
        cin >> tiepTuc;
    }

    cout << "\n----------------------------------------------------\n";
    cout << "TONG TIEN F&B TAM TINH: " << fixed << setprecision(0) << tienFB << " VND\n";
    cout << "----------------------------------------------------\n";

    double tongTienHang = tongTienGhe + tienFB;

    bool tatCaGheConGiu = true;
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        if (!db.kiemTraGiuGheSuatChieu(selectedSC, dsGheChon[i], maPhienGiu)) {
            tatCaGheConGiu = false;
            break;
        }
    }
    if (!tatCaGheConGiu) {
        giaiPhongGheCuaPhien(db, selectedSC, dsGheChon, maPhienGiu);
        cout << "[CANH BAO]: Het thoi gian giu ghe. Giao dich ban ve da dung truoc khi thanh toan.\n";
        return;
    }

    double soTienDoiDiem = 0.0;
    vector<Customer> danhSachKHCapNhat;
    vector<GiaoDichDiem> danhSachGDCapNhat;
    bool khachHangMoi = false;
    Customer khachHangThanhToan = xuLyThongTinKhachHangCRM(
        db, tongTienHang, soTienDoiDiem, danhSachKHCapNhat, danhSachGDCapNhat,
        khachHangMoi);
    string maKH = khachHangThanhToan.getMaKH();
    string tenKH = khachHangThanhToan.getHoTen();
    string sdt = khachHangThanhToan.getSoDienThoai();

    InvoiceManager invoiceMgr;
    invoiceMgr.taiDuLieu(db);
    if (!invoiceMgr.taoHoaDonMoi(db, maNV, maKH, tongTienHang, dsFBItems, selectedMovie,
                                 soTienDoiDiem, selectedSC, dsGheChon, maPhienGiu)) {
        giaiPhongGheCuaPhien(db, selectedSC, dsGheChon, maPhienGiu);
        return;
    }

    vector<Invoice> danhSachHD;
    db.docDanhSachHoaDon(danhSachHD);
    string maHD = danhSachHD.empty() ? "HD_UNKNOWN" : danhSachHD.back().getMaHoaDon();
    double soTienThanhToan = danhSachHD.empty()
                           ? tongTienHang - soTienDoiDiem
                           : danhSachHD.back().getTongTien();
    int soDiemThuong = static_cast<int>(soTienThanhToan / 10000.0);

    time_t thoiDiemThanhToan = time(0);
    char bufThoiDiemThanhToan[80];
    strftime(bufThoiDiemThanhToan, sizeof(bufThoiDiemThanhToan),
             "%d/%m/%Y %H:%M:%S", localtime(&thoiDiemThanhToan));
    string thoiGianThanhToan(bufThoiDiemThanhToan);

    bool daCapNhatKhachHang = false;
    for (size_t i = 0; i < danhSachKHCapNhat.size(); ++i) {
        if (danhSachKHCapNhat[i].getMaKH() != maKH) {
            continue;
        }

        danhSachKHCapNhat[i].themChiTieu(static_cast<long long>(soTienThanhToan));
        if (soDiemThuong > 0) {
            string noiDung = khachHangMoi
                           ? "Tich diem hoa don dau tien"
                           : "Tich diem tu hoa don ban ve";
            danhSachKHCapNhat[i].tichDiem(soDiemThuong, noiDung, thoiGianThanhToan);

            stringstream ssGD;
            ssGD << "GD0" << (danhSachGDCapNhat.size() + 1);
            danhSachGDCapNhat.push_back(GiaoDichDiem(
                ssGD.str(), maKH, "TichDiem", soDiemThuong, noiDung, thoiGianThanhToan));
        }
        daCapNhatKhachHang = true;
        break;
    }

    if (!daCapNhatKhachHang) {
        cerr << "[LOI]: Khong tim thay khach hang " << maKH
             << " de cap nhat diem sau thanh toan.\n";
    }

    cout << "[THONG BAO]: Da luu trang thai dat ghe cho suat chieu " << selectedSC << " thanh cong!\n";

    vector<Ticket> dsVeHienTai;
    db.docDanhSachVe(dsVeHienTai);

    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        stringstream ssMaVe, ssQR;
        ssMaVe << "V_" << selectedSC << "_" << dsGheChon[i];
        ssQR << "QR_" << maHD << "_" << dsGheChon[i];

        Ticket veMoi(ssMaVe.str(), maHD, selectedSC, dsGheChon[i], dsGiaVeGheChon[i], ssQR.str(), "ChuaCheckIn");
        dsVeHienTai.push_back(veMoi);
    }
    db.ghiDanhSachVe(dsVeHienTai);
    db.ghiDanhSachUsers(danhSachKHCapNhat);
    db.ghiLichSuGiaoDichDiem(danhSachGDCapNhat);

    cout << "\n===================================================================\n";
    cout << "                    XUAT THONG TIN VE XEM PHIM                     \n";
    cout << "===================================================================\n";
    cout << "Suat chieu: Phim [" << selectedMovie << "] luc " << selectedShowtime.getGioBatDau()
         << " ngay " << selectedShowtime.getNgayChieu() << " tai " << maPhongChieu << "\n\n";

    cout << "--- CHI TIET GHE DA DAT ---\n";
    for (size_t i = 0; i < dsGheChon.size(); ++i) {
        cout << (i + 1) << ". Ghe: " << dsGheChon[i]
             << " | Gia ve: " << fixed << setprecision(0) << dsGiaVeGheChon[i]
             << " VND | Trang thai: DaDat\n";
    }
    cout << "Tong tien ve: " << fixed << setprecision(0) << tongTienGhe << " VND\n";

    if (!dsFBItems.empty()) {
        cout << "--- CHI TIET F&B DA CHON ---\n";
        for (size_t i = 0; i < dsFBItems.size(); ++i) {
            cout << "- Ma mon: " << dsFBItems[i].maMatHang
                 << " | So luong: " << dsFBItems[i].soLuong
                 << " | Thanh tien: " << (int)dsFBItems[i].thanhTien << "d\n";
        }
        cout << "F&B Tong cong: " << (int)tienFB << "d\n";
    } else {
        cout << "F&B: Khong\n";
    }

    cout << "\nDiem tich luy khach hang moi nhan: +"
         << soDiemThuong << " diem.\n";
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
    cout << left << setw(13) << "Ma Ve"
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

void TicketManager::checkInVe(Database& db, const string& maNhanVien) {
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
                ActivityLog::ghiNhan(maNhanVien, "Staff", "CHECK_IN_VE",
                                     "Ma ve: " + danhSach[i].getMaVe() +
                                     ", ma suat chieu: " + danhSach[i].getMaSuatChieu() +
                                     ", ma ghe: " + danhSach[i].getMaGhe());
                cout << "[THANH CONG]: Check-in hop le! Moi khach vao phong chieu ghe [" << danhSach[i].getMaGhe() << "].\n";
            }
            break;
        }
    }

    if (!found) {
        cout << "[LOI]: Khong tim thay ma ve hoac ma QR hop le trong he thong!\n";
    }
}
