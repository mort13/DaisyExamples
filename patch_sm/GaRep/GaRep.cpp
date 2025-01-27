#include "daisysp.h"
#include "daisy_patch_sm.h"


using namespace daisy;
using namespace daisysp;
using namespace patch_sm;


DaisyPatchSM patch;
Switch toggle, modeButton;
VoctCalibration calibration;
ReverbSc reverb;
Wavefolder wf;

// GaRep variables
int minorPentatonic[25] = {  0,  3,  5,  7, 10,            // C, Eb, F, G, Bb
                            12, 15, 17, 19, 22,
                            24, 27, 29, 31, 34,
                            36, 39, 41, 43, 46,
                            48, 51, 53, 55, 58};
int Dorian[30] =          {  0,  2,  3,  5,  7, 10,        // C, D, Eb, F, G, A, Bb
                            12, 14, 15, 17, 19, 22,
                            24, 26, 27, 29, 31, 34,
                            36, 38, 39, 41, 43, 46,
                            48, 50, 51, 53, 55, 58};
int Lydian[35]=           {  0,  2,  4,  6,  7,  9, 11,    // C, D, E, F#, G, A, B 
                            12, 14, 16, 18, 19, 21, 23,
                            24, 26, 28, 30, 31, 33, 35,
                            36, 38, 40, 42, 43, 45, 47,
                            48, 50, 52, 54, 55, 57, 59};
int scalesLengths[3] = {5,6,7};
int lenScale = scalesLengths[0];
int bufferLength[8] = {2,4,6,7,8,12,16,16};           // Bufferlength for CV values
int midiBuffer[16] = {0};
int RHead = 0;
int WHead = 0;
int loopLength = 2;
int counter = 0;
bool sendnote = false;
int calstep = 0;
bool calibrating = false;
int calcounter = 0;
int selScale = 0;
int selMode = 0;
int* scales[3] = {minorPentatonic,Dorian,Lydian};
float led_brightness{0.f};
float cal1v;
float cal3v;

int pitch = 0;
int spread = 0;
int shift = 0;
int lengthKnob = 0;


const float kDampFreqMin = log(1000.f);
const float kDampFreqMax = log(19000.f);
float inlevel =0.5;
float revSend = 0;
float foldSend = 0;
float foldAmount = 0.1;

float mtocv(int midi)
{
    return static_cast<float>(midi) / 12.0f;
}

float selectNote(int note, int root, int spread, int shift,int scale[])
{
    int selNote = lenScale + (note %spread) + shift;
    return (scale[selNote] + root);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();
    toggle.Debounce();
    modeButton.Debounce();
    bool tr1 = patch.gate_in_1.Trig();
    bool tr2 = patch.gate_in_2.Trig();

    dsy_gpio_write(&patch.gate_out_1, patch.gate_in_1.State());
    dsy_gpio_write(&patch.gate_out_2, patch.gate_in_2.State());

//Control to GaRep
    if (selMode == 0)
    {
        int pitchKnob = int(fmap(patch.GetAdcValue(CV_1),0,12));
        int pitchCV = calibration.ProcessInput(patch.GetAdcValue(CV_5)); //fix this with proper calibration
        pitch = pitchKnob+pitchCV;

        int spreadKnob = fmap(patch.GetAdcValue(CV_2),1,2*lenScale);
        int spreadCV = patch.GetAdcValue(CV_6)*3.5;//Fix with proper calib
        spread = DSY_CLAMP(int(spreadKnob+spreadCV),0,2*lenScale);
        float led_brightness1 = patch.GetAdcValue(CV_6);

        patch.WriteCvOut(CV_OUT_2, led_brightness1);

        lengthKnob = 1+int(patch.GetAdcValue(CV_3)*8);
        int lengthCV = 0;//int(DSY_CLAMP(patch.GetAdcValue(CV_7),0,5)/5,0,8));
        int length = DSY_CLAMP(lengthKnob + lengthCV,0,8);
        loopLength = bufferLength[length];

        int shiftKnob = int(fmap(patch.GetAdcValue(CV_4),-lenScale-1,lenScale+1));
        int shiftCV = int(patch.GetAdcValue(CV_8)*5);
        shift = DSY_CLAMP(shiftKnob+shiftCV,-lenScale,lenScale);
    }
//Control for reverb
    else if (selMode == 1)
    {
        float rev_time = 0.3 + (0.67 * patch.GetAdcValue(CV_1));
        reverb.SetFeedback(rev_time);

        float damp_control = patch.GetAdcValue(CV_2);
        float damping = exp(kDampFreqMin + (damp_control * (kDampFreqMax - kDampFreqMin)));
        reverb.SetLpFreq(damping);

        inlevel = patch.GetAdcValue(CV_3);
        revSend = patch.GetAdcValue(CV_4);
    }

//Calibration (not done)
    if(modeButton.TimeHeldMs() >= 5000 && calstep == 0)
    {
        calibrating = true;
        calstep = 1;
    }

    if (calibrating)
    {
        if (modeButton.RisingEdge())
        {
            calstep = calstep + 1;
        }
        if (calstep == 1)
        {
            calcounter = (calcounter + 1) % 500;
            led_brightness = calcounter/100;
            cal1v = patch.GetAdcValue(CV_5);
        }

        if (calstep == 2)
        {
            calcounter = (calcounter + 1) % 1000;
            led_brightness = calcounter/200;
            cal3v = patch.GetAdcValue(CV_5); 
        }
        if (calstep == 3)
        {
            calibration.Record(cal1v,cal3v);
            led_brightness = 0;
            calstep = 0;
            calibrating = false;
        }


    }


//Selection of mode
    if (modeButton.RisingEdge())
    {
        selMode = (selMode + 1)%2;
        //led_brightness = selMode+2;
    }

//GaRep triggering
    if (tr2)
    {
        RHead = 0;
        WHead = 0;
    }

    if (tr1)
    {
        if (lengthKnob >= 8)
        {
            //led_brightness = 5;
            sendnote = true;
            WHead = (WHead + 1) %loopLength;
            midiBuffer[WHead] = rand()%(2*lenScale);
            RHead = WHead;
        }
        else
        {
            RHead = (RHead +1) %loopLength;
        }
        float noteOut = selectNote(midiBuffer[RHead],pitch,spread,shift,scales[selScale]);
        patch.WriteCvOut(1,mtocv(noteOut));
    }
    
    if (sendnote&&counter < 100)
    {
        counter = (counter + 1);
        //led_brightness = 5-(counter/20);
    }
    else
    {
        counter = 0;
        sendnote = false;
        //led_brightness = 0;
    }

    //patch.WriteCvOut(CV_OUT_2, led_brightness);

    for(size_t i = 0; i < size; i++)
    {
        /** Let's scale the input for the two destinations we want to send it to using multiplication. */
        float dryl  = IN_L[i] * inlevel;
        float dryr  = IN_R[i] * inlevel;
        float sendl = IN_L[i] * revSend;
        float sendr = IN_L[i] * revSend;
        float wetl, wetr;
        /** Process the send signal through the reverb */
        reverb.Process(sendl, sendr, &wetl, &wetr);

        /** Add the dry and the wet together, and assign those to the output */
        OUT_L[i] = dryl + wetl;
        OUT_R[i] = dryr + wetr;
    }
}

int main(void)
{
    patch.Init();
    reverb.Init(patch.AudioSampleRate());
    wf.Init();
    float sample_rate = patch.AudioSampleRate();
    modeButton.Init(patch.B7,
                    sample_rate,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    Switch::PULL_UP);
    toggle.Init(patch.B8);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
