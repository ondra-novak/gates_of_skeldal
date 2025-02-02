#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>


class WaveSource {
public:
    using WaveDeleter = void (*)(const void *);
    using WavePtr = std::unique_ptr<const void, WaveDeleter>;

    struct StereoInt16 {
        std::int16_t left;
        std::int16_t right;
    };

    struct StereoFloat {
        float left;
        float right;
    };

    enum class Format {
        uint8,
        int8,
        uint16,
        int16,
        stereo_int16
    };

    constexpr static std::size_t length_in_samples(Format f, std::size_t bytes) {
        if (f == Format::stereo_int16) return bytes>>2;
        if (f == Format::uint16 || f == Format::int16) return bytes>>1;
        else return bytes;
    }
    constexpr static std::size_t length_in_bytes(Format f, std::size_t samples) {
        if (f == Format::stereo_int16) return samples<<2;
        if (f == Format::uint16 || f == Format::int16) return samples<<1;
        else return samples;

    }
    Format get_format() const {return _format;}

    ///retrieve overall play time in samples of source wave
    /**
     * @return play time in samples  - this value can be non-integer - when pointer
     * can point between two samples
     *
     */
    double get_play_time() const {
        return _position;
    }
    ///retrieves output position in samples.
    /** useful for cycle buffer returns sample index in wave buffer which has been
     * recently sent to the wave device, so it is save to overwrite it. As the
     * @return output position in samples
     */
    std::size_t get_output_position_samples() const {
        std::lock_guard _(_mx);
        if (_position == 0) return _length-1;   //assume loop
        return static_cast<std::size_t>(std::round(wrap_pos(_position - 1.0)));
    }

    ///retrieves output position in samples.
    /** useful for cycle buffer returns byte offset in wave buffer which has been
     * recently sent to the wave device, so it is save to overwrite it. As the
     * @return output position in bytes
     */
    std::size_t get_output_position_bytes() const {
        return length_in_bytes(_format,get_output_position_samples());
    }

    ///break loop - so wave finishes current cycle and ends
    void break_loop() {
        std::lock_guard _(_mx);
        break_loop_finish_at(_length);
    }

    ///break loop by extending play beyond the loop - must be already defined in wave buffer
    void break_loop(std::size_t final_len) {
        std::lock_guard _(_mx);
        break_loop_finish_at(final_len);
    }

    ///stop playing now
    void stop() {
        std::lock_guard _(_mx);
        _wrap_offset = _position - _length;
        _loop_pos = _length;
    }

    ///stops playing after given samples played
    void stop_after_samples(std::size_t pos) {
        std::lock_guard _(_mx);
        _stop_at = _position + pos;
    }
    ///stops playing after given bytes played
    void stop_after_bytes(std::size_t pos) {
        stop_after_samples(length_in_samples(_format, pos));
    }
    std::size_t length() const {return _length;}
    std::size_t length_bytes() const {return length_in_bytes(_format, _length);}


    ///create wave source
    /**
     * @param data pointer to data, can be allocated or static, depend on deleter
     * @param format format - one of enum
     * @param speed frequency in ratio to base frequency. For instance if base frequency is 44100
     *          and wave frequency is 11025, then speed is 0.25
     * @param len_samples count of samples
     * @param loop_pos
     */
    WaveSource(WavePtr data, Format format, double speed, std::size_t len_samples, std::size_t loop_pos)
    :_wave(std::move(data))
    ,_format(format)
    ,_speed(speed)
    ,_length(len_samples)
    ,_loop_pos(std::min(loop_pos, len_samples))
    {
    }




