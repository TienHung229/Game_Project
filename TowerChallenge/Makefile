# Trình biên dịch
CXX = g++

# Cờ biên dịch: include folder SDL2 và include folder src (các file .h trong project)
CXXFLAGS = -I./src/include -I./src

# Thư viện để link (thêm SDL2_ttf, SDL2_mixer và các thư viện cần thiết)
LDFLAGS = -L./src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# Tất cả file nguồn .cpp trong src/
SRC = $(wildcard src/*.cpp)

# Tên file đầu ra
OUT = main.exe

# Mặc định: build
all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Xóa file exe
clean:
	del $(OUT)

# Chạy chương trình
run: all
	./$(OUT)


