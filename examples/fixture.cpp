#include <vector>
#include <iostream>

#include <sndfile.h>

#define DSP_BRICKS_BLOCK_SIZE 32

#include "bricks.h"

constexpr int EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./output.wav";
constexpr int SECONDS_TO_RENDER = 10;

using namespace bricks;

/* Test fixture for generating audio to disk from brick */

int main ()
{
    SF_INFO file_info;
    file_info.channels = 1;
    file_info.samplerate = EXAMPLE_SAMPLERATE;
    file_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE* output_file = sf_open(EXAMPLE_FILE_NAME, SFM_WRITE, &file_info);
    if (output_file == nullptr)
    {
        std::cout << "Couldn't open file for writing: " << sf_strerror(nullptr) << std::endl;
        return -1;
    }

    float pitch = 0.25f;
    float gain = 0.0f;
    //float pwm = 0.0;
    WtOscillatorBrick test_brick{pitch};
    VcaBrick gain_brick{gain, test_brick.audio_output(0)};
    test_brick.set_samplerate(EXAMPLE_SAMPLERATE);
    test_brick.set_waveform(WtOscillatorBrick::Waveform::PULSE   );

    const AudioBuffer& out = gain_brick.audio_output(0);

    int samplecount = 0;
    int max_samples = SECONDS_TO_RENDER * EXAMPLE_SAMPLERATE;
    while (samplecount < max_samples)
    {
        pitch = static_cast<float_t>(samplecount) / max_samples;
        gain = gain + 0.00007f;
        test_brick.render();
        gain_brick.render();

        sf_writef_float(output_file, out.data(), PROC_BLOCK_SIZE);
        samplecount += PROC_BLOCK_SIZE;
    }

    sf_close(output_file);
    return 0;
}