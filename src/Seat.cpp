#include "../include/Seat.h"
#include "../include/Database.h"
#include "../include/Room.h"
#include "../include/Showtime.h"
#include "../include/Ticket.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cctype>
#include <utility>
#include <algorithm>

using namespace std;

Seat::Seat() {
    this->donGiaGhe = 0.0;
    this->trangThai = "Trong";
}

Seat::Seat(string maGhe, string maPhong, string loaiGhe, double donGiaGhe, string trangThai) {
    this->maGhe = maGhe;
    this->maPhong = maPhong;
    this->loaiGhe = loaiGhe;
    this->donGiaGhe = donGiaGhe;
    this->trangThai = trangThai;
}

string Seat::getMaGhe() const { return maGhe; }
string Seat::getMaPhong() const { return maPhong; }
string Seat::getLoaiGhe() const { return loaiGhe; }
double Seat::getDonGiaGhe() const { return donGiaGhe; }
string Seat::getTrangThai() const { return trangThai; }

void Seat::setLoaiGhe(const string& loai) { loaiGhe = loai; }
void Seat::setDonGiaGhe(double gia) { if (gia > 0) donGiaGhe = gia; }
void Seat::setTrangThai(const string& tt) { trangThai = tt; }

string Seat::toCSV() const {
    stringstream ss;
    ss << fixed << setprecision(0) << donGiaGhe;
    return maGhe + "," + maPhong + "," + loaiGhe + "," + ss.str() + "," + trangThai;
}

Seat Seat::fromCSV(const string& line) {
    stringstream ss(line);
    string maGhe, maPhong, loaiGhe, strGia, trangThai;

    getline(ss, maGhe, ',');
    getline(ss, maPhong, ',');
    getline(ss, loaiGhe, ',');
    getline(ss, strGia, ',');
    getline(ss, trangThai, ',');

    double donGia = 0.0;
    if (!strGia.empty()) {
        stringstream ssGia(strGia);
        ssGia >> donGia;
    }

    return Seat(maGhe, maPhong, loaiGhe, donGia, trangThai);
}

void Seat::hienThi() const {
    string hienThiTT = "[ ]";
    if (trangThai == "DangChon" || trangThai.find("DangChon|") == 0) hienThiTT = "[?]";
    else if (trangThai == "DaDat") hienThiTT = "[X]";
    else if (trangThai == "KhongSuDung") hienThiTT = "[-]";

    cout << maGhe << hienThiTT << "(" << loaiGhe << ": "
         << fixed << setprecision(0) << donGiaGhe << " VND) ";
}

SeatManager::SeatManager() {}

void SeatManager::taiDuLieu(Database& db) {
    db.docDanhSachGhe(danhSachGhe);
}

static bool phanTichMaGhe(const string& maGhe, string& hang, int& cotDau, int& cotCuoi) {
    size_t i = 0;
    while (i < maGhe.size() &&
           ((maGhe[i] >= 'A' && maGhe[i] <= 'Z') ||
            (maGhe[i] >= 'a' && maGhe[i] <= 'z'))) {
        ++i;
    }
    if (i == 0 || i == maGhe.size()) return false;

    hang = maGhe.substr(0, i);
    for (size_t j = 0; j < hang.size(); ++j) {
        hang[j] = static_cast<char>(toupper(static_cast<unsigned char>(hang[j])));
    }

    size_t endFirst = i;
    while (endFirst < maGhe.size() && isdigit(static_cast<unsigned char>(maGhe[endFirst]))) {
        ++endFirst;
    }
    if (endFirst == i) return false;

    stringstream firstStream(maGhe.substr(i, endFirst - i));
    if (!(firstStream >> cotDau) || cotDau <= 0) return false;
    cotCuoi = cotDau;
    if (endFirst == maGhe.size()) return true;

    if (maGhe[endFirst] != '-') return false;
    size_t secondStart = endFirst + 1;
    if (secondStart == maGhe.size()) return false;
    for (size_t j = secondStart; j < maGhe.size(); ++j) {
        if (!isdigit(static_cast<unsigned char>(maGhe[j]))) return false;
    }
    stringstream secondStream(maGhe.substr(secondStart));
    return (secondStream >> cotCuoi) && cotCuoi > cotDau;
}