    template<std::invocable<float> Fn>
    bool output(Fn &&fn, std::size_t samples, double speed = 1.0) {
        std::lock_guard _(_mx);
        switch (_format) {
            case Format::int8: return do_output(std::forward<Fn>(fn),
                    reinterpret_cast<const std::int8_t *>(_wave.get()),
                    samples, speed) > 0;
            case Format::int16: return do_output(std::forward<Fn>(fn),
                    reinterpret_cast<const std::int16_t *>(_wave.get()),
                    samples, speed) > 0;
            case Format::uint8: return do_output(std::forward<Fn>(fn),
                    reinterpret_cast<const std::uint8_t *>(_wave.get()),
                    samples, speed) > 0;
            case Format::uint16: return do_output(std::forward<Fn>(fn),
                    reinterpret_cast<const std::uint16_t *>(_wave.get()),
                    samples, speed) > 0;
            case Format::stereo_int16: return do_output(std::forward<Fn>(fn),
                    reinterpret_cast<const StereoInt16 *>(_wave.get()),
                    samples, speed) > 0;
        }
        return false;

    }










    ///for locking buffer - but probably not needed if you use memory barriers
    std::recursive_mutex &get_lock() {return _mx;}

protected:
    mutable std::recursive_mutex _mx;
    WavePtr _wave;
    Format _format;
    double _speed;
    double _length;
    double _loop_pos;
    double _position = 0;
    double _wrap_offset = 0;
    double _interpolation_step = 1.0;
    double _stop_at = std::numeric_limits<double>::max();



    constexpr float interpolate(float s1, float s2, float frac) {
        return s1 + (s2 - s1) * frac;
    }

    constexpr float interpolate(uint8_t s1, uint8_t s2, float frac) {
        float sampl1 = static_cast<float>(s1)/static_cast<float>(128)- 1.0f;
        float sampl2 = static_cast<float>(s2)/static_cast<float>(128)- 1.0f;
        return interpolate(sampl1, sampl2, frac);
    }
    constexpr float interpolate(uint16_t s1, uint16_t s2, float frac) {
        float sampl1 = static_cast<float>(s1)/static_cast<float>(32768) - 1.0f;
        float sampl2 = static_cast<float>(s2)/static_cast<float>(32768) - 1.0f;
        return interpolate(sampl1, sampl2, frac);
    }

    constexpr float interpolate(int16_t s1, int16_t s2, float frac) {
        float sampl1 = static_cast<float>(s1)/static_cast<float>(32768);
        float sampl2 = static_cast<float>(s2)/static_cast<float>(32768);
        return interpolate(sampl1, sampl2, frac);
    }
    constexpr StereoFloat interpolate(const StereoInt16 &s1, const StereoInt16 &s2, float frac) {
        return {
            interpolate(s1.left, s1.left, frac),
            interpolate(s2.right, s2.right, frac)
        };
    }
    constexpr float interpolate(int8_t s1, int8_t s2, float frac) {
        float sampl1 = static_cast<float>(s1)/static_cast<float>(128);
        float sampl2 = static_cast<float>(s2)/static_cast<float>(128);
        return interpolate(sampl1, sampl2, frac);
    }


    template<typename T, typename Fn >
    std::size_t do_output(Fn &&fn, const T *wave, std::size_t samples, double speed) {
        float spd = speed * _speed;
        for (std::size_t i = 0; i < samples; ++i) {
            if (_position>= _stop_at) {
                return i;
            }
            double p = wrap_pos(_position - _wrap_offset);
            double sp;
            double fp = std::modf(p,&sp);
            std::size_t s1 = static_cast<std::size_t>(wrap_pos(sp));
            std::size_t s2 = static_cast<std::size_t>(wrap_pos(sp+1));
            if (s1 == s2) return i;
            fn(interpolate(wave[s1], wave[s2], fp));
            _position = _position + spd;
        }
        return samples;
    }


    double wrap_pos(double position) const {
        if (position >= _length) {
            if (_loop_pos == _length) {
                position = _length-1;
            } else {
                position = std::fmod((position - _length),(_length - _loop_pos)) + _loop_pos;
            }
        }
        return position;
    }


    void break_loop_finish_at(double position) {
        double adj = wrap_pos(_position);
        _wrap_offset = _position - adj;
        _loop_pos = _length = position;
    }

};

