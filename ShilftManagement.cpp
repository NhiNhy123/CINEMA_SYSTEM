#include "../include/ShilftManagement.h"
#include "../include/Database.h"
#include "../include/User.h"
#include "../include/DateFormat.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>

static int chuyenNgayThanhSo(string ngay)
{
    return DateFormat::dateKey(ngay);
}

static bool trongKhoangThoiGian(string ngay, string tuNgay, string denNgay)
{
    int ngayHienTai = chuyenNgayThanhSo(ngay);
    int ngayBatDau = chuyenNgayThanhSo(tuNgay);
    int ngayKetThuc = chuyenNgayThanhSo(denNgay);

    if (ngayHienTai == 0 || ngayBatDau == 0 || ngayKetThuc == 0)
        return false;

    return ngayHienTai >= ngayBatDau && ngayHienTai <= ngayKetThuc;
}

ShiftReport::ShiftReport()
{
    maCa = maNhanVien = tenNhanVien = ngayLam = gioBatDau = gioKetThuc = "";
    doanhThuHeThong = tienThucTe = chenhLech = 0;
    trangThai = "ChoDuyet";
    daKhoaBaoCao = false;
}

ShiftReport::ShiftReport(string maCa, string maNhanVien, string tenNhanVien, string ngayLam, string gioBatDau, string gioKetThuc, long long doanhThuHeThong, long long tienThucTe, string trangThai, bool daKhoaBaoCao)
{
    this->maCa = maCa;
    this->maNhanVien = maNhanVien;
    this->tenNhanVien = tenNhanVien;
    this->ngayLam = DateFormat::normalizeDate(ngayLam);
    this->gioBatDau = gioBatDau;
    this->gioKetThuc = gioKetThuc;
    this->doanhThuHeThong = doanhThuHeThong;
    this->tienThucTe = tienThucTe;
    this->trangThai = trangThai;
    this->daKhoaBaoCao = daKhoaBaoCao;
    tinhChenhLech();
}

string ShiftReport::getMaCa() const { return maCa; }
string ShiftReport::getMaNhanVien() const { return maNhanVien; }
string ShiftReport::getTenNhanVien() const { return tenNhanVien; }
string ShiftReport::getNgayLam() const { return ngayLam; }
string ShiftReport::getGioBatDau() const { return gioBatDau; }
string ShiftReport::getGioKetThuc() const { return gioKetThuc; }
long long ShiftReport::getDoanhThuHeThong() const { return doanhThuHeThong; }
long long ShiftReport::getTienThucTe() const { return tienThucTe; }
long long ShiftReport::getChenhLech() const { return chenhLech; }
string ShiftReport::getTrangThai() const { return trangThai; }
bool ShiftReport::getDaKhoaBaoCao() const { return daKhoaBaoCao; }

void ShiftReport::setTienThucTe(long long tienThucTe)
{
    if (daKhoaBaoCao) { cout << "Bao cao da bi khoa, khong the chinh sua!\n"; return; }
    if (tienThucTe < 0) { cout << "Tien thuc te khong hop le!\n"; return; }
    this->tienThucTe = tienThucTe;
    tinhChenhLech();
}

void ShiftReport::tinhChenhLech() { chenhLech = tienThucTe - doanhThuHeThong; }
void ShiftReport::duyetCa() { if (!daKhoaBaoCao) trangThai = "DaDuyet"; }
void ShiftReport::khoaBaoCao() { daKhoaBaoCao = true; }

void ShiftReport::hienThi() const
{
    cout << "\n==================================================\n";
    cout << "                    BAO CAO CA                    \n";
    cout << "==================================================\n";
    cout << "Ma ca              : " << maCa << "\n";
    cout << "Ma nhan vien       : " << maNhanVien << "\n";
    cout << "Ten nhan vien      : " << tenNhanVien << "\n";
    cout << "Ngay lam           : " << ngayLam << "\n";
    cout << "Gio bat dau        : " << gioBatDau << "\n";
    cout << "Gio ket thuc       : " << gioKetThuc << "\n";
    cout << "Doanh thu he thong : " << doanhThuHeThong << " VND\n";
    cout << "Tien thuc te       : " << tienThucTe << " VND\n";
    cout << "Chenh lech         : " << chenhLech << " VND";

    if (chenhLech < 0) cout << " (THIEU TIEN)\n";
    else if (chenhLech > 0) cout << " (THUA TIEN)\n";
    else cout << " (KHOP)\n";

    cout << "Trang thai         : " << trangThai << "\n";
    cout << "Bao cao            : " << (daKhoaBaoCao ? "DA KHOA" : "CHUA KHOA") << "\n";
    cout << "==================================================\n";
}

ShiftManagement::ShiftManagement() { danhSachCa.clear(); }
void ShiftManagement::taiDuLieu(Database& db) { db.docDanhSachCa(danhSachCa); }
void ShiftManagement::luuDuLieu(Database& db) const { db.ghiDanhSachCa(danhSachCa); }
void ShiftManagement::themBaoCaoCa(ShiftReport ca) { danhSachCa.push_back(ca); }

