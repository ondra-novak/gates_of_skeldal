# Download and Extract the GOG Galaxy SDK

1. Log in to the [GOG Developer Portal](https://devportal.gog.com/).
2. Open *Galaxy* -> *Components* -> *SDK*.
3. Download **Windows MSVC 19, Visual Studio 2015 (64 bit)** (or the 32 bit package for a 32-bit build).
4. Extract the archive into this directory.

## Required Directory Structure

After extraction the following must exist in this directory:

- `Include/galaxy/GalaxyApi.h`
- `Libraries/Galaxy64.lib` and `Libraries/Galaxy64.dll` (64 bit)

## Credentials

The Galaxy backend also needs the game's client ID and client secret from the
developer portal. Put them in a CMake file **outside the repository**:

```
set(GOG_CLIENT_ID     "...")
set(GOG_CLIENT_SECRET "...")
```

and pass it with `-DGOG_CREDENTIALS_FILE=<path>` (default lookup:
`<source>/gog_credentials.cmake`, then `<source>/../gog_credentials.cmake`).

Tested with Galaxy SDK 1.152.10. Windows only: there is no Galaxy client for
Linux, so the Linux GOG build is compiled with `-DACHIEVEMENTS_BACKEND=none`.
