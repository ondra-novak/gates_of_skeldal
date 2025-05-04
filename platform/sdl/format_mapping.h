#pragma once
#include <SDL_pixels.h>


template<Uint32 format>
struct FormatMapping;

template<>
struct FormatMapping<SDL_PIXELFORMAT_ARGB2101010> {
    static constexpr int RShift = 20;
    static constexpr int GShift = 10;
    static constexpr int BShift = 0;
    static constexpr int AShift = 30;

    static constexpr int RBits = 10;
    static constexpr int GBits = 10;
    static constexpr int BBits = 10;
    static constexpr int ABits = 2;
};


template<>
struct FormatMapping<SDL_PIXELFORMAT_ABGR8888> {
    static constexpr int RShift = 0;
    static constexpr int GShift = 8;
    static constexpr int BShift = 16;
    static constexpr int AShift = 24;

    static constexpr int RBits = 8;
    static constexpr int GBits = 8;
    static constexpr int BBits = 8;
    static constexpr int ABits = 8;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_ARGB8888> {
    static constexpr int RShift = 16;
    static constexpr int GShift = 8;
    static constexpr int BShift = 0;
    static constexpr int AShift = 24;

    static constexpr int RBits = 8;
    static constexpr int GBits = 8;
    static constexpr int BBits = 8;
    static constexpr int ABits = 8;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_RGBA8888> {
    static constexpr int RShift = 24;
    static constexpr int GShift = 16;
    static constexpr int BShift = 8;
    static constexpr int AShift = 0;

    static constexpr int RBits = 8;
    static constexpr int GBits = 8;
    static constexpr int BBits = 8;
    static constexpr int ABits = 8;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_BGRA8888> {
    static constexpr int RShift = 8;
    static constexpr int GShift = 16;
    static constexpr int BShift = 24;
    static constexpr int AShift = 0;

    static constexpr int RBits = 8;
    static constexpr int GBits = 8;
    static constexpr int BBits = 8;
    static constexpr int ABits = 8;
};


template<>
struct FormatMapping<SDL_PIXELFORMAT_ABGR4444> {
    static constexpr int RShift = 0;
    static constexpr int GShift = 4;
    static constexpr int BShift = 8;
    static constexpr int AShift = 12;

    static constexpr int RBits = 4;
    static constexpr int GBits = 4;
    static constexpr int BBits = 4;
    static constexpr int ABits = 4;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_ARGB4444> {
    static constexpr int RShift = 8;
    static constexpr int GShift = 4;
    static constexpr int BShift = 0;
    static constexpr int AShift = 12;

    static constexpr int RBits = 4;
    static constexpr int GBits = 4;
    static constexpr int BBits = 4;
    static constexpr int ABits = 4;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_RGBA4444> {
    static constexpr int RShift = 12;
    static constexpr int GShift = 8;
    static constexpr int BShift = 4;
    static constexpr int AShift = 0;

    static constexpr int RBits = 4;
    static constexpr int GBits = 4;
    static constexpr int BBits = 4;
    static constexpr int ABits = 4;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_BGRA4444> {
    static constexpr int RShift = 4;
    static constexpr int GShift = 8;
    static constexpr int BShift = 12;
    static constexpr int AShift = 0;

    static constexpr int RBits = 4;
    static constexpr int GBits = 4;
    static constexpr int BBits = 4;
    static constexpr int ABits = 4;
};


template<>
struct FormatMapping<SDL_PIXELFORMAT_ABGR1555> {
    static constexpr int RShift = 0;
    static constexpr int GShift = 5;
    static constexpr int BShift = 10;
    static constexpr int AShift = 15;

    static constexpr int RBits = 5;
    static constexpr int GBits = 5;
    static constexpr int BBits = 5;
    static constexpr int ABits = 1;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_ARGB1555> {
    static constexpr int RShift = 10;
    static constexpr int GShift = 5;
    static constexpr int BShift = 0;
    static constexpr int AShift = 15;

    static constexpr int RBits = 8;
    static constexpr int GBits = 8;
    static constexpr int BBits = 8;
    static constexpr int ABits = 1;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_RGBA5551> {
    static constexpr int RShift = 11;
    static constexpr int GShift = 6;
    static constexpr int BShift = 1;
    static constexpr int AShift = 0;

    static constexpr int RBits = 5;
    static constexpr int GBits = 5;
    static constexpr int BBits = 5;
    static constexpr int ABits = 1;
};

template<>
struct FormatMapping<SDL_PIXELFORMAT_BGRA5551> {
    static constexpr int RShift = 1;
    static constexpr int GShift = 6;
    static constexpr int BShift = 11;
    static constexpr int AShift = 0;

    static constexpr int RBits = 5;
    static constexpr int GBits = 5;
    static constexpr int BBits = 5;
    static constexpr int ABits = 1;
};




