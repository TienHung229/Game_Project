#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <ctime> // Required for time()

static const SDL_Color COLORS[5] = {
    {100,200,100,255}, // xanh lá
    {200, 50, 50,255}, // đỏ
    {150,  50,150,255},// tím
    {255,215,  0,255}, // vàng
    { 50,150,200,255}  // xanh dương
};
// --- Configurations ---
const int WINDOW_WIDTH = 450;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "Tower Challenge";

const int MAX_SPEED = 60;          // max px/sec
const int INITIAL_TILE_WIDTH = 200;
const int TILE_HEIGHT = 40;
const int INITIAL_SPEED = 200;      // starting px/sec
const int MIN_SPEED = 50;           // minimum px/sec
const int SPEED_INCREMENT = 1.25;     // px/sec decrease per placement
const int SCREEN_MARGIN_TOP = 400;  // margin before panning
const float CAMERA_LERP = 0.1f;     // smoothing factor [0..1]

// UI button sizes
const SDL_Rect START_BTN_RECT = {(WINDOW_WIDTH - 200) / 2, 453, 200, 50};
const int MUTE_BTN_SIZE = 40;
const SDL_Rect MUTE_BTN_RECT = {
    WINDOW_WIDTH - MUTE_BTN_SIZE - 10,  // x = cửa sổ rộng – kích thước – padding
    10,                                 // y = padding 10px
    MUTE_BTN_SIZE,                      // w
    MUTE_BTN_SIZE                       // h
};



// Assets paths
const char* FONT_PATH = "assets/fonts/font.ttf";
const char* MUSIC_PATH = "assets/audio/background.mp3";
const int MUSIC_VOLUME = MIX_MAX_VOLUME / 2;

// --- Game States ---
enum class GameState { MENU, PLAYING, GAME_OVER };

// --- Tile struct ---
struct Tile {
    SDL_Rect rect;
    int speed;  // px per second horizontal
    bool movingRight;
    SDL_Color color;
};

// --- Globals ---
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;
Mix_Music* gMusic = nullptr;
Mix_Chunk*   gPlaceSfx = nullptr;
Mix_Chunk*   gPerfectSfx = nullptr;
SDL_Texture* gBgTex = nullptr;
SDL_Texture* gMenuBg = nullptr;
SDL_Texture* gIconMute   = nullptr;
SDL_Texture* gIconUnmute = nullptr;
int perfectTimer = 0;       
const int PERFECT_SHOW_MS = 1500;




GameState gState = GameState::MENU;
bool gMute = false;
int gTopScore = 0;
float cameraY_f = 0.0f;  // smooth camera offset

// --- Forward declarations ---
bool initSDL();
void cleanupSDL();
bool loadAssets();
void unloadAssets();
void run();
SDL_Texture* renderText(const std::string& text, SDL_Color color);
void drawMenu();

// --- Initialization & cleanup ---
bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return false;
    }
    gWindow = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                               WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!gWindow) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(
        gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gRenderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("Mix_OpenAudio Error: %s", Mix_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    SDL_Log("IMG_Init Error: %s", IMG_GetError());
    return false;
    }

    return true;
}

void cleanupSDL() {
    unloadAssets();
    if (gRenderer) SDL_DestroyRenderer(gRenderer);
    if (gWindow) SDL_DestroyWindow(gWindow);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// --- Assets load/unload ---
bool loadAssets() {
    gFont = TTF_OpenFont(FONT_PATH, 24);
    if (!gFont) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        return false;
    }
    gMusic = Mix_LoadMUS(MUSIC_PATH);
    if (!gMusic) {
        SDL_Log("Failed to load music: %s", Mix_GetError());
        return false;
    }

    // Load icon mute/unmute
    gIconMute   = IMG_LoadTexture(gRenderer, "assets/images/icon_mute.png");
    gIconUnmute = IMG_LoadTexture(gRenderer, "assets/images/icon_unmute.png");
    if (!gIconMute || !gIconUnmute) {
        SDL_Log("Failed to load icon: %s", IMG_GetError());
        return false;
    }
    

    // sau khi load gMusic:
    gPlaceSfx   = Mix_LoadWAV("assets/audio/block_place.mp3");
    gPerfectSfx = Mix_LoadWAV("assets/audio/block_perfect.mp3");
    if (!gPlaceSfx || !gPerfectSfx) {
        SDL_Log("Failed to load SFX: %s / %s", 
                Mix_GetError(), Mix_GetError());
        return false;
    }

    // Load background image
    SDL_Surface* bgSurf = IMG_Load("assets/images/background.png");
    if (!bgSurf) {
        SDL_Log("Failed to load background.png: %s", IMG_GetError());
        return false;
    }
    gBgTex = SDL_CreateTextureFromSurface(gRenderer, bgSurf);
    SDL_FreeSurface(bgSurf);

    
    Mix_VolumeMusic(MUSIC_VOLUME);

    srand((unsigned)time(nullptr));

    if (!gMute) {
        Mix_PlayMusic(gMusic, -1);  // phát nhạc nền lặp vô tận ngay từ menu
    }
    // Load menu background
    SDL_Surface* surf = IMG_Load("assets/images/menu_bg.png");
    if (!surf) {
        SDL_Log("IMG_Load Error: %s", IMG_GetError());
        return false;
    }


    gMenuBg = SDL_CreateTextureFromSurface(gRenderer, surf);
    SDL_FreeSurface(surf);

    return true;
}