void ShiftManagement::chotCa(string maCa, string maNhanVien, string gioBatDau, long long doanhThuHeThong, long long tienThucTe, Database& db)
{

    string tenNhanVien = db.layTenNhanVienQuaMa(maNhanVien);

    time_t now = time(0);
    tm* ltm = localtime(&now);

    stringstream ssDate;
    ssDate << setfill('0') << setw(2) << ltm->tm_mday << "/"
           << setfill('0') << setw(2) << (1 + ltm->tm_mon) << "/"
           << (1900 + ltm->tm_year);
    string ngayHienTai = ssDate.str();

    char bufTime[80];
    strftime(bufTime, sizeof(bufTime), "%H:%M:%S", ltm);

    ShiftReport ca(maCa, maNhanVien, tenNhanVien, ngayHienTai, gioBatDau, string(bufTime), doanhThuHeThong, tienThucTe, "ChoDuyet", false);
    danhSachCa.push_back(ca);
    luuDuLieu(db);

    cout << "\n==============================================\n";
    cout << "               CHOT CA THANH CONG             \n";
    cout << "==============================================\n";
    cout << "Ma ca      : " << maCa << "\n";
    cout << "Nhan vien  : " << tenNhanVien << " (" << maNhanVien << ")\n";
    cout << "Ngay lam   : " << ngayHienTai << "\n";
    cout << "Thoi gian  : " << gioBatDau << " -> " << bufTime << "\n";
    cout << "Bao cao dang cho Admin duyet va da duoc luu vao he thong.\n";
}

ShiftReport* ShiftManagement::timCa(string maCa)
{
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        if (danhSachCa[i].getMaCa() == maCa) return &danhSachCa[i];
    }
    return NULL;
}

bool ShiftManagement::duyetBaoCao(string maCa, Database& db)
{
    ShiftReport* ca = timCa(maCa);
    if (!ca || ca->getDaKhoaBaoCao() || ca->getTrangThai() == "DaDuyet") return false;
    ca->duyetCa();
    luuDuLieu(db);
    cout << "Duyet bao cao thanh cong va da luu file!\n";
    return true;
}

bool ShiftManagement::khoaBaoCao(string maCa, Database& db)
{
    ShiftReport* ca = timCa(maCa);
    if (!ca || ca->getDaKhoaBaoCao()) return false;
    ca->khoaBaoCao();
    luuDuLieu(db);
    cout << "Khoa bao cao thanh cong va da luu file!\n";
    return true;
}

vector<ShiftReport> ShiftManagement::locTheoThoiGian(string tuNgay, string denNgay) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
        if (trongKhoangThoiGian(danhSachCa[i].getNgayLam(), tuNgay, denNgay))
            ketQua.push_back(danhSachCa[i]);
    return ketQua;
}

vector<ShiftReport> ShiftManagement::locTheoNhanVien(string maNhanVien) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
        if (danhSachCa[i].getMaNhanVien() == maNhanVien)
            ketQua.push_back(danhSachCa[i]);
    return ketQua;
}

vector<ShiftReport> ShiftManagement::locBaoCao(string tuNgay, string denNgay, string maNhanVien) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        if (trongKhoangThoiGian(danhSachCa[i].getNgayLam(), tuNgay, denNgay) && (maNhanVien.empty() || danhSachCa[i].getMaNhanVien() == maNhanVien))
            ketQua.push_back(danhSachCa[i]);
    }
    return ketQua;
}

void ShiftManagement::hienThiBaoCaoNhanVien(string maNhanVien, string ngayLam) const
{
    bool timThay = false;
    cout << "\n==================================================\n";
    cout << "          BAO CAO CHOT CA NHAN VIEN               \n";
    cout << "==================================================\n";
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        if (danhSachCa[i].getMaNhanVien() == maNhanVien && danhSachCa[i].getNgayLam() == ngayLam)
        {
            danhSachCa[i].hienThi();
            timThay = true;
        }
    }
    if (!timThay) cout << "Khong tim thay bao cao ca cua nhan vien!\n";
}

void ShiftManagement::hienThiCaChoDuyet() const
{
    for (size_t i = 0; i < danhSachCa.size(); ++i)
        if (danhSachCa[i].getTrangThai() == "ChoDuyet") danhSachCa[i].hienThi();
}

void ShiftManagement::hienThiCaDaXuLy() const
{
    for (size_t i = 0; i < danhSachCa.size(); ++i)
        if (danhSachCa[i].getTrangThai() == "DaDuyet") danhSachCa[i].hienThi();
}

long long ShiftManagement::tinhTongDoanhThu() const
{
    long long tong = 0;
    for (size_t i = 0; i < danhSachCa.size(); ++i) tong += danhSachCa[i].getDoanhThuHeThong();
    return tong;
}

long long ShiftManagement::tinhTongChenhLech() const
{
    long long tong = 0;
    for (size_t i = 0; i < danhSachCa.size(); ++i) tong += danhSachCa[i].getChenhLech();
    return tong;
}

void ShiftManagement::hienThiTongHop() const
{
    int choDuyet = 0, daXuLy = 0;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        if (danhSachCa[i].getTrangThai() == "ChoDuyet") choDuyet++;
        if (danhSachCa[i].getTrangThai() == "DaDuyet") daXuLy++;
    }
    cout << "\n==================================================\n";
    cout << "                 TONG HOP BAO CAO CA              \n";
    cout << "==================================================\n";
    cout << "Tong so ca       : " << danhSachCa.size() << "\n";
    cout << "Ca cho duyet     : " << choDuyet << "\n";
    cout << "Ca da xu ly      : " << daXuLy << "\n";
    cout << "Tong doanh thu   : " << tinhTongDoanhThu() << " VND\n";
    cout << "Tong chenh lech  : " << tinhTongChenhLech() << " VND\n";
    cout << "==================================================\n";
}

void ShiftManagement::hienThiTatCa() const
{
    if (danhSachCa.empty()) { cout << "Chua co bao cao ca nao.\n"; return; }
    for (size_t i = 0; i < danhSachCa.size(); ++i) danhSachCa[i].hienThi();
}
