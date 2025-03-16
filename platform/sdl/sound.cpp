#include "../platform.h"
#include "../sound.h"
#include <libs/logfile.h>
#include "global_context.h"

#include "sound_mixer.h"

#include <cstring>
#include <iostream>
#include <array>
#include <thread>

#define countof(array) (sizeof(array)/sizeof(array[0]))
static void SDLCALL mixing_callback(void *userdata, Uint8 * stream, int len);
static SoundMixer<2> sound_mixer;

static float master_volume = 0.5;
static float sound_effect_volume = 1.0;
static float music_volume = 0.5;
static float bass_boost = 0;
static float treble_boost = 0;
static float base_freq;
bool swap_channels = false;
static void empty_deleter(const void *) {}

constexpr int BACK_BUFF_SIZE = 0x40000;
static uint8_t *music_buffer = {};
std::shared_ptr<WaveSource> music_source;
static std::unique_ptr<IMusicStream> current_music = NULL;

struct VolumePreset {
    float left = 1.0;
    float right = 1.0;
};

static std::unordered_map<int, VolumePreset > channel_volume_presset;

void game_sound_init_device(const INI_CONFIG_SECTION *audio_section) {

    const char *dev = ini_get_string(audio_section, "device", "");
    if (!dev[0]) dev = nullptr;

    SDLContext::AudioConfig cfg;
    cfg.audioDevice = dev;

    auto r = get_sdl_global_context().init_audio(cfg, mixing_callback, nullptr);
    base_freq = r.freq;
    sound_mixer.set_mix_freq(r.freq);

}

static void SDLCALL mixing_callback(void *userdata, Uint8 * stream, int len) {
    static char crashed = 0;
        float *s = reinterpret_cast<float *>(stream);
        int samples = len/8;
        std::fill(s, s+2*samples, 0.0f);
        if (crashed) return;
    try {
         sound_mixer.mix_to_buffer(s, samples);
   } catch (std::exception &e) {
        crashed = 1;
        display_error("Unhandled exception in sound mixer: %s", e.what());
    } catch (...) {
        crashed = 1;
        display_error("Crash in sound mixer: %s");
    }
}

char start_mixing() {
    get_sdl_global_context().pause_audio(false);
   return 0;
}
void stop_mixing() {
    get_sdl_global_context().pause_audio(true);

}


static std::array<float,2> make_channel_volume(float left, float right) {
    if (swap_channels) std::swap(left, right);
    return {
        left * master_volume * sound_effect_volume,
        right * master_volume * sound_effect_volume,
    };
}

void play_sample(int channel,const void *sample,int32_t size,int32_t lstart,int32_t sfreq,int type) {
    float speed = sfreq/base_freq;
    int step = type==1?1:2;
    auto samples = size/step;
    auto start = lstart/step;
    WaveSource::Format fmt = step==1?WaveSource::Format::uint8:WaveSource::Format::int16;
    auto src = std::make_shared<WaveSource>(WaveSource::WavePtr(sample,&empty_deleter),fmt,speed,samples,start);

    VolumePreset &vp = channel_volume_presset[channel];




    sound_mixer.set_track(channel, WaveMixer<2>(src, make_channel_volume(vp.left, vp.right), 1.0));

}
void set_channel_volume(int channel,int left,int right) {
    float l = left / 32768.0;
    float r = right / 32768.0;
    VolumePreset &vp = channel_volume_presset[channel];
    vp.left = l;
    vp.right = r;
    sound_mixer.visit_track(channel, [&](WaveMixer<2> &mx){
        mx.set_channel_volumes_fade(make_channel_volume(vp.left, vp.right), {2e-4f,2e-4f});
    });
}

static size_t music_buffer_write_pos = 0;


size_t copy_to_music_buffer(const void *data, size_t data_size) {
    size_t rdpos = music_source->get_output_position_bytes();
    if (static_cast<std::make_signed_t<std::size_t> >(rdpos) < 0) return 0;
    size_t l = music_source->length_bytes();
    size_t space = rdpos < music_buffer_write_pos?l - music_buffer_write_pos: rdpos - music_buffer_write_pos;
    if (space == 0) return 0;
    data_size = std::min(space,data_size);
    std::memcpy(music_buffer+music_buffer_write_pos, data, data_size);
    music_buffer_write_pos += data_size;
    if (music_buffer_write_pos >= l) {
        music_buffer_write_pos = 0;
    }
    return data_size;
}