static void sapXepSoDoGhe(vector<Seat>& gheTrongPhong) {
    sort(gheTrongPhong.begin(), gheTrongPhong.end(),
         [](const Seat& a, const Seat& b) {
             string hangA, hangB;
             int cotDauA, cotCuoiA, cotDauB, cotCuoiB;
             bool hopLeA = phanTichMaGhe(a.getMaGhe(), hangA, cotDauA, cotCuoiA);
             bool hopLeB = phanTichMaGhe(b.getMaGhe(), hangB, cotDauB, cotCuoiB);
             if (hopLeA != hopLeB) return hopLeA;
             if (!hopLeA) return a.getMaGhe() < b.getMaGhe();
             if (hangA != hangB) return hangA < hangB;
             if (cotDauA != cotDauB) return cotDauA < cotDauB;
             return a.getMaGhe() < b.getMaGhe();
         });
}

static bool laLoaiGheDoi(const string& loaiGhe) {
    string normalized = loaiGhe;
    for (size_t i = 0; i < normalized.size(); ++i) {
        normalized[i] = static_cast<char>(
            tolower(static_cast<unsigned char>(normalized[i])));
    }
    return normalized == "couple" || normalized == "double";
}

static bool gheDaBanHoacDangGiu(Database& db, const string& maPhong,
                                const string& maGhe) {
    vector<Showtime> danhSachSuatChieu;
    vector<Ticket> danhSachVe;
    db.docDanhSachSuatChieu(danhSachSuatChieu);
    db.docDanhSachVe(danhSachVe);

    for (size_t i = 0; i < danhSachSuatChieu.size(); ++i) {
        if (danhSachSuatChieu[i].getMaPhong() != maPhong) continue;

        for (size_t j = 0; j < danhSachVe.size(); ++j) {
            if (danhSachVe[j].getMaSuatChieu() ==
                    danhSachSuatChieu[i].getMaSuatChieu() &&
                danhSachVe[j].getMaGhe() == maGhe) {
                return true;
            }
        }

        vector<pair<string, string> > trangThaiGhe;
        db.docTrangThaiGheSuatChieu(
            danhSachSuatChieu[i].getMaSuatChieu(), trangThaiGhe);
        for (size_t j = 0; j < trangThaiGhe.size(); ++j) {
            if (trangThaiGhe[j].first == maGhe &&
                trangThaiGhe[j].second != "Trong") {
                return true;
            }
        }
    }
    return false;
}

static void hienThiSoDoGhe(const vector<Seat>& gheTrongPhong,
                           const string& maPhong) {
    vector<Seat> gheDaSapXep = gheTrongPhong;
    sapXepSoDoGhe(gheDaSapXep);
    cout << "\n==================== SO DO GHE PHONG CHIEU: "
         << maPhong << " ====================\n";
    cout << " [MAN HINH CHIEU PHIM]\n";
    cout << "-----------------------------------------------------------------------\n";

    string hangTruoc;
    for (size_t i = 0; i < gheDaSapXep.size(); ++i) {
        string hang;
        int cotDau, cotCuoi;
        if (!phanTichMaGhe(gheDaSapXep[i].getMaGhe(), hang, cotDau, cotCuoi)) {
            hang = gheDaSapXep[i].getMaGhe();
        }
        if (!hangTruoc.empty() && hang != hangTruoc) cout << "\n";
        hangTruoc = hang;

        string trangThai = "[ ]";
        if (gheDaSapXep[i].getTrangThai() == "DangChon" ||
            gheDaSapXep[i].getTrangThai().find("DangChon|") == 0) {
            trangThai = "[?]";
        } else if (gheDaSapXep[i].getTrangThai() == "DaDat") {
            trangThai = "[X]";
        } else if (gheDaSapXep[i].getTrangThai() == "KhongSuDung") {
            trangThai = "[-]";
        }
        cout << left << setw(12) << (gheDaSapXep[i].getMaGhe() + trangThai);
    }

    cout << "\n-----------------------------------------------------------------------\n";
    cout << "Trang thai: [ ] Trong | [?] Dang chon | [X] Da dat | [-] Khong su dung\n";
    vector<pair<string, double> > chuThichLoaiGia;
    for (size_t i = 0; i < gheDaSapXep.size(); ++i) {
        bool daCo = false;
        for (size_t j = 0; j < chuThichLoaiGia.size(); ++j) {
            if (chuThichLoaiGia[j].first == gheDaSapXep[i].getLoaiGhe() &&
                chuThichLoaiGia[j].second == gheDaSapXep[i].getDonGiaGhe()) {
                daCo = true;
                break;
            }
        }
        if (!daCo) {
            chuThichLoaiGia.push_back(make_pair(
                gheDaSapXep[i].getLoaiGhe(), gheDaSapXep[i].getDonGiaGhe()));
        }
    }
    for (size_t i = 0; i < chuThichLoaiGia.size(); ++i) {
        cout << "  - " << chuThichLoaiGia[i].first << ": "
             << fixed << setprecision(0) << chuThichLoaiGia[i].second << " VND\n";
    }
    cout << "=======================================================================\n";
}

