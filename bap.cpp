#define SDL_MAIN_HANDLED

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <SDL.h>
#include <SDL_image.h>
#include <unistd.h>
#include <string>
using namespace std;

#define WORLD_WIDTH 1440
#define WORLD_LENGTH 1080
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 540
#define SNUMBER 100000
#define LIFE 10

/* */

/*

cd /Users/jirnyak/Mirror/bacaple

*/

using rng_t = std::mt19937;

std::random_device dev;

std::mt19937 rng(dev());

uint32_t randomer(rng_t& rng, uint32_t range) 
{
    range += 1;
    uint32_t x = rng();
    uint64_t m = uint64_t(x) * uint64_t(range);
    uint32_t l = uint32_t(m);
    if (l < range) {
        uint32_t t = -range;
        if (t >= range) {
            t -= range;
            if (t >= range) 
                t %= range;
        }
        while (l < t) {
            x = rng();
            m = uint64_t(x) * uint64_t(range);
            l = uint32_t(m);
        }
    }
    return m >> 32;
}

default_random_engine generator;
normal_distribution<double> distribution(100.0, 10.0);

class bac
{
    public:
        int x;
        int y;
        double hp;
        double hp_max;
        bool dead;
        int age;

        bac(int x, int y)
        {
            this->x = x;
            this->y = y;
            this->hp = distribution(generator)/10;
            this->hp_max = this->hp;
            this->dead = 0;
            this->age = distribution(generator)/5;
        }
};

class cell 
{
    public:  
        int x;  
        int y;   
        int number;

        cell *sosed_up;   
        cell *sosed_left; 
        cell *sosed_down; 
        cell *sosed_right; 
        cell *sosed_upleft;
        cell *sosed_upright;
        cell *sosed_downleft;
        cell *sosed_downright; 

        int R;
        int G;
        int B;

        bool food;

        bool white;

        cell(int x, int y) 
        { 
            this->x = x;
            this->y = y;

            this->food = 0;

            this->number = x*WORLD_WIDTH + y;

            this->R = 0;
            this->G = 0;
            this->B = 0;

            this->white = 0;
        }

        void up(cell *c)
        {
            sosed_up = c;
        }
        void down(cell *c)
        {
            sosed_down = c;
        }
        void left(cell *c)
        {
            sosed_left = c;
        }
        void right(cell *c)
        {
            sosed_right = c;
        }
        void upright(cell *c)
        {
            sosed_upright = c;
        }
        void upleft(cell *c)
        {
            sosed_upleft = c;
        }
        void downright(cell *c)
        {
            sosed_downright = c;
        }
        void downleft(cell *c)
        {
            sosed_downleft = c;
        }
        cell* side(int d) 
        {
            if (d == 0)
                return sosed_up; 
            if (d == 1)
                return sosed_upleft;
            if (d == 2)
                return sosed_left;
            if (d == 3)
                return sosed_downleft;
            if (d == 4)
                return sosed_down;
            if (d == 5)
                return sosed_downright;
            if (d == 6)
                return sosed_right;
            if (d == 7)
                return sosed_upright;
            else
                exit(1);
        }
        cell* side_spiral(int d) const
        {
            if (d == 0)
                return sosed_up;
            if (d == 1)
                return sosed_left;
            if (d == 2)
                return sosed_down;
            if (d == 3)
                return sosed_right;
            else
                exit(1);
        }
};

int tor_cord(int x)
{
    if (x < 0)
    {
        x = WORLD_WIDTH + x%WORLD_WIDTH;
    }
    else if (x >= WORLD_WIDTH)
    {
        x = x%WORLD_WIDTH;
    }
    return x;
}

void read_im(std::vector<cell>& cells, int frame_n) 
{
    int frama = frame_n;
    string frame_name = "mapple/bad_apple_" + to_string(frama) + ".png";
    SDL_Surface* surface = IMG_Load(frame_name.c_str());
    while (!surface)
    {
        frama -= 1;
        string frame_name = "mapple/bad_apple_" + to_string(frama) + ".png";
        surface = IMG_Load(frame_name.c_str());
    }
    int black = 0;
    int white = 0;
    SDL_LockSurface(surface);
    Uint8* pixels = (Uint8*)surface->pixels;

    //#pragma omp parallel
    //#pragma omp for
    for (auto& cell : cells) 
    {
        int x = cell.x;
        int y = cell.y;
        if (y <= WORLD_LENGTH)
        {
            Uint8 r, g, b;
            SDL_GetRGB(*(Uint32*)(pixels + y * surface->pitch + x * surface->format->BytesPerPixel), surface->format, &r, &g, &b);
            
            cell.R = r;
            cell.G = g;
            cell.B = b;
            
            if (r > 100)
            {
                cell.white = 1;
                white += 1;
            }
            else
            {
                cell.white = 0;
                black += 1;
            }
        }
        else
            cell.white = 0;
    }
    SDL_UnlockSurface(surface);
    SDL_FreeSurface(surface);
}

