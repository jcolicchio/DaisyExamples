#include "daisy_pod.h"
#include "daisysp.h"

#define NUM_WAVEFORMS 4

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc;
Parameter  p_freq;

WaveformBuffer waveformBuffer;
uint32_t _sampleRate;
uint32_t _desiredFrequency;

void UsbCallback(uint8_t* buf, uint32_t* len)
{
    _desiredFrequency = 0;
    uint32_t _refreshRate = 15;
    uint32_t _maxBufferLength = 60;

    uint32_t *buffer = (uint32_t *)buf;
    if (*len >= 4) {
        _desiredFrequency = buffer[0];
        if (*len >= 8) {
            _refreshRate = buffer[1];
            if (*len >= 12) {
                _maxBufferLength = buffer[2];
            }
        }
    }
    waveformBuffer.Init(_sampleRate, _desiredFrequency, _refreshRate, _maxBufferLength);
    hw.seed.SetLed(true);
}

uint8_t waveforms[NUM_WAVEFORMS] = {
    Oscillator::WAVE_SIN,
    Oscillator::WAVE_TRI,
    Oscillator::WAVE_POLYBLEP_SAW,
    Oscillator::WAVE_POLYBLEP_SQUARE,
};

static float   freq;
float          sig;
static int     waveform, octave;

static void AudioCallback(float *in, float *out, size_t size)
{
    hw.DebounceControls();

    waveform += hw.encoder.Increment();
    waveform = DSY_CLAMP(waveform, 0, NUM_WAVEFORMS);
    osc.SetWaveform(waveforms[waveform]);

    if(hw.button2.RisingEdge())
        octave++;
    if(hw.button1.RisingEdge())
        octave--;
    octave = DSY_CLAMP(octave, 0, 4);

    // convert MIDI to frequency and multiply by octave size
    freq = mtof(p_freq.Process() + (octave * 12));
    osc.SetFreq(freq);
    // if there's no desired fixed frequency, use frequency of oscillator
    if (_desiredFrequency == 0) {
        waveformBuffer.setFrequency(freq);
    }

    // Audio Loop
    for(size_t i = 0; i < size; i += 2)
    {
        // Process
        sig        = osc.Process();
        float limit = hw.knob2.Process();
        if (sig > limit) {
            sig = limit;
        } else if (sig < -limit) {
            sig = -limit;
        }
        out[i]     = sig;
        out[i + 1] = sig;

        // Fill buffer and transmit if full
        if (waveformBuffer.Process(sig)) {
            hw.seed.usb_handle.TransmitInternal(waveformBuffer.buffer(), waveformBuffer.bufferSize());
        }
    }
}

void InitSynth(float samplerate)
{
    // Init freq Parameter to knob1 using MIDI note numbers
    // min 10, max 127, curve linear
    p_freq.Init(hw.knob1, 0, 127, Parameter::LINEAR);

    osc.Init(samplerate);
    osc.SetAmp(1.f);

    waveform = 0;
    octave   = 0;

    _sampleRate = samplerate;
}

int main(void)
{
    float samplerate;

    // Init everything
    hw.Init();

    hw.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
	dsy_system_delay(500);
    hw.seed.usb_handle.SetReceiveCallback(UsbCallback, UsbHandle::FS_INTERNAL);

    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);

    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}
