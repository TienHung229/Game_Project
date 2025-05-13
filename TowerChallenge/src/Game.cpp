#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <ctime>

using namespace std; 


static const SDL_Color COLORS[5] = {
    {100, 200, 100, 255}, // xanh lá
    {200, 50, 50, 255}, // đỏ
    {150, 50, 150, 255},// tím
    {255, 215,  0, 255}, // vàng
    {50, 150, 200, 255}  // xanh dương
};


const int WINDOW_WIDTH = 450;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "Tower Challenge";


const int MAX_SPEED = 200;          
const int INITIAL_TILE_WIDTH = 200;
const int TILE_HEIGHT = 40;
const int INITIAL_SPEED = 200;
const int SPEED_INCREMENT = 1.25;
const int SCREEN_MARGIN_TOP = 400; // camera cách mép trên 400px
const float CAMERA_LERP = 0.1f; // tốc độ camera di chuyển


const SDL_Rect START_BTN_RECT = {(WINDOW_WIDTH - 200) / 2, 453, 200, 50};
const int MUTE_BTN_SIZE = 40;
const SDL_Rect MUTE_BTN_RECT = {WINDOW_WIDTH - MUTE_BTN_SIZE - 10, 10, MUTE_BTN_SIZE, MUTE_BTN_SIZE};


const char* FONT_PATH = "assets/fonts/font.ttf";
const char* MUSIC_PATH = "assets/audio/background.mp3";
const int MUSIC_VOLUME = MIX_MAX_VOLUME / 2;


enum class GameState { MENU, PLAYING, GAME_OVER };


struct Tile {
    SDL_Rect rect;
    int speed;
    bool movingRight;
    SDL_Color color;
};


SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;
Mix_Music* gMusic = nullptr;
Mix_Chunk* gPlaceSfx = nullptr;
Mix_Chunk* gPerfectSfx = nullptr;
SDL_Texture* gBgTex = nullptr;
SDL_Texture* gMenuBg = nullptr;
SDL_Texture* gIconMute   = nullptr;
SDL_Texture* gIconUnmute = nullptr;
int perfectTimer = 0;
const int PERFECT_SHOW_MS = 1500;
GameState gState = GameState::MENU;
bool gMute = false;
int gTopScore = 0;
float cameraY_f = 0.0f;


bool initSDL();
void cleanupSDL();
bool loadAssets();
void unloadAssets();
void run();
SDL_Texture* renderText(const string& text, SDL_Color color);
void drawMenu();
void handlePlaceTile(vector<Tile>& stack, int& score, float& desiredCamY);


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


    gIconMute   = IMG_LoadTexture(gRenderer, "assets/images/icon_mute.png");
    gIconUnmute = IMG_LoadTexture(gRenderer, "assets/images/icon_unmute.png");
    if (!gIconMute || !gIconUnmute) {
        SDL_Log("Failed to load icon: %s", IMG_GetError());
        return false;
    }


    gPlaceSfx = Mix_LoadWAV("assets/audio/block_place.mp3");
    gPerfectSfx = Mix_LoadWAV("assets/audio/block_perfect.mp3");
    if (!gPlaceSfx || !gPerfectSfx) {
        SDL_Log("Failed to load SFX: %s / %s",
                Mix_GetError(), Mix_GetError());
        return false;
    }


    SDL_Surface* bgSurf = IMG_Load("assets/images/background.png");
    if (!bgSurf) {
        SDL_Log("Failed to load background.png: %s", IMG_GetError());
        return false;
    }
    gBgTex = SDL_CreateTextureFromSurface(gRenderer, bgSurf);
    SDL_FreeSurface(bgSurf);
    if (!gBgTex) { 
        SDL_Log("Failed to create texture from background.png: %s", SDL_GetError());
        return false;
    }


    Mix_VolumeMusic(MUSIC_VOLUME);

    srand((unsigned)time(0));

    if (!gMute && gMusic) { 
        Mix_PlayMusic(gMusic, -1);
    }

    SDL_Surface* surf = IMG_Load("assets/images/menu_bg.png");
    if (!surf) {
        SDL_Log("IMG_Load Error (menu_bg.png): %s", IMG_GetError());
        return false;
    }
    gMenuBg = SDL_CreateTextureFromSurface(gRenderer, surf);
    SDL_FreeSurface(surf);
    if (!gMenuBg) { 
        SDL_Log("Failed to create texture from menu_bg.png: %s", SDL_GetError());
        return false;
    }

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
    gIconMute = nullptr;
    gIconUnmute = nullptr;
}