static const char * (*end_of_song_callback)(void *ctx) = NULL;
static void *end_of_song_callback_ctx = NULL;

void set_end_of_song_callback(const char * (*cb)(void *), void *ctx) {
    end_of_song_callback = cb;
    end_of_song_callback_ctx = ctx;
}

void fade_music() {
    std::lock_guard _(music_source->get_lock());
    size_t rdpos = music_source->get_output_position_samples();
    WaveSource::StereoInt16 *rdptr = reinterpret_cast<WaveSource::StereoInt16 *>(music_buffer);
    size_t wrsamples = music_buffer_write_pos / sizeof(WaveSource::StereoInt16);
    size_t l =music_source->length();
    float_t size = (rdpos < wrsamples?0:l) + wrsamples - rdpos;
    if (l) {
        for (size_t i = 0; i < size; ++i) {
            float fact = (size - i)/size;
            rdptr[rdpos].left = rdptr[rdpos].left * fact;
            rdptr[rdpos].right = rdptr[rdpos].right * fact;
            ++rdpos;
            if (rdpos >=l) rdpos = 0;
        }
    }
}

constexpr int music_track_id_base = 0x1000;

static void music_buffer_deleter(const void *ptr) {
    uint8_t *p = reinterpret_cast<uint8_t *>(const_cast<void *>(ptr));
    delete [] p;
}

static void stop_music() {
    if (music_source) {
        std::lock_guard _(music_source->get_lock());
        size_t l =music_source->length_bytes();
        size_t rdpos = music_source->get_output_position_bytes();
        size_t size = (rdpos < music_buffer_write_pos?0:l) +  music_buffer_write_pos - rdpos;
        music_source->stop_after_bytes(size);
    }
    music_source.reset();
}

void create_new_music_track(int freq, int buffsize) {
    stop_music();
    static int track_id = music_track_id_base;
    double basef = base_freq;
    double rel_speed = freq/basef;
    music_buffer = new uint8_t[buffsize];
    std::memset(music_buffer, 0, buffsize);
    music_source = std::make_shared<WaveSource>(
            WaveSource::WavePtr(music_buffer,music_buffer_deleter),
            WaveSource::Format::stereo_int16, rel_speed,
            buffsize/sizeof(WaveSource::StereoInt16),0);

    sound_mixer.set_track(track_id, WaveMixer<2>(music_source,{music_volume*master_volume,music_volume*master_volume},1.0));
    track_id = track_id ^ 1;
    music_buffer_write_pos = buffsize/2;
}

static std::unique_ptr<IMusicStream> load_music_ex(const char *name) {
    if (name) {
        TMUSIC_STREAM *stream = music_open(name);
        if (stream) {
            TMUSIC_STREAM_INFO nfo = music_get_info(stream);
            if (nfo.channels != 2) {
                SEND_LOG("(MUSIC) Reject music - only stereo is supported");
            } else if (nfo.format != 2){
                SEND_LOG("(MUSIC) Reject music - only 16bit is supported");
            } else {
                return std::unique_ptr<IMusicStream>(static_cast<IMusicStream *>(stream));
            }
        }
    }
    return {};

}

static void handle_end_of_song() {
    current_music.reset();
    if (end_of_song_callback != NULL) {
        const char *new_music = end_of_song_callback(end_of_song_callback_ctx);
        current_music = load_music_ex(new_music);
        if (current_music) {
            auto nfo = current_music->get_info();
            create_new_music_track(nfo.freq, BACK_BUFF_SIZE);
            mix_back_sound(0);
        }
    }
}

int mix_back_sound(int _) {

    if (current_music==NULL) {
        stop_music();
    } else {
        auto data = current_music->read();
        while (!data.empty()) {
            auto sz = copy_to_music_buffer(data.data(), data.size());
            if (sz == 0) {
                current_music->put_back(data);
                break;
            }
            data = data.substr(sz);
            if (data.empty()) data = current_music->read();
        }
        if (data.empty()) {
            handle_end_of_song();
            return mix_back_sound(_);
        }
    }
    return 1;

}

//int open_backsound(char *filename);
void change_music(const char *filename) {
    if (music_source) {
        fade_music();
        stop_music();
    }
    current_music = load_music_ex(filename);
    if (current_music) {
        auto nfo = current_music->get_info();
        create_new_music_track(nfo.freq, BACK_BUFF_SIZE);
        mix_back_sound(0);
    }
}