static bool chonLoaiGiaGhe(vector<pair<string, double> >& danhSachLoaiGhe,
                           string& loaiGhe, double& donGia) {
    if (danhSachLoaiGhe.empty()) {
        cout << "Danh muc loai ghe trong. Hay them loai ghe truoc.\n";
        return false;
    }

    bool coLoaiGheDoi = false;
    cout << "\nDanh muc loai ghe:\n";
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        cout << (i + 1) << ". " << danhSachLoaiGhe[i].first << " - "
             << fixed << setprecision(0) << danhSachLoaiGhe[i].second << " VND\n";
        if (laLoaiGheDoi(danhSachLoaiGhe[i].first)) coLoaiGheDoi = true;
    }
    if (!coLoaiGheDoi) {
        cout << (danhSachLoaiGhe.size() + 1)
             << ". Tao loai ghe doi Couple va nhap gia\n";
    }
    cout << "Chon loai ghe: ";
    size_t luaChon;
    size_t luaChonCuoi = danhSachLoaiGhe.size() + (coLoaiGheDoi ? 0 : 1);
    if (!(cin >> luaChon) || luaChon == 0 || luaChon > luaChonCuoi) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Lua chon loai ghe khong hop le.\n";
        return false;
    }
    if (!coLoaiGheDoi && luaChon == danhSachLoaiGhe.size() + 1) {
        cout << "Nhap gia cho ca ghe doi Couple (VND, > 0): ";
        if (!(cin >> donGia) || donGia <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Gia ghe doi phai la so lon hon 0.\n";
            return false;
        }
        loaiGhe = "Couple";
        danhSachLoaiGhe.push_back(make_pair(loaiGhe, donGia));
        return true;
    }
    loaiGhe = danhSachLoaiGhe[luaChon - 1].first;
    donGia = danhSachLoaiGhe[luaChon - 1].second;
    return donGia > 0;
}

