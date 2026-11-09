BEGIN IMMEDIATE;

UPDATE movies SET tenPhim = 'Lật Mặt 7' WHERE tenPhim = 'Lat Mat 7';
UPDATE movies SET tenPhim = 'Thỏ Ơi!' WHERE tenPhim = 'Tho Oi!';
UPDATE movies SET tenPhim = 'Avatar: Lửa và Tro' WHERE tenPhim = 'Avatar: Lua va Tro';
UPDATE movies SET tenPhim = 'Chị Chị Em Em 3' WHERE tenPhim = 'Chi Chi Em Em 3';
UPDATE movies SET tenPhim = 'Trại Buôn Người' WHERE tenPhim = 'Trai Buon Nguoi';

UPDATE movies SET moTa = 'Phim về gia đình và tình mẫu tử'
WHERE moTa = 'Phim ve gia dinh va tinh mau tu';
UPDATE movies SET moTa = 'Câu chuyện tình yêu của Mai và Dương'
WHERE moTa = 'Cau chuyen tinh yeu cua Mai va Duong';
UPDATE movies SET moTa = 'Câu chuyện thực tế về tình yêu và xã hội'
WHERE moTa = 'Cau chuyen thuc te ve tinh yeu va xa hoi';
UPDATE movies SET moTa = 'Hành trình tiếp theo tại Pandora'
WHERE moTa = 'Hanh trinh tiep theo tai Pandora';
UPDATE movies SET moTa = 'Không' WHERE moTa = 'Khong';

UPDATE rooms
SET tenPhong = replace(tenPhong, 'Phong Chieu', 'Phòng Chiếu')
WHERE tenPhong LIKE 'Phong Chieu%';
UPDATE rooms
SET tenPhong = replace(tenPhong, 'Phong ROOM', 'Phòng ROOM')
WHERE tenPhong LIKE 'Phong ROOM%';

UPDATE food_and_beverages SET tenMatHang = 'Bắp ngọt' WHERE tenMatHang = 'Bap ngot';
UPDATE food_and_beverages SET tenMatHang = 'Bắp Phô Mai' WHERE tenMatHang = 'Bap Pho Mai';
UPDATE food_and_beverages
SET tenMatHang = 'Combo Solo (1 Bắp + 1 Nước)'
WHERE tenMatHang = 'Combo Solo (1 Bap + 1 Nuoc)';

UPDATE invoice_fb_items SET tenMatHangTaiThoiDiem = 'Bắp Phô Mai'
WHERE tenMatHangTaiThoiDiem = 'Bap Pho Mai';
UPDATE invoice_fb_items
SET tenMatHangTaiThoiDiem = 'Combo Solo (1 Bắp + 1 Nước)'
WHERE tenMatHangTaiThoiDiem = 'Combo Solo (1 Bap + 1 Nuoc)';

UPDATE promotions SET tenKhuyenMai = 'Ưu đãi khai trương'
WHERE tenKhuyenMai = 'Uu dai khai truong';

UPDATE customer_points_history SET noiDung = 'Đổi điểm khi thanh toán'
WHERE noiDung = 'Doi diem khi thanh toan';
UPDATE customer_points_history SET noiDung = 'Tích điểm hóa đơn đầu tiên'
WHERE noiDung = 'Tich diem hoa don dau tien';
UPDATE customer_points_history SET noiDung = 'Tích điểm từ hóa đơn bán vé'
WHERE noiDung = 'Tich diem tu hoa don ban ve';
UPDATE customer_points_history SET noiDung = 'Tích điểm từ hóa đơn HD01'
WHERE noiDung = 'Tich diem tu hoa don HD01';

UPDATE nhat_ky_hoat_dong
SET chiTiet = 'Đăng nhập thành công'
WHERE chiTiet = 'Dang nhap thanh cong';
UPDATE nhat_ky_hoat_dong
SET chiTiet = 'Đăng nhập thành công qua giao diện UI'
WHERE chiTiet = 'Dang nhap thanh cong qua giao dien UI';
UPDATE nhat_ky_hoat_dong
SET chiTiet = 'Đăng xuất thành công'
WHERE chiTiet = 'Dang xuat thanh cong';
UPDATE nhat_ky_hoat_dong
SET chiTiet = 'Nhân viên chưa chốt ca'
WHERE chiTiet = 'Nhan vien chua chot ca';
UPDATE nhat_ky_hoat_dong
SET chiTiet = 'Nhân viên chưa chốt ca hôm nay'
WHERE chiTiet = 'Nhan vien chua chot ca hom nay';

UPDATE nhat_ky_hoat_dong
SET chiTiet = replace(
    replace(
        replace(
            replace(
                replace(
                    replace(
                        replace(
                            replace(
                                replace(
                                    replace(chiTiet, 'Ma hoa don:', 'Mã hóa đơn:'),
                                    'ma khach hang:', 'mã khách hàng:'),
                                'tong tien:', 'tổng tiền:'),
                            'phuong thuc:', 'phương thức:'),
                        'Ma ve:', 'Mã vé:'),
                    'ma suat chieu:', 'mã suất chiếu:'),
                'ma ghe:', 'mã ghế:'),
            'Ma ca:', 'Mã ca:'),
        'doanh thu he thong:', 'doanh thu hệ thống:'),
    'tien thuc te:', 'tiền thực tế:')
WHERE chiTiet LIKE 'Ma hoa don:%'
   OR chiTiet LIKE 'Ma ve:%'
   OR chiTiet LIKE 'Ma ca:%';

COMMIT;
