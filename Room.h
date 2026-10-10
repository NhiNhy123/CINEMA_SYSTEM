#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
using namespace std;

class Database;

class Room {
	private:
    	string maPhong;
    	string tenPhong;
    	string loaiPhong;
    	int sucChua;
    	string trangThaiPhong;
	public:
    	Room();
    	Room(string ma, string ten, string loai, int sucChua, string trangThai = "HoatDong");

    	string getMaPhong() const;
    	string getTenPhong() const;
    	string getLoaiPhong() const;
    	int getSucChua() const;
    	string getTrangThaiPhong() const;

    	void setTenPhong(const std::string& ten);
    	void setLoaiPhong(const std::string& loai);
    	void setSucChua(int sucChua);
    	void setTrangThaiPhong(const std::string& trangThai);
    	
    	void hienThi() const;
    	string toCSV() const;
    	static Room fromCSV(const std::string& line);
};

class RoomManager {
	private:
    	vector<Room> danhSachPhong;
	public:
    	void hienThiDanhSach() const;
    	void themPhong(Database& db);
    	void suaPhong(Database& db);
    	void xoaPhong(Database& db);
    	void taiDuLieu(Database& db);
    	bool kiemTraPhongDangChieu(const std::string& maPhong) const;
};

#endif