void unloadAssets() {
    if (gMusic) Mix_FreeMusic(gMusic);
    if (gFont) TTF_CloseFont(gFont);
    if (gPerfectSfx) Mix_FreeChunk(gPerfectSfx);
    if (gPlaceSfx)   Mix_FreeChunk(gPlaceSfx);
    if (gBgTex) {
    SDL_DestroyTexture(gBgTex);
    gBgTex = nullptr;
    }

    if (gMenuBg) {
    SDL_DestroyTexture(gMenuBg);
    gMenuBg = nullptr;
    }
    if (gIconMute)   SDL_DestroyTexture(gIconMute);
    if (gIconUnmute) SDL_DestroyTexture(gIconUnmute);


    gMusic = nullptr;
    gFont = nullptr;        
    gPerfectSfx = nullptr;
    gPlaceSfx   = nullptr;
}

// --- Render text to texture ---
SDL_Texture* renderText(const std::string& text, SDL_Color color) {
    SDL_Surface* surf = TTF_RenderText_Blended(gFont, text.c_str(), color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(gRenderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

// --- Draw menu screen ---
void drawMenu() {
    // 1) Vẽ nền menu (có sẵn cả logo “Tower Challenge”)
    SDL_RenderCopy(gRenderer, gMenuBg, nullptr, nullptr);

    // 2) Vẽ Top Score ngay dưới phần logo trong ảnh:
    //    Bạn có thể tinh chỉnh 'logoBottomY' cho vừa khớp ảnh logo thực tế.
    int logoBottomY = 400;   // ví dụ: logo nằm cao ~ đến y=180
    int padding = 10;

    SDL_Color white = {255, 255, 0}; // màu nâu
    std::string scoreStr = "Top Score: " + std::to_string(gTopScore);
    SDL_Texture* scoreTex = renderText(scoreStr, white);

    int sw, sh;
    SDL_QueryTexture(scoreTex, nullptr, nullptr, &sw, &sh);
    SDL_Rect scoreRect = {
        (WINDOW_WIDTH - sw) / 2,
        logoBottomY + padding,
        sw, sh
    };
    SDL_RenderCopy(gRenderer, scoreTex, nullptr, &scoreRect);
    SDL_DestroyTexture(scoreTex);

    // 3) Vẽ nút Start nền vàng, chữ đen, nằm ngay dưới Top Score:
    SDL_Rect startRect = {
        (WINDOW_WIDTH - 200) / 2,
        scoreRect.y + scoreRect.h + 20,  // cách Top Score 20px
        200, 50
    };

    SDL_SetRenderDrawColor(gRenderer, 255,215,0,255);
    SDL_RenderFillRect(gRenderer, &START_BTN_RECT);
    SDL_SetRenderDrawColor(gRenderer, 0,0,0,255);
    SDL_RenderDrawRect(gRenderer, &START_BTN_RECT);

    SDL_Texture* startTex = renderText("Start", {139,37,0});
    SDL_QueryTexture(startTex, nullptr, nullptr, &sw, &sh);
    SDL_Rect sr = {
        startRect.x + (startRect.w - sw)/2,
        startRect.y + (startRect.h - sh)/2,
        sw, sh
    };
    SDL_RenderCopy(gRenderer, startTex, nullptr, &sr);
    SDL_DestroyTexture(startTex);

    // 4) Vẽ icon mute/unmute ở góc trên phải
    SDL_Rect mr = MUTE_BTN_RECT;
    SDL_Texture* ico = gMute ? gIconMute : gIconUnmute;
    SDL_RenderCopy(gRenderer, ico, nullptr, &mr);

    // 5) Present cả menu
    SDL_RenderPresent(gRenderer);
}




// --- Main game loop ---
void run() {
    bool quit = false;
    SDL_Event e;
    std::vector<Tile> stack;
    int score = 0;
    Uint32 lastTime = SDL_GetTicks();
    float desiredCamY = 0.0f;

    while (!quit) {
        Uint32 now = SDL_GetTicks();
        float delta = (now - lastTime) / 1000.0f;
        lastTime = now;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (gState == GameState::MENU && e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point p{e.button.x, e.button.y};

                if (SDL_PointInRect(&p, &MUTE_BTN_RECT)) {
                    gMute = !gMute;
                    if (gMute) Mix_PauseMusic();
                    else       Mix_ResumeMusic();
                }

                
                if (SDL_PointInRect(&p, &START_BTN_RECT)) {
                    // init game
                    stack.clear();
                    score = 0;
                    cameraY_f = 0;
                    desiredCamY = 0;
                    int baseY = WINDOW_HEIGHT - TILE_HEIGHT;
                    stack.push_back(Tile{
                        {(WINDOW_WIDTH - INITIAL_TILE_WIDTH) / 2, baseY, INITIAL_TILE_WIDTH, TILE_HEIGHT},
                        0,
                        false,
                        COLORS[rand() % 5]   // màu ngẫu nhiên
                    });
                    stack.push_back(Tile{
                        {0, baseY - TILE_HEIGHT, INITIAL_TILE_WIDTH, TILE_HEIGHT},
                        INITIAL_SPEED,
                        true,
                        COLORS[rand() % 5]
                    });
                    if (!gMute) Mix_PlayMusic(gMusic, -1);
                    gState = GameState::PLAYING;
                } 
                
            } else if (gState == GameState::PLAYING &&
                       e.type == SDL_MOUSEBUTTONDOWN) {

                SDL_Point p{e.button.x, e.button.y}; // Lấy tọa độ click chuột

                if (SDL_PointInRect(&p, &MUTE_BTN_RECT)) { // Kiểm tra nếu click vào nút Mute
                    gMute = !gMute;
                    if (gMute) {
                        Mix_PauseMusic(); // Nếu đang phát thì tạm dừng
                    } else {
                        // Nếu đang tạm dừng thì tiếp tục, nếu không thì có thể phát mới (nếu cần)
                        // Tuy nhiên, game thường tự động phát nhạc khi bắt đầu nếu không Mute,
                        // nên Mix_ResumeMusic() thường là đủ.
                        Mix_ResumeMusic(); 
                    }
                } else {
                    Tile& curr = stack.back();
                    Tile& prev = stack[stack.size() - 2];
                    int L = std::max(curr.rect.x, prev.rect.x);
                    int R = std::min(curr.rect.x + curr.rect.w,
                                    prev.rect.x + prev.rect.w);
                    int W = R - L;

                    
                    if (W <= 0) {
                        // đặt hụt -> GAME OVER
                        gState = GameState::GAME_OVER;
                    }
                    else {
                        // đặt thành công
                        if (W == prev.rect.w) {
                            // perfect: width không thay đổi
                            perfectTimer = PERFECT_SHOW_MS;  // hiển thị 500ms
                            Mix_PlayChannel(-1, gPerfectSfx, 0);
                            score += 5;
                            // vẫn giữ nguyên vị trí và kích thước cũ
                            curr.rect.x = prev.rect.x;
                            curr.rect.w = prev.rect.w;
                        }
                        else {
                            // đặt lệch nhưng không rớt: +1 điểm
                            Mix_PlayChannel(-1, gPlaceSfx, 0);
                            score += 1;
                            // cắt phần thừa
                            curr.rect.x = L;
                            curr.rect.w = W;
                            // giảm tốc độ hoặc tăng tùy logic
                            curr.speed = std::max(MAX_SPEED, curr.speed + SPEED_INCREMENT);
                        }
                        // place tile xuống tháp
                        curr.rect.y = prev.rect.y - TILE_HEIGHT;

                        // determine camera target after 5th tile
                        // determine camera target after 5th tile (hoặc "xác định mục tiêu camera")
                        if (stack.size() >= 5) { // <--- Quay lại điều kiện như code gốc v1
                            desiredCamY = curr.rect.y - SCREEN_MARGIN_TOP; // <--- Quay lại cách tính như code gốc v1
                            // if (desiredCamY < 0.0f) desiredCamY = 0.0f; // <--- Dùng 0.0f cho float
                        }
                        // add next moving tile
                        stack.push_back(Tile{
                            {0, curr.rect.y - TILE_HEIGHT, curr.rect.w, TILE_HEIGHT},
                            curr.speed,
                            true,
                            COLORS[rand() % 5]
                        });
                }
                
                }
            } else if (gState == GameState::GAME_OVER &&
                       e.type == SDL_MOUSEBUTTONDOWN) {
                gTopScore = std::max(gTopScore, score);
                gState = GameState::MENU;
            }
        }
        if (quit) break;

        // update moving tile
        if (gState == GameState::PLAYING && stack.size() >= 2) {
            Tile& t = stack.back();
            int dir = t.movingRight ? 1 : -1;
            t.rect.x += int(dir * t.speed * delta);
            if (t.rect.x + t.rect.w >= WINDOW_WIDTH) {
                t.movingRight = false;
                t.rect.x = WINDOW_WIDTH - t.rect.w;
            }
            if (t.rect.x <= 0) {
                t.movingRight = true;
                t.rect.x = 0;
            }
        }

        // smooth camera lerp
        // smooth camera lerp
        if (gState == GameState::PLAYING) {
            cameraY_f += (desiredCamY - cameraY_f) * CAMERA_LERP;
        }
        int cameraY = int(std::round(cameraY_f));

        // render
        // SDL_SetRenderDrawColor(gRenderer, 255,255,255,255);
        // SDL_RenderClear(gRenderer);

        // draw background image full-window
        SDL_RenderCopy(gRenderer, gBgTex, nullptr, nullptr);



        if (gState == GameState::MENU) {
            drawMenu();
        } else if (gState == GameState::PLAYING) {

            SDL_Color black = {0, 0, 0};
            // chuyển score thành string
            SDL_Texture* currScoreTex = renderText(std::to_string(score), {255, 255, 255});
            int sw, sh;
            SDL_QueryTexture(currScoreTex, nullptr, nullptr, &sw, &sh);
            SDL_Rect scoreRect = { 10, 10, sw, sh };
            SDL_RenderCopy(gRenderer, currScoreTex, nullptr, &scoreRect);
            SDL_DestroyTexture(currScoreTex);
            // draw stack
            for (auto& tile : stack) {
                SDL_Rect dst = tile.rect;
                dst.y -= cameraY;
                // fill bằng màu đã gán:
                SDL_SetRenderDrawColor(
                    gRenderer,
                    tile.color.r,
                    tile.color.g,
                    tile.color.b,
                    tile.color.a
                );
                SDL_RenderFillRect(gRenderer, &dst);
                // viền đen:
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(gRenderer, &dst);
            }

            if (perfectTimer > 0) {
                perfectTimer -= int(delta * 1000); // delta là giây
                // vẽ text “+5” ở đúng tile vừa đặt hoặc góc màn hình
                SDL_Color c = {255,215,0};  // vàng
                auto txt = renderText("Perfect +5", c);
                int tw, th; SDL_QueryTexture(txt, nullptr, nullptr, &tw, &th);
                SDL_Rect trg = { (WINDOW_WIDTH-tw)/2, 300, tw, th };
                SDL_RenderCopy(gRenderer, txt, nullptr, &trg);
                SDL_DestroyTexture(txt);
            }
            
            // Vẽ icon Mute/Unmute
            SDL_Rect mr = MUTE_BTN_RECT;
            SDL_Texture* ico = gMute ? gIconMute : gIconUnmute;
            SDL_RenderCopy(gRenderer, ico, nullptr, &mr);


            SDL_RenderPresent(gRenderer);
        } else if (gState == GameState::GAME_OVER) {
            SDL_Color black = {0, 0, 0};
            SDL_Texture* overTex = renderText("Game Over", black);
            int ow, oh;
            SDL_QueryTexture(overTex, nullptr, nullptr, &ow, &oh);
            SDL_Rect orc = {(WINDOW_WIDTH - ow) / 2, WINDOW_HEIGHT / 2 - 30, ow,
                            oh};
            SDL_RenderCopy(gRenderer, overTex, nullptr, &orc);
            SDL_DestroyTexture(overTex);

            std::string s = "Score: " + std::to_string(score);
            SDL_Texture* scoreTex = renderText(s, black);
            SDL_QueryTexture(scoreTex, nullptr, nullptr, &ow, &oh);
            SDL_Rect src = {(WINDOW_WIDTH - ow) / 2, WINDOW_HEIGHT / 2 + 10, ow,
                            oh};
            SDL_RenderCopy(gRenderer, scoreTex, nullptr, &src);
            SDL_DestroyTexture(scoreTex);
            SDL_RenderPresent(gRenderer);
        }
        SDL_Delay(16);
    }
}

int main(int argc, char* argv[]) {
    if (!initSDL()) return -1;
    if (!loadAssets()) return -1;
    run();
    cleanupSDL();
    return 0;
}
