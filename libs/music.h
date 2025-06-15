#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum music_source_type {
    MUSIC_SOURCE_MUS,
    MUSIC_SOURCE_MP3
} TMUSIC_SOURCE_TYPE;

typedef struct music_source_t {
    const void *data;
    size_t size;
    void (*destructor)(struct music_source_t *me);
} TMUSIC_SOURCE;

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




///open music stream MUS file
/**
 * @param source source descriptor - it is copied into reader
 * @return music stream if valid, or NULL, if not
 */
TMUSIC_STREAM *music_open_mus(const TMUSIC_SOURCE *source);

///open music stream MP3 file
/**
 * @param source source descriptor - it is copied into reader
 * @return music stream if valid, or NULL, if not
 */
TMUSIC_STREAM *music_open_mp3(const TMUSIC_SOURCE *source);

TMUSIC_STREAM *music_open(const TMUSIC_SOURCE *source, TMUSIC_SOURCE_TYPE type);

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
