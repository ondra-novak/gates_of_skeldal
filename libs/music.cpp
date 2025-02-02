#include "music.h"
#include <platform/platform.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>
#include <utility>



class MUSStreamParser: public IMusicStream {
public:

    MUSStreamParser(const char *name) {
        data = map_file_to_memory(file_icase_find(name), &sz);
        if (data) {
            _loaded = true;
            iter = reinterpret_cast<const uint8_t *>(data);
            chans = read_short();
            freq = read_int();
            iter += 4;
            blocks = read_int();
            iter += 8;
            btable = reinterpret_cast<const short *>(iter);
            iter += 512;
        }
    }

    ~MUSStreamParser() {
        if (data) unmap_file(data, sz);
    }
    MUSStreamParser(const MUSStreamParser &) = delete;
    MUSStreamParser& operator=(const MUSStreamParser &) = delete;

    bool is_loaded() const {return _loaded;}


    virtual TMUSIC_STREAM_INFO get_info() const override {
        TMUSIC_STREAM_INFO r;
        r.format = 2;
        r.channels = chans;
        r.freq = freq;
        return r;
    }

    virtual std::string_view read() override {
        if (unprocessed_buffer.empty()) {
            if (!process_next()) return {};
        }
        return std::exchange(unprocessed_buffer, {});
    }
    virtual void put_back(std::string_view data) override {
        unprocessed_buffer = data;
    }

protected:
    bool _loaded = false;
    void *data;
    std::size_t sz;

    std::int16_t chans;
    std::int32_t freq;
    std::int32_t blocks;
    const short *btable;
    const uint8_t *iter;
    std::vector<int16_t> outbuff;
    std::string_view unprocessed_buffer;

    int32_t read_int() {
        int32_t r = iter[0] | (iter[1] << 8) | (iter[2]<<16) | (iter[3] << 24);
        iter+=4;
        return r;
    }
    int16_t read_short() {
        int16_t r = iter[0] | (iter[1] << 8);
        iter+=2;
        return r;
    }

    bool process_next() {
        if (blocks == 0) {
            return false;
        }
        --blocks;
        std::int32_t p = read_int();
        read_int();
        outbuff.clear();

        short accum[2]={0,0};
        int c = 0;
        for (int i = 0; i < p; ++i) {
            uint8_t p = *iter++;
            short val=accum[c]+btable[p];
            accum[c]=val;
            if (p==0)  //a bug in compression algorithm
                  {
                  val-=31767;
                  }
             c = (c + 1) % chans;
             outbuff.push_back(val);
        }
        unprocessed_buffer = {reinterpret_cast<const char *>(outbuff.data()), outbuff.size()*2};
        return true;
    }


};


void music_close(TMUSIC_STREAM *stream) {
    if (stream) {
        IMusicStream *s = static_cast<IMusicStream *>(stream);
        delete s;
    }
}

TMUSIC_STREAM *music_open(const char *filename) {
    MUSStreamParser *player =  new MUSStreamParser(filename);
    if (player->is_loaded()) return player;
    delete player;
    return NULL;
}

TMUSIC_STREAM_CHUNK music_read(TMUSIC_STREAM *stream) {
    TMUSIC_STREAM_CHUNK r = {NULL,0};
    if (stream) {
        IMusicStream *s = static_cast<IMusicStream *>(stream);
        auto data = s->read();
        r.ptr = data.data();
        r.sz = data.size();
    }
    return r;
}

char music_is_eof(const TMUSIC_STREAM_CHUNK *chunk) {
    return chunk->sz == 0;
}

void music_put_back_chunk(TMUSIC_STREAM *stream, const TMUSIC_STREAM_CHUNK *chunk){
    if (stream) {
        IMusicStream *s = static_cast<IMusicStream *>(stream);
        s->put_back({reinterpret_cast<const char *>(chunk->ptr), chunk->sz});
    }
}

TMUSIC_STREAM_INFO music_get_info(const TMUSIC_STREAM *stream) {
    if (stream) {
        const IMusicStream *s = static_cast<const IMusicStream *>(stream);
        return s->get_info();
    } else {
        TMUSIC_STREAM_INFO r = {};
        return r;
    }
}

