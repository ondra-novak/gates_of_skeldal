#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct music_stream_t {
    char dummy;
} TMUSIC_STREAM;

typedef struct music_stream_info_t {
    int freq;
    int channels;
    int format; //1 - uint8_t. 2 - int16_t
} TMUSIC_STREAM_INFO;


typedef struct music_stream_chunk_t {
    const void *ptr;
    size_t sz;
} TMUSIC_STREAM_CHUNK;



///open music stream
TMUSIC_STREAM *music_open(const char *filename);
///retrieve information
TMUSIC_STREAM_INFO music_get_info(const TMUSIC_STREAM *stream);

TMUSIC_STREAM_CHUNK music_read(TMUSIC_STREAM *stream);

char music_is_eof(const TMUSIC_STREAM_CHUNK *chunk);

void music_put_back_chunk(TMUSIC_STREAM *stream, const TMUSIC_STREAM_CHUNK *chunk);

///close music stream
void music_close(TMUSIC_STREAM *stream);


#ifdef __cplusplus
}

#include <string_view>
class IMusicStream: public TMUSIC_STREAM {
public:
    virtual ~IMusicStream() = default;
    virtual TMUSIC_STREAM_INFO get_info() const = 0;
    virtual std::string_view read() = 0;
    virtual void put_back(std::string_view data) = 0;
};

#endif
