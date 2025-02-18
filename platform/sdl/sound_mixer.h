#pragma once
#include "wave_mixer.h"
#include "sound_filter.h"

#include <algorithm>
#include <optional>
#include <vector>


template<int channels>
class SoundMixer {
public:


    void mix_to_buffer(float *buffer, std::size_t samples_len) {
        std::lock_guard _(_mx);
        for (std::size_t i = 0; i < _tracks.size(); ++i) {
            Track &t = _tracks[i];
            bool playing = t.wave.output_mix(buffer, samples_len);
            if (!playing) {
                Track &b = _tracks.back();
                if (&b != &t) std::swap(b,t);
                _tracks.pop_back();
                --i;
            }
        }
        visit_all_filters([&](BassTrebleFilter &flt, auto idx){
            flt.processBuffer<channels, idx.value>(buffer, samples_len);
        });        
        for (std::size_t i = 0; i < samples_len; ++i) {
            for (int j = 0; j < channels; ++j) {
                float f = buffer[i*channels+j] * clip_factor;
                if (f > 1.0) {
                    clip_factor *= clip_factor / f;
                    f = 1.0; 
                } else if (f < -1.0) {
                    clip_factor *= -clip_factor / f;
                    f = -1.0;
                }
                buffer[i*channels+j] = f;
            }
        }
        ++_calls;
        _calls.notify_all();
        if (clip_factor<1)  clip_factor*=1.001f;
    }

    void set_mix_freq(int freq) {
        visit_all_filters([&](BassTrebleFilter &flt, auto){
            flt.setSampleFreq(static_cast<float>(freq));
        });
    }

    void set_bass(float val) {
        visit_all_filters([&](BassTrebleFilter &flt, auto){
            flt.setBass(val);
        });
    }

    void set_treble(float val) {
        visit_all_filters([&](BassTrebleFilter &flt, auto){
            flt.setTreble(val);
        });
    }

    void wait_for_advance_write_pos() {
        auto v = _calls.load();
        _calls.wait(v);
    }

    template<std::invocable<WaveMixer<channels> &> Fn>
    bool visit_track(int trackID, Fn &&fn) {
        std::lock_guard _(_mx);
        Track *t = find_track_by_id(trackID);
        if (t) {
            fn(t->wave);
            return true;
        } else {
            return false;
        }

    }
    template<std::invocable<const WaveMixer<channels> &> Fn>
    bool visit_track(int trackID, Fn &&fn) const {
        std::lock_guard _(_mx);
        const Track *t = find_track_by_id(trackID);
        if (t) {
            fn(t->wave);
            return true;
        } else {
            return false;
        }

    }

    void set_track(int trackID, WaveMixer<channels> wave) {
        std::lock_guard _(_mx);
        Track *t = find_track_by_id(trackID);
        if (t) t->wave = std::move(wave);
        else _tracks.push_back({trackID, std::move(wave)});
    }

    struct Track {
        int trackID;
        WaveMixer<channels> wave;
    };


    void set_tracks(Track *tracks, int count) {
        std::lock_guard _(_mx);
        for (int i = 0; i < count; ++i) {
            Track *t = find_track_by_id(tracks[i].trackID);
            if (t) t->wave = std::move(tracks[i].wave);
            else _tracks.push_back(std::move(tracks[i]));
        }
    }


protected:

    Track *find_track_by_id(int track) {
        auto iter = std::find_if(_tracks.begin(), _tracks.end(), [&](const Track &t){
            return t.trackID == track;
        });
        if (iter == _tracks.end()) {
            return nullptr;
        } else {
            return &(*iter);
        }
    }

    const Track *find_track_by_id(int track) const {
        auto iter = std::find_if(_tracks.begin(), _tracks.end(), [&](const Track &t){
            return t.trackID == track;
        });
        if (iter == _tracks.end()) {
            return nullptr;
        } else {
            return &(*iter);
        }
    }

    template<typename Fn, int ... idx>
    void visit_all_filters_idx(Fn &&fn, std::integer_sequence<int, idx...>) {
        (fn(_filters[idx], std::integral_constant<int, idx>{}),...);
    }


    template<typename Fn>
    void visit_all_filters(Fn &&fn) {
        visit_all_filters_idx(std::forward<Fn>(fn), std::make_integer_sequence<int, channels>{});
    }

    std::mutex _mx;
    std::vector<Track> _tracks;
    std::atomic<unsigned int> _calls = {0};
    std::array<BassTrebleFilter, channels> _filters;
    float clip_factor = 1.0; 

};
