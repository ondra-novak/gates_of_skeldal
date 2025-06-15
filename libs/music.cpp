#include <string_view>

#include "music.h"
#include "minimp3.h"
#include <platform/platform.h>

#include "memman.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>
#include <utility>
#include <memory>

struct MResDeleter {
    TMUSIC_SOURCE src;
    void operator()(const void *ptr) {
        src.destructor(&src);
    }
};

using MusicData = std::unique_ptr<const void, MResDeleter>;


class MUSStreamParser: public IMusicStream {
public:

    MUSStreamParser(const TMUSIC_SOURCE *src)
        :_mdata(src->data, {*src}) {

        if (_mdata) {
            _data = _mdata.get();
            if (_data) {
                _loaded = true;
                iter = reinterpret_cast<const uint8_t *>(_data);
                chans = read_short();
                freq = read_int();
                iter += 4;
                blocks = read_int();
                iter += 8;
                btable = reinterpret_cast<const short *>(iter);
                iter += 512;
            }
        }
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
    MusicData _mdata;
    const void *_data;

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

class MP3StreamParser: public IMusicStream {
public:
    MP3StreamParser(const TMUSIC_SOURCE *src)
       :_data(src->data, {*src})
       ,_mp3_size(src->size) {


       if (_data && find_music_info()) {
           _info.format = 2;
           _loaded = true;
           mp3dec_init(&dec);
       }
    }

    virtual std::string_view read() override {
        if (!_put_back_buffer.empty() || _eof) return std::exchange(_put_back_buffer, {});
        while (_mp3_offset < _mp3_size) {
            mp3dec_frame_info_t frame_info;
            int samples = mp3dec_decode_frame(&dec,
                    reinterpret_cast<const uint8_t *>(_data.get())+_mp3_offset,
                    _mp3_size - _mp3_offset, pcm_buffer,&frame_info);

           if (samples == 0) {
               // Nenalezen platný frame, posuneme se o 1 bajt a zkusíme dál
               _mp3_offset++;
               continue;
           }

           _mp3_offset += frame_info.frame_bytes ;

           size_t data_size = samples* frame_info.channels  * sizeof(mp3d_sample_t);

           return std::string_view(reinterpret_cast<const char*>(pcm_buffer), data_size);
        }
        _eof = true;
        return {};
    }
    virtual void put_back(std::string_view data) override {
        _put_back_buffer = data;
    }
    virtual TMUSIC_STREAM_INFO get_info() const override {
        return _info;
    }

    bool is_loaded() const {return _loaded;}

protected:
    MusicData _data;
    size_t _mp3_size;
    size_t _mp3_offset = 0;
    TMUSIC_STREAM_INFO _info = {};
    mp3dec_t dec{};

    std::string_view _put_back_buffer = {};
    mp3d_sample_t pcm_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME] = {};
    bool _eof = false;
    bool _loaded = false;

    bool find_music_info() {
        mp3dec_t tmpdec{};
        mp3dec_init(&tmpdec);
        size_t ofs = 0;
        size_t max_ofs = std::min<size_t>(_mp3_size,4096);
        mp3dec_frame_info_t frame_info;
        while (ofs < max_ofs) {
            int samples = mp3dec_decode_frame(&tmpdec,
                    reinterpret_cast<const uint8_t *>(_data.get())+ofs,
                    _mp3_size-ofs, pcm_buffer, &frame_info);
            if (samples > 0) {
                _info.freq = frame_info.hz;
                _info.channels = frame_info.channels;
                return true;
            }
            ++ofs;
        }
        return false;
    }
};


void music_close(TMUSIC_STREAM *stream) {
    if (stream) {
        IMusicStream *s = static_cast<IMusicStream *>(stream);
        delete s;
    }
}

TMUSIC_STREAM *music_open_mus(const TMUSIC_SOURCE *src) {
    auto player = new MUSStreamParser(src);
    if (player->is_loaded()) return player;
    delete player;
    return NULL;
}

TMUSIC_STREAM *music_open_mp3(const TMUSIC_SOURCE *src) {
    auto player = new MP3StreamParser(src);
    if (player->is_loaded()) return player;
    delete player;
    return NULL;

}


TMUSIC_STREAM *music_open(const TMUSIC_SOURCE *source, TMUSIC_SOURCE_TYPE t) {
    if (t == MUSIC_SOURCE_MUS) {
        return music_open_mus(source);
    } else {
        return music_open_mp3(source);
    }
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
