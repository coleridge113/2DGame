#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <format>
#include <glm/glm.hpp>

#include "gameobject.h"


struct SDLState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width, height, logW, logH;
    const bool* keys;

    SDLState() : keys(SDL_GetKeyboardState(nullptr))
    {}
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
    int playerIndex;

    GameState()
    {
        playerIndex = -1;
    }

    GameObject& getPlayer() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

struct Resources {
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUN = 1;
    const int ANIM_PLAYER_SLIDE = 2;
    std::vector<Animation> playerAnims;
    std::vector<SDL_Texture *> textures;
    SDL_Texture *texIdle, *texRun, *texBrick, *texGrass, *texGround, *texPanel, 
                *texSlide;

    SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath)
    {
        SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.push_back(tex);
        return tex;
    }

    void load(SDLState& state)
    {
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
        playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
        playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);

        texIdle = loadTexture(state.renderer, "data/idle.png");
        texRun = loadTexture(state.renderer, "data/run.png");
        texSlide = loadTexture(state.renderer, "data/slide.png");
        texBrick = loadTexture(state.renderer, "data/tiles/brick.png");
        texGrass = loadTexture(state.renderer, "data/tiles/grass.png");
        texGround = loadTexture(state.renderer, "data/tiles/ground.png");
        texPanel = loadTexture(state.renderer, "data/tiles/panel.png");
    }

    void unload()
    {
        for (SDL_Texture *tex : textures)
        {
            SDL_DestroyTexture(tex);
        }
    }
};

void cleanup(SDLState& state);
bool initialize(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& a, GameObject& b, float deltaTime);
void collisionResponse(
        const SDLState& state, GameState& gs, Resources& res, 
        const SDL_FRect& rectA, const SDL_FRect& rectB, const SDL_FRect& rectC,
        GameObject& a, GameObject& b, float deltaTime
        );
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keydown);


int main()
{
    SDLState state;
    state.width = 1600;
    state.height = 900;
    state.logW = 640;
    state.logH = 320;

    if (!initialize(state))
    {
        return 1;
    }

    // load game assets
    Resources res;
    res.load(state);

    // setup game data
    GameState gs;
    createTiles(state, gs, res);

    uint64_t prevTime = SDL_GetTicks();

    // start game loop
    bool running = true;
    while (running)
    {
        uint64_t nowTime = SDL_GetTicks();
        float deltaTime = (nowTime - prevTime) / 1000.0f;
        SDL_Event event{ 0 };
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                {
                    running = false;
                    break;
                }

                case SDL_EVENT_WINDOW_RESIZED:
                {
                    state.width = event.window.data1;
                    state.height = event.window.data2;
                    break;
                }

                case SDL_EVENT_KEY_DOWN:
                {
                    handleKeyInput(state, gs, gs.getPlayer(), event.key.scancode, true);
                    break;
                }

                case SDL_EVENT_KEY_UP:
                {
                    handleKeyInput(state, gs, gs.getPlayer(), event.key.scancode, false);
                    break;
                }
            }
        }

        // perform drawing commands
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        // update and draw all objects
        for (auto& layer : gs.layers)
        {
            for (GameObject& obj : layer)
            {
                update(state, gs, res, obj, deltaTime);
                if (obj.currentAnimation != -1)
                {
                    obj.animations[obj.currentAnimation].step(deltaTime);
                }
                drawObject(state, gs, obj, deltaTime);
            }
        }

        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(state.renderer, 5, 5, 
                std::format("State: {}", static_cast<int>(gs.getPlayer().data.player.state)).c_str()
                );

        // swap buffers and present
        SDL_RenderPresent(state.renderer);

        prevTime = nowTime;
    }

    res.unload();
    cleanup(state);

    return 0;
}

