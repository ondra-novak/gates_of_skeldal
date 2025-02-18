#include <cmath>
#include <vector>

// Biquad filter class using Direct Form II Transposed
class Biquad {
public:
    Biquad();

    // Set coefficients for a low-shelf filter using RBJ's formulas.
    // gainDB: boost/cut in dB, cutoff: shelf cutoff frequency in Hz, sampleRate: sampling rate in Hz.
    void setLowShelf(float gainDB, float cutoff, float sampleRate);

    // Set coefficients for a high-shelf filter using RBJ's formulas.
    void setHighShelf(float gainDB, float cutoff, float sampleRate);

    // Process a single sample (Direct Form II Transposed)
    inline float process(float in);
    // (Optional) Reset the filter state (e.g. when starting a new buffer)
    void reset();

private:
    float b0, b1, b2, a1, a2;
    float z1, z2;
};


// BassTrebleFilter: combines a low-shelf (bass) and a high-shelf (treble) filter.
class BassTrebleFilter {
public:
    // sampleRate: sample frequency in Hz.
    // bassDB: bass gain (in dB). Positive boosts bass, negative cuts.
    // trebleDB: treble gain (in dB). Positive boosts treble, negative cuts.
    BassTrebleFilter(float sampleRate=44100, float bassDB=0, float trebleDB=0);
    // Update bass gain (in dB)
    void setBass(float bassDB);

    // Update treble gain (in dB)
    void setTreble(float trebleDB);

    void setSampleFreq(float freq);

    // Process a buffer of float samples (in-place).
    // One common approach is to run the input through both filters.
    // In many designs the filters are applied in cascade so that each only affects its target band.
    void processBuffer(float* buffer, int numSamples);

        // Process a buffer of float samples (in-place).
    // One common approach is to run the input through both filters.
    // In many designs the filters are applied in cascade so that each only affects its target band.
    template<int channels, int channel_id>
    void processBuffer(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; i++) {
            // You can choose to cascade the filters:
            float processed = lowShelf.process(buffer[i*channels+channel_id]);
            processed = highShelf.process(processed);
            buffer[i*channels+channel_id] = processed;
        }
}


    // (Optional) Process a single sample:
    float processSample(float sample);



private:
    float sampleRate;
    float bassDB, trebleDB;
    float bassCutoff, trebleCutoff;
    Biquad lowShelf;
    Biquad highShelf;
};
