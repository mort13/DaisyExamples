#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;
ReverbSc reverb;
Oscillator osc;
Wavefolder wf;
Switch toggle;

// Some controll logic


// Reverb variables
float feedbackTime = 0.3f; // Stores the feedback time
float dampingFreq = 1000.0f; // Stores the damping frequency
float in_level = 1.0f; // Stores the input level
float send_level = 0.0f; // Stores the send level
float foldAmount = 1;

// Timing variables
int sampleRate;             // To hold the audio sample rate
int sampleCounter = 0;      // Sample counter to track elapsed samples
int msInterval = 500;       // Send every 500 ms

// Turing variables
float midiNotes[5] = {0, 0.167, 0.25, 0.333, 0.417};    // Selection of notes in CV
float groundKey[4] = {0, 0.417, 0.25, 1};               // Some key changes
float groundKeyMidi[4] = {62, 69, 65, 62};
int pow2[7] = {2,4,8,16,32,64,128};                     // Lookup table for pow2
float oscLevel = 0.0f;
float turingBuffer[64] = {0};                           // Buffer for CV
int turingGates[64] = {0};                              // Buffer for gates
int turingLength = 4;                                   // Length of loop
int counter = 0;                                        // Counter to count the time
float turingChance = 0;                                 // Chance for new random value

void turingMachineCV()
{   
    turingBuffer[counter] = midiNotes[rand() %5];
    patch.WriteCvOut(2,5);
}

void turingMachineGate()
{   
    turingGates[counter] = rand() %2;
    patch.WriteCvOut(2,3);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{ 
    patch.ProcessAnalogControls();

    toggle.Debounce();
    bool controlEnabled  = toggle.Pressed();
    int samplesToSend = 44100 * msInterval / 1000; // Calculate how many samples to wait
    sampleCounter += size;

    if (controlEnabled)
    {
        patch.WriteCvOut(2,4);
        float time_knob = patch.GetAdcValue(CV_1);
        float time      = fmap(time_knob, 0.3f, 0.99f);

        float damp_knob = patch.GetAdcValue(CV_2);
        float damp      = fmap(damp_knob, 1000.f, 19000.f, Mapping::LOG);

        in_level = patch.GetAdcValue(CV_3);

        send_level = patch.GetAdcValue(CV_4);

        reverb.SetFeedback(time);
        reverb.SetLpFreq(damp);
    }
    else
    {
        // Adjust chance with CV_1
        float chanceKnob = patch.GetAdcValue(CV_1);
        turingChance = static_cast<int>(fmap(chanceKnob,-50,50));

        // Adjust timing with CV_2
        float timingControlKnob = patch.GetAdcValue(CV_2);
        msInterval = static_cast<int>(1010 - (timingControlKnob * 1000));

        // Adjust buffer length with CV_3
        float lengthControlKnob = patch.GetAdcValue(CV_3);
        turingLength = pow2[static_cast<int>(fmap(lengthControlKnob,1,6))];

        float foldKnob = patch.GetAdcValue(CV_4);
        float foldCV = 1 + abs(patch.GetAdcValue(CV_5));
        foldAmount = 1 + foldCV*foldKnob*3;
        wf.SetGain(foldAmount);

        if (sampleCounter >= (int (samplesToSend/2)))
        {
            patch.WriteCvOut(2,0);
        }
    }
    for(size_t i = 0; i < size; i++)
    {
        float sig = osc.Process();
        float dryl  = wf.Process(IN_L[i]) * (in_level / foldAmount);
        float dryr  = wf.Process(IN_R[i]) * (in_level / foldAmount);
        float sendl = wf.Process(IN_L[i])  * (send_level / foldAmount);
        float sendr = wf.Process(IN_R[i]) * (send_level / foldAmount);
        float wetl, wetr;
        reverb.Process(sendl, sendr, &wetl, &wetr);
        OUT_L[i] = dryl + wetl + sig;
        OUT_R[i] = dryr + wetr + sig;
    }


    if (sampleCounter >= (int (samplesToSend/2)))
    {
        // Deactivate gate signal (set to low)
        dsy_gpio_write(&patch.gate_out_1, 0);
        dsy_gpio_write(&patch.gate_out_2, 0);
    }
    if (sampleCounter >= samplesToSend)
    {
        if (turingChance > 10 + rand() %40)
        {
            turingMachineCV();
        }
        if (turingChance < -10 -rand()%40)
        {
            turingMachineGate();
        }

        // Send the next MIDI note to CV output when gate is high
        if (turingGates[counter] == 1)
        {

            patch.WriteCvOut(1, turingBuffer[counter] +  (groundKey[(counter/16)])); // Send current note
        }
        osc.SetFreq(mtof(groundKeyMidi[(counter/16)]));
        dsy_gpio_write(&patch.gate_out_1, turingGates[counter]);
        dsy_gpio_write(&patch.gate_out_2, (turingGates[counter]*-1) + 1);
        counter = (counter + 1) % turingLength; // Cycle through `midiNotes`
        sampleCounter = 0; 
    }

}

int main(void)
{
    patch.Init();
    toggle.Init(patch.B8);
    reverb.Init(patch.AudioSampleRate());
    //osc.Init(patch.AudioSampleRate());
    wf.Init();
    patch.StartAudio(AudioCallback);
    while(1) {}
}
