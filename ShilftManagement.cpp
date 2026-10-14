#include "ShilftManagement.h"
#include "Database.h"
#include <iostream>
#include <cstdlib>

// =====================================================
// HAM CHUYEN NGAY DD/MM/YYYY SANG SO
// DUNG DE SO SANH NGAY
// =====================================================

static int chuyenNgayThanhSo(string ngay)
{
    if (ngay.length() != 10)
        return 0;

    if (ngay[2] != '/' || ngay[5] != '/')
        return 0;

    try
    {
        int ngayInt = atoi(ngay.substr(0, 2).c_str());
        int thangInt = atoi(ngay.substr(3, 2).c_str());
        int namInt = atoi(ngay.substr(6, 4).c_str());

        return namInt * 10000
             + thangInt * 100
             + ngayInt;
    }
    catch (...)
    {
        return 0;
    }
}


// =====================================================
// KIEM TRA NGAY CO NAM TRONG KHOANG KHONG
// =====================================================

static bool trongKhoangThoiGian(
    string ngay,
    string tuNgay,
    string denNgay)
{
    int ngayHienTai = chuyenNgayThanhSo(ngay);
    int ngayBatDau = chuyenNgayThanhSo(tuNgay);
    int ngayKetThuc = chuyenNgayThanhSo(denNgay);

    if (ngayHienTai == 0 ||
        ngayBatDau == 0 ||
        ngayKetThuc == 0)
    {
        return false;
    }

    return ngayHienTai >= ngayBatDau &&
           ngayHienTai <= ngayKetThuc;
}


// =====================================================
// SHIFT REPORT
// =====================================================

ShiftReport::ShiftReport()
{
    maCa = "";
    maNhanVien = "";
    tenNhanVien = "";

    ngayLam = "";
    gioBatDau = "";
    gioKetThuc = "";

    doanhThuHeThong = 0;
    tienThucTe = 0;
    chenhLech = 0;

    trangThai = "ChoDuyet";

    daKhoaBaoCao = false;
}

ShiftReport::ShiftReport(
    string maCa,
    string maNhanVien,
    string tenNhanVien,
    string ngayLam,
    string gioBatDau,
    string gioKetThuc,
    long long doanhThuHeThong,
    long long tienThucTe,
    string trangThai,
    bool daKhoaBaoCao)
{
    this->maCa = maCa;
    this->maNhanVien = maNhanVien;
    this->tenNhanVien = tenNhanVien;

    this->ngayLam = ngayLam;
    this->gioBatDau = gioBatDau;
    this->gioKetThuc = gioKetThuc;

    this->doanhThuHeThong = doanhThuHeThong;
    this->tienThucTe = tienThucTe;

    this->trangThai = trangThai;
    this->daKhoaBaoCao = daKhoaBaoCao;

    tinhChenhLech();
}


// =====================================================
// GETTER
// =====================================================

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


// =====================================================
// CAP NHAT TIEN THUC TE
// =====================================================

void ShiftReport::setTienThucTe(long long tienThucTe)
{
    if (daKhoaBaoCao)
    {
        cout << "Bao cao da bi khoa, khong the chinh sua!" << endl;
        return;
    }

    if (tienThucTe < 0)
    {
        cout << "Tien thuc te khong hop le!" << endl;
        return;
    }

    this->tienThucTe = tienThucTe;
    tinhChenhLech();
}


// =====================================================
// TINH CHENH LECH
// =====================================================

void ShiftReport::tinhChenhLech()
{
    chenhLech = tienThucTe - doanhThuHeThong;
}


// =====================================================
// DUYET CA
// =====================================================

void ShiftReport::duyetCa()
{
    if (daKhoaBaoCao)
    {
        cout << "Bao cao da khoa, khong the thay doi!" << endl;
        return;
    }

    trangThai = "DaDuyet";
}


// =====================================================
// KHOA BAO CAO
// =====================================================

void ShiftReport::khoaBaoCao()
{
    daKhoaBaoCao = true;
}


// =====================================================
// HIEN THI BAO CAO
// =====================================================

