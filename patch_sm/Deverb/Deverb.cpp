#include "daisysp.h"
#include "daisy_patch_sm.h"


using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

#define MAX_DELAY ((size_t)(10.0f * 48000.0f))

DaisyPatchSM patch;
Switch toggle, modeButton;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayl;
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayr;
ReverbSc reverb;
Wavefolder wf;

//
float led_brightness{0.f};

const float kDampFreqMin = log(1000.f);
const float kDampFreqMax = log(19000.f);
float inlevel =0.5;
float revSend = 0;
float delSend = 0;
float foldSend = 0;
float foldAmount = 0.1;

float deltime = 0.5;
float delfb = 0.5;
float kval = 0.5;  // Delay Vars
// Persistent filtered Value for smooth delay time changes.
float smooth_time;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();
    toggle.Debounce();
    modeButton.Debounce();
    bool selMode = toggle.Pressed();

    dsy_gpio_write(&patch.gate_out_1, patch.gate_in_1.State());
    dsy_gpio_write(&patch.gate_out_2, patch.gate_in_2.State());

//Control for reverb
    if (selMode == true)
    {
        led_brightness = 4;
        
        float rev_time = 0.3 + (patch.GetAdcValue(CV_1));
        reverb.SetFeedback(rev_time);

        float damp_control = patch.GetAdcValue(CV_2);
        float damping = exp(kDampFreqMin + (damp_control * (kDampFreqMax - kDampFreqMin)));
        reverb.SetLpFreq(damping);

        inlevel = patch.GetAdcValue(CV_3);
        revSend = patch.GetAdcValue(CV_4);
        
    }
//Control to delay
    else if (selMode == false)
    {
        led_brightness = 0;
        
        kval    = patch.GetAdcValue(CV_1);
        deltime = (0.001f + (kval * kval) * 5.0f) * patch.AudioSampleRate();
        delfb   = patch.GetAdcValue(CV_2);
        inlevel = patch.GetAdcValue(CV_3);
        delSend = patch.GetAdcValue(CV_4);
        
    }
 

    for(size_t i = 0; i < size; i++)
    {
        float dryl ReverbSc = IN_L[i] * inlevel;
        float dryr  = IN_R[i] * inlevel;

        float sendDell = IN_L[i] * delSend;
        float sendDelr = IN_R[i] * delSend;

        // Smooth delaytime, and set.
        fonepole(smooth_time, deltime, 0.0005f);
        delayl.SetDelay(smooth_time);
        delayr.SetDelay(smooth_time);

        float delSigl = delayr.Read();
        float delSigr = delayl.Read();

        delayl.Write(sendDell + (delSigl * delfb));
        delayr.Write(sendDelr + (delSigr * delfb));

        float sendrevl = (IN_L[i] + delSigl) * revSend;
        float sendrevr = (IN_R[i] + delSigr) * revSend;

        float wetl, wetr;
        
        reverb.Process(sendrevl, sendrevr, &wetl, &wetr);
        
        
        OUT_L[i] = dryl + wetl + delSigl;
        OUT_R[i] = dryr + wetr + delSigr;
        

        patch.WriteCvOut(CV_OUT_2, led_brightness);
    }
}

int main(void)
{
    patch.Init();
    reverb.Init(patch.AudioSampleRate());
    float sampleRate = patch.AudioSampleRate();
    wf.Init();
    delayl.Init();
    delayl.SetDelay(sampleRate * 0.8f); // half second delay
    delayr.Init();
    delayr.SetDelay(sampleRate * 0.8f); // half second delay
    
    modeButton.Init(patch.B7,
                    sampleRate,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    Switch::PULL_UP);
    toggle.Init(patch.B8);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
