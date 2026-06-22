#define SDL_MAIN_HANDLED

#include <iostream>
#include <vector>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <cmath>
#include <chrono>

using namespace std;

#define WORLD_WIDTH 1440
#define WORLD_LENGTH 1080
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 540
#define SNUMBER 4000000
#define MAX_SPORES 100000 
#define BLOCK_TARGET 50  
#define BLOCK_MAX 100

// Fast PRNG: Xorshift32
uint32_t xorshift32_state = 123456789;

inline uint32_t xorshift32() {
    uint32_t x = xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift32_state = x;
    return x;
}

inline float fast_randf() {
    return (xorshift32() & 0xFFFFFF) / (float)0x1000000;
}

struct Bac {
    float x_loc;
    float y_loc;
    float hp;
    float hp_max;
    int age;
    bool is_spore;
    uint8_t r, g, b;
};

inline int tor_cord(int val, int max_val) {
    if (val < 0) return val + max_val;
    if (val >= max_val) return val - max_val;
    return val;
}

Bac create_bac(float x, float y, bool as_spore, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, bool inherit_color = false) {
    Bac bac;
    bac.x_loc = x;
    bac.y_loc = y;
    bac.hp = 0.5f + fast_randf();
    bac.hp_max = bac.hp;
    bac.age = 150 + (xorshift32() % 100);
    bac.is_spore = as_spore;
    if (inherit_color) {
        bac.r = r; bac.g = g; bac.b = b;
    } else {
        bac.r = xorshift32() % 256;
        bac.g = xorshift32() % 256;
        bac.b = xorshift32() % 256;
    }
    return bac;
}

// Safer pixel extraction for various formats
Uint32 getpixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    switch(bpp) {
        case 1: return *p;
        case 2: return *(Uint16 *)p;
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) return p[0] << 16 | p[1] << 8 | p[2];
            else return p[0] | p[1] << 8 | p[2] << 16;
        case 4: return *(Uint32 *)p;
        default: return 0;
    }
}

std::unordered_map<int, std::vector<bool>> frame_cache;

void read_im(std::vector<bool>& world_white, int frame_n) 
{
    if (frame_cache.count(frame_n)) {
        world_white = frame_cache[frame_n];
        return;
    }

    int frama = frame_n;
    string frame_name = "mapple/bad_apple_" + to_string(frama) + ".png";
    SDL_Surface* surface = IMG_Load(frame_name.c_str());
    while (!surface && frama > 0)
    {
        frama -= 1;
        if (frame_cache.count(frama)) {
            world_white = frame_cache[frama];
            frame_cache[frame_n] = frame_cache[frama];
            return;
        }
        frame_name = "mapple/bad_apple_" + to_string(frama) + ".png";
        surface = IMG_Load(frame_name.c_str());
    }
    
    if (!surface) {
        std::fill(world_white.begin(), world_white.end(), false);
        return;
    }

    SDL_LockSurface(surface);
    for (int y = 0; y < WORLD_LENGTH; ++y) 
    {
        for (int x = 0; x < WORLD_WIDTH; ++x) 
        {
            if (y < surface->h && x < surface->w)
            {
                Uint32 pixel = getpixel(surface, x, y);
                Uint8 r, g, b;
                SDL_GetRGB(pixel, surface->format, &r, &g, &b);
                world_white[y * WORLD_WIDTH + x] = (r > 100);
            }
            else {
                world_white[y * WORLD_WIDTH + x] = false;
            }
        }
    }
    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);

    frame_cache[frame_n] = world_white;
}

int main(int argc, char **argv) 
{
    xorshift32_state = std::chrono::steady_clock::now().time_since_epoch().count();
    if (xorshift32_state == 0) xorshift32_state = 1;

    vector<bool> world_white(WORLD_WIDTH * WORLD_LENGTH, false);

    int blocks_x = WORLD_WIDTH / 10;
    int blocks_y = WORLD_LENGTH / 10;
    vector<int> block_density(blocks_x * blocks_y, 0);

    SDL_Init(SDL_INIT_VIDEO);
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << endl;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowFullscreen(window, 0);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    vector<uint32_t> pixels(WINDOW_WIDTH * WINDOW_HEIGHT, 0);

    vector<Bac> bacs;
    vector<Bac> bacs_buff;
    // Make sure we have enough capacity for both maximum awake bacteria and the spore cloud
    bacs.reserve(SNUMBER + MAX_SPORES);
    bacs_buff.reserve(SNUMBER + MAX_SPORES);

    int current_spores = 0;
    for(int k = 0; k < MAX_SPORES; k++) {
        bacs.push_back(create_bac(xorshift32() % WORLD_WIDTH, xorshift32() % WORLD_LENGTH, true));
        current_spores++;
    }

    bool quit = false;
    bool paused = false;
    bool fullscreen = false;
    SDL_Event event;

    int frames = 0;
    int frames_total = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    // START AT FRAME 40 EXACTLY!
    // The previous implementation had simulation_ticks = 400, but divided by TICKS_PER_FRAME (30), 
    // which resulted in frame 13 being requested. Frame 13 is COMPLETELY BLACK. 
    // This is why you saw a black screen!
    int TICKS_PER_FRAME = 30; 
    int simulation_ticks = 40 * TICKS_PER_FRAME; 

    while (!quit) 
    {
        Uint32 loop_start = SDL_GetTicks();

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_SPACE: paused = !paused; break;
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_0:
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
                        break;
                }
            }
            if (event.type == SDL_QUIT) quit = true;
        }

        if (!paused)
        {
            int PHYSICS_STEPS = 10;
            
            for (int step = 0; step < PHYSICS_STEPS; step++) 
            {
                std::fill(block_density.begin(), block_density.end(), 0);
                for (const auto& b : bacs) {
                    if (!b.is_spore) {
                        int bx = ((int)b.x_loc) / 10;
                        int by = ((int)b.y_loc) / 10;
                        if (bx >= 0 && bx < blocks_x && by >= 0 && by < blocks_y) {
                            block_density[by * blocks_x + bx]++;
                        }
                    }
                }

                current_spores = 0; 

                for (auto& b : bacs)
                {
                    int ix = (int)b.x_loc;
                    int iy = (int)b.y_loc;
                    if (ix < 0) ix = 0; else if (ix >= WORLD_WIDTH) ix = WORLD_WIDTH - 1;
                    if (iy < 0) iy = 0; else if (iy >= WORLD_LENGTH) iy = WORLD_LENGTH - 1;

                    bool is_white = world_white[iy * WORLD_WIDTH + ix];

                    int block_idx = (iy / 10) * blocks_x + (ix / 10);
                    int local_pop = block_density[block_idx];

                    if (b.is_spore) {
                        b.x_loc += (int)(xorshift32() % 60) - 30;
                        b.y_loc += (int)(xorshift32() % 60) - 30;
                        b.x_loc = tor_cord((int)b.x_loc, WORLD_WIDTH);
                        b.y_loc = tor_cord((int)b.y_loc, WORLD_LENGTH);

                        if (is_white && local_pop < BLOCK_MAX) { 
                            b.is_spore = false; 
                            b.hp = b.hp_max; 
                            b.age = 150 + (xorshift32() % 100); 
                            block_density[block_idx]++;
                        } else {
                            current_spores++;
                        }
                        
                        bacs_buff.push_back(b);
                        continue;
                    }
                    
                    // For bacteria, calculate regeneration
                    if (is_white)
                    {
                        double local_celok_p = (BLOCK_TARGET * 1.0) / (local_pop + 1.0);
                        b.hp += (0.8f + 0.4f * fast_randf()) * local_celok_p * b.hp_max / 10.0f;
                    }
                    else
                    {
                        b.hp -= (0.8f + 0.4f * fast_randf()) * 5.0f * (xorshift32() % 2);
                    }

                    if (b.hp > 2.0f * b.hp_max && local_pop < BLOCK_MAX)
                    {
                        b.hp = b.hp_max;
                        Bac child = create_bac(b.x_loc, b.y_loc, false, b.r, b.g, b.b, true);
                        
                        // 50% chance to mutate one of the RGB channels by +/- 1
                        if (xorshift32() % 2 == 0) {
                            int channel = xorshift32() % 3;
                            if (channel == 0) {
                                if (child.r == 0) child.r++; else if (child.r == 255) child.r--; else child.r += (xorshift32() % 2 == 0 ? 1 : -1);
                            } else if (channel == 1) {
                                if (child.g == 0) child.g++; else if (child.g == 255) child.g--; else child.g += (xorshift32() % 2 == 0 ? 1 : -1);
                            } else {
                                if (child.b == 0) child.b++; else if (child.b == 255) child.b--; else child.b += (xorshift32() % 2 == 0 ? 1 : -1);
                            }
                        }

                        bacs_buff.push_back(child); 
                        block_density[block_idx]++; // Prevent massive overpopulation in a single tick
                    }

                    b.age -= (xorshift32() % 2);
                    
                    int walk_x = (int)(xorshift32() % 40) - 20;
                    int walk_y = (int)(xorshift32() % 40) - 20;
                    b.x_loc += walk_x;
                    b.y_loc += walk_y;

                    b.x_loc = tor_cord((int)b.x_loc, WORLD_WIDTH);
                    b.y_loc = tor_cord((int)b.y_loc, WORLD_LENGTH);

                    if (b.hp >= 0.0f && b.age >= 0)
                    {
                        bacs_buff.push_back(b);
                    }
                }

                // Cloud of spores is maintained completely independent of SNUMBER limit.
                // Always keep 100,000 sleeping spores ready to instantly seed new white frames!
                while (current_spores < MAX_SPORES) {
                    bacs_buff.push_back(create_bac(xorshift32() % WORLD_WIDTH, xorshift32() % WORLD_LENGTH, true));
                    current_spores++;
                }

                bacs.swap(bacs_buff);
                bacs_buff.clear();

                simulation_ticks++;
            }

            read_im(world_white, simulation_ticks / TICKS_PER_FRAME);

            std::fill(pixels.begin(), pixels.end(), 0xFF000000); 

            for (const auto& b : bacs) {
                if (b.is_spore) continue; 
                int px = (int)b.x_loc / 2;
                int py = (int)b.y_loc / 2;
                if (px >= 0 && px < WINDOW_WIDTH && py >= 0 && py < WINDOW_HEIGHT) {
                    uint32_t color = 0xFF000000 | (b.r << 16) | (b.g << 8) | b.b;
                    pixels[py * WINDOW_WIDTH + px] = color; 
                }
            }

            SDL_UpdateTexture(texture, NULL, pixels.data(), WINDOW_WIDTH * sizeof(uint32_t));
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            // Save frame to output_frames directory safely as 24-bit BMP
            SDL_Surface* out_surf_32 = SDL_CreateRGBSurfaceFrom(pixels.data(), WINDOW_WIDTH, WINDOW_HEIGHT, 32, WINDOW_WIDTH * 4, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            SDL_Surface* out_surf_24 = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
            SDL_BlitSurface(out_surf_32, NULL, out_surf_24, NULL);
            string out_path = "output_frames/frame_" + to_string(frames_total) + ".bmp";
            SDL_SaveBMP(out_surf_24, out_path.c_str());
            SDL_FreeSurface(out_surf_24);
            SDL_FreeSurface(out_surf_32);
            frames_total++;

            if (frames_total >= 19530) {
                cout << "Reached 19530 frames. Stopping simulation automatically!" << endl;
                quit = true;
            }

            frames++;
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = current_time - last_time;
            if (elapsed.count() >= 1.0) {
                cout << "FPS: " << frames << " | Awake: " << (bacs.size() - current_spores) << " | Spores: " << current_spores << " | Frame: " << (simulation_ticks / TICKS_PER_FRAME) << endl;
                frames = 0;
                last_time = current_time;
            }
        }
        else
        {
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        Uint32 loop_end = SDL_GetTicks();
        if (loop_end - loop_start < 33) {
            SDL_Delay(33 - (loop_end - loop_start));
        }
    }

    IMG_Quit();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}