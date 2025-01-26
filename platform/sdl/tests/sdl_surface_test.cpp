#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 640
#define HEIGHT 480
#define PITCH 1024  // Pro optimalizaci paměti (pitch může být širší než WIDTH)

int main(int argc, char *argv[]) {
    // Inicializace SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Chyba při inicializaci SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Vytvoření SDL okna
    SDL_Window *window = SDL_CreateWindow("DOS Game Port", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Chyba při vytváření okna: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Vytvoření SDL rendereru
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Chyba při vytváření rendereru: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Vytvoření softwarového backbufferu (surface)
    SDL_Surface *backbuffer = SDL_CreateRGBSurfaceWithFormat(0, PITCH, HEIGHT, 16, SDL_PIXELFORMAT_RGB565);
    if (!backbuffer) {
        fprintf(stderr, "Chyba při vytváření surface: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Vytvoření textury pro zobrazení backbufferu
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "Chyba při vytváření textury: %s\n", SDL_GetError());
        SDL_FreeSurface(backbuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Hlavní smyčka
    int running = 1;
    int frame = 0;
    while (running) {
        frame++;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Přímý přístup do paměti backbufferu
        Uint16 *pixels = (Uint16 *)backbuffer->pixels;
        int pitch = backbuffer->pitch / 2; // Pitch v počtu pixelů (ne bajtů)

        // Software rendering - příklad (čtení a zápis do backbufferu)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                Uint16 color = (x ^ y ^ frame) & 0xFFFF; // Vzor barvy (testovací)
                pixels[y * pitch + x] = color;   // Zápis pixelu
            }
        }

        // Kopírování backbufferu do textury
        SDL_UpdateTexture(texture, NULL, backbuffer->pixels, backbuffer->pitch);

        // Vykreslení textury na obrazovku
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Zpoždění pro demonstraci (30 FPS)
        SDL_Delay(1000 / 30);
    }

    // Uvolnění zdrojů
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(backbuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