int main(int argc, char **argv) 
{
    default_random_engine generator;
    normal_distribution<double> distribution(10.0, 1.0);

    vector<cell> world;
    vector<cell>::iterator it;

    for (int i=0; i<WORLD_WIDTH; ++i)
    {
        for (int j=0; j<WORLD_WIDTH; ++j)
        {       
            world.push_back(cell(i,j));
        }
    }

    for (it = world.begin(); it != world.end(); ++it)
    {
        vector<cell>::iterator it1;
        it->up(&world[tor_cord(it->x)*WORLD_WIDTH + tor_cord(it->y-1)]);
        it->down(&world[tor_cord(it->x)*WORLD_WIDTH + tor_cord(it->y+1)]);
        it->left(&world[tor_cord(it->x-1)*WORLD_WIDTH + tor_cord(it->y)]);
        it->right(&world[tor_cord(it->x+1)*WORLD_WIDTH + tor_cord(it->y)]);
        it->upleft(&world[tor_cord(it->x-1)*WORLD_WIDTH + tor_cord(it->y-1)]);
        it->upright(&world[tor_cord(it->x+1)*WORLD_WIDTH + tor_cord(it->y-1)]);
        it->downleft(&world[tor_cord(it->x-1)*WORLD_WIDTH + tor_cord(it->y+1)]);
        it->downright(&world[tor_cord(it->x+1)*WORLD_WIDTH + tor_cord(it->y+1)]);
    }

    vector<SDL_Surface*> badapple;

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

    SDL_SetWindowFullscreen(window, 0);

    bool paused = false;
    bool fullscreen = false;
    int key;
    bool quit = 0;

    int i;

    int count = 400;

    string frame_name;

    i = 0;

    vector<bac> bacs;
    vector<bac> bacs_buff;
    vector<bac>::iterator b;

    for(int k = 0; k<SNUMBER; k++)
        bacs.push_back(bac(randomer(rng, WORLD_WIDTH), randomer(rng, WORLD_LENGTH)));

    int k = 0;

    SDL_Rect minimap_dot;
    minimap_dot.w = 10;
    minimap_dot.h = 10;

    Uint32 last_update;
    Uint32 current_time;

    double celok_m;
    double celok_p;

    while (quit == false) 
    {
        current_time = SDL_GetTicks();
        //USER INPUT CHECK

        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_0: //SDLK_BACKQUOTE:
                        fullscreen = not fullscreen;
                        if (fullscreen == true)
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                        else
                            SDL_SetWindowFullscreen(window, 0);
                        break;
                    default:
                        break;
                }
            }
        }

            //PHYSICS

            if (paused == false)
            {

                /*
                #pragma omp parallel
                #pragma omp for
                for (it = world.begin(); it != world.end(); ++it)
                {

                }
                */
                celok_m = (bacs.size()*10)/SNUMBER;
                celok_p = (SNUMBER/10)/bacs.size();

                for (b = bacs.begin(); b != bacs.end(); ++b)
                {
                    if (world[b->x*WORLD_WIDTH + b->y].white == 1)
                    {
                        b->hp += distribution(generator)/distribution(generator)*celok_p*b->hp_max/10;
                        //world[b->x*WORLD_WIDTH + b->y].white = 0;
                    }
                    else
                    {
                        b->hp -= distribution(generator)/distribution(generator)*celok_m*randomer(rng,1);
                    }
                    if (b->hp > 2*b->hp_max)
                    {
                        b->hp = b->hp_max;
                        bacs_buff.push_back(bac(b->x, b->y));
                    }
                    b->age -= randomer(rng,1);
                    b->x += randomer(rng, distribution(generator));
                    b->y += randomer(rng, distribution(generator));
                    b->x -= randomer(rng, distribution(generator));
                    b->y -= randomer(rng, distribution(generator));
                    b->x = tor_cord(b->x);
                    b->y = tor_cord(b->y);
                    if (b->hp < 0 or  b->age < 0)
                    {
                        b->dead = 1;
                    }
                    else
                    {
                        bacs_buff.push_back(*b);
                    }
                }

                bacs_buff.push_back(bac(randomer(rng, WORLD_WIDTH), randomer(rng, WORLD_LENGTH)));

                read_im(world, count/10);

                count += 1;

        //DRAW

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 

        for (b = bacs.begin(); b != bacs.end(); ++b)
        {
            /*
            if (world[b->x*WORLD_WIDTH + b->y].white == 1)
            {
                //minimap_dot.x = b->x/2;
                //minimap_dot.y = b->y/2;
                //SDL_RenderFillRect(renderer, &minimap_dot);
                SDL_RenderDrawPoint(renderer, b->x/2, b->y/2);
            }
            */
            
            SDL_RenderDrawPoint(renderer, b->x/2, b->y/2);
        }

        SDL_RenderPresent(renderer);

    }

        //CLEANING AND REFILING BUFFERS

    bacs.swap(bacs_buff);
    bacs_buff.clear();


    last_update = SDL_GetTicks();

    while (last_update - current_time < 10) 
        {
            last_update = SDL_GetTicks();
        }

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}