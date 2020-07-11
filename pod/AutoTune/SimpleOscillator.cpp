#include "daisy_pod.h"
#include "daisysp.h"

#define NUM_WAVEFORMS 4

using namespace daisy;
using namespace daisysp;

DaisyPod   hw;
Oscillator osc, osc2;
Parameter  p_freq;
PitchAmdf pitchAmdf;
PitchAmdf pitchAmdf2;

uint8_t waveforms[NUM_WAVEFORMS] = {
    Oscillator::WAVE_SIN,
    Oscillator::WAVE_TRI,
    Oscillator::WAVE_POLYBLEP_SAW,
    Oscillator::WAVE_POLYBLEP_SQUARE,
};

// static float   freq;
float          sig;
static int     waveform, octave;

int samplesProcessed = 0;
float totalCps = 0.0;
int sampleIdx = 0;
int sampleFactor = 6;
static void AudioCallback(float *in, float *out, size_t size)
{
    hw.DebounceControls();

    waveform += hw.encoder.Increment();
    waveform = DSY_CLAMP(waveform, 0, NUM_WAVEFORMS);
    osc.SetWaveform(waveforms[waveform]);

    // if(hw.button2.RisingEdge())
    //     octave++;
    // if(hw.button1.RisingEdge())
    //     octave--;
    // octave = DSY_CLAMP(octave, 0, 4);

    // convert MIDI to frequency and multiply by octave size
    // freq = mtof(p_freq.Process() + (octave * 12));
    osc.SetFreq(p_freq.Process());

    bool isOn = hw.button1.Pressed();
    hw.seed.SetLed(isOn);
    bool isOn2 = hw.button2.Pressed();

    // Audio Loop
    // float lastFreq = 0.f;
    for(size_t i = 0; i < size; i += 2)
    {
        // if button2 is down, short-circuit osc2 -> sig
        if (isOn2) {
            sig = osc2.Process();
        } else {
            // Process
            sig        = osc.Process();
            if (isOn) {
                // feed signal to pitch detection
                // lastFreq = pitchAmdf.Process(sig);
                if (sampleIdx == 0) {
                    if (pitchAmdf.Process(sig) > 0) {
                        totalCps += pitchAmdf.GetCps();
                        samplesProcessed += 1;
                    }
                }
                sampleIdx = (sampleIdx + 1) % sampleFactor;
                // osc2.SetFreq();
                sig = osc2.Process();
            }
        }
        out[i]     = sig;
        out[i + 1] = sig;
    }

    if (samplesProcessed > 480 / sampleFactor) {
        // instead of averaging, could we do something that's a little more resilient to outliers?
        // can we also leverage rms to decide on the best aggregate frequency?
        osc2.SetFreq((int)(totalCps / samplesProcessed));
        samplesProcessed = 0;
        totalCps = 0.f;
        // osc2.SetFreq(pitchAmdf.GetCps());
    }

    // set frequency and hope this oscillator works well
    // if (lastFreq > 0.0) {
    //     osc2.SetFreq(lastFreq);
    // } else {
    //     osc2.SetFreq(440.0);
    // }
}

void InitSynth(float samplerate)
{
    // Init freq Parameter to knob1 using MIDI note numbers
    // min 10, max 127, curve linear
    int minFreq = 100;
    int midFreq = 400;
    int maxFreq = 800;
    p_freq.Init(hw.knob1, minFreq, maxFreq, Parameter::LINEAR);

    osc.Init(samplerate);
    osc.SetAmp(1.f);

    osc2.Init(samplerate);
    osc2.SetAmp(1.f);
    osc2.SetWaveform(Oscillator::WAVE_SIN);

    pitchAmdf.Init(samplerate / sampleFactor, minFreq, midFreq);
    pitchAmdf2.Init(samplerate / sampleFactor, midFreq, maxFreq);
    // let's explore running two amdf's, 200-350, 350-650
    // if the output of the lower is 350 or garbage, go with higher?
    // this wouldn't prevent lower from identifying 250 if the actual frequency was 500

    waveform = 0;
    octave   = 0;
}

int main(void)
{
    float samplerate;

    // Init everything
    hw.Init();
    samplerate = hw.AudioSampleRate();
    InitSynth(samplerate);

    // start callbacks
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1) {}
}