void SeatManager::taoGheTuDongChoPhong(const string& maPhong, Database& db) {
    vector<Room> danhSachPhong;
    db.docDanhSachPhong(danhSachPhong);

    size_t roomIndex = danhSachPhong.size();
    for (size_t i = 0; i < danhSachPhong.size(); ++i) {
        if (danhSachPhong[i].getMaPhong() == maPhong) {
            roomIndex = i;
            break;
        }
    }
    if (roomIndex == danhSachPhong.size()) {
        cout << "Khong tim thay phong " << maPhong << "!\n";
        return;
    }

    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);

    vector<Seat> ghePhong;
    vector<Seat> gheKhacPhong;
    vector<Seat> danhSachGheHienTai;
    db.docDanhSachGhe(danhSachGheHienTai);
    for (size_t i = 0; i < danhSachGheHienTai.size(); ++i) {
        if (danhSachGheHienTai[i].getMaPhong() == maPhong) {
            bool duplicate = false;
            for (size_t j = 0; j < ghePhong.size(); ++j) {
                if (ghePhong[j].getMaGhe() == danhSachGheHienTai[i].getMaGhe()) {
                    if (ghePhong[j].getLoaiGhe() != danhSachGheHienTai[i].getLoaiGhe() ||
                        ghePhong[j].getDonGiaGhe() != danhSachGheHienTai[i].getDonGiaGhe()) {
                        cout << "Loi: Ma ghe " << danhSachGheHienTai[i].getMaGhe()
                             << " bi trung voi thong tin khac nhau. Hay sua du lieu phong truoc.\n";
                        return;
                    }
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) ghePhong.push_back(danhSachGheHienTai[i]);
        } else {
            gheKhacPhong.push_back(danhSachGheHienTai[i]);
        }
    }

    if (ghePhong.empty()) {
        int sucChua = danhSachPhong[roomIndex].getSucChua();
        if (sucChua <= 0 || sucChua > 260) {
            cout << "Loi: Suc chua phai tu 1 den 260 de tao hang ghe A-Z, moi hang 10 ghe.\n";
            return;
        }

        string loaiChuan;
        double giaChuan = 0;
        for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
            string normalized = danhSachLoaiGhe[i].first;
            for (size_t j = 0; j < normalized.size(); ++j) {
                normalized[j] = static_cast<char>(
                    tolower(static_cast<unsigned char>(normalized[j])));
            }
            if (normalized == "standard") {
                loaiChuan = danhSachLoaiGhe[i].first;
                giaChuan = danhSachLoaiGhe[i].second;
                break;
            }
        }

        if (loaiChuan.empty()) {
            cout << "Chua co loai ghe Standard. Nhap gia co ban cho ghe Standard (VND, > 0): ";
            if (!(cin >> giaChuan) || giaChuan <= 0) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Loi: Gia co ban phai la so lon hon 0.\n";
                return;
            }
            loaiChuan = "Standard";
            danhSachLoaiGhe.push_back(make_pair(loaiChuan, giaChuan));
        }
        if (giaChuan <= 0) {
            cout << "Loi: Gia loai Standard phai lon hon 0.\n";
            return;
        }

        for (int i = 0; i < sucChua; ++i) {
            stringstream maGhe;
            maGhe << static_cast<char>('A' + i / 10) << (i % 10 + 1);
            ghePhong.push_back(Seat(maGhe.str(), maPhong, loaiChuan,
                                    giaChuan, "Trong"));
        }
    } else {
        cout << "Phong da co so do ghe; se chinh sua so do hien tai de tranh tao trung ghe.\n";
        for (size_t i = 0; i < ghePhong.size(); ++i) {
            bool coTrongDanhMuc = false;
            for (size_t j = 0; j < danhSachLoaiGhe.size(); ++j) {
                if (danhSachLoaiGhe[j].first == ghePhong[i].getLoaiGhe()) {
                    coTrongDanhMuc = true;
                    break;
                }
            }
            if (!coTrongDanhMuc) {
                danhSachLoaiGhe.push_back(make_pair(
                    ghePhong[i].getLoaiGhe(), ghePhong[i].getDonGiaGhe()));
            }
        }
    }

    while (true) {
        cout << "\n--- TUY CHINH SO DO GHE " << maPhong << " ---\n";
        cout << "1. Chinh loai va gia theo hang\n";
        cout << "2. Chinh loai/gia tung ghe (chon ghe doi de gop ghe lien tiep)\n";
        cout << "3. Xem truoc va xac nhan luu\n";
        cout << "0. Huy, khong luu thay doi\n";
        cout << "Chon: ";
        int luaChon;
        if (!(cin >> luaChon)) {
            if (cin.eof()) return;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Lua chon khong hop le.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (luaChon == 0) {
            cout << "Da huy; so do ghe chua duoc thay doi.\n";
            return;
        }
        if (luaChon == 1) {
            vector<string> danhSachHang;
            for (size_t i = 0; i < ghePhong.size(); ++i) {
                string hang;
                int cotDau, cotCuoi;
                if (!phanTichMaGhe(ghePhong[i].getMaGhe(), hang, cotDau, cotCuoi)) continue;
                bool daCo = false;
                for (size_t j = 0; j < danhSachHang.size(); ++j) {
                    if (danhSachHang[j] == hang) daCo = true;
                }
                if (!daCo) danhSachHang.push_back(hang);
            }
            cout << "Cac hang hien co: ";
            for (size_t i = 0; i < danhSachHang.size(); ++i) {
                cout << danhSachHang[i] << (i + 1 < danhSachHang.size() ? ", " : "\n");
            }
            string hangChon;
            cout << "Nhap hang can chinh: ";
            cin >> hangChon;
            for (size_t i = 0; i < hangChon.size(); ++i) {
                hangChon[i] = static_cast<char>(
                    toupper(static_cast<unsigned char>(hangChon[i])));
            }

            bool tonTaiHang = false;
            for (size_t i = 0; i < danhSachHang.size(); ++i) {
                if (danhSachHang[i] == hangChon) tonTaiHang = true;
            }
            if (!tonTaiHang) {
                cout << "Hang ghe khong ton tai.\n";
                continue;
            }

            string loaiMoi;
            double giaMoi;
            if (!chonLoaiGiaGhe(danhSachLoaiGhe, loaiMoi, giaMoi)) continue;
            if (laLoaiGheDoi(loaiMoi)) {
                cout << "Loai ghe doi chi co the gan cho tung cap ghe lien tiep.\n";
                continue;
            }
            for (size_t i = 0; i < ghePhong.size(); ++i) {
                string hangGhe;
                int cotDau, cotCuoi;
                if (phanTichMaGhe(ghePhong[i].getMaGhe(), hangGhe, cotDau, cotCuoi) &&
                    hangGhe == hangChon) {
                    ghePhong[i].setLoaiGhe(loaiMoi);
                    ghePhong[i].setDonGiaGhe(giaMoi);
                }
            }
            cout << "Da cap nhat loai va gia cho hang " << hangChon << ".\n";
            continue;
        }
        if (luaChon == 2) {
            string maGhe;
            cout << "Nhap ma ghe can chinh: ";
            cin >> maGhe;
            for (size_t i = 0; i < maGhe.size(); ++i) {
                maGhe[i] = static_cast<char>(
                    toupper(static_cast<unsigned char>(maGhe[i])));
            }
            size_t seatIndex = ghePhong.size();
            for (size_t i = 0; i < ghePhong.size(); ++i) {
                if (ghePhong[i].getMaGhe() == maGhe) {
                    seatIndex = i;
                    break;
                }
            }
            if (seatIndex == ghePhong.size()) {
                cout << "Khong tim thay ghe " << maGhe << " trong phong.\n";
                continue;
            }

            cout << "1. Doi loai ghe\n";
            cout << "2. Chinh gia rieng\n";
            cout << "0. Quay lai\n";
            cout << "Chon: ";
            int chinhGhe;
            if (!(cin >> chinhGhe)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Lua chon khong hop le.\n";
                continue;
            }
            if (chinhGhe == 1) {
                string loaiMoi;
                double giaMoi;
                if (!chonLoaiGiaGhe(danhSachLoaiGhe, loaiMoi, giaMoi)) continue;

                string hang;
                int cotDau, cotCuoi;
                if (laLoaiGheDoi(loaiMoi)) {
                    const bool maGheHopLe =
                        phanTichMaGhe(maGhe, hang, cotDau, cotCuoi);
                    if (!maGheHopLe) {
                        cout << "Ma ghe khong co dang hang-so cot hop le.\n";
                        continue;
                    }
                    if (cotCuoi == cotDau + 1) {
                        ghePhong[seatIndex].setLoaiGhe(loaiMoi);
                        ghePhong[seatIndex].setDonGiaGhe(giaMoi);
                    } else if (cotCuoi > cotDau + 1) {
                        cout << "Ma ghe doi khong hop le: ghe doi phai gom dung hai vi tri lien tiep.\n";
                        continue;
                    } else {
                        if (gheDaBanHoacDangGiu(db, maPhong, maGhe)) {
                            cout << "Khong the gop ghe da ban hoac dang duoc giu.\n";
                            continue;
                        }
                        if (cotDau != cotCuoi) {
                            cout << "Ghe nay khong the gop thanh ghe doi.\n";
                            continue;
                        }
                        stringstream maGheKeTiep;
                        maGheKeTiep << hang << (cotDau + 1);
                        size_t partnerIndex = ghePhong.size();
                        for (size_t i = 0; i < ghePhong.size(); ++i) {
                            if (ghePhong[i].getMaGhe() == maGheKeTiep.str()) {
                                partnerIndex = i;
                                break;
                            }
                        }
                        if (partnerIndex == ghePhong.size()) {
                            cout << "Khong tim thay ghe lien tiep " << maGheKeTiep.str()
                                 << " de gop thanh ghe doi.\n";
                            continue;
                        }
                        if (gheDaBanHoacDangGiu(db, maPhong, maGheKeTiep.str())) {
                            cout << "Khong the gop ghe lien tiep da ban hoac dang duoc giu.\n";
                            continue;
                        }
                        stringstream maGheDoi;
                        maGheDoi << hang << cotDau << "-" << (cotDau + 1);
                        ghePhong[seatIndex] = Seat(
                            maGheDoi.str(), maPhong, loaiMoi, giaMoi, "Trong");
                        ghePhong.erase(ghePhong.begin() + partnerIndex);
                    }
                } else {
                    string maGheCu = ghePhong[seatIndex].getMaGhe();
                    if (phanTichMaGhe(maGheCu, hang, cotDau, cotCuoi) &&
                        cotCuoi > cotDau) {
                        if (cotCuoi != cotDau + 1) {
                            cout << "Ma ghe doi khong hop le: khong the tach so do nay.\n";
                            continue;
                        }
                        if (gheDaBanHoacDangGiu(db, maPhong, maGheCu)) {
                            cout << "Khong the tach ghe doi da ban hoac dang duoc giu.\n";
                            continue;
                        }
                        stringstream maGheDon, maGheDonThuHai;
                        maGheDon << hang << cotDau;
                        maGheDonThuHai << hang << cotCuoi;
                        size_t partnerIndex = ghePhong.size();
                        for (size_t i = 0; i < ghePhong.size(); ++i) {
                            if (i != seatIndex &&
                                (ghePhong[i].getMaGhe() == maGheDon.str() ||
                                 ghePhong[i].getMaGhe() == maGheDonThuHai.str())) {
                                cout << "Khong the tach: ma ghe sau khi tach da ton tai.\n";
                                partnerIndex = i;
                                break;
                            }
                        }
                        if (partnerIndex != ghePhong.size()) continue;
                        Seat gheTachThuHai(maGheDonThuHai.str(), maPhong, loaiMoi,
                                           giaMoi, "Trong");
                        ghePhong[seatIndex] = Seat(
                            maGheDon.str(), maPhong, loaiMoi, giaMoi, "Trong");
                        ghePhong.push_back(gheTachThuHai);
                    } else {
                        ghePhong[seatIndex].setLoaiGhe(loaiMoi);
                        ghePhong[seatIndex].setDonGiaGhe(giaMoi);
                    }
                }
                cout << "Da cap nhat loai ghe va gia theo danh muc.\n";
            } else if (chinhGhe == 2) {
                double giaMoi;
                cout << "Nhap gia rieng cho ghe (VND, > 0): ";
                if (!(cin >> giaMoi) || giaMoi <= 0) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Gia ghe phai la so lon hon 0.\n";
                } else {
                    ghePhong[seatIndex].setDonGiaGhe(giaMoi);
                    cout << "Da cap nhat gia ghe.\n";
                }
            } else if (chinhGhe != 0) {
                cout << "Lua chon khong hop le.\n";
            }
            continue;
        }
        if (luaChon == 3) {
            hienThiSoDoGhe(ghePhong, maPhong);
            cout << "Xac nhan luu so do nay? (y/n): ";
            char xacNhan;
            cin >> xacNhan;
            if (xacNhan != 'y' && xacNhan != 'Y') {
                cout << "Chua luu. Ban co the tiep tuc chinh sua.\n";
                continue;
            }

            vector<Seat> gheDeLuu = gheKhacPhong;
            gheDeLuu.insert(gheDeLuu.end(), ghePhong.begin(), ghePhong.end());
            if (!db.ghiSoDoGhe(gheDeLuu, danhSachLoaiGhe)) {
                cout << "Loi: Khong the luu so do ghe vao file.\n";
                continue;
            }
            taiDuLieu(db);
            cout << "Da luu so do ghe thanh cong.\n";
            return;
        }
        cout << "Lua chon khong hop le.\n";
    }

}

