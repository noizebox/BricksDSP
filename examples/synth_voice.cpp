#include <vector>
#include <iostream>

#include <sndfile.h>

#include "bricks_dsp/bricks.h"

constexpr float EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./example.wav";
constexpr int SECONDS_TO_RENDER = 5;

using namespace bricks;

/* Short example of how to combine a few bricks into a synth voice and
 * rendering a few seconds of audio to disk */
constexpr float CLIP_LEVEL = 1.4f;
constexpr float VOLUME = 1.8f;

class Voice
{
public:
    Voice()
    {
        _osc.set_waveform(WtOscillatorBrick::Waveform::SAW);
        _osc2.set_waveform(WtOscillatorBrick::Waveform::SAW);
    }

    const AudioBuffer& render()
    {
        for (auto& brick : _audio_graph)
        {
            brick->render();
        }
        /* Parameter modulation */
        _cutoff -= 0.00001;
        return _amp.audio_output(VcaBrick<Response::LINEAR>::VCA_OUT);
    }

    void note_on() {_env.gate(true);}
    void note_off() {_env.gate(false);}

private:
    float _attack{.0f};
    float _decay{1.6f};
    float _sustain{0.3f};
    float _release{1.6f};
    float _rate{0.3f};
    float _pitch{0.20f}; /* in 0.1 / octave on a range from 20 to 20kHz */
    float _pitch_2{_pitch + 0.001f};
    float _cutoff{0.6};
    float _res{0.97f};
    float _clip{0.2f};
    float _gain{1.0f};


    LfoBrick                    _lfo{_rate};
    AudioADSREnvelopeBrick      _env{_attack, _decay, _sustain, _release};
    WtOscillatorBrick           _osc{_pitch};
    WtOscillatorBrick           _osc2{_pitch_2};
    AudioSummerBrick<2>         _mixer{_osc.audio_output(WtOscillatorBrick::OSC_OUT), _osc2.audio_output(WtOscillatorBrick::OSC_OUT)};
    SVFFilterBrick              _filt{_env.control_output(0), _res, _mixer.audio_output(AudioSummerBrick<2>::SUM_OUT)};
    AASaturationBrick             _dist{_clip, _filt.audio_output(BiquadFilterBrick::FILTER_OUT)};
    ControlMultiplierBrick<2>   _amp_level{VOLUME, _env.control_output(LinearADSREnvelopeBrick::ENV_OUT)};
    VcaBrick<Response::LINEAR>  _amp{_amp_level.control_output(ControlMultiplierBrick<2>::MULT_OUT), _dist.audio_output(BiquadFilterBrick::FILTER_OUT)};

    /* The order of creation automatically becomes a valid process order */
    std::vector<DspBrick*> _audio_graph{&_lfo, &_env, &_osc, &_osc2, &_mixer, &_filt, &_dist, &_amp_level, &_amp};
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
        if (samplecount > max_samples * 0.66)
        {
            voice.note_off();
        }
        samplecount += DSP_BRICKS_BLOCK_SIZE;
    }

    sf_close(output_file);
    return 0;
}