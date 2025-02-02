#pragma once
#include "wave_mixer.h"

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


    std::mutex _mx;
    std::vector<Track> _tracks;


};
