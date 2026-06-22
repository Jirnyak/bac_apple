
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
    int pos;               // Grid coordinates
    int food;
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

int grid_neighbors[WORLD_WIDTH * WORLD_LENGTH][4];

void init_neighbors() {
    for (int y = 0; y < WORLD_LENGTH; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            int pos = y * WORLD_WIDTH + x;
            int up_y = (y == 0) ? WORLD_LENGTH - 1 : y - 1;
            int down_y = (y == WORLD_LENGTH - 1) ? 0 : y + 1;
            int left_x = (x == 0) ? WORLD_WIDTH - 1 : x - 1;
            int right_x = (x == WORLD_WIDTH - 1) ? 0 : x + 1;
            
            grid_neighbors[pos][0] = up_y * WORLD_WIDTH + x;
            grid_neighbors[pos][1] = y * WORLD_WIDTH + right_x;
            grid_neighbors[pos][2] = down_y * WORLD_WIDTH + x;
            grid_neighbors[pos][3] = y * WORLD_WIDTH + left_x;
        }
    }
}

Bac create_bac(int pos, bool as_spore, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, bool inherit_color = false) {
    Bac bac;
    bac.pos = pos;
    bac.food = 10;
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

    init_neighbors();
    vector<bool> world_white(WORLD_WIDTH * WORLD_LENGTH, false);
    
    // 8 layers for 8 phenotypes
    vector<int> world_grid[8];
    for (int i=0; i<8; i++) {
        world_grid[i].assign(WORLD_WIDTH * WORLD_LENGTH, -1);
    }
    
    vector<Bac> bacs;
    vector<Bac> new_children;
    bacs.reserve(WORLD_WIDTH * WORLD_LENGTH * 2);
    new_children.reserve(100000);

    for (int k = 0; k < WORLD_WIDTH * WORLD_LENGTH / 2; k++) {
        Bac bac = create_bac(xorshift32() % (WORLD_WIDTH * WORLD_LENGTH), false);
        int layer = bac.phenotype_idx();
        if (world_grid[layer][bac.pos] == -1) {
            world_grid[layer][bac.pos] = bacs.size();
            bacs.push_back(bac);
        }
    }

    int TICKS_PER_FRAME = 100; 
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
                        world_grid[layer][bacs[i].pos] = i;
                    }
                }

                for (int i = 0; i < bacs.size(); i++) {
                    Bac& b = bacs[i];
                    if (b.is_dead) continue;

                    bool is_white = world_white[b.pos];

                    if (b.is_spore) {
                        b.pos = grid_neighbors[b.pos][xorshift32() % 4];
                        
                        is_white = world_white[b.pos];
                        
                        if (is_white) {
                            int layer = b.phenotype_idx();
                            if (world_grid[layer][b.pos] == -1) {
                                b.is_spore = false;
                                world_grid[layer][b.pos] = i;
                            }
                        }
                    }

                    int b_layer = b.phenotype_idx();

                    if (b.is_dormant) {
                        bool can_wake = false;
                        if (!b.is_predator() && is_white) {
                            can_wake = true;
                        } else if (b.is_predator()) {
                            // Check for prey
                            for (int dir = 0; dir < 4; dir++) {
                                int npos = grid_neighbors[b.pos][dir];
                                for (int l = 0; l < 8; l++) {
                                    if (l == b_layer) continue;
                                    int n_idx = world_grid[l][npos];
                                    if (n_idx >= 0 && !bacs[n_idx].is_dead && !bacs[n_idx].is_spore) {
                                        can_wake = true; break;
                                    }
                                }
                                if (can_wake) break;
                            }
                        }
                        if (can_wake) {
                            b.is_dormant = false;
                            b.food = 5;
                        } else {
                            continue;
                        }
                    }
                    if (b.is_spore) continue;

                    // Metabolism
                    b.food -= 1;

                    if (!b.is_predator() && is_white) {
                        b.food += 2; 
                    }

                    if (b.food <= 0) {
                        if (b.is_hibernator() && !b.is_spore) {
                            b.is_dormant = true;
                            b.food = 0; // lock at 0
                        } else {
                            b.is_dead = true;
                            if (!b.is_spore) world_grid[b_layer][b.pos] = -1;
                        }
                        continue;
                    }

                    if (b.is_predator()) {
                        int non_predators[40];
                        int non_predator_count = 0;
                        int other_predators[40];
                        int other_predator_count = 0;

                        // Gather potential prey
                        for (int dir = 0; dir < 4; dir++) {
                            int npos = grid_neighbors[b.pos][dir];
                            for (int l = 0; l < 8; l++) {
                                if (l == b_layer) continue; // Don't eat same phenotype
                                int n_idx = world_grid[l][npos];
                                if (n_idx >= 0) {
                                    Bac& n = bacs[n_idx];
                                    if (!n.is_spore && !n.is_dead && !n.is_dormant) {
                                        if (!n.is_predator()) {
                                            non_predators[non_predator_count++] = n_idx;
                                        } else {
                                            other_predators[other_predator_count++] = n_idx;
                                        }
                                    }
                                }
                            }
                        }

                        if (non_predator_count > 0) {
                            int target_idx = non_predators[xorshift32() % non_predator_count];
                            Bac& n = bacs[target_idx];
                            b.food += n.food;
                            n.food = 0;
                            n.is_dead = true;
                            world_grid[n.phenotype_idx()][n.pos] = -1; 
                        } else if (other_predator_count > 0) {
                            int my_allies = 0;
                            for (int d = 0; d < 4; d++) {
                                int npos = grid_neighbors[b.pos][d];
                                if (world_grid[b_layer][npos] >= 0) my_allies++;
                            }

                            int valid_other_predators[40];
                            int valid_count = 0;

                            for (int i = 0; i < other_predator_count; i++) {
                                int n_idx = other_predators[i];
                                Bac& n = bacs[n_idx];
                                int n_layer = n.phenotype_idx();
                                
                                int their_allies = 0;
                                for (int d = 0; d < 4; d++) {
                                    int npos = grid_neighbors[n.pos][d];
                                    if (world_grid[n_layer][npos] >= 0) their_allies++;
                                }

                                if (my_allies > their_allies) {
                                    valid_other_predators[valid_count++] = n_idx;
                                }
                            }

                            if (valid_count > 0) {
                                int target_idx = valid_other_predators[xorshift32() % valid_count];
                                Bac& n = bacs[target_idx];
                                b.food += n.food;
                                n.food = 0;
                                n.is_dead = true;
                                world_grid[n.phenotype_idx()][n.pos] = -1; 
                            }
                        }
                    }

                    if (!b.is_motile()) {
                        // Diffusion
                        for (int dir = 0; dir < 4; dir++) {
                            int npos = grid_neighbors[b.pos][dir];
                            int n_idx = world_grid[b_layer][npos];
                            if (n_idx >= 0) {
                                Bac& n = bacs[n_idx];
                                if (!n.is_spore && !n.is_dead && !n.is_dormant) {
                                    if (b.food > n.food + 1) {
                                        int diff = (b.food - n.food) / 2;
                                        b.food -= diff;
                                        n.food += diff;
                                    }
                                }
                            }
                        }
                    } else {
                        // Movement
                        int dir = 1 + (xorshift32() % 4);
                        int npos = grid_neighbors[b.pos][dir];
                        if (world_grid[b_layer][npos] == -1) {
                            world_grid[b_layer][b.pos] = -1;
                            b.pos = npos;
                            world_grid[b_layer][b.pos] = i;
                        }
                    }

                    if (b.food >= 20) {
                        int dir = 1 + (xorshift32() % 4);
                        int npos = grid_neighbors[b.pos][dir];
                        
                        bool is_child_spore = (xorshift32() % 2 == 0);
                        Bac child = create_bac(npos, is_child_spore, b.r, b.g, b.b, true);
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
                        
                        if (is_child_spore) {
                            b.food -= 10;
                            new_children.push_back(child);
                        } else {
                            int c_layer = child.phenotype_idx();
                            if (world_grid[c_layer][npos] == -1) {
                                b.food -= 10;
                                new_children.push_back(child);
                                world_grid[c_layer][npos] = -2; // Reserve
                            }
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
                    for (int dy2 = 0; dy2 < 2; dy2++) {
                        for (int dx2 = 0; dx2 < 2; dx2++) {
                            int bx = px * 2 + dx2;
                            int by = py * 2 + dy2;
                            for (int l = 0; l < 8; l++) {
                                int bpos = by * WORLD_WIDTH + bx;
                                int b_idx = world_grid[l][bpos];
                                if (b_idx >= 0 && !bacs[b_idx].is_dormant) {
                                    occupying.push_back(b_idx);
                                }
                            }
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
                cout << "FPS: " << frames << " | Pop: " << bacs.size() << " | Frame: " << (simulation_ticks / TICKS_PER_FRAME) << endl;
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
