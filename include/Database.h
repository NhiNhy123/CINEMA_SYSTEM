#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>
#include <utility>
#include "Customer.h"

class Seat;
class FoodAndBeverage;
class Promotion;
class Invoice;
class Room;
class Movie;
class Showtime;
class Ticket;
class ShiftReport;
class User;

using namespace std;

class Database {
private:
    bool giaoDichBanVeDangMo;
    string banSaoLichVaVe;
    string banSaoGiaoDichFnb;
    string banSaoNhanSu;

public:
    Database();

	void docDanhSachUsers(vector<Customer>& danhSachKH);
	string layTenNhanVienQuaMa(string maNhanVien);
	bool ghiDanhSachUsers(const vector<Customer>& danhSachKH);
	void docLichSuGiaoDichDiem(vector<GiaoDichDiem>& danhSachGD);
	bool ghiLichSuGiaoDichDiem(const vector<GiaoDichDiem>& danhSachGD);
    bool batDauGiaoDichBanVe();
    bool khoiPhucGiaoDichBanVe();
    void hoanTatGiaoDichBanVe();
	void docDanhSachTheLoai(vector<pair<string, string> >& danhSachTheLoai);
	bool themLoaiTheLoai(const string& tenTheLoai);
	bool suaLoaiTheLoai(const string& maTheLoai, const string& tenMoi);
	bool xoaLoaiTheLoai(const string& maTheLoai);
	void docMovieGenres(vector<pair<string, string> >& movieGenres);
	string layTheLoaiCuaPhim(string maPhimToFind);
	bool themTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi);
	bool capNhatTheLoaiChoPhim(string maPhim, string tenTheLoaiMoi);
    void docDanhSachPhong(vector<Room>& danhSachPhong);
    void ghiDanhSachPhong(const vector<Room>& danhSachPhong);

    void docDanhSachGhe(vector<Seat>& danhSachGhe);
    void ghiDanhSachGhe(const vector<Seat>& danhSachGhe);
    void docDanhSachLoaiGhe(vector<pair<string, double> >& danhSachLoaiGhe);
    bool themLoaiGhe(const string& tenLoaiGhe, double donGia);
    bool suaLoaiGhe(const string& tenLoaiGhe, const string& tenMoi, double donGiaMoi);
    bool xoaLoaiGhe(const string& tenLoaiGhe);
    void docTrangThaiGheSuatChieu(const string& maSuatChieuCanTim, vector<pair<string, string> >& trangThaiGheList);
    void taoGheTuDongChoPhong(const string& maPhongCanTao, int sucChuaPhong);
    
    void docDanhSachPhim(vector<Movie>& danhSachPhim);
    void ghiDanhSachPhim(const vector<Movie>& danhSachPhim);

    void docDanhSachSuatChieu(vector<Showtime>& danhSachSuatChieu);
    void ghiDanhSachSuatChieu(const vector<Showtime>& danhSachSuatChieu);
    bool giuGheSuatChieu(const string& maSuatChieu, const string& maGhe, const string& maPhienGiu);
    bool kiemTraGiuGheSuatChieu(const string& maSuatChieu, const string& maGhe, const string& maPhienGiu);
    bool giaiPhongGheSuatChieu(const string& maSuatChieu, const string& maGhe, const string& maPhienGiu);
    bool chotGheSuatChieu(const string& maSuatChieu, const vector<string>& danhSachGhe, const string& maPhienGiu);
    bool capNhatTrangThaiGheSuatChieu(const string& maSuatChieu, const string& maGhe,
                                      const string& trangThaiMoi, const string& maPhienGiu = "");

    void docDanhSachVe(vector<Ticket>& danhSachVe);
    bool ghiDanhSachVe(const vector<Ticket>& danhSachVe);

    void docDanhSachFAndB(vector<FoodAndBeverage>& danhSachFAndB);
    void ghiDanhSachFAndB(const vector<FoodAndBeverage>& danhSachFAndB);

    void docDanhSachKhuyenMai(vector<Promotion>& danhSachKhuyenMai);
    void ghiDanhSachKhuyenMai(const vector<Promotion>& danhSachKhuyenMai);

    void docDanhSachHoaDon(vector<Invoice>& danhSachHoaDon);
    bool ghiDanhSachHoaDon(const vector<Invoice>& danhSachHoaDon);

    void docDanhSachCa(vector<ShiftReport>& danhSachCa);
	void ghiDanhSachCa(const vector<ShiftReport>& danhSachCa);
};

#endif
