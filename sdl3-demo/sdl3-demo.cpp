#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>


struct SDLState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
};

void cleanup(SDLState& state);

int main()
{
    SDLState state;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
        return 1;
    }


    // create the window
    constexpr int width = 800;
    constexpr int height = 600;
    state.window = SDL_CreateWindow("SDL3 Demo", width, height, 0);

    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup(state);
        return 1;
    }

    // create renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
        cleanup(state);
        return 1;
    }

    // load game assets
    SDL_Texture *idleTex = IMG_LoadTexture(state.renderer, "data/idle.png");

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

        // perform drawing commands
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        SDL_RenderClear(state.renderer);

        SDL_RenderTexture(state.renderer, idleTex, nullptr, nullptr);

        // swap buffers 
        SDL_RenderPresent(state.renderer);
    }

    SDL_DestroyTexture(idleTex);
    cleanup(state);

    return 0;
}

void cleanup(SDLState& state)
{
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}