void SeatManager::saoChepBucLuc(const string& phongDich, const string& phongNguon, Database& db) {
    int count = 0;

    size_t n = danhSachGhe.size();
    for (size_t i = 0; i < n; ++i) {
        if (danhSachGhe[i].getMaPhong() == phongNguon) {
            string maGheMoi = danhSachGhe[i].getMaGhe();
            Seat copiedSeat(maGheMoi, phongDich, danhSachGhe[i].getLoaiGhe(), danhSachGhe[i].getDonGiaGhe(), "Trong");
            danhSachGhe.push_back(copiedSeat);
            count++;
        }
    }

    if (count > 0) {
        db.ghiDanhSachGhe(danhSachGhe);
        cout << "Da sao chep thanh cong " << count << " ghe tu phong " << phongNguon << " sang phong " << phongDich << "!\n";
    } else {
        cout << "Khong tim thay du lieu ghe cua phong nguon " << phongNguon << " de sao chep!\n";
    }
}

void SeatManager::hienThiSoDoPhong(Database& db) const {
    vector<Room> danhSachPhong;
    db.docDanhSachPhong(danhSachPhong);

    string maPhong;
    while (true) {
        cout << "Nhap ma phong can xem so do (nhap 0 de quay lai): ";
        if (!(cin >> maPhong)) {
            if (cin.eof()) return;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Loi: Ma phong khong hop le, vui long nhap lai.\n";
            continue;
        }
        if (maPhong == "0") return;

        bool tonTaiPhong = false;
        for (size_t i = 0; i < danhSachPhong.size(); ++i) {
            if (danhSachPhong[i].getMaPhong() == maPhong) {
                tonTaiPhong = true;
                break;
            }
        }
        if (tonTaiPhong) break;

        cout << "Khong tim thay phong co ma '" << maPhong
             << "'. Vui long nhap lai.\n";
    }

    cout << "\n==================== SO DO GHE PHONG CHIEU: " << maPhong << " ====================\n";
    cout << " [MAN HINH CHIEU PHIM]\n";
    cout << "-----------------------------------------------------------------------\n";

    bool found = false;
    vector<pair<string, double> > chuThichLoaiGia;

    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        const Seat& ghe = danhSachGhe[i];
        if (ghe.getMaPhong() != maPhong) continue;

        string trangThai = "[ ]";
        if (ghe.getTrangThai() == "DangChon" ||
            ghe.getTrangThai().find("DangChon|") == 0) {
            trangThai = "[?]";
        } else if (ghe.getTrangThai() == "DaDat") {
            trangThai = "[X]";
        } else if (ghe.getTrangThai() == "KhongSuDung") {
            trangThai = "[-]";
        }
        cout << left << setw(8) << (ghe.getMaGhe() + trangThai);
        found = true;

        bool daCoChuThich = false;
        for (size_t j = 0; j < chuThichLoaiGia.size(); ++j) {
            if (chuThichLoaiGia[j].first == ghe.getLoaiGhe() &&
                chuThichLoaiGia[j].second == ghe.getDonGiaGhe()) {
                daCoChuThich = true;
                break;
            }
        }
        if (!daCoChuThich) {
            chuThichLoaiGia.push_back(
                make_pair(ghe.getLoaiGhe(), ghe.getDonGiaGhe()));
        }
    }

    if (!found) {
        cout << "Phong nay chua duoc thiet lap so do ghe!\n";
    }
    cout << "\n-----------------------------------------------------------------------\n";
    cout << "Trang thai: [ ] Trong | [?] Dang chon | [X] Da dat | [-] Khong su dung\n";
    if (!chuThichLoaiGia.empty()) {
        cout << "Loai ghe va gia:\n";
        for (size_t i = 0; i < chuThichLoaiGia.size(); ++i) {
            cout << "  - " << chuThichLoaiGia[i].first << ": "
                 << fixed << setprecision(0) << chuThichLoaiGia[i].second
                 << " VND\n";
        }
    }
    cout << "=======================================================================\n";
}

