#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;
Switch toggle;


// GaRep variables
float minorPentatonic[5] = {0, 3, 5, 7, 10};    // C, Eb, F, G, Bb
int groundKey = 48;                             // Some key changes
int pow2[7] = {2,4,8,16,32,64,128};             // Lookup table for pow2
int bufferLength[7] = {2,4,6,7,8,16,32};           // Bufferlength for CV values
int cvBuffer[32] = {0};

void mtocv(int midi)
{
    float cv = midi * 1/12;
    return cv;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{ 
    patch.ProcessAnalogControls();

    toggle.Debounce();

}

int main(void)
{
    patch.Init();
    toggle.Init(patch.B8);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
