LƯU Ý: *TIẾN TRÌNH SERVER VÀ CÁC TIẾN TRÌNH CLIENT PHẢI ĐƯỢC CHẠY TRÊN CÙNG MỘT MÁY TÍNH THÌ MỚI CÓ THỂ KẾT NỐI VỚI NHAU
       *ĐỂ CHẠY ĐƯỢC TIẾN TRÌNH CLIENT THÌ TIẾN TRÌNH SERVER PHẢI ĐƯỢC CHẠY VÀ HOẠT ĐỘNG TRONG SUỐT THỜI GIAN CLIENT KẾT NỐI ĐẾN SERVER
thông tin tài khoản của client:
  tài khoản: myaccount
  mật khẩu: 123456789

Có thể sử dụng Visual Studio để compile, hoặc nếu sử dụng g++ để compile thì nhập vào command prompt lệnh sau:
  g++ client.cpp -o client.exe -lws2_32 (cho file client.cpp)
  g++ server.cpp -o server.exe -lws2_32 (cho file server.cpp)