SDL_Texture* renderText(const string& text, SDL_Color color) {
    if (!gFont) return nullptr; 
    SDL_Surface* surf = TTF_RenderText_Blended(gFont, text.c_str(), color);
    if (!surf) {
        SDL_Log("TTF_RenderText_Blended error: %s", TTF_GetError());
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(gRenderer, surf);
    SDL_FreeSurface(surf);
    if (!tex) {
        SDL_Log("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
    }
    return tex;
}


void drawMenu() {
    if (gMenuBg) SDL_RenderCopy(gRenderer, gMenuBg, nullptr, nullptr);

    int logoBottomY = 400;
    int padding = 10;

    SDL_Color yellowTextColor = {255, 255, 0, 255}; 
    string scoreStr = "Top Score: " + to_string(gTopScore);
    SDL_Texture* scoreTex = renderText(scoreStr, yellowTextColor);

    if (scoreTex) {
        int sw, sh;
        SDL_QueryTexture(scoreTex, nullptr, nullptr, &sw, &sh);
        SDL_Rect scoreRect = {(WINDOW_WIDTH - sw) / 2, logoBottomY + padding, sw, sh};
        SDL_RenderCopy(gRenderer, scoreTex, nullptr, &scoreRect);
        SDL_DestroyTexture(scoreTex);
    }

    // Nút Start
    SDL_SetRenderDrawColor(gRenderer, 255,215,0,255); 
    SDL_RenderFillRect(gRenderer, &START_BTN_RECT);
    SDL_SetRenderDrawColor(gRenderer, 0,0,0,255);
    SDL_RenderDrawRect(gRenderer, &START_BTN_RECT);

    SDL_Texture* startTex = renderText("Start", {139,37,0,255});
     if (startTex) {
        int sw, sh;
        SDL_QueryTexture(startTex, nullptr, nullptr, &sw, &sh);
        SDL_Rect sr = {
            START_BTN_RECT.x + (START_BTN_RECT.w - sw)/2,
            START_BTN_RECT.y + (START_BTN_RECT.h - sh)/2,
            sw, sh
        };
        SDL_RenderCopy(gRenderer, startTex, nullptr, &sr);
        SDL_DestroyTexture(startTex);
    }

    SDL_Rect mr = MUTE_BTN_RECT;
    SDL_Texture* ico = gMute ? gIconMute : gIconUnmute;
    if (ico) SDL_RenderCopy(gRenderer, ico, nullptr, &mr);

    SDL_RenderPresent(gRenderer);
}


void handlePlaceTile(vector<Tile>& stack, int& score, float& desiredCamY) {
    if (stack.size() < 2) return; 

    Tile& curr = stack.back();
    Tile& prev = stack[stack.size() - 2];
    int L = max(curr.rect.x, prev.rect.x);
    int R = min(curr.rect.x + curr.rect.w, prev.rect.x + prev.rect.w);
    int W = R - L;

    if (W <= 0) {
        gState = GameState::GAME_OVER;

    } else {
        
        bool isPerfect = (abs(curr.rect.x - prev.rect.x) < 2 && W >= prev.rect.w - 2);

        if (isPerfect) {
            perfectTimer = PERFECT_SHOW_MS;
            if (gPerfectSfx) Mix_PlayChannel(-1, gPerfectSfx, 0);
            score += 5;
            curr.rect.x = prev.rect.x;
            curr.rect.w = prev.rect.w;
        } else {
            if (gPlaceSfx) Mix_PlayChannel(-1, gPlaceSfx, 0);
            score += 1;
            curr.rect.x = L;
            curr.rect.w = W;
            curr.speed = min(MAX_SPEED, (int)(curr.speed + SPEED_INCREMENT));

        }

        curr.rect.y = prev.rect.y - TILE_HEIGHT;

        if (stack.size() >= 5) {
            desiredCamY = (float)(curr.rect.y - SCREEN_MARGIN_TOP);
        }

        // Thêm thanh di chuyển tiếp theo
        stack.push_back(Tile{
            {0, curr.rect.y - TILE_HEIGHT, curr.rect.w, TILE_HEIGHT},
            curr.speed, 
            (rand() % 2 == 0), 
            COLORS[rand() % 5]
        });
    }
}


void run() {
    bool quit = false;
    SDL_Event e;
    vector<Tile> stack;
    int score = 0;
    Uint32 lastTime = SDL_GetTicks();
    float desiredCamY = 0.0f;

    while (!quit) {
        Uint32 now = SDL_GetTicks();
        float delta = (now - lastTime) / 1000.0f;
        if (delta == 0) { 
            delta = 1.0f / 60.0f; 
        }
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
                    else {
                        if (Mix_PausedMusic()) Mix_ResumeMusic();
                        else if (gMusic) Mix_PlayMusic(gMusic, -1);
                    }
                } else if (SDL_PointInRect(&p, &START_BTN_RECT)) {
                    stack.clear();
                    score = 0;
                    cameraY_f = 0;
                    desiredCamY = 0;
                    int baseY = WINDOW_HEIGHT - TILE_HEIGHT;
                    stack.push_back(Tile{
                        {(WINDOW_WIDTH - INITIAL_TILE_WIDTH) / 2, baseY, INITIAL_TILE_WIDTH, TILE_HEIGHT},
                        0, // Thanh mốc đứng im
                        false,
                        COLORS[rand() % 5]
                    });
                    stack.push_back(Tile{
                        {0, baseY - TILE_HEIGHT, INITIAL_TILE_WIDTH, TILE_HEIGHT},
                        INITIAL_SPEED,
                        true,
                        COLORS[rand() % 5]
                    });
                    if (!gMute && gMusic) Mix_PlayMusic(gMusic, -1);
                    gState = GameState::PLAYING;
                }

            } else if (gState == GameState::PLAYING) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Point p{e.button.x, e.button.y};
                    if (SDL_PointInRect(&p, &MUTE_BTN_RECT)) {
                        gMute = !gMute;
                        if (gMute) Mix_PauseMusic();
                        else {
                             if (Mix_PausedMusic()) Mix_ResumeMusic();
                             else if (gMusic) Mix_PlayMusic(gMusic, -1);
                        }
                    } else {
                        handlePlaceTile(stack, score, desiredCamY);
                    }
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        // Nhấn phím Space -> đặt gạch
                        handlePlaceTile(stack, score, desiredCamY);
                    }
                }
            } else if (gState == GameState::GAME_OVER && e.type == SDL_MOUSEBUTTONDOWN) {
                gTopScore = max(gTopScore, score);
                gState = GameState::MENU;
                cameraY_f = 0.0f;
                desiredCamY = 0.0f;
            }
        }
        if (quit) break;


        if (gState == GameState::PLAYING && stack.size() >= 2) { 
            Tile& t = stack.back();
            int dir = t.movingRight ? 1 : -1;
            t.rect.x += int(dir * t.speed * delta);

            if (t.rect.x + t.rect.w > WINDOW_WIDTH) {
                t.movingRight = false;
                t.rect.x = WINDOW_WIDTH - t.rect.w; 
            }
            if (t.rect.x < 0) {
                t.movingRight = true;
                t.rect.x = 0; 
            }
        }


        if (gState == GameState::PLAYING) {
            cameraY_f += (desiredCamY - cameraY_f) * CAMERA_LERP;
            if (abs(cameraY_f - desiredCamY) < 0.5f) { 
                cameraY_f = desiredCamY;
            }
        }
        int cameraY = int(round(cameraY_f));


        if (gBgTex) SDL_RenderCopy(gRenderer, gBgTex, nullptr, nullptr);
        else {
            SDL_SetRenderDrawColor(gRenderer, 135, 206, 235, 255); 
            SDL_RenderClear(gRenderer);
        }


        if (gState == GameState::MENU) {
            drawMenu();
        } else if (gState == GameState::PLAYING) {
            SDL_Color whiteColor = {255, 255, 255, 255};
            SDL_Texture* currScoreTex = renderText(to_string(score), whiteColor);
            if (currScoreTex) {
                int sw, sh;
                SDL_QueryTexture(currScoreTex, nullptr, nullptr, &sw, &sh);
                SDL_Rect scoreRect = { 10, 10, sw, sh };
                SDL_RenderCopy(gRenderer, currScoreTex, nullptr, &scoreRect);
                SDL_DestroyTexture(currScoreTex);
            }

            for (const auto& tile : stack) { 
                SDL_Rect dst = tile.rect;
                dst.y -= cameraY;

                SDL_SetRenderDrawColor(gRenderer, tile.color.r, tile.color.g, tile.color.b, tile.color.a);
                SDL_RenderFillRect(gRenderer, &dst);

                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255); 
                SDL_RenderDrawRect(gRenderer, &dst);
            }

            if (perfectTimer > 0) {
                perfectTimer -= int(delta * 1000);
                if (perfectTimer < 0) perfectTimer = 0;

                SDL_Color c = {255,215,0, 255}; 
                SDL_Texture* txt = renderText("Perfect +5", c);
                if (txt) {
                    int tw, th;
                    SDL_QueryTexture(txt, nullptr, nullptr, &tw, &th);
                    SDL_Rect trg = { (WINDOW_WIDTH-tw)/2, WINDOW_HEIGHT / 3, tw, th };
                    SDL_RenderCopy(gRenderer, txt, nullptr, &trg);
                    SDL_DestroyTexture(txt);
                }
            }

            SDL_Rect mr = MUTE_BTN_RECT;
            SDL_Texture* ico = gMute ? gIconMute : gIconUnmute;
            if (ico) SDL_RenderCopy(gRenderer, ico, nullptr, &mr);

            SDL_RenderPresent(gRenderer);
        } else if (gState == GameState::GAME_OVER) {
            SDL_Color black = {0, 0, 0, 255}; 
            SDL_Texture* overTex = renderText("Game Over", black);
            if (overTex) {
                int ow, oh;
                SDL_QueryTexture(overTex, nullptr, nullptr, &ow, &oh);
                SDL_Rect orc = {(WINDOW_WIDTH - ow) / 2, WINDOW_HEIGHT / 2 - 60, ow, oh};
                SDL_RenderCopy(gRenderer, overTex, nullptr, &orc);
                SDL_DestroyTexture(overTex);
            }

            string s = "Score: " + to_string(score);
            SDL_Texture* scoreTex = renderText(s, black);
            if (scoreTex) {
                int sw, sh;
                SDL_QueryTexture(scoreTex, nullptr, nullptr, &sw, &sh);
                SDL_Rect src = {(WINDOW_WIDTH - sw) / 2, WINDOW_HEIGHT / 2 - 20, sw, sh};
                SDL_RenderCopy(gRenderer, scoreTex, nullptr, &src);
                SDL_DestroyTexture(scoreTex);
            }

            string finalTopScore = "Top Score: " + to_string(max(gTopScore, score));
            SDL_Texture* topScoreText = renderText(finalTopScore, black);
            if(topScoreText){
                int tsw, tsh;
                SDL_QueryTexture(topScoreText, nullptr, nullptr, &tsw, &tsh);
                SDL_Rect tsr = {(WINDOW_WIDTH - tsw)/2, WINDOW_HEIGHT/2 + 20, tsw, tsh};
                SDL_RenderCopy(gRenderer, topScoreText, nullptr, &tsr);
                SDL_DestroyTexture(topScoreText);
             }


            SDL_RenderPresent(gRenderer);
        }
        SDL_Delay(16); 
    }
}

int main(int argc, char* argv[]) {
    if (!initSDL()) {
         SDL_Log("Exiting: initSDL failed.");
         return -1;
    }
    if (!loadAssets()) {
        SDL_Log("Exiting: loadAssets failed.");
        cleanupSDL(); 
        return -1;
    }

    run();
    cleanupSDL();
    return 0;
}
