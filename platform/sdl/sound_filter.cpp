#include <cmath>
#include <vector>
#include "sound_filter.h"

static constexpr float M_PI = 3.1415926535f;

Biquad::Biquad() : b0(1), b1(0), b2(0), a1(0), a2(0), z1(0), z2(0) {}
void Biquad::setLowShelf(float gainDB, float cutoff, float sampleRate) {
    const float A = std::pow(10.0f, gainDB / 40.0f);
    const float omega = 2.0f * float(M_PI) * cutoff / sampleRate;
    const float sn = std::sin(omega);
    const float cs = std::cos(omega);
    const float S = 1.0f; // shelf slope (1.0 is a typical choice)
    const float alpha = sn / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

    const float b0_new =    A * ((A + 1.0f) - (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha);
    const float b1_new =  2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
    const float b2_new =    A * ((A + 1.0f) - (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha);
    const float a0_new =         (A + 1.0f) + (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha;
    const float a1_new =   -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
    const float a2_new =         (A + 1.0f) + (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha;

    // Normalize coefficients so that a0 is 1
    b0 = b0_new / a0_new;
    b1 = b1_new / a0_new;
    b2 = b2_new / a0_new;
    a1 = a1_new / a0_new;
    a2 = a2_new / a0_new;

    // Reset filter state
    z1 = z2 = 0.0f;
}

void Biquad::setHighShelf(float gainDB, float cutoff, float sampleRate) {
    const float A = std::pow(10.0f, gainDB / 40.0f);
    const float omega = 2.0f * float(M_PI) * cutoff / sampleRate;
    const float sn = std::sin(omega);
    const float cs = std::cos(omega);
    const float S = 1.0f; // shelf slope
    const float alpha = sn / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

    const float b0_new =    A * ((A + 1.0f) + (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha);
    const float b1_new = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
    const float b2_new =    A * ((A + 1.0f) + (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha);
    const float a0_new =         (A + 1.0f) - (A - 1.0f) * cs + 2.0f * std::sqrt(A) * alpha;
    const float a1_new =    2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
    const float a2_new =         (A + 1.0f) - (A - 1.0f) * cs - 2.0f * std::sqrt(A) * alpha;

    // Normalize coefficients
    b0 = b0_new / a0_new;
    b1 = b1_new / a0_new;
    b2 = b2_new / a0_new;
    a1 = a1_new / a0_new;
    a2 = a2_new / a0_new;

    // Reset filter state
    z1 = z2 = 0.0f;
}

float Biquad::process(float in) {
    float out = b0 * in + z1;
    z1 = b1 * in - a1 * out + z2;
    z2 = b2 * in - a2 * out;
    return out;
}

void Biquad::reset() { z1 = z2 = 0.0f; }


BassTrebleFilter::BassTrebleFilter(float sampleRate, float bassDB, float trebleDB)
    : sampleRate(sampleRate), bassDB(bassDB), trebleDB(trebleDB)
    {
        // Choose cutoff frequencies (adjust these as desired)
        bassCutoff = 200.0f;    // e.g. boost/cut below 250 Hz
        trebleCutoff = 2000.0f; // e.g. boost/cut above 4 kHz

        lowShelf.setLowShelf(bassDB, bassCutoff, sampleRate);
        highShelf.setHighShelf(trebleDB, trebleCutoff, sampleRate);
    }

void BassTrebleFilter::setBass(float bassDBp) {
        this->bassDB = bassDBp;
        lowShelf.setLowShelf(bassDBp, bassCutoff, sampleRate);
    }

    // Update treble gain (in dB)
void BassTrebleFilter::setTreble(float trebleDBp) {
        this->trebleDB = trebleDBp;
        highShelf.setHighShelf(trebleDBp, trebleCutoff, sampleRate);
    }

void BassTrebleFilter::setSampleFreq(float freq) {
    this->sampleRate = freq;
    lowShelf.setLowShelf(bassDB, bassCutoff, sampleRate);
    highShelf.setHighShelf(trebleDB, trebleCutoff, sampleRate);

}


    // (Optional) Process a single sample:
float BassTrebleFilter::processSample(float sample) {
        return highShelf.process(lowShelf.process(sample));
}