void ShiftReport::hienThi() const
{
    cout << endl;
    cout << "==================================================" << endl;
    cout << "                    BAO CAO CA                    " << endl;
    cout << "==================================================" << endl;

    cout << "Ma ca              : " << maCa << endl;
    cout << "Ma nhan vien       : " << maNhanVien << endl;
    cout << "Ten nhan vien      : " << tenNhanVien << endl;
    cout << "Ngay lam           : " << ngayLam << endl;
    cout << "Gio bat dau        : " << gioBatDau << endl;
    cout << "Gio ket thuc       : " << gioKetThuc << endl;
    cout << "Doanh thu he thong : " << doanhThuHeThong << " VND" << endl;
    cout << "Tien thuc te       : " << tienThucTe << " VND" << endl;
    cout << "Chenh lech         : " << chenhLech << " VND";

    if (chenhLech < 0) cout << " (THIEU TIEN)";
    else if (chenhLech > 0) cout << " (THUA TIEN)";
    else cout << " (KHOP)";

    cout << endl;
    cout << "Trang thai         : " << trangThai << endl;
    cout << "Bao cao            : " << (daKhoaBaoCao ? "DA KHOA" : "CHUA KHOA") << endl;
    cout << "==================================================" << endl;
}


// =====================================================
// SHIFT MANAGEMENT & DATABASE SYNC
// =====================================================

ShiftManagement::ShiftManagement()
{
    danhSachCa.clear();
}

void ShiftManagement::taiDuLieu(Database& db)
{
    db.docDanhSachCa(danhSachCa);
}

void ShiftManagement::luuDuLieu(Database& db) const
{
    db.ghiDanhSachCa(danhSachCa);
}


// =====================================================
// THEM BAO CAO CA
// =====================================================

void ShiftManagement::themBaoCaoCa(ShiftReport ca)
{
    danhSachCa.push_back(ca);
}


// =====================================================
// CHOT CA (T? d?ng luu file)
// =====================================================

void ShiftManagement::chotCa(
    string maCa,
    string maNhanVien,
    string tenNhanVien,
    string ngayLam,
    string gioBatDau,
    string gioKetThuc,
    long long doanhThuHeThong,
    long long tienThucTe,
    Database& db) // B? sung tham s? Database d? g?i luu tr?c ti?p
{
    ShiftReport ca(
        maCa,
        maNhanVien,
        tenNhanVien,
        ngayLam,
        gioBatDau,
        gioKetThuc,
        doanhThuHeThong,
        tienThucTe,
        "ChoDuyet",
        false
    );

    danhSachCa.push_back(ca);

    // T? d?ng d?ng b? luu xu?ng file van_hanh_he_thong.csv
    luuDuLieu(db);

    cout << endl;
    cout << "==============================================" << endl;
    cout << "             CHOT CA THANH CONG               " << endl;
    cout << "==============================================" << endl;
    cout << "Ma ca: " << maCa << endl;
    cout << "Nhan vien: " << tenNhanVien << endl;
    cout << "Ngay: " << ngayLam << endl;
    cout << "Bao cao dang cho Admin duyet va da duoc luu vao he thong." << endl;
}


// =====================================================
// TIM CA
// =====================================================

ShiftReport* ShiftManagement::timCa(string maCa)
{
    for (vector<ShiftReport>::iterator it = danhSachCa.begin();
         it != danhSachCa.end();
         ++it)
    {
        if (it->getMaCa() == maCa)
        {
            return &(*it);
        }
    }
    return NULL;
}


// =====================================================
// DUYET BAO CAO (T? d?ng luu file)
// =====================================================

bool ShiftManagement::duyetBaoCao(string maCa, Database& db)
{
    ShiftReport* ca = timCa(maCa);

    if (ca == NULL)
    {
        cout << "Khong tim thay ma ca!" << endl;
        return false;
    }

    if (ca->getDaKhoaBaoCao())
    {
        cout << "Bao cao da khoa, khong the duyet!" << endl;
        return false;
    }

    if (ca->getTrangThai() == "DaDuyet")
    {
        cout << "Ca nay da duoc duyet!" << endl;
        return false;
    }

    ca->duyetCa();

    // T? d?ng d?ng b? luu xu?ng file
    luuDuLieu(db);

    cout << "Duyet bao cao thanh cong va da luu file!" << endl;
    return true;
}


// =====================================================
// KHOA BAO CAO (T? d?ng luu file)
// =====================================================

bool ShiftManagement::khoaBaoCao(string maCa, Database& db)
{
    ShiftReport* ca = timCa(maCa);

    if (ca == NULL)
    {
        cout << "Khong tim thay ma ca!" << endl;
        return false;
    }

    if (ca->getDaKhoaBaoCao())
    {
        cout << "Bao cao nay da duoc khoa!" << endl;
        return false;
    }

    ca->khoaBaoCao();

    // T? d?ng d?ng b? luu xu?ng file
    luuDuLieu(db);

    cout << "Khoa bao cao thanh cong va da luu file!" << endl;
    return true;
}


// =====================================================
// CÁC HÀM L?C VÀ HI?N TH? (Gi? nguyên logic cu)
// =====================================================