void SeatManager::hienThiDanhSachLoaiGhe(Database& db) const {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    if (danhSachLoaiGhe.empty()) {
        cout << "Danh muc loai ghe dang trong!\n";
        return;
    }

    cout << "\n========== DANH MUC LOAI GHE ==========\n";
    cout << left << setw(6) << "STT" << setw(24) << "Loai ghe" << "Gia ban day du (VND)\n";
    for (size_t i = 0; i < danhSachLoaiGhe.size(); ++i) {
        cout << left << setw(6) << (i + 1) << setw(24) << danhSachLoaiGhe[i].first
             << fixed << setprecision(0) << danhSachLoaiGhe[i].second << "\n";
    }
    cout << "=======================================\n";
}

void SeatManager::themLoaiGhe(Database& db) {
    string tenLoaiGhe;
    double donGia;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten loai ghe moi: ";
    getline(cin, tenLoaiGhe);
    cout << "Nhap gia ban day du cho moi ghe (VND, > 0): ";
    if (!(cin >> donGia) || donGia <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Loi: Don gia phai la so lon hon 0!\n";
        return;
    }
    if (!db.themLoaiGhe(tenLoaiGhe, donGia)) {
        cout << "Loi: Ten loai ghe da ton tai, du lieu khong hop le hoac khong the luu file!\n";
        return;
    }
    cout << "Da them loai ghe thanh cong!\n";
}

