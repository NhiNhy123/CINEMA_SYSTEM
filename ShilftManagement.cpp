#include "ShilftManagement.h"
#include <iostream>
#include <cstdlib>

// =====================================================
// HAM CHUYEN NGAY DD/MM/YYYY SANG SO
// DUNG DE SO SANH NGAY
// =====================================================

static int chuyenNgayThanhSo(string ngay)
{
    // Vi du:
    // 26/09/2026
    // ngay = 26
    // thang = 09
    // nam = 2026
    //
    // Ket qua:
    // 20260926

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

// Ham dung mac dinh
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


// Ham dung day du
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

string ShiftReport::getMaCa() const
{
    return maCa;
}


string ShiftReport::getMaNhanVien() const
{
    return maNhanVien;
}


string ShiftReport::getTenNhanVien() const
{
    return tenNhanVien;
}


string ShiftReport::getNgayLam() const
{
    return ngayLam;
}


string ShiftReport::getGioBatDau() const
{
    return gioBatDau;
}


string ShiftReport::getGioKetThuc() const
{
    return gioKetThuc;
}


long long ShiftReport::getDoanhThuHeThong() const
{
    return doanhThuHeThong;
}


long long ShiftReport::getTienThucTe() const
{
    return tienThucTe;
}


long long ShiftReport::getChenhLech() const
{
    return chenhLech;
}


string ShiftReport::getTrangThai() const
{
    return trangThai;
}


bool ShiftReport::getDaKhoaBaoCao() const
{
    return daKhoaBaoCao;
}


// =====================================================
// CAP NHAT TIEN THUC TE
// =====================================================

void ShiftReport::setTienThucTe(long long tienThucTe)
{
    // Neu bao cao da khoa thi khong duoc sua
    if (daKhoaBaoCao)
    {
        cout << "Bao cao da bi khoa, khong the chinh sua!"
             << endl;

        return;
    }

    if (tienThucTe < 0)
    {
        cout << "Tien thuc te khong hop le!"
             << endl;

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
    // Chenh lech =
    // Tien thuc te - Doanh thu he thong

    chenhLech = tienThucTe - doanhThuHeThong;
}


// =====================================================
// DUYET CA
// =====================================================

void ShiftReport::duyetCa()
{
    if (daKhoaBaoCao)
    {
        cout << "Bao cao da khoa, khong the thay doi!"
             << endl;

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
    cout << "                 BAO CAO CA                        " << endl;
    cout << "==================================================" << endl;

    cout << "Ma ca              : " << maCa << endl;
    cout << "Ma nhan vien       : " << maNhanVien << endl;
    cout << "Ten nhan vien      : " << tenNhanVien << endl;

    cout << "Ngay lam            : " << ngayLam << endl;
    cout << "Gio bat dau         : " << gioBatDau << endl;
    cout << "Gio ket thuc        : " << gioKetThuc << endl;

    cout << "Doanh thu he thong  : "
         << doanhThuHeThong
         << " VND" << endl;

    cout << "Tien thuc te        : "
         << tienThucTe
         << " VND" << endl;

    cout << "Chenh lech          : "
         << chenhLech
         << " VND";

    if (chenhLech < 0)
    {
        cout << " (THIEU TIEN)";
    }
    else if (chenhLech > 0)
    {
        cout << " (THUA TIEN)";
    }
    else
    {
        cout << " (KHOP)";
    }

    cout << endl;

    cout << "Trang thai          : "
         << trangThai << endl;

    cout << "Bao cao             : ";

    if (daKhoaBaoCao)
        cout << "DA KHOA";
    else
        cout << "CHUA KHOA";

    cout << endl;

    cout << "==================================================" << endl;
}


// =====================================================
// SHIFT MANAGEMENT
// =====================================================

ShiftManagement::ShiftManagement()
{
    danhSachCa.clear();
}


// =====================================================
// THEM BAO CAO CA
// =====================================================

void ShiftManagement::themBaoCaoCa(ShiftReport ca)
{
    danhSachCa.push_back(ca);
}


// =====================================================
// CHOT CA
// =====================================================

void ShiftManagement::chotCa(
    string maCa,
    string maNhanVien,
    string tenNhanVien,
    string ngayLam,
    string gioBatDau,
    string gioKetThuc,
    long long doanhThuHeThong,
    long long tienThucTe)
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

    cout << endl;
    cout << "==============================================" << endl;
    cout << "           CHOT CA THANH CONG                " << endl;
    cout << "==============================================" << endl;

    cout << "Ma ca: " << maCa << endl;
    cout << "Nhan vien: " << tenNhanVien << endl;
    cout << "Ngay: " << ngayLam << endl;

    cout << "Bao cao dang cho Admin duyet."
         << endl;
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
// DUYET BAO CAO
// =====================================================

bool ShiftManagement::duyetBaoCao(string maCa)
{
    ShiftReport* ca = timCa(maCa);

    if (ca == NULL)
    {
        cout << "Khong tim thay ma ca!"
             << endl;

        return false;
    }

    if (ca->getDaKhoaBaoCao())
    {
        cout << "Bao cao da khoa, khong the duyet!"
             << endl;

        return false;
    }

    if (ca->getTrangThai() == "DaDuyet")
    {
        cout << "Ca nay da duoc duyet!"
             << endl;

        return false;
    }

    ca->duyetCa();

    cout << "Duyet bao cao thanh cong!"
         << endl;

    return true;
}


// =====================================================
// KHOA BAO CAO
// =====================================================

bool ShiftManagement::khoaBaoCao(string maCa)
{
    ShiftReport* ca = timCa(maCa);

    if (ca == NULL)
    {
        cout << "Khong tim thay ma ca!"
             << endl;

        return false;
    }

    if (ca->getDaKhoaBaoCao())
    {
        cout << "Bao cao nay da duoc khoa!"
             << endl;

        return false;
    }

    ca->khoaBaoCao();

    cout << "Khoa bao cao thanh cong!"
         << endl;

    return true;
}


// =====================================================
// LOC THEO THOI GIAN
// =====================================================

vector<ShiftReport> ShiftManagement::locTheoThoiGian(
    string tuNgay,
    string denNgay) const
{
    vector<ShiftReport> ketQua;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    if (trongKhoangThoiGian(
            ca.getNgayLam(),
            tuNgay,
            denNgay))
    {
        ketQua.push_back(ca);
    }
}


    return ketQua;
}


// =====================================================
// LOC THEO NHAN VIEN
// =====================================================

vector<ShiftReport> ShiftManagement::locTheoNhanVien(
    string maNhanVien) const
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


// =====================================================
// LOC THEO THOI GIAN + NHAN VIEN
// =====================================================

vector<ShiftReport> ShiftManagement::locBaoCao(
    string tuNgay,
    string denNgay,
    string maNhanVien) const
{
    vector<ShiftReport> ketQua;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    bool dungThoiGian =
        trongKhoangThoiGian(
            ca.getNgayLam(),
            tuNgay,
            denNgay
        );

    bool dungNhanVien =
        maNhanVien.empty() ||
        ca.getMaNhanVien() == maNhanVien;

    if (dungThoiGian && dungNhanVien)
    {
        ketQua.push_back(ca);
    }
}


    return ketQua;
}


// =====================================================
// HIEN THI BAO CAO NHAN VIEN TRONG NGAY
// =====================================================

void ShiftManagement::hienThiBaoCaoNhanVien(
    string maNhanVien,
    string ngayLam) const
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

    if (ca.getMaNhanVien() == maNhanVien &&
        ca.getNgayLam() == ngayLam)
    {
        ca.hienThi();

        timThay = true;
    }
}


    if (!timThay)
    {
        cout << "Khong tim thay bao cao ca cua nhan vien!"
             << endl;
    }
}