vector<ShiftReport> ShiftManagement::locTheoThoiGian(string tuNgay, string denNgay) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        if (trongKhoangThoiGian(ca.getNgayLam(), tuNgay, denNgay))
        {
            ketQua.push_back(ca);
        }
    }
    return ketQua;
}

vector<ShiftReport> ShiftManagement::locTheoNhanVien(string maNhanVien) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        if (ca.getMaNhanVien() == maNhanVien)
        {
            ketQua.push_back(ca);
        }
    }
    return ketQua;
}

vector<ShiftReport> ShiftManagement::locBaoCao(string tuNgay, string denNgay, string maNhanVien) const
{
    vector<ShiftReport> ketQua;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        bool dungThoiGian = trongKhoangThoiGian(ca.getNgayLam(), tuNgay, denNgay);
        bool dungNhanVien = maNhanVien.empty() || ca.getMaNhanVien() == maNhanVien;

        if (dungThoiGian && dungNhanVien)
        {
            ketQua.push_back(ca);
        }
    }
    return ketQua;
}

void ShiftManagement::hienThiBaoCaoNhanVien(string maNhanVien, string ngayLam) const
{
    bool timThay = false;
    cout << endl;
    cout << "==================================================" << endl;
    cout << "          BAO CAO CHOT CA NHAN VIEN               " << endl;
    cout << "==================================================" << endl;
    cout << "Ma nhan vien: " << maNhanVien << endl;
    cout << "Ngay: " << ngayLam << endl;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        if (ca.getMaNhanVien() == maNhanVien && ca.getNgayLam() == ngayLam)
        {
            ca.hienThi();
            timThay = true;
        }
    }

    if (!timThay)
    {
        cout << "Khong tim thay bao cao ca cua nhan vien!" << endl;
    }
}

void ShiftManagement::hienThiCaChoDuyet() const
{
    bool timThay = false;
    cout << endl;
    cout << "==================================================" << endl;
    cout << "                     CA CHO DUYET                 " << endl;
    cout << "==================================================" << endl;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        if (ca.getTrangThai() == "ChoDuyet")
        {
            ca.hienThi();
            timThay = true;
        }
    }

    if (!timThay)
    {
        cout << "Khong co ca nao dang cho duyet." << endl;
    }
}

void ShiftManagement::hienThiCaDaXuLy() const
{
    bool timThay = false;
    cout << endl;
    cout << "==================================================" << endl;
    cout << "                      CA DA XU LY                 " << endl;
    cout << "==================================================" << endl;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        const ShiftReport& ca = danhSachCa[i];
        if (ca.getTrangThai() == "DaDuyet")
        {
            ca.hienThi();
            timThay = true;
        }
    }

    if (!timThay)
    {
        cout << "Chua co ca nao duoc duyet." << endl;
    }
}

long long ShiftManagement::tinhTongDoanhThu() const
{
    long long tong = 0;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        tong += danhSachCa[i].getDoanhThuHeThong();
    }
    return tong;
}

long long ShiftManagement::tinhTongChenhLech() const
{
    long long tong = 0;
    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        tong += danhSachCa[i].getChenhLech();
    }
    return tong;
}

void ShiftManagement::hienThiTongHop() const
{
    int soCaChoDuyet = 0;
    int soCaDaXuLy = 0;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        if (danhSachCa[i].getTrangThai() == "ChoDuyet") soCaChoDuyet++;
        if (danhSachCa[i].getTrangThai() == "DaDuyet") soCaDaXuLy++;
    }

    long long tongChenhLech = tinhTongChenhLech();

    cout << endl;
    cout << "==================================================" << endl;
    cout << "                 TONG HOP BAO CAO CA              " << endl;
    cout << "==================================================" << endl;
    cout << "Tong so ca       : " << danhSachCa.size() << endl;
    cout << "Ca cho duyet     : " << soCaChoDuyet << endl;
    cout << "Ca da xu ly      : " << soCaDaXuLy << endl;
    cout << "Tong doanh thu   : " << tinhTongDoanhThu() << " VND" << endl;
    cout << "Tong chenh lech  : " << tongChenhLech << " VND";

    if (tongChenhLech < 0) cout << " (THIEU TIEN)";
    else if (tongChenhLech > 0) cout << " (THUA TIEN)";
    else cout << " (KHOP)";

    cout << endl;
    cout << "==================================================" << endl;
}

void ShiftManagement::hienThiTatCa() const
{
    if (danhSachCa.empty())
    {
        cout << "Chua co bao cao ca nao." << endl;
        return;
    }

    for (size_t i = 0; i < danhSachCa.size(); ++i)
    {
        danhSachCa[i].hienThi();
    }
}
