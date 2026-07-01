#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <iostream>

void cleanup(SDL_Window *win);

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
        return 1;
    }


    // create the window
    constexpr int width = 800;
    constexpr int height = 600;
    SDL_Window *win = SDL_CreateWindow("SDL3 Demo", width, height, 0);

    if (!win)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", win);
        cleanup(win);
        return 1;
    }

    // start game loop
    bool running = true;
    while (running)
    {
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }
    }

    cleanup(win);

    return 0;
}

void cleanup(SDL_Window *win)
{
    SDL_DestroyWindow(win);
    SDL_Quit();
}
