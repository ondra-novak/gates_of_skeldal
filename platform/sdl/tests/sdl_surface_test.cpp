#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 640
#define HEIGHT 480


int main(int argc, char *argv[]) {
    // Inicializace SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Chyba při inicializaci SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Vytvoření SDL okna
    SDL_Window *window = SDL_CreateWindow("DOS Game Port", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "Chyba při vytváření okna: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Vytvoření rendereru (pouze pro zobrazení)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Chyba při vytváření rendereru: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Vytvoření SDL Surface (backbuffer)
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "Chyba při vytváření surface: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    void *raw_pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &raw_pixels, &pitch);
    // Přímý přístup do paměti surface
    Uint16 *pixels = reinterpret_cast<Uint16 *>(raw_pixels);
    printf("Adresa pixelů: %p, pitch: %d bajtů\n", (void *)pixels, pitch);

    // Software rendering - příklad kreslení do paměti
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            Uint16 color = (x ^ y) & 0xFFFF; // Příklad barvy
            pixels[y * (pitch / 2) + x] = color; // Zápis pixelu
        }
    }

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000); // Zobrazení okna na 3 sekundy

    // Uvolnění zdrojů
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