void SeatManager::suaLoaiGhe(Database& db) {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    hienThiDanhSachLoaiGhe(db);
    if (danhSachLoaiGhe.empty()) return;

    size_t stt;
    cout << "Chon STT loai ghe can sua (0 de huy): ";
    if (!(cin >> stt) || stt > danhSachLoaiGhe.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Lua chon khong hop le!\n";
        return;
    }
    if (stt == 0) return;

    string tenMoi;
    double donGiaMoi;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Nhap ten loai ghe moi: ";
    getline(cin, tenMoi);
    cout << "Nhap gia ban day du moi (VND, > 0): ";
    if (!(cin >> donGiaMoi) || donGiaMoi <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Loi: Don gia phai la so lon hon 0!\n";
        return;
    }
    if (!db.suaLoaiGhe(danhSachLoaiGhe[stt - 1].first, tenMoi, donGiaMoi)) {
        cout << "Loi: Ten loai ghe bi trung, khong ton tai hoac khong the luu file!\n";
        return;
    }
    taiDuLieu(db);
    cout << "Da cap nhat loai ghe va gia ghe dang su dung!\n";
}

void SeatManager::xoaLoaiGhe(Database& db) {
    vector<pair<string, double> > danhSachLoaiGhe;
    db.docDanhSachLoaiGhe(danhSachLoaiGhe);
    hienThiDanhSachLoaiGhe(db);
    if (danhSachLoaiGhe.empty()) return;

    size_t stt;
    cout << "Chon STT loai ghe can xoa (0 de huy): ";
    if (!(cin >> stt) || stt > danhSachLoaiGhe.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Lua chon khong hop le!\n";
        return;
    }
    if (stt == 0) return;
    if (!db.xoaLoaiGhe(danhSachLoaiGhe[stt - 1].first)) {
        cout << "Loi: Khong tim thay loai ghe, khong the luu, hoac van con ghe dang su dung loai nay!\n";
        return;
    }
    cout << "Da xoa loai ghe!\n";
}

void SeatManager::chonGheTamThoi(const string& maGhe, const string& maPhong, Database& db) {
    bool found = false;

    for (size_t i = 0; i < danhSachGhe.size(); ++i) {
        if (danhSachGhe[i].getMaPhong() == maPhong && danhSachGhe[i].getMaGhe() == maGhe) {
            found = true;
            if (danhSachGhe[i].getTrangThai() == "DaDat") {
                cout << "Ghe " << maGhe << " da co khach dat! Vui long chon ghe khac.\n";
                return;
            }
            if (danhSachGhe[i].getTrangThai() == "DangChon") {
                cout << "Ghe " << maGhe << " dang duoc giu cho tam thoi o quay khac!\n";
                return;
            }
            danhSachGhe[i].setTrangThai("DangChon");
            db.ghiDanhSachGhe(danhSachGhe);
            cout << "Da giu cho thanh cong ghe " << maGhe << " (Khoa tam 15 phut).\n";
            return;
        }
    }

    if (!found) {
        cout << "Khong tim thay ghe " << maGhe << " trong phong nay!\n";
    }
}
