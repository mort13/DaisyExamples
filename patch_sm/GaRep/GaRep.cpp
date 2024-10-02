#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;
Switch toggle;


// GaRep variables
int minorPentatonic[25] = { 0,  3,  5,  7, 10,
                            12, 15, 17, 19, 22,
                            24, 27, 29, 31, 34,
                            36, 39, 41, 43, 46,
                            48, 51, 53, 55, 58};// C, Eb, F, G, Bb
int lenScale = 5;
int pow2[7] = {2,4,8,16,32,64,128};             // Lookup table for pow2
int bufferLength[8] = {2,4,6,7,8,12,16,16};           // Bufferlength for CV values
int midiBuffer[16] = {0};
int RHead = 0;
int WHead = 0;
int loopLength = 2;
int counter = 0;
bool sendnote = false;

float mtocv(int midi)
{
    return static_cast<float>(midi) / 12.0f;;
}

float selectNote(int note, int root, int spread, int shift)
{
    int selNote = 5 + (note %spread) + shift;
    return (minorPentatonic[selNote] + root);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{ 
    patch.ProcessAnalogControls();
    toggle.Debounce();
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
            midiBuffer[WHead] = rand()%5;
            RHead = WHead;
        }
        else
        {
            RHead = (RHead +1) %loopLength;
        }
        float noteOut = selectNote(midiBuffer[RHead],pitch,spread,shift);
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
    toggle.Init(patch.B8);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
