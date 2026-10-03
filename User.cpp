#include "User.h"

// =====================================================
// USER
// =====================================================

User::User()
{
    maNguoiDung = "";
    hoTen = "";
    soDienThoai = "";
    ngaySinh = "";
    gioiTinh = "";
    email = "";
    cccd = "";

    taiKhoan = "";
    matKhau = "";
    vaiTro = "";
}


// =====================================================
// USER CONSTRUCTOR
// =====================================================

User::User(string maNguoiDung,
           string hoTen,
           string soDienThoai,
           string ngaySinh,
           string gioiTinh,
           string email,
           string cccd,
           string taiKhoan,
           string matKhau,
           string vaiTro)
{
    this->maNguoiDung = maNguoiDung;
    this->hoTen = hoTen;
    this->soDienThoai = soDienThoai;
    this->ngaySinh = ngaySinh;
    this->gioiTinh = gioiTinh;
    this->email = email;
    this->cccd = cccd;

    this->taiKhoan = taiKhoan;
    this->matKhau = matKhau;
    this->vaiTro = vaiTro;
}


// =====================================================
// DESTRUCTOR
// =====================================================

User::~User()
{
}


// =====================================================
// GETTER
// =====================================================

string User::getMaNguoiDung() const
{
    return maNguoiDung;
}

string User::getHoTen() const
{
    return hoTen;
}

string User::getSoDienThoai() const
{
    return soDienThoai;
}

string User::getNgaySinh() const
{
    return ngaySinh;
}

string User::getGioiTinh() const
{
    return gioiTinh;
}

string User::getEmail() const
{
    return email;
}

string User::getCCCD() const
{
    return cccd;
}

string User::getTaiKhoan() const
{
    return taiKhoan;
}

string User::getVaiTro() const
{
    return vaiTro;
}


// =====================================================
// SETTER
// =====================================================

void User::setHoTen(string hoTen)
{
    this->hoTen = hoTen;
}

void User::setSoDienThoai(string soDienThoai)
{
    this->soDienThoai = soDienThoai;
}

void User::setNgaySinh(string ngaySinh)
{
    this->ngaySinh = ngaySinh;
}

void User::setGioiTinh(string gioiTinh)
{
    this->gioiTinh = gioiTinh;
}

void User::setEmail(string email)
{
    this->email = email;
}

void User::setTaiKhoan(string taiKhoan)
{
    this->taiKhoan = taiKhoan;
}

void User::setMatKhau(string matKhau)
{
    this->matKhau = matKhau;
}


// =====================================================
// USER KIEM TRA DANG NHAP
// =====================================================

bool User::kiemTraDangNhap(
    string taiKhoan,
    string matKhau) const
{
    return this->taiKhoan == taiKhoan &&
           this->matKhau == matKhau;
}


// =====================================================
// USER HIEN THI THONG TIN
// =====================================================

void User::hienThiThongTin() const
{
    cout << "Ma nguoi dung: " << maNguoiDung << endl;
    cout << "Ho ten: " << hoTen << endl;
    cout << "So dien thoai: " << soDienThoai << endl;
    cout << "Ngay sinh: " << ngaySinh << endl;
    cout << "Gioi tinh: " << gioiTinh << endl;
    cout << "Email: " << email << endl;
    cout << "CCCD: " << cccd << endl;
    cout << "Tai khoan: " << taiKhoan << endl;
    cout << "Vai tro: " << vaiTro << endl;
}


// =====================================================
// ADMIN
// =====================================================

Admin::Admin()
    : User()
{
    vaiTro = "Admin";
}


Admin::Admin(string maNguoiDung,
             string hoTen,
             string soDienThoai,
             string ngaySinh,
             string gioiTinh,
             string email,
             string cccd,
             string taiKhoan,
             string matKhau)
    : User(maNguoiDung,
           hoTen,
           soDienThoai,
           ngaySinh,
           gioiTinh,
           email,
           cccd,
           taiKhoan,
           matKhau,
           "Admin")
{
}


// =====================================================
// ADMIN CAP TAI KHOAN STAFF
// =====================================================