//char *device_name(int device);
//void force_music_volume(int volume);

//void set_backsnd_freq(int freq);

char get_channel_state(int channel) {
    char c = 0;
    sound_mixer.visit_track(channel, [&](auto){c = 1;});
    return c;
}
void get_channel_volume(int channel,int *left,int *right) {
    VolumePreset &vp = channel_volume_presset[channel];
    *left = static_cast<int>(vp.left*32768.0);
    *right = static_cast<int>(vp.right*32768.0);
}
void mute_channel(int channel) {
    sound_mixer.visit_track(channel, [&](WaveMixer<2> &mx){
        mx.get_source()->stop();
    });
}
void chan_break_loop(int channel) {
    sound_mixer.visit_track(channel, [&](WaveMixer<2> &mx){
        mx.get_source()->break_loop();
    });

}
void chan_break_ext(int channel,const void *org_sample,int32_t size_sample) {
    sound_mixer.visit_track(channel, [&](WaveMixer<2> &mx){
        auto src = mx.get_source();
        src->break_loop(src->length_in_samples(src->get_format(), size_sample));
    });

}


static void update_music_volume(){
    float v = music_volume * master_volume;
    for (int i = 0; i <2; i++)
        sound_mixer.visit_track(music_track_id_base+i,[&](WaveMixer<2> &m){
        m.set_channel_volume(v, v);
    });
}

char set_snd_effect(AUDIO_PROPERTY funct,int data) {
    switch (funct) {
        case SND_PING: break;
        case SND_GFX: sound_effect_volume = data/256.0;break;
        case SND_MUSIC: music_volume = data/128.0;update_music_volume();break;
        case SND_GVOLUME: master_volume = data/512.0;update_music_volume();break;        
        case SND_SWAP: swap_channels = !!data;break;
        case SND_BASS: bass_boost = data/25.0;sound_mixer.set_bass(bass_boost);break;
        case SND_TREBL: treble_boost = data/25.0;sound_mixer.set_treble(treble_boost);break;
        case SND_OUTFILTER: get_sdl_global_context().enable_crt_filter(!!data);break;
        default: return 0;
    }
    return 1;
}
char check_snd_effect(AUDIO_PROPERTY funct) {
    if (funct == SND_PING|| funct == SND_GFX || funct == SND_MUSIC || funct == SND_GVOLUME || funct ==SND_BASS || funct == SND_TREBL||funct==SND_OUTFILTER) return 1;
    return 0;
}
int  get_snd_effect(AUDIO_PROPERTY funct) {
    switch (funct) {
        case SND_PING: return 1;
        case SND_GFX: return static_cast<int>(sound_effect_volume * 256.0);break;
        case SND_MUSIC: return static_cast<int>(music_volume * 128.0);break;
        case SND_GVOLUME: return static_cast<int>(master_volume * 512.0);break;
        case SND_BASS: return static_cast<int>(bass_boost * 25.0);break;
        case SND_TREBL: return static_cast<int>(treble_boost * 25.0);break;
        case SND_SWAP:return swap_channels?1:0;
        case SND_OUTFILTER: return get_sdl_global_context().is_crt_enabled()?1:0;break;
        default: return 0;
    }
}

void *PrepareVideoSound(int mixfreq, int buffsize) {
    create_new_music_track(mixfreq, buffsize);
    return 0;
}
char LoadNextVideoFrame(void *buffer, const char *data, int size, const short *xlat, short *accnums, int32_t *writepos) {
    std::size_t rdpos = music_source->get_output_position_bytes();
    std::size_t wrpos = *writepos;
    auto l = music_source->length_bytes();
    auto space = (rdpos < wrpos?l:0) + rdpos - wrpos;
    if (space < static_cast<std::size_t>(size)*2) {
        sound_mixer.wait_for_advance_write_pos();
        return 0;
    }
    short *wave = reinterpret_cast<short *>(music_buffer);
    auto w = wrpos/2;
    auto ls = l /2;
    for (int i = 0; i < size; ++i) {
        auto acc = accnums+(i & 1);
        *acc  = wave[(w+i) % ls] = xlat[static_cast<uint8_t>(data[i])]+*acc;
    }
    music_buffer_write_pos = *writepos = (wrpos + size*2)%l;
    return 1;

}
void DoneVideoSound(void *buffer) {
    //empty
}

const char *device_name(int )
  {
  return "SDL sound device";
  }



