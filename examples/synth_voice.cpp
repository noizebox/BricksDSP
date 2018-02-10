#include <vector>
#include <iostream>

#include <sndfile.h>

#include "dsp_brick.h"
#include "envelope_bricks.h"
#include "oscillator_bricks.h"
#include "utility_bricks.h"

constexpr float EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./example.wav";
constexpr int SECONDS_TO_RENDER = 5;

using namespace bricks;

/* Short example of how to combine a few bricks into a synth voice and
 * rendering a few seconds of audio to disk */

class Voice
{
public:

    const AudioBuffer& render()
    {
        for (auto& brick : _audio_graph)
        {
            brick->render();
        }
        /* Parameter modulation */
        _pitch += 0.0001f;
        return _amp.audio_output(VcaBrick::VCA_OUT);
    }

    void note_on() {_env.gate(true);}
    void note_off() {_env.gate(false);}

private:
    float _attack{0.1f};
    float _decay{0.5f};
    float _sustain{0.3f};
    float _release{1.0f};
    float _pitch{0.0f}; /* in 0.1 / octave on a range from 20 to 20kHz */

    ADSREnvelopeBrick _env{_attack, _decay, _sustain, _release};
    OscillatorBrick   _osc{_pitch};
    VcaBrick          _amp{_env.control_output(ADSREnvelopeBrick::ENV_OUT), _osc.audio_output(OscillatorBrick::OSC_OUT)};

    /* The order of creation automatically becomes a valid process order */
    std::vector<DspBrick*> _audio_graph{&_env, &_osc, &_amp};
};



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

    Voice voice;
    voice.note_on();
    int samplecount = 0;
    int max_samples = SECONDS_TO_RENDER * EXAMPLE_SAMPLERATE;

    while (samplecount < max_samples)
    {
        const auto& buffer = voice.render();
        sf_writef_float(output_file, buffer.data(), DSP_BRICKS_BLOCK_SIZE);
        if (samplecount > max_samples * 0.75)
        {
            voice.note_off();
        }
        samplecount += DSP_BRICKS_BLOCK_SIZE;
    }

    sf_close(output_file);
    return 0;
}