
#include <array>
#include <memory>
#include <cmath>
#include <cassert>

#include "oscillator_bricks.h"
#include "data/wavetables_x4.h"

namespace bricks
{

inline void add_harmonic(float* buffer, int size, int harmonic, float level)
{
    const float two_pi = 2.0f * static_cast<float>(M_PI);

    for (int i = 0; i < size; ++i)
    {
        buffer[i] += 0.5f * level * std::sin(harmonic * two_pi * static_cast<float>(i) / size);
    }
}

WaveTable create_wavetable(Waveform waveform, int initial_table_length, int shortest_table_length)
{
    std::array<int, 10> sizes;
    int table_len = initial_table_length;
    for (int i = 0; i < sizes.size(); ++i)
    {
        sizes[i] = table_len;
        table_len = std::max(table_len / 2, shortest_table_length);
    }
    return create_wavetable(waveform, sizes);
}

/* For better control of the sizes, call this function directly */
WaveTable create_wavetable(Waveform waveform, const std::array<int,WT_OCTAVES>& table_sizes)
{
    constexpr int PRE_PADDING = 1;
    constexpr int POST_PADDING = 2;

    WaveTable wt;

    // First pass: compute sizes and total memory needed
    int total_samples = 0;

    for (int i = 0; i < table_sizes.size(); ++i)
    {
        wt._sizes[i] = table_sizes[i];
        total_samples += wt._sizes[i] + PRE_PADDING + POST_PADDING;
    }
    total_samples += PRE_PADDING + POST_PADDING;
    wt._data = std::make_unique<float[]>(total_samples);
    std::fill(wt._data.get(), wt._data.get() + total_samples, 0.0f);
    float* data = wt._data.get() + PRE_PADDING;

    /* Generate 10 octaves spanning 20Hz to 20 kHz. The base frequency
     * of the first table is 20 hz, thus we need 1000 harmonics */
    int harmonics = 1000.0f;
    for (int oct = 0; oct < WT_OCTAVES; ++oct)
    {
        wt._tables[oct] = data;
        int table_len = wt._sizes[oct];

        int max_h = static_cast<int>(std::round(harmonics));

        for (int h = 1; h <= max_h; ++h)
        {
            switch (waveform)
            {
            case Waveform::SAW:
                add_harmonic(data, table_len, h, 1.0f / h);
                break;

            case Waveform::PULSE:
                if (h % 2 == 1)
                    add_harmonic(data, table_len, h, 2.0f / h);
                break;

            case Waveform::TRIANGLE:
                if (h % 4 == 1)
                    add_harmonic(data, table_len, h, 1.6f / (h * h));
                else if (h % 4 == 3)
                    add_harmonic(data, table_len, h, -1.6f / (h * h));
                break;

            case Waveform::SINE:
                if (h == 1)
                    add_harmonic(data, table_len, h, 1.0f);
                break;
            }
        }
        *(data - 1)  = data[table_len - 1];
        data[table_len] = data[0];
        data[table_len + 1]  = data[1];


        data += table_len + PRE_PADDING + POST_PADDING;
        harmonics /= 2.0f;
    }
    return wt;
}

WaveTable *create_default_wavetable()
{
    static WaveTable default_wavetable;
    std::copy(&wavetables::lengths[0], &wavetables::lengths[default_wavetable._sizes.size()], default_wavetable._sizes.data());
    for (int i = 0; i < default_wavetable._tables.size(); ++i)
    {
        default_wavetable._sizes[i] = wavetables::lengths[i];
        default_wavetable._tables[i] = wavetables::saw + wavetables::offsets[i];
    }
    return &default_wavetable;
}
}