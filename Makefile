# Trình biên dịch
CXX = g++ 

# Cờ biên dịch (-I để chỉ định thư mục chứa file header)
CXXFLAGS = -Isrc/include/SDL2

# Các thư viện cần link (-L để chỉ định thư mục lib, -l để chỉ định các thư viện cần dùng)
LDFLAGS = -Lsrc/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

# Tên file nguồn
SRC = main.cpp

# Tên file đầu ra
OUT = main.exe

# Lệnh biên dịch
all:
	$(CXX) $(SRC) -o $(OUT) $(CXXFLAGS) $(LDFLAGS)

# Lệnh dọn dẹp file biên dịch
clean:
	rm -f $(OUT)