// =====================================================
// HIEN THI CA CHO DUYET
// =====================================================

void ShiftManagement::hienThiCaChoDuyet() const
{
    bool timThay = false;

    cout << endl;
    cout << "==================================================" << endl;
    cout << "                 CA CHO DUYET                      " << endl;
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
        cout << "Khong co ca nao dang cho duyet."
             << endl;
    }
}


// =====================================================
// HIEN THI CA DA XU LY
// =====================================================

void ShiftManagement::hienThiCaDaXuLy() const
{
    bool timThay = false;

    cout << endl;
    cout << "==================================================" << endl;
    cout << "                  CA DA XU LY                      " << endl;
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
        cout << "Chua co ca nao duoc duyet."
             << endl;
    }
}


// =====================================================
// TINH TONG DOANH THU
// =====================================================

long long ShiftManagement::tinhTongDoanhThu() const
{
    long long tong = 0;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    tong += ca.getDoanhThuHeThong();
}


    return tong;
}


// =====================================================
// TINH TONG CHENH LECH
// =====================================================

long long ShiftManagement::tinhTongChenhLech() const
{
    long long tong = 0;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    tong += ca.getChenhLech();
}


    return tong;
}


// =====================================================
// HIEN THI TONG HOP
// =====================================================

void ShiftManagement::hienThiTongHop() const
{
    int soCaChoDuyet = 0;
    int soCaDaXuLy = 0;

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    if (ca.getTrangThai() == "ChoDuyet")
    {
        soCaChoDuyet++;
    }

    if (ca.getTrangThai() == "DaDuyet")
    {
        soCaDaXuLy++;
    }
}


    long long tongChenhLech =
        tinhTongChenhLech();

    cout << endl;

    cout << "==================================================" << endl;
    cout << "              TONG HOP BAO CAO CA                 " << endl;
    cout << "==================================================" << endl;

    cout << "Tong so ca       : "
         << danhSachCa.size() << endl;

    cout << "Ca cho duyet     : "
         << soCaChoDuyet << endl;

    cout << "Ca da xu ly      : "
         << soCaDaXuLy << endl;

    cout << "Tong doanh thu   : "
         << tinhTongDoanhThu()
         << " VND" << endl;

    cout << "Tong chenh lech  : "
         << tongChenhLech
         << " VND";

    if (tongChenhLech < 0)
    {
        cout << " (THIEU TIEN)";
    }
    else if (tongChenhLech > 0)
    {
        cout << " (THUA TIEN)";
    }
    else
    {
        cout << " (KHOP)";
    }

    cout << endl;

    cout << "==================================================" << endl;
}


// =====================================================
// HIEN THI TAT CA BAO CAO
// =====================================================

void ShiftManagement::hienThiTatCa() const
{
    if (danhSachCa.empty())
    {
        cout << "Chua co bao cao ca nao."
             << endl;

        return;
    }

    for (size_t i = 0; i < danhSachCa.size(); ++i)
{
    const ShiftReport& ca = danhSachCa[i];

    ca.hienThi();
}

}