void Admin::capTaiKhoanStaff(
    Staff& staff,
    string taiKhoan,
    string matKhau)
{
    if (staff.getCoTaiKhoan())
    {
        cout << "Nhan vien da co tai khoan!" << endl;
        return;
    }

    staff.taiKhoan = taiKhoan;
    staff.matKhau = matKhau;

    staff.coTaiKhoan = true;
    staff.trangThaiTaiKhoan = "HoatDong";

    cout << "Cap tai khoan cho Staff thanh cong!" << endl;
    cout << "Tai khoan: " << taiKhoan << endl;
}


// =====================================================
// ADMIN XOA TAI KHOAN STAFF
// =====================================================

void Admin::xoaTaiKhoanStaff(Staff& staff)
{
    if (!staff.getCoTaiKhoan())
    {
        cout << "Nhan vien chua co tai khoan!" << endl;
        return;
    }

    staff.taiKhoan = "";
    staff.matKhau = "";

    staff.coTaiKhoan = false;
    staff.trangThaiTaiKhoan = "ChuaCoTaiKhoan";

    cout << "Da xoa tai khoan Staff." << endl;
}


// =====================================================
// ADMIN KHOA TAI KHOAN STAFF
// =====================================================

void Admin::khoaTaiKhoanStaff(Staff& staff)
{
    if (!staff.getCoTaiKhoan())
    {
        cout << "Nhan vien chua co tai khoan!" << endl;
        return;
    }

    staff.trangThaiTaiKhoan = "BiKhoa";

    cout << "Da khoa tai khoan Staff: "
         << staff.getTaiKhoan()
         << endl;
}


// =====================================================
// ADMIN MO KHOA TAI KHOAN STAFF
// =====================================================

void Admin::moKhoaTaiKhoanStaff(Staff& staff)
{
    if (!staff.getCoTaiKhoan())
    {
        cout << "Nhan vien chua co tai khoan!" << endl;
        return;
    }

    if (staff.getTrangThaiLamViec() == "DaNghiViec")
    {
        cout << "Nhan vien da nghi viec." << endl;
        cout << "Khong the mo khoa tai khoan!" << endl;
        return;
    }

    staff.trangThaiTaiKhoan = "HoatDong";

    cout << "Da mo khoa tai khoan Staff: "
         << staff.getTaiKhoan()
         << endl;
}


// =====================================================
// ADMIN CHO STAFF NGHI VIEC
// =====================================================

void Admin::choNghiViecStaff(Staff& staff)
{
    staff.trangThaiLamViec = "DaNghiViec";

    // Neu co tai khoan thi tu dong khoa
    if (staff.getCoTaiKhoan())
    {
        staff.trangThaiTaiKhoan = "BiKhoa";
    }

    cout << "Staff "
         << staff.getHoTen()
         << " da nghi viec."
         << endl;

    if (staff.getCoTaiKhoan())
    {
        cout << "Tai khoan da duoc khoa."
             << endl;
    }
}


// =====================================================
// ADMIN CHO STAFF LAM LAI
// =====================================================

void Admin::choLamLaiStaff(Staff& staff)
{
    staff.trangThaiLamViec = "DangLam";

    // Khong tu dong mo khoa tai khoan
    cout << "Staff "
         << staff.getHoTen()
         << " da duoc chuyen sang DangLam."
         << endl;

    if (staff.getCoTaiKhoan())
    {
        cout << "Tai khoan van giu nguyen trang thai."
             << endl;
    }
}


// =====================================================
// ADMIN HIEN THI THONG TIN
// =====================================================

void Admin::hienThiThongTin() const
{
    cout << endl;
    cout << "========== ADMIN =========="
         << endl;

    User::hienThiThongTin();
}


// =====================================================
// ADMIN CHUC NANG
// =====================================================

