#include "video/screen.h"
#include "defines.h"
#include "SDL3/SDL.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

void screen_init(bool super, int width, int height)
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("CHIP-8", width, height, 0);
    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, super ? CHIP_SUPER_DISPLAY_WIDTH : CHIP_DISPLAY_WIDTH, super ? CHIP_SUPER_DISPLAY_HEIGHT : CHIP_DISPLAY_HEIGHT);
}

void screen_draw(void)
{
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void screen_update(screen_t *screen)
{
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);
}

void screen_stop(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}