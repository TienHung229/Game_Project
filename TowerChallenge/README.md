### Cài đặt SDL2
Trước tiên, bạn cần cài đặt SDL2. Bạn có thể tải xuống từ trang chính thức của SDL2 và làm theo hướng dẫn cài đặt cho hệ điều hành của bạn.

### Mã nguồn

```cpp
#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

bool isRunning = true;
int score = 0;
bool gameOver = false;
SDL_Rect baseRect;
SDL_Rect currentRect;
int currentSpeed = 5;
bool isPerfect = false;

void init() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Tower Challenge", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("arial.ttf", 24); // Đảm bảo bạn có file font arial.ttf trong thư mục làm việc
}

void close() {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void renderText(const std::string& text, int x, int y) {
    SDL_Color color = {255, 255, 255}; // Màu trắng
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void resetGame() {
    score = 0;
    gameOver = false;
    baseRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 50, 200, 20};
    currentRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 200, 20};
    isPerfect = false;
}

void update() {
    if (!gameOver) {
        currentRect.x += currentSpeed;
        if (currentRect.x + currentRect.w > SCREEN_WIDTH || currentRect.x < 0) {
            currentSpeed = -currentSpeed; // Đảo chiều
        }
    }
}

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && !gameOver) {
            // Xử lý đặt thanh
            int offset = currentRect.x - baseRect.x;
            if (offset >= 0 && offset <= baseRect.w) {
                // Đặt thẳng hàng
                isPerfect = true;
                score += 5;
                renderText("Perfect!", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2);
            } else if (offset < 0 || offset > baseRect.w) {
                // Thua
                gameOver = true;
            } else {
                // Cắt phần bị lệch
                isPerfect = false;
                int cut = std::abs(offset);
                baseRect.w -= cut;
            }
            // Cập nhật thanh mốc
            baseRect = currentRect;
            currentRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 200, 20};
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ thanh mốc
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &baseRect);

    // Vẽ thanh hiện tại
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &currentRect);

    // Hiển thị điểm
    renderText("Score: " + std::to_string(score), 10, 10);

    if (gameOver) {
        renderText("Game Over! Score: " + std::to_string(score), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    init();
    resetGame();

    while (isRunning) {
        handleInput();
        update();
        render();
        SDL_Delay(16); // Khoảng thời gian giữa các khung hình
    }

    close();
    return 0;
}
```

### Giải thích mã
1. **Khởi tạo SDL**: Hàm `init()` khởi tạo SDL và TTF, tạo cửa sổ và renderer.
2. **Xử lý sự kiện**: Hàm `handleInput()` xử lý các sự kiện từ bàn phím và chuột.
3. **Cập nhật trạng thái**: Hàm `update()` cập nhật vị trí của thanh hiện tại.
4. **Vẽ**: Hàm `render()` vẽ các thanh và điểm số lên màn hình.
5. **Reset game**: Hàm `resetGame()` khởi tạo lại các biến khi bắt đầu game mới.

### Lưu ý
- Bạn cần có file font `arial.ttf` trong thư mục làm việc của bạn.
- Mã này chỉ là một khởi đầu và có thể cần nhiều cải tiến, chẳng hạn như âm thanh, hiệu ứng, và các tính năng khác mà bạn đã mô tả.
- Đảm bảo rằng bạn đã cài đặt SDL2 và SDL_ttf đúng cách trước khi biên dịch mã này.