void Admin::hienThiChucNang() const
{
    cout << endl;
    cout << "====== CHUC NANG ADMIN ======"
         << endl;

    cout << "1. Quan ly khach hang" << endl;
    cout << "2. Quan ly doanh thu" << endl;
    cout << "3. Quan ly phong" << endl;
    cout << "4. Quan ly ghe" << endl;
    cout << "5. Quan ly suat chieu" << endl;
    cout << "6. Quan ly phim & the loai" << endl;
    cout << "7. Quan ly F&B" << endl;
    cout << "8. Quan ly bao cao ca" << endl;
    cout << "9. Quan ly khuyen mai" << endl;
    cout << "10. Quan ly nhan su" << endl;
    cout << "11. Quan ly tai khoan" << endl;
}


// =====================================================
// STAFF
// =====================================================

Staff::Staff()
    : User()
{
    vaiTro = "Staff";

    taiKhoan = "";
    matKhau = "";

    trangThaiLamViec = "DangLam";
    trangThaiTaiKhoan = "ChuaCoTaiKhoan";

    coTaiKhoan = false;
}


// =====================================================
// STAFF CONSTRUCTOR NHAN VIEN MOI
// =====================================================

Staff::Staff(string maNguoiDung,
             string hoTen,
             string soDienThoai,
             string ngaySinh,
             string gioiTinh,
             string email,
             string cccd)
    : User(maNguoiDung,
           hoTen,
           soDienThoai,
           ngaySinh,
           gioiTinh,
           email,
           cccd,
           "",
           "",
           "Staff")
{
    trangThaiLamViec = "DangLam";
    trangThaiTaiKhoan = "ChuaCoTaiKhoan";
    coTaiKhoan = false;
}


// =====================================================
// STAFF CONSTRUCTOR DAY DU
// DUNG KHI DOC CSV
// =====================================================

Staff::Staff(string maNguoiDung,
             string hoTen,
             string soDienThoai,
             string ngaySinh,
             string gioiTinh,
             string email,
             string cccd,
             string taiKhoan,
             string matKhau,
             string trangThaiLamViec,
             string trangThaiTaiKhoan,
             bool coTaiKhoan)
    : User(maNguoiDung,
           hoTen,
           soDienThoai,
           ngaySinh,
           gioiTinh,
           email,
           cccd,
           taiKhoan,
           matKhau,
           "Staff")
{
    this->trangThaiLamViec = trangThaiLamViec;
    this->trangThaiTaiKhoan = trangThaiTaiKhoan;
    this->coTaiKhoan = coTaiKhoan;
}


// =====================================================
// STAFF GETTER
// =====================================================

string Staff::getTrangThaiLamViec() const
{
    return trangThaiLamViec;
}

string Staff::getTrangThaiTaiKhoan() const
{
    return trangThaiTaiKhoan;
}

bool Staff::getCoTaiKhoan() const
{
    return coTaiKhoan;
}


// =====================================================
// STAFF KIEM TRA DANG NHAP
// =====================================================

bool Staff::kiemTraDangNhap(
    string taiKhoan,
    string matKhau) const
{
    // Chua co tai khoan
    if (!coTaiKhoan)
    {
        return false;
    }

    // Tai khoan bi khoa
    if (trangThaiTaiKhoan == "BiKhoa")
    {
        return false;
    }

    // Nhan vien da nghi viec
    if (trangThaiLamViec == "DaNghiViec")
    {
        return false;
    }

    return User::kiemTraDangNhap(
        taiKhoan,
        matKhau);
}


// =====================================================
// STAFF HIEN THI THONG TIN
// =====================================================

void Staff::hienThiThongTin() const
{
    cout << endl;
    cout << "========== STAFF =========="
         << endl;

    User::hienThiThongTin();

    cout << "Trang thai lam viec: "
         << trangThaiLamViec
         << endl;

    cout << "Co tai khoan: "
         << (coTaiKhoan ? "Co" : "Chua co")
         << endl;

    if (coTaiKhoan)
    {
        cout << "Trang thai tai khoan: "
             << trangThaiTaiKhoan
             << endl;
    }
}


// =====================================================
// STAFF CHUC NANG
// =====================================================

void Staff::hienThiChucNang() const
{
    cout << endl;
    cout << "====== CHUC NANG STAFF ======"
         << endl;

    cout << "1. Ban ve" << endl;
    cout << "2. Khach hang (CRM)" << endl;
    cout << "3. Chot ca & bao cao" << endl;
}
