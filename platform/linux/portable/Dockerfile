# Build environment for a portable Linux build of Skeldal.
#
# Base: Steam Runtime 3 "sniper" SDK - Debian 11 based (glibc 2.31 baseline,
# runs on Ubuntu 20.04+, Debian 11+ and practically every current distro),
# maintained, with gcc-14 (plain Debian 11 has gcc 10 which lacks C++20
# std::atomic::wait). The result does NOT depend on Steam in any way.
#
# SDL2 is built from source as a static library. Its backends (X11, Wayland,
# ALSA, PulseAudio, PipeWire, libdecor, ...) are loaded at runtime via dlopen,
# so their headers are needed here, but the libraries are optional on target.
#
# The image contains only the toolchain + SDL2. The game source is bind-mounted
# at runtime, see build.sh.

FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

ARG SDL_VERSION=2.32.10

ENV CC=gcc-14 CXX=g++-14

RUN curl -fsSL -o /tmp/sdl.tar.gz \
        https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz \
    && tar -xzf /tmp/sdl.tar.gz -C /tmp \
    && cmake -S /tmp/SDL2-${SDL_VERSION} -B /tmp/sdl-build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/sdl2 \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_STATIC_PIC=ON \
        -DSDL_TEST=OFF \
    && cmake --build /tmp/sdl-build -j"$(nproc)" \
    && cmake --install /tmp/sdl-build \
    && rm -rf /tmp/sdl.tar.gz /tmp/SDL2-${SDL_VERSION} /tmp/sdl-build

ENV CMAKE_PREFIX_PATH=/opt/sdl2
# source is bind-mounted and owned by the host user
RUN git config --system --add safe.directory '*'
