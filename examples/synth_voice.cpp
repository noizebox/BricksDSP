#include <vector>
#include <iostream>

#include <sndfile.h>

#include "bricks_dsp/bricks.h"

constexpr float EXAMPLE_SAMPLERATE = 44100;
constexpr char EXAMPLE_FILE_NAME[] = "./example.wav";
constexpr int SECONDS_TO_RENDER = 5;

/* Modify to run the chain with x times oversampling */
constexpr int NORMAL_SR = 1;
constexpr int X_OVERSAMPLING = 4;

using namespace bricks;

/* Short example of how to combine a few bricks into a synth voice and
 * rendering a few seconds of audio to disk */
constexpr float CLIP_LEVEL = 1.4f;
constexpr float VOLUME = 1.8f;

int counter = 0;

class Voice
{
public:
    Voice()
    {
        for (auto& b : _audio_graph)
        {
            b->set_samplerate(EXAMPLE_SAMPLERATE);
        }
        _osc.set_waveform(Waveform::SAW);
        _osc2.set_waveform(Waveform::SAW);
    }

    const AudioBuffer& render()
    {
        float env_level;
        AudioOsBuffer<X_OVERSAMPLING> buffer_1;
        AudioOsBuffer<X_OVERSAMPLING> buffer_2;
        AudioOsBuffer<X_OVERSAMPLING> buffer_3;
        AudioBuffer buffer_4;

        buffer_2.fill(0);

        _env.render(_attack, _decay, _sustain, _release, env_level);
        _osc.render(_pitch, buffer_1);
        _osc2.render(_pitch - 0.0015f, buffer_2);
        _mixer.render({&buffer_1, &buffer_2}, buffer_3);
        _filt.render(_cutoff, _res, buffer_3, buffer_1, buffer_2, buffer_2);
        _dist.render(_clip, buffer_1, buffer_2);
        _downsampler.render(buffer_2, buffer_4);
        _amp.render(_gain * env_level, buffer_4, _buffer);

        /* Parameter modulation */
        _cutoff -= 0.00005;
        _pitch -= 0.000002;
        return _buffer;
    }

    void note_on() {_env.gate(true);}
    void note_off() {_env.gate(false);}

private:
    float _attack{.0f};
    float _decay{1.6f};
    float _sustain{0.3f};
    float _release{0.6f};
    float _pitch{0.22f}; /* in 0.1 / octave on a range from 20 to 20kHz */
    float _cutoff{0.85};
    float _res{0.85f};
    float _clip{1.5f};
    float _gain{0.3f};

    AudioBuffer _buffer;

    LinearADSREnvelopeBrick                           _env;
    OscillatorBrick<X_OVERSAMPLING>                   _osc;
    OscillatorBrick<X_OVERSAMPLING>                   _osc2;
    AudioSummerBrick<2, X_OVERSAMPLING>               _mixer;
    SVFFilterBrick<X_OVERSAMPLING>                    _filt;
    AASaturationBrick<ClipType::SOFT, X_OVERSAMPLING> _dist;
    DownSampleBrick<X_OVERSAMPLING, NORMAL_SR>        _downsampler;
    VcaBrick<Response::LINEAR>                        _amp;

    /* The order of creation automatically becomes a valid process order */
    std::vector<DspBrick*> _audio_graph{&_env, &_osc, &_osc2, &_mixer, &_filt, &_dist, &_downsampler, &_amp};
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