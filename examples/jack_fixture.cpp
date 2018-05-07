#include <vector>
#include <iostream>
#include <thread>
#include <csignal>

#include <jack/jack.h>
#include <jack/midiport.h>

#define DSP_BRICKS_BLOCK_SIZE 32

#include "bricks.h"

constexpr int EXAMPLE_SAMPLERATE = 44100;
constexpr int MAX_CC_VALUE = 127;
constexpr int MAX_PITCH_BEND = 16384;

constexpr unsigned char MESSAGE_MASK = 0b11110000;
constexpr unsigned char CC_MSG       = 0b10110000;

using namespace bricks;

class Processor
{
public:
    Processor()
    {
        _osc.set_samplerate(EXAMPLE_SAMPLERATE);
        _osc.set_waveform(WtOscillatorBrick::Waveform::SINE);
        _mod_osc.set_waveform(WtModOscillatorBrick::Waveform::PULSE);
    }

    static void render(void* inst, const AudioBuffer& in, AudioBuffer& out)
    {
        reinterpret_cast<Processor*>(inst)->render(in, out);
    }

    void midi_cc(int controller, int value)
    {
        //std::cout << "Controller: " << controller << ", value: " << value <<std::endl;
        switch (controller)
        {
            case 9:
                _mod_lag.set(static_cast<float>(value) / MAX_CC_VALUE * 2.0f);
                break;
            case 10:
                _pitch_lag.set(static_cast<float>(value) / MAX_CC_VALUE);
                break;
            case 11:
                _pitch2_lag.set(static_cast<float>(value) / MAX_CC_VALUE);
                break;
        }
    }

    void render(const AudioBuffer& in, AudioBuffer& out)
    {
        _pitch = _pitch_lag.get();
        _pitch2 = _pitch2_lag.get();
        _fm_mod = _mod_lag.get();

        _osc.render();
        _mod.render();
        _mod_osc.render();
        out = _mod_osc.audio_output(0);
    }

private:
    OnePoleLag<512> _pitch_lag;
    OnePoleLag<512> _pitch2_lag;
    OnePoleLag<512> _mod_lag;

    float _dummy = 0;
    float _pitch;
    float _pitch2;
    float _fm_mod;
    float gain = 0.0f;
    //float pwm = 0.0;
    WtOscillatorBrick       _osc{_pitch};
    VcaBrick                _mod{_fm_mod, _osc.audio_output(OscillatorBrick::OSC_OUT)};
    WtModOscillatorBrick    _mod_osc{_pitch2, _dummy, _osc.audio_output(WtOscillatorBrick::SYNC_OUT),
                                                      _mod.audio_output(VcaBrick::VCA_OUT)};

    /*OscillatorBrick       _osc{_pitch};
    VcaBrick              _mod{_fm_mod, _osc.audio_output(OscillatorBrick::OSC_OUT)};
    FmOscillatorBrick     _mod_osc{_pitch2, _mod.audio_output(VcaBrick::VCA_OUT)};*/


};

Processor processor;
jack_client_t* client;
jack_port_t* out_port;
jack_port_t* in_port;
jack_port_t* midi_port;
bool running(false);


int audio_cb(jack_nframes_t nframes, void *arg)
{
    auto* buffer = jack_port_get_buffer(midi_port, nframes);
    auto no_events = jack_midi_get_event_count(buffer);
    for (auto i = 0u; i < no_events; ++i)
    {
        jack_midi_event_t midi_event;
        int ret = jack_midi_event_get(&midi_event, buffer, i);
        if ((midi_event.buffer[0] & MESSAGE_MASK) == CC_MSG)
        {
            processor.midi_cc(midi_event.buffer[1], midi_event.buffer[2]);
        }
    }
    assert(nframes % DSP_BRICKS_BLOCK_SIZE == 0);
    AudioBuffer in;
    AudioBuffer out;
    auto jack_in = static_cast<float*>(jack_port_get_buffer(in_port, nframes));
    auto jack_out = static_cast<float*>(jack_port_get_buffer(out_port, nframes));

    for (int i = 0; i < nframes; i+= DSP_BRICKS_BLOCK_SIZE)
    {
        std::copy(jack_in, jack_in + DSP_BRICKS_BLOCK_SIZE, in.data());
        processor.render(in, out);
        std::copy(out.data(), out.data() + DSP_BRICKS_BLOCK_SIZE, jack_out);

        jack_in += DSP_BRICKS_BLOCK_SIZE;
        jack_out += DSP_BRICKS_BLOCK_SIZE;
    }
    return 0;
}

void signal_handler(int sig_number)
{
    running = false;
}

int main ()
{
    jack_status_t jack_status;
    jack_options_t options = JackNullOption;
    client = jack_client_open("Bricks", options, &jack_status, nullptr);
    int status = jack_set_process_callback(client, audio_cb, nullptr);
    assert(status == 0);

    out_port = jack_port_register (client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    in_port = jack_port_register (client, "audio_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    midi_port = jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    signal(SIGINT, signal_handler);
    running = true;
    status = jack_activate(client);
    assert(status == 0);
    while(running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    jack_client_close(client);
    return 0;
}