#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <string>
#include <stdio.h>

#define WORLD_WIDTH 1000
#define WORLD_HEIGHT 750
#define PARTICLE_COUNT 100000

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

struct Particle {
    float x, y;
    float vx, vy;
};

struct Coord { short x, y; };

void run_jfa(const std::vector<bool>& seed_mask, std::vector<Coord>& closest, int W, int H) {
    for(int i=0; i<W*H; ++i) {
        if(seed_mask[i]) {
            closest[i] = {(short)(i%W), (short)(i/W)};
        } else {
            closest[i] = {-1, -1};
        }
    }
    
    std::vector<Coord> next_closest = closest;
    for(int step = 512; step >= 1; step /= 2) {
        for(int y=0; y<H; ++y) {
            for(int x=0; x<W; ++x) {
                Coord best = closest[y*W+x];
                float min_d2 = 1e9f;
                if (best.x != -1) {
                    float dx = best.x - x; if (dx > W/2) dx -= W; else if (dx < -W/2) dx += W;
                    float dy = best.y - y; if (dy > H/2) dy -= H; else if (dy < -H/2) dy += H;
                    min_d2 = dx*dx + dy*dy;
                }
                
                for(int dy_step=-1; dy_step<=1; ++dy_step) {
                    for(int dx_step=-1; dx_step<=1; ++dx_step) {
                        if (dx_step==0 && dy_step==0) continue;
                        int nx = x + dx_step * step;
                        int ny = y + dy_step * step;
                        if (nx < 0) nx += W; else if (nx >= W) nx -= W;
                        if (ny < 0) ny += H; else if (ny >= H) ny -= H;
                        
                        Coord c = closest[ny*W+nx];
                        if (c.x != -1) {
                            float cdx = c.x - x; if (cdx > W/2) cdx -= W; else if (cdx < -W/2) cdx += W;
                            float cdy = c.y - y; if (cdy > H/2) cdy -= H; else if (cdy < -H/2) cdy += H;
                            float d2 = cdx*cdx + cdy*cdy;
                            if (d2 < min_d2) {
                                min_d2 = d2;
                                best = c;
                            }
                        }
                    }
                }
                next_closest[y*W+x] = best;
            }
        }
        closest = next_closest;
    }
}

