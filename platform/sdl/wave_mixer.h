#pragma once
#include "wave_source.h"

#include <atomic>


template<typename X>
class NoCopyObject: public X {
public:
    using X::X;

    NoCopyObject(const NoCopyObject &other):X() {}
    NoCopyObject &operator=(const NoCopyObject &other) {
        return *this;
    }
};

template<int channels>
class WaveMixer {
public:

    enum State {
        playing,
        done
    };

    WaveMixer(std::shared_ptr<WaveSource> source)
        :_src(std::move(source)) {}
    WaveMixer(std::shared_ptr<WaveSource> source, const std::array<float,channels> &volumes, float speed = 1.0)
        :_src(std::move(source))
        ,_volume{volumes}
        ,_speed(speed) {}



    static_assert(channels > 0);


    ///output to buffer by adding values (mixing)
    /**
     * @param buffer sound buffer - add values here
     * @param samples_count count of samples in buffer (so samples * channels for count of floats)
     * @retval true still playing
     * @retval false done playing
     */
    bool output_mix(float *buffer, std::size_t samples_count)  {
        std::array<float, channels> vol;
        float speed;
        {
            std::lock_guard _(_mx);
            if (_state == done) {
                return 0;
            }
            vol = _volume;
            speed = _speed;
        }

        bool ch = false;

        auto do_volume_fade = [this,&ch](int c){
            float d = _target_volume[c] - _volume[c];
            if (d > 0) {
                d = std::min(d, _volume_change[c]);
                ch = true;
            }
            else if (d < 0) {
                d = std::max(d, -_volume_change[c]);
                ch = true;
            }
            _volume[c] += d;
        };

        auto iter = buffer;
        bool playing = _src->output([&](const auto &value){
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, float>) {
                for (int c = 0; c < channels; ++c) {
                    if (_volume_fade) do_volume_fade(c);
                    float v = value * vol[c];
                    v = v + (*iter);
                    (*iter) = v;
                    ++iter;
                }
            } else {
                //TODO: handle 1 channel audio
                static_assert(std::is_same_v<T, WaveSource::StereoFloat>);
                if (_volume_fade) for (int c  =0; c< channels; ++c) do_volume_fade(c);
                if constexpr(channels == 1) {
                    float v = (value.left + value.right) * 0.5;
                    (*iter) += v * vol[0];
                } else {
                    (*iter) += value.left * vol[0];
                    ++iter;
                    (*iter) += value.right * vol[0];
                    ++iter;
                }
            }

        }, samples_count, speed);
        _volume_fade = ch;
        return playing;
    }

    void set_channel_volumes(const std::array<float,channels> &volumes) {
        std::lock_guard _(_mx);
        _volume = volumes;
    }

    void set_channel_volumes_fade(const std::array<float,channels> &volumes, const std::array<float,channels> &change_speed) {
        std::lock_guard _(_mx);
        _volume_fade = true;
        _target_volume = volumes;
        _volume_change = change_speed;
    }

    void set_channel_volume(int channel, float volume) {
        std::lock_guard _(_mx);
        if (channel >= 0 && channel < channels) {
            _volume[channel] = volume;
        }
    }

    void set_speed(float speed) {
        std::lock_guard _(_mx);
        _speed = speed;
    }

    std::shared_ptr<WaveSource> get_source() const {return _src;}

protected:



    std::shared_ptr<WaveSource> _src;
    mutable  NoCopyObject<std::mutex> _mx;
    std::array<float, channels> _volume = {};
    std::array<float, channels> _target_volume = {};
    std::array<float, channels> _volume_change = {};
    bool _volume_fade = false;
    float _speed = 1.0;
    State _state = playing;



};
