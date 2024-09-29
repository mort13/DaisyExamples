#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"
#include "calibrate.h"
#include "settings.h"


using namespace daisysp;
using namespace daisy;
using namespace patch_sm;
using namespace nimbus;

GranularProcessorClouds     processor;
DaisyPatchSM                hw;
Switch                      toggle, mode_button;
Calibrate                   calibration;
PersistentStorage<Settings> storage(hw.qspi);
Settings                    default_settings{};

SettingsState settings_state{NONE};
// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[(118784*2)];
uint8_t block_ccm[(65536*2) - 128];

Parameters* parameters;
enum SettingsState
{
    NONE,
    CALIBRATE
};

bool    trigger_save;
int     pbmode = 0;
float   led_mode = 5;
int     quality = 0;
float   led_quality = 0;

void controls()
{
    Settings& settings_data = storage.GetSettings();
    hw.ProcessAllControls();
    toggle.Debounce();
    mode_button.Debounce();

    bool shift  = toggle.Pressed();

    bool freezeGate = hw.gate_in_1.State();
    parameters->freeze = freezeGate;

    bool grainGate = hw.gate_in_2.State();
    parameters->trigger = grainGate;

    dsy_gpio_write(&hw.gate_out_1, freezeGate);
    dsy_gpio_write(&hw.gate_out_2, grainGate);

    float posKnob;
    float sizeKnob;
    float densityKnob;
    float textureknob;
    float pitchKnob;
    float panKnob;
    float dryWetKnob;
    float revKnob;

    float pitchCV = hw.GetAdcValue(CV_5);
    float posCV = hw.GetAdcValue(CV_6);
    float densityCV = hw.GetAdcValue(CV_7);
    float textureCV = hw.GetAdcValue(CV_8);

    if (shift == true)
    {

        if(mode_button.TimeHeldMs() >= 5000)
        {
            // Light up the LED to indicate that releasing the button will enter
            // the calibration state.
            led_mode = 1.f;
            if(settings_state != CALIBRATE)
            {
                // Holding the mode button while in shift mode enters the calibration
                // state.
                settings_state = CALIBRATE;
                calibration.Start();
            }
        }
        else if(settings_state == CALIBRATE)
        {
            float cv      = hw.GetAdcValue(CV_5);
            bool  pressed = mode_button.RisingEdge();
            if(calibration.ProcessCalibration(cv, pressed))
            {
                calibration.cal.GetData(settings_data.scale,
                                        settings_data.offset);
                trigger_save   = true;
                settings_state = NONE;
            }
            led_mode = calibration.GetBrightness();
        }


        if (mode_button.RisingEdge())
        {
            pbmode = (pbmode + 1) %4;
            processor.set_playback_mode((PlaybackMode)pbmode);
            led_mode = 5 - pbmode;
        }
        hw.WriteCvOut(2,led_mode);

        posKnob = hw.GetAdcValue(CV_1);
        sizeKnob = hw.GetAdcValue(CV_2);
        densityKnob = hw.GetAdcValue(CV_3);
        textureknob = hw.GetAdcValue(CV_4);
    }
    else if (shift == false)
    {
        if (mode_button.RisingEdge())
        {
            quality = (quality + 1) %4;
            processor.set_quality((GrainQuality)quality);
            led_quality = 1 + quality;
        }
        hw.WriteCvOut(2,led_quality);

        pitchKnob = hw.GetAdcValue(CV_1);
        panKnob = hw.GetAdcValue(CV_2);
        dryWetKnob = hw.GetAdcValue(CV_3);
        revKnob = hw.GetAdcValue(CV_4);
    }
    parameters->position = DSY_CLAMP((posKnob + posCV),0,1);
    parameters->size = DSY_CLAMP((sizeKnob),0.01,1); //too small values for the size leads to crashes.
    parameters->density = DSY_CLAMP((densityKnob + densityCV),0,1);
    parameters->texture = DSY_CLAMP((textureknob + textureCV),0,1);

    parameters->pitch = pitchKnob+pitchCV;
    parameters->stereo_spread = panKnob;
    parameters->dry_wet = dryWetKnob;
    parameters->reverb = revKnob;
    
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    controls();

    FloatFrame input[size];
    FloatFrame output[size];

    for(size_t i = 0; i < size; i++)
    {
        input[i].l  = in[0][i];
        input[i].r  = in[1][i];
        output[i].l = output[i].r = 0.f;
        
    }

    processor.Process(input, output, size);

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = output[i].l;
        out[1][i] = output[i].r;
    }
}


int main(void)
{

    hw.Init();
    float sample_rate = hw.AudioSampleRate();
    toggle.Init(hw.B8);
    mode_button.Init(hw.B7,
                    sample_rate,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    Switch::PULL_UP);

    hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    parameters = processor.mutable_parameters();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        processor.Prepare();
        hw.Delay(1);
        if(trigger_save)
        {
            storage.Save();
            trigger_save = false;
        }
    }
}
