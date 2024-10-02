#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;
Switch toggle, scaleButton;



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
int selScale = 0;
int* scales[3] = {minorPentatonic,Dorian,Lydian};

float mtocv(int midi)
{
    return static_cast<float>(midi) / 12.0f;
}

float selectNote(int note, int root, int spread, int shift,int scale[])
{
    int selNote = 5 + (note %spread) + shift;
    return (scale[selNote] + root);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{ 
    patch.ProcessAnalogControls();
    toggle.Debounce();
    scaleButton.Debounce();
    bool tr1 = patch.gate_in_1.Trig();
    bool tr2 = patch.gate_in_2.Trig();
    dsy_gpio_write(&patch.gate_out_1, patch.gate_in_1.State());


    int pitchKnob = int(fmap(patch.GetAdcValue(CV_1),0,12));
    float pitchCV = patch.GetAdcValue(CV_5)*12;
    int pitch = int(DSY_CLAMP(pitchKnob+pitchCV,0,72));

    int spreadKnob = int(fmap(patch.GetAdcValue(CV_2),1,2*lenScale));
    int spreadCV = int(patch.GetAdcValue(CV_6)*2*(lenScale/5));
    int spread = DSY_CLAMP(spreadKnob+spreadCV,0,2*lenScale);

    int lengthKnob = 1+int(patch.GetAdcValue(CV_3)*8);
    int lengthCV = 0;//int(fmap(DSY_CLAMP(patch.GetAdcValue(CV_7),0,5)/5,0,8));
    int length = DSY_CLAMP(lengthKnob + lengthCV,0,8);
    loopLength = bufferLength[length];

    int shiftKnob = int(fmap(patch.GetAdcValue(CV_4),-lenScale,lenScale));
    int shiftCV = int(patch.GetAdcValue(CV_8)*(lenScale/5));
    int shift = DSY_CLAMP(shiftKnob+shiftCV,-lenScale,lenScale);

    if (scaleButton.RisingEdge())
    {
        selScale = (selScale + 1)%3;
        lenScale = scalesLengths[selScale];
        patch.WriteCvOut(2,3+selScale);
    }

    if (tr2)
    {
        RHead = 0;
        WHead = 0;
    }

    if (tr1)
    {
        if (lengthKnob >= 8)
        {
            patch.WriteCvOut(2,5);
            sendnote = true;
            WHead = (WHead + 1) %loopLength;
            midiBuffer[WHead] = rand()%lenScale;
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
        patch.WriteCvOut(2,5-(counter/20));
    }
    else
    {
        counter = 0;
        sendnote = false;
        patch.WriteCvOut(2,0);
    }

}

int main(void)
{
    patch.Init();
    float sample_rate = patch.AudioSampleRate();
    scaleButton.Init(patch.B7,
                    sample_rate,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    Switch::PULL_UP);
    toggle.Init(patch.B8);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
