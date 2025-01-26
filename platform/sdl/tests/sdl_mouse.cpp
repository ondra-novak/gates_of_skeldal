#include <SDL2/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL Mouse Event Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Event e;
    bool quit = false;
    int isFullscreen = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Získání souřadnic myši
                int mouseX = e.motion.x, mouseY = e.motion.y;

                // Přepočet souřadnic na poměr vůči rozměrům okna
                int windowWidth, windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                float normalizedX = (float)mouseX / windowWidth;
                float normalizedY = (float)mouseY / windowHeight;

                std::cout << "Mouse moved to: (" << mouseX << ", " << mouseY << ") -> Normalized: ("
                          << normalizedX << ", " << normalizedY << ")" << std::endl;
            } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                int mouseX = e.button.x, mouseY = e.button.y;
                int button = e.button.button;

                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    std::cout << "Mouse button pressed at: (" << mouseX << ", " << mouseY << ") button " << button << std::endl;
                } else {
                    std::cout << "Mouse button released at: (" << mouseX << ", " << mouseY << ") button " << button << std::endl;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                if (e.wheel.y > 0) {
                    std::cout << "Mouse wheel scrolled up." << std::endl;
                } else if (e.wheel.y < 0) {
                    std::cout << "Mouse wheel scrolled down." << std::endl;
                }
            } else  if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)) {
                    // Přepnutí mezi fullscreen a oknem
                    isFullscreen = !isFullscreen;
                    SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                }
            }
        }

        // Renderování a další logika hry
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
