#include "video/screen.h"
#include "SDL3/SDL.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

void screen_init(bool super, int width, int height)
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("CHIP-8", width, height, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STATIC, super ? CHIP_SUPER_DISPLAY_WIDTH : CHIP_DISPLAY_WIDTH, super ? CHIP_SUPER_DISPLAY_HEIGHT : CHIP_DISPLAY_HEIGHT);
}

void screen_draw(void)
{
    SDL_RenderClear(renderer);
    int width = 0;
    int height = 0;
    int size = 0;
    int w_offset = 0;
    int h_offset = 0;

    SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
    if (width < height)
    {
        size = width;
        h_offset = (height - width) / 2;
    }
    else
    {
        size = height;
        w_offset = (width - height) / 2;
    }

    const SDL_FRect dest_rect = {w_offset, h_offset, size, size};
    SDL_RenderTexture(renderer, texture, NULL, &dest_rect);
    SDL_RenderPresent(renderer);
}

void screen_update(screen_t *screen)
{
    SDL_UpdateTexture(texture, NULL, screen->data, sizeof(screen->data[0]) * screen->width);
}

void screen_stop(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}