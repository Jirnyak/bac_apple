
#define SDL_MAIN_HANDLED
#include <iostream>
#include <vector>
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>

using namespace std;

#define WORLD_WIDTH 1440
#define WORLD_LENGTH 1080
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 540
#define SNUMBER 3000000
#define MAX_SPORES 100000 

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
    float float_x, float_y; // For spores
    int x, y;               // Grid coordinates
    float hp;
    float hp_max;
    bool is_spore;
    bool is_dormant;
    bool is_dead;
    uint8_t r, g, b;

    bool is_predator() const { return g < 127; }
    bool is_motile() const { return r > 127; }
    bool is_hibernator() const { return b > 127; }

    int phenotype_idx() const {
        int idx = 0;
        if (is_predator()) idx |= 1;
        if (is_motile()) idx |= 2;
        if (is_hibernator()) idx |= 4;
        return idx;
    }
};

inline int tor_cord(int val, int max_val) {
    val = val % max_val;
    if (val < 0) val += max_val;
    return val;
}

Bac create_bac(float fx, float fy, bool as_spore, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, bool inherit_color = false) {
    Bac bac;
    bac.float_x = fx; bac.float_y = fy;
    bac.x = tor_cord((int)fx, WORLD_WIDTH);
    bac.y = tor_cord((int)fy, WORLD_LENGTH);
    bac.hp = 0.5f + fast_randf();
    bac.hp_max = 2.0f;
    bac.is_spore = as_spore;
    bac.is_dormant = false;
    bac.is_dead = false;
    
    if (inherit_color) {
        bac.r = r; bac.g = g; bac.b = b;
    } else {
        bac.r = xorshift32() % 256;
        bac.g = xorshift32() % 256;
        bac.b = xorshift32() % 256;
    }
    return bac;
}

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

void read_im(vector<bool>& ww, int idx) {
    string name = "mapple/bad_apple_" + to_string(idx) + ".png";
    SDL_Surface* surface = IMG_Load(name.c_str());
    if (surface) {
        SDL_LockSurface(surface);
        for (int y = 0; y < WORLD_LENGTH; ++y) {
            for (int x = 0; x < WORLD_WIDTH; ++x) {
                Uint32 pixel = getpixel(surface, x, y);
                Uint8 r, g, b;
                SDL_GetRGB(pixel, surface->format, &r, &g, &b);
                ww[y * WORLD_WIDTH + x] = (r > 100);
            }
        }
        SDL_UnlockSurface(surface);
        SDL_FreeSurface(surface);
    }
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("BacApple V5", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    vector<bool> world_white(WORLD_WIDTH * WORLD_LENGTH, false);
    
    // 8 layers for 8 phenotypes
    vector<int> world_grid[8];
    for (int i=0; i<8; i++) {
        world_grid[i].assign(WORLD_WIDTH * WORLD_LENGTH, -1);
    }
    
    vector<Bac> bacs;
    vector<Bac> new_children;
    bacs.reserve(SNUMBER + MAX_SPORES);
    new_children.reserve(SNUMBER);

    int current_spores = 0;
    for (int k = 0; k < MAX_SPORES; k++) {
        bacs.push_back(create_bac(xorshift32() % WORLD_WIDTH, xorshift32() % WORLD_LENGTH, true));
        current_spores++;
    }

    int TICKS_PER_FRAME = 10; 
    int PHYSICS_STEPS = TICKS_PER_FRAME;
    int simulation_ticks = 40 * TICKS_PER_FRAME;

    bool quit = false;
    bool paused = false;
    SDL_Event event;

    int frames = 0;
    int frames_total = 0;
    auto last_time = std::chrono::steady_clock::now();

    read_im(world_white, simulation_ticks / TICKS_PER_FRAME);
    vector<uint32_t> pixels(WINDOW_WIDTH * WINDOW_HEIGHT, 0xFF000000);

    const int dx[5] = {0, 0, 0, -1, 1};
    const int dy[5] = {0, -1, 1, 0, 0};

    while (!quit) {
        Uint32 loop_start = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_SPACE: paused = !paused; break;
                    case SDLK_ESCAPE: quit = true; break;
                }
            }
            if (event.type == SDL_QUIT) quit = true;
        }

        if (!paused) {
            for (int step = 0; step < PHYSICS_STEPS; step++) {
                // Populate grids
                for (int i=0; i<8; i++) std::fill(world_grid[i].begin(), world_grid[i].end(), -1);
                
                for (int i = 0; i < bacs.size(); i++) {
                    if (!bacs[i].is_spore && !bacs[i].is_dead) {
                        int layer = bacs[i].phenotype_idx();
                        world_grid[layer][bacs[i].y * WORLD_WIDTH + bacs[i].x] = i;
                    }
                }

                current_spores = 0; 

                for (int i = 0; i < bacs.size(); i++) {
                    Bac& b = bacs[i];
                    if (b.is_dead) continue;

                    bool is_white = world_white[b.y * WORLD_WIDTH + b.x];

                    if (b.is_spore) {
                        b.float_x += (int)(xorshift32() % 60) - 30;
                        b.float_y += (int)(xorshift32() % 60) - 30;
                        b.x = tor_cord((int)b.float_x, WORLD_WIDTH);
                        b.y = tor_cord((int)b.float_y, WORLD_LENGTH);
                        
                        is_white = world_white[b.y * WORLD_WIDTH + b.x];
                        
                        if (is_white) {
                            int layer = b.phenotype_idx();
                            if (world_grid[layer][b.y * WORLD_WIDTH + b.x] == -1) {
                                b.is_spore = false;
                                b.hp = b.hp_max;
                                world_grid[layer][b.y * WORLD_WIDTH + b.x] = i;
                                continue;
                            }
                        }
                        current_spores++;
                        continue;
                    }

                    int b_layer = b.phenotype_idx();

                    if (b.is_dormant) {
                        bool can_wake = false;
                        if (!b.is_predator() && is_white) {
                            can_wake = true;
                        } else if (b.is_predator()) {
                            // Check for prey
                            for (int dir = 0; dir < 5; dir++) {
                                int nx = tor_cord(b.x + dx[dir], WORLD_WIDTH);
                                int ny = tor_cord(b.y + dy[dir], WORLD_LENGTH);
                                for (int l = 0; l < 8; l++) {
                                    if (l == b_layer) continue;
                                    int n_idx = world_grid[l][ny * WORLD_WIDTH + nx];
                                    if (n_idx != -1 && !bacs[n_idx].is_dead && !bacs[n_idx].is_spore) {
                                        can_wake = true; break;
                                    }
                                }
                                if (can_wake) break;
                            }
                        }
                        if (can_wake) {
                            b.is_dormant = false;
                            b.hp = 0.5f;
                        } else {
                            continue;
                        }
                    }

                    // Metabolism
                    b.hp -= 0.1f;

                    if (!b.is_predator() && is_white) {
                        b.hp += 0.5f; 
                    }

                    if (b.is_predator()) {
                        // Look for prey across all layers in the same cell and adjacent cells
                        for (int dir = 0; dir < 5; dir++) {
                            int nx = tor_cord(b.x + dx[dir], WORLD_WIDTH);
                            int ny = tor_cord(b.y + dy[dir], WORLD_LENGTH);
                            for (int l = 0; l < 8; l++) {
                                if (l == b_layer) continue; // Don't eat same phenotype
                                int n_idx = world_grid[l][ny * WORLD_WIDTH + nx];
                                if (n_idx != -1) {
                                    Bac& n = bacs[n_idx];
                                    if (!n.is_spore && !n.is_dead) {
                                        b.hp += n.hp;
                                        n.hp = 0;
                                        n.is_dead = true;
                                        world_grid[l][ny * WORLD_WIDTH + nx] = -1; 
                                    }
                                }
                            }
                        }
                    }

                    if (!b.is_motile()) {
                        // Diffusion
                        for (int dir = 1; dir < 5; dir++) {
                            int nx = tor_cord(b.x + dx[dir], WORLD_WIDTH);
                            int ny = tor_cord(b.y + dy[dir], WORLD_LENGTH);
                            int n_idx = world_grid[b_layer][ny * WORLD_WIDTH + nx];
                            if (n_idx != -1) {
                                Bac& n = bacs[n_idx];
                                if (!n.is_spore && !n.is_dead && !n.is_dormant) {
                                    if (b.hp > n.hp) {
                                        float diff = (b.hp - n.hp) * 0.1f;
                                        b.hp -= diff;
                                        n.hp += diff;
                                    }
                                }
                            }
                        }
                    } else {
                        // Movement
                        int dir = 1 + (xorshift32() % 4);
                        int nx = tor_cord(b.x + dx[dir], WORLD_WIDTH);
                        int ny = tor_cord(b.y + dy[dir], WORLD_LENGTH);
                        if (world_grid[b_layer][ny * WORLD_WIDTH + nx] == -1) {
                            world_grid[b_layer][b.y * WORLD_WIDTH + b.x] = -1;
                            b.x = nx; b.y = ny;
                            b.float_x = nx; b.float_y = ny;
                            world_grid[b_layer][b.y * WORLD_WIDTH + b.x] = i;
                        }
                    }

                    if (b.hp > 2.0f && (bacs.size() + new_children.size() < SNUMBER)) {
                        int dir = 1 + (xorshift32() % 4);
                        int nx = tor_cord(b.x + dx[dir], WORLD_WIDTH);
                        int ny = tor_cord(b.y + dy[dir], WORLD_LENGTH);
                        
                        Bac child = create_bac((float)nx, (float)ny, false, b.r, b.g, b.b, true);
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
                        
                        int c_layer = child.phenotype_idx();
                        if (world_grid[c_layer][ny * WORLD_WIDTH + nx] == -1) {
                            b.hp = 1.0f;
                            new_children.push_back(child);
                            world_grid[c_layer][ny * WORLD_WIDTH + nx] = -2; // Reserve
                        }
                    }

                    if (b.hp <= 0) {
                        if (b.is_hibernator()) {
                            b.is_dormant = true;
                            b.hp = 0; // lock at 0
                        } else {
                            b.is_dead = true;
                            world_grid[b_layer][b.y * WORLD_WIDTH + b.x] = -1;
                        }
                    }
                }

                // Compact and add children
                int write_idx = 0;
                for (int i = 0; i < bacs.size(); i++) {
                    if (!bacs[i].is_dead) {
                        bacs[write_idx++] = bacs[i];
                    }
                }
                bacs.resize(write_idx);
                for (auto& child : new_children) {
                    bacs.push_back(child);
                }
                new_children.clear();

                while (current_spores < MAX_SPORES) {
                    bacs.push_back(create_bac(xorshift32() % WORLD_WIDTH, xorshift32() % WORLD_LENGTH, true));
                    current_spores++;
                }

                simulation_ticks++;
            }

            read_im(world_white, simulation_ticks / TICKS_PER_FRAME);

            std::fill(pixels.begin(), pixels.end(), 0xFF000000); 

            // Rendering: For each pixel, collect all occupying phenotypes
            // and pick one randomly to draw
            vector<int> occupying;
            occupying.reserve(8);
            for (int py = 0; py < WINDOW_HEIGHT; py++) {
                for (int px = 0; px < WINDOW_WIDTH; px++) {
                    occupying.clear();
                    // Each 2x2 block maps to 1 window pixel. We just sample the top-left for speed,
                    // or sample all 8 layers at (px*2, py*2).
                    int bx = px * 2;
                    int by = py * 2;
                    for (int l = 0; l < 8; l++) {
                        int b_idx = world_grid[l][by * WORLD_WIDTH + bx];
                        if (b_idx >= 0 && !bacs[b_idx].is_dormant) {
                            occupying.push_back(b_idx);
                        }
                    }
                    if (!occupying.empty()) {
                        int chosen = occupying[xorshift32() % occupying.size()];
                        const Bac& b = bacs[chosen];
                        pixels[py * WINDOW_WIDTH + px] = 0xFF000000 | (b.r << 16) | (b.g << 8) | b.b;
                    }
                }
            }

            SDL_UpdateTexture(texture, NULL, pixels.data(), WINDOW_WIDTH * sizeof(uint32_t));
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

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
        } else {
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