void cleanup(SDLState& state)
{
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

bool initialize(SDLState& state)
{
    bool initializeSuccess = true;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
        initializeSuccess = false;
    }


    // create the window
    state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height, SDL_WINDOW_RESIZABLE);

    if (!state.window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup(state);
        initializeSuccess = false;
    }

    // create renderer
    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
        cleanup(state);
        initializeSuccess = false;
    }

    // configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return initializeSuccess;
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime)
{
    const float spriteSize = 32;
    float srcX = obj.currentAnimation != -1
        ? obj.animations[obj.currentAnimation].getCurrentFrame() * spriteSize 
        : 0.0f;

    SDL_FRect src{
        .x = srcX,
        .y = 0,
        .w = spriteSize,
        .h = spriteSize
    };

    SDL_FRect dst{
        .x = obj.position.x,
        .y = obj.position.y,
        .w = spriteSize,
        .h = spriteSize
    };

    SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);

}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
    if (obj.dynamic)
        obj.velocity += glm::vec2(0, 500) * deltaTime;

    if (obj.type == ObjectType::player)
    {
        float currentDirection = 0;
        if (state.keys[SDL_SCANCODE_A])     currentDirection += -1;
        if (state.keys[SDL_SCANCODE_D])     currentDirection += 1;
        if (currentDirection)               obj.direction = currentDirection;

        switch (obj.data.player.state)
        {
            case PlayerState::idle:
            {
                if (currentDirection)
                {
                    obj.data.player.state = PlayerState::running;
                }
                else
                {
                    if (obj.velocity.x)
                    {
                        const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
                        float amount = factor * obj.acceleration.x * deltaTime;
                        if (std::abs(obj.velocity.x) < std::abs(amount))
                        {
                            obj.velocity.x = 0;
                        }
                        else
                        {
                            obj.velocity.x += amount;
                        }
                    }
                }

                obj.texture = res.texIdle;
                obj.currentAnimation = res.ANIM_PLAYER_IDLE;

                break;
            }

            case PlayerState::running: 
            { 
                if (!currentDirection)
                {
                    obj.data.player.state = PlayerState::idle;
                }

                if (obj.velocity.x * obj.direction < 0 && obj.grounded)
                {
                    obj.texture = res.texSlide;
                    obj.currentAnimation = res.ANIM_PLAYER_SLIDE;
                }
                else
                {
                    obj.texture = res.texRun;
                    obj.currentAnimation = res.ANIM_PLAYER_RUN;
                }

                break; 
            }

            case PlayerState::jumping: 
            {

                obj.texture = res.texRun;
                obj.currentAnimation = res.ANIM_PLAYER_RUN;

                break;
            }
        }

        // add acceleration to velocity
        obj.velocity += currentDirection * obj.acceleration * deltaTime;
        if (std::abs(obj.velocity.x) > obj.maxSpeedX)
            obj.velocity.x = currentDirection * obj.maxSpeedX;
    }

    // add velocity position
    obj.position += obj.velocity * deltaTime;

    // handle collision detection
    bool foundGround = false;
    for (auto& layer : gs.layers)
    {
        for (GameObject& objB : layer)
        {
            if (&obj != &objB)
            {
                checkCollision(state, gs, res, obj, objB, deltaTime);

                SDL_FRect sensor{
                    .x = obj.position.x + obj.collider.x,
                    .y = obj.position.y + obj.collider.y + obj.collider.h,
                    .w = obj.collider.w, .h = 1
                };

                SDL_FRect rectB{
                    .x = objB.position.x + objB.collider.x,
                    .y = objB.position.y + objB.collider.y,
                    .w = objB.collider.w, .h = objB.collider.h
                };

                if (SDL_HasRectIntersectionFloat(&sensor, &rectB))
                {
                    foundGround = true;
                }
            }
        }
    }

    if (obj.grounded != foundGround)
    {
        obj.grounded = foundGround;
        if (foundGround && obj.type == ObjectType::player)
        {
            obj.data.player.state = PlayerState::running;
        }
    }
}

void collisionResponse(const SDLState& state, GameState& gs, Resources& res, 
        const SDL_FRect& rectA, const SDL_FRect& rectB, const SDL_FRect& rectC,
        GameObject& a, GameObject& b, float deltaTime)
{
    if (a.type == ObjectType::player)
    {
        switch (b.type)
        {
            case ObjectType::level:
            {
                // horizontal collision
                if (rectC.w < rectC.h)
                {
                    if (a.velocity.x > 0) // going right
                    {
                        a.position.x -= rectC.w + 0.1f;
                    }
                    else if (a.velocity.x < 0) // going left
                    {
                        a.position.x += rectC.w;
                    }
                    a.velocity.x = 0;
                }
                else 
                {
                    // vertical collision
                    if (a.velocity.y > 0) // going down
                    {
                        a.position.y -= rectC.h;
                    }
                    else if (a.velocity.y < 0)// going up
                    {
                        a.position.y += rectC.h;
                    }
                    a.velocity.y = 0;
                }
                break;
            }
            
            case ObjectType::player:    break;
            case ObjectType::enemy:     break;
        }
    }
}

void checkCollision(const SDLState& state, GameState& gs, Resources& res, 
        GameObject& a, GameObject& b, float deltaTime)
{
    SDL_FRect rectA{
        .x = a.position.x + a.collider.x, 
        .y = a.position.y + a.collider.y,
        .w = a.collider.w, 
        .h = a.collider.h
    };

    SDL_FRect rectB{
        .x = b.position.x + b.collider.x, 
        .y = b.position.y + b.collider.y,
        .w = b.collider.w, 
        .h = b.collider.h
    };

    SDL_FRect rectC{ 0 };

    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
    {
        collisionResponse(state, gs, res, rectA, rectB, rectC, a, b, deltaTime);
    }
}

void createTiles(const SDLState& state, GameState& gs, const Resources& res)
{
    /*
     * 1 - Ground
     * 2 - Panel
     * 3 - Enemy
     * 4 - Player
     * 5 - Grass
     * 6 - Brick
     */
    short map[MAP_ROWS][MAP_COLS] = { 
        0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    };

    const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
    {
        GameObject o;
        o.type = type;
        o.position = glm::vec2(
                c * TILE_SIZE,
                state.logH - (MAP_ROWS - r) * TILE_SIZE
                );
        o.texture = tex;
        o.collider = { .x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE };
        return o;
    };

    for (int r = 0; r < MAP_ROWS; ++r)
    {
        for (int c = 0; c < MAP_COLS; ++c)
        {
            switch (map[r][c])
            {
                case 1: // ground
                {
                    GameObject o = createObject(r, c, res.texGround, ObjectType::level);
                    gs.layers[LAYER_IDX_LEVEL].push_back(o);
                    break;
                }

                case 2: // panel
                {
                    GameObject o = createObject(r, c, res.texPanel, ObjectType::level);
                    gs.layers[LAYER_IDX_LEVEL].push_back(o);
                    break;
                }

                case 4: // player
                {
                    // create player object
                    GameObject player = createObject(r, c, res.texIdle, ObjectType::player);  
                    player.data.player = PlayerData();
                    player.animations = res.playerAnims;
                    player.currentAnimation = res.ANIM_PLAYER_IDLE;
                    player.acceleration = glm::vec2(300, 0);
                    player.maxSpeedX = 100;
                    player.dynamic = true;
                    player.collider = { 
                        .x = 11, .y = 6,
                        .w = 10, .h = 26
                    };
                    gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
                    gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
                    break;
                }
            }
        }
    }
    assert(gs.playerIndex != -1);
}

void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keyDown)
{
    const float JUMP_FORCE = -200.0f;
    if (obj.type == ObjectType::player)
    {
        switch (obj.data.player.state) 
        {
            case PlayerState::idle:
            {
                if (key == SDL_SCANCODE_K && keyDown)
                {
                    obj.data.player.state = PlayerState::jumping;
                    obj.velocity.y += JUMP_FORCE;
                }
                break;
            }

            case PlayerState::running:
            {
                if (key == SDL_SCANCODE_K && keyDown)
                {
                    obj.data.player.state = PlayerState::jumping;
                    obj.velocity.y += JUMP_FORCE;
                }
                break;
            }

            case PlayerState::jumping: break;

        }
    }
}