void blur_grid_toroidal(const std::vector<float>& src, std::vector<float>& dst, int W, int H) {
    for(int y=0; y<H; ++y) {
        int y0 = (y == 0) ? H - 1 : y - 1;
        int y1 = (y == H - 1) ? 0 : y + 1;
        for(int x=0; x<W; ++x) {
            int x0 = (x == 0) ? W - 1 : x - 1;
            int x1 = (x == W - 1) ? 0 : x + 1;

            dst[y*W+x] = (src[y*W+x]*4.0f + 
                          src[y0*W+x]*2.0f + src[y1*W+x]*2.0f + 
                          src[y*W+x0]*2.0f + src[y*W+x1]*2.0f +
                          src[y0*W+x0] + src[y0*W+x1] + 
                          src[y1*W+x0] + src[y1*W+x1]) / 16.0f;
        }
    }
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to init SDL\n";
        return 1;
    }
    IMG_Init(IMG_INIT_PNG);

    std::vector<uint32_t> pixels(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<uint8_t> rgb_buffer(WORLD_WIDTH * WORLD_HEIGHT * 3, 0);

    std::vector<bool> mask_white(WORLD_WIDTH * WORLD_HEIGHT, false);
    std::vector<bool> mask_black(WORLD_WIDTH * WORLD_HEIGHT, false);
    std::vector<Coord> closest_white(WORLD_WIDTH * WORLD_HEIGHT);
    std::vector<Coord> closest_black(WORLD_WIDTH * WORLD_HEIGHT);
    
    std::vector<float> Gx(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<float> Gy(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<float> Gx_blurred(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<float> Gy_blurred(WORLD_WIDTH * WORLD_HEIGHT, 0);

    std::vector<Particle> particles(PARTICLE_COUNT);
    for(int i=0; i<PARTICLE_COUNT; ++i) {
        particles[i].x = fast_randf() * WORLD_WIDTH;
        particles[i].y = fast_randf() * WORLD_HEIGHT;
        particles[i].vx = (fast_randf() - 0.5f) * 2.0f;
        particles[i].vy = (fast_randf() - 0.5f) * 2.0f;
    }

    std::vector<float> rho(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<float> rho_blurred(WORLD_WIDTH * WORLD_HEIGHT, 0);
    std::vector<float> V_electro(WORLD_WIDTH * WORLD_HEIGHT, 0);

    const float k_grav = 0.05f;
    const float k_elec = 0.5f;
    char image_path[256];

    FILE* ffmpeg = popen("ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 1000x750 -r 30 -i - -c:v libx264 -preset ultrafast -tune zerolatency -pix_fmt yuv420p output.mp4", "w");
    if (!ffmpeg) {
        std::cerr << "Failed to open ffmpeg pipe\n";
        return 1;
    }

    std::cout << "Starting render to output.mp4 (frames 40 to 6510)...\n";

    for(int current_frame = 40; current_frame <= 6510; current_frame++) {
        // Load new gravity field frame
        snprintf(image_path, sizeof(image_path), "../mapple/bad_apple_%d.png", current_frame);
        SDL_Surface* surf = IMG_Load(image_path);
        
        if (surf) {
            SDL_Surface* surf_conv = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ARGB8888, 0);
            uint32_t* src_pixels = (uint32_t*)surf_conv->pixels;
            for(int y=0; y<WORLD_HEIGHT; ++y) {
                for(int x=0; x<WORLD_WIDTH; ++x) {
                    int sx = x * surf_conv->w / WORLD_WIDTH;
                    int sy = y * surf_conv->h / WORLD_HEIGHT;
                    uint32_t c = src_pixels[sy * surf_conv->w + sx];
                    uint8_t r = (c >> 16) & 0xFF;
                    uint8_t g = (c >> 8) & 0xFF;
                    uint8_t b = c & 0xFF;
                    float luma = 0.299f*r + 0.587f*g + 0.114f*b;
                    bool white = luma > 127.0f;
                    mask_white[y*WORLD_WIDTH+x] = white;
                    mask_black[y*WORLD_WIDTH+x] = !white;
                }
            }
            SDL_FreeSurface(surf_conv);
            SDL_FreeSurface(surf);

            run_jfa(mask_white, closest_white, WORLD_WIDTH, WORLD_HEIGHT);
            run_jfa(mask_black, closest_black, WORLD_WIDTH, WORLD_HEIGHT);

            for(int y=0; y<WORLD_HEIGHT; ++y) {
                for(int x=0; x<WORLD_WIDTH; ++x) {
                    int idx = y*WORLD_WIDTH+x;
                    if (!mask_white[idx]) {
                        // Black pixel, point towards white
                        Coord cw = closest_white[idx];
                        if (cw.x != -1) {
                            float dx = cw.x - x; if (dx > WORLD_WIDTH/2) dx -= WORLD_WIDTH; else if (dx < -WORLD_WIDTH/2) dx += WORLD_WIDTH;
                            float dy = cw.y - y; if (dy > WORLD_HEIGHT/2) dy -= WORLD_HEIGHT; else if (dy < -WORLD_HEIGHT/2) dy += WORLD_HEIGHT;
                            Gx[idx] = dx; Gy[idx] = dy;
                        } else {
                            Gx[idx] = 0; Gy[idx] = 0;
                        }
                    } else {
                        // White pixel, point away from black
                        Coord cb = closest_black[idx];
                        if (cb.x != -1) {
                            float dx = x - cb.x; if (dx > WORLD_WIDTH/2) dx -= WORLD_WIDTH; else if (dx < -WORLD_WIDTH/2) dx += WORLD_WIDTH;
                            float dy = y - cb.y; if (dy > WORLD_HEIGHT/2) dy -= WORLD_HEIGHT; else if (dy < -WORLD_HEIGHT/2) dy += WORLD_HEIGHT;
                            Gx[idx] = dx; Gy[idx] = dy;
                        } else {
                            Gx[idx] = 0; Gy[idx] = 0;
                        }
                    }
                }
            }

            blur_grid_toroidal(Gx, Gx_blurred, WORLD_WIDTH, WORLD_HEIGHT);
            blur_grid_toroidal(Gx_blurred, Gx, WORLD_WIDTH, WORLD_HEIGHT);
            blur_grid_toroidal(Gy, Gy_blurred, WORLD_WIDTH, WORLD_HEIGHT);
            blur_grid_toroidal(Gy_blurred, Gy, WORLD_WIDTH, WORLD_HEIGHT);
        }

        // 1. Accumulate Charge Density (Toroidal)
        std::fill(rho.begin(), rho.end(), 0.0f);
        for(const auto& p : particles) {
            int ix = (int)p.x;
            int iy = (int)p.y;
            // Coordinates are already bounded by physics loop, no modulo needed
            rho[iy*WORLD_WIDTH+ix] += 1.0f;
        }
        blur_grid_toroidal(rho, rho_blurred, WORLD_WIDTH, WORLD_HEIGHT);

        // 2. Electrostatic Poisson Solver (Toroidal)
        for(int iter=0; iter<5; ++iter) {
            for(int y=0; y<WORLD_HEIGHT; ++y) {
                int y0 = (y == 0) ? WORLD_HEIGHT - 1 : y - 1;
                int y1 = (y == WORLD_HEIGHT - 1) ? 0 : y + 1;
                for(int x=0; x<WORLD_WIDTH; ++x) {
                    int x0 = (x == 0) ? WORLD_WIDTH - 1 : x - 1;
                    int x1 = (x == WORLD_WIDTH - 1) ? 0 : x + 1;

                    float v = 0.25f * (
                        V_electro[y*WORLD_WIDTH+x0] + V_electro[y*WORLD_WIDTH+x1] + 
                        V_electro[y0*WORLD_WIDTH+x] + V_electro[y1*WORLD_WIDTH+x]
                    ) + 0.5f * rho_blurred[y*WORLD_WIDTH+x];
                    
                    v *= 0.99f;
                    V_electro[y*WORLD_WIDTH+x] = v;
                }
            }
        }

        // 3. Physics Ticks (10 ticks per frame)
        for(int tick=0; tick<10; ++tick) {
            for(auto& p : particles) {
                int ix = (int)p.x;
                int iy = (int)p.y;
                // No modulo needed, strictly bounded

                int ix0 = (ix == 0) ? WORLD_WIDTH - 1 : ix - 1;
                int ix1 = (ix == WORLD_WIDTH - 1) ? 0 : ix + 1;
                int iy0 = (iy == 0) ? WORLD_HEIGHT - 1 : iy - 1;
                int iy1 = (iy == WORLD_HEIGHT - 1) ? 0 : iy + 1;

                float dDdx = Gx[iy*WORLD_WIDTH+ix];
                float dDdy = Gy[iy*WORLD_WIDTH+ix];

                float dist = sqrt(dDdx*dDdx + dDdy*dDdy);
                if (dist > 0.0f) {
                    // Inverse square law: F = C / (r^2 + epsilon)
                    // Makes attraction very strong near the surface and weak far away
                    float force_mag = 400.0f / (dist * dist + 10.0f);
                    dDdx = (dDdx / dist) * force_mag;
                    dDdy = (dDdy / dist) * force_mag;
                }

                float dVdx = (V_electro[iy*WORLD_WIDTH+ix1] - V_electro[iy*WORLD_WIDTH+ix0]) * 0.5f;
                float dVdy = (V_electro[iy1*WORLD_WIDTH+ix] - V_electro[iy0*WORLD_WIDTH+ix]) * 0.5f;

                float fx = k_grav * dDdx - k_elec * dVdx;
                float fy = k_grav * dDdy - k_elec * dVdy;

                p.vx += fx;
                p.vy += fy;

                p.vx *= 0.95f; 
                p.vy *= 0.95f;

                p.x += p.vx;
                p.y += p.vy;

                while(p.x < 0) p.x += WORLD_WIDTH;
                while(p.x >= WORLD_WIDTH) p.x -= WORLD_WIDTH;
                while(p.y < 0) p.y += WORLD_HEIGHT;
                while(p.y >= WORLD_HEIGHT) p.y -= WORLD_HEIGHT;
            }
        }

        // 4. Render to buffer
        std::fill(pixels.begin(), pixels.end(), 0xFF000000);
        for(const auto& p : particles) {
            int ix = (int)p.x;
            int iy = (int)p.y;
            for(int dy=-1; dy<=1; ++dy) {
                for(int dx=-1; dx<=1; ++dx) {
                    if(dx*dx + dy*dy > 1) continue;
                    int px = ix + dx;
                    int py = iy + dy;
                    if (px < 0) px += WORLD_WIDTH; else if (px >= WORLD_WIDTH) px -= WORLD_WIDTH;
                    if (py < 0) py += WORLD_HEIGHT; else if (py >= WORLD_HEIGHT) py -= WORLD_HEIGHT;
                    pixels[py*WORLD_WIDTH+px] = 0xFFFFFFFF;
                }
            }
        }

        // Output to FFmpeg RGB pipe
        for(int i=0; i<WORLD_WIDTH*WORLD_HEIGHT; ++i) {
            uint8_t c = (pixels[i] == 0xFFFFFFFF) ? 255 : 0;
            rgb_buffer[i*3] = c;
            rgb_buffer[i*3+1] = c;
            rgb_buffer[i*3+2] = c;
        }
        fwrite(rgb_buffer.data(), 1, rgb_buffer.size(), ffmpeg);

        if (current_frame % 100 == 0) {
            std::cout << "Rendered frame " << current_frame << " / 6510...\n";
        }
    }

    std::cout << "Finished! Closing ffmpeg pipe...\n";
    pclose(ffmpeg);

    IMG_Quit();
    SDL_Quit();
    return 0;
}
