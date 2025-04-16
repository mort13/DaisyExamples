#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <array>
#include "mix-matrix.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;



DaisyPatchSM patch;

DSY_SDRAM_BSS uint8_t g_delayMemory[8][sizeof(DelayLine<float, 48000>)];


template<int channels=8>
struct MultiChannelMixedFeedback {
	using Array = std::array<double, channels>;
	double delayMs = 150;
	double decayGain = 0.85;

	std::array<int, channels> delaySamples;
	std::array<DelayLine<float, 48000>*, channels> delays;


	
	void configure(double sampleRate) {
		double delaySamplesBase = delayMs*0.001*sampleRate;
		
		for (int c = 0; c < channels; ++c) {
			double r = c*1.0/channels;
			delaySamples[c] = static_cast<int>(delaySamplesBase);
			
			delays[c] = new (g_delayMemory[c]) DelayLine<float, 48000>();
			delays[c]->Init();
			delays[c]->SetDelay(static_cast<float>(delaySamples[c] + 1));
			delays[c]->Reset();
		}
	}
	
	Array process(Array input) {
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delayed[c] = delays[c]->Read(delaySamples[c]);
		}
		
		// Mix using a Householder matrix
		Array mixed = delayed;
		Householder<double, channels>::inPlace(mixed.data());
		
		for (int c = 0; c < channels; ++c) {
			double sum = input[c] + mixed[c]*decayGain;
			delays[c]->Write(sum);
		}
		
		return delayed;
	}
};


MultiChannelMixedFeedback<8> reverb;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    float timeKnob = patch.GetAdcValue(CV_1);
    float feedbackKnob = patch.GetAdcValue(CV_2);
    float inLevel = patch.GetAdcValue(CV_3);
    float sendLevel = patch.GetAdcValue(CV_4);

	float delayMS = timeKnob * 20000 + 50;
	reverb.delayMs = delayMS;

	float feedback = feedbackKnob;
	reverb.decayGain = feedback;


    for(size_t i = 0; i < size; i++)
    {
        float dryL  = IN_L[i] * inLevel;
        float dryR  = IN_R[i] * inLevel;
        float sendL = IN_L[i] * sendLevel;
        float sendR = IN_R[i] * sendLevel;

		MultiChannelMixedFeedback<8>::Array inputFrame = {};
		for (int c=0; c<8; ++c){
			inputFrame[c] = (c%2==0) ? sendL : sendR;
		}

		auto outputFrame = reverb.process(inputFrame);
		
		float wetL = 0.0f;
		float wetR = 0.0f;

        for (int c = 0; c < 8; ++c){
			if (c%2 == 0)
				wetL += outputFrame[c];
			else
				wetR += outputFrame[c];
		}


        OUT_L[i] = dryL + wetL;
        OUT_R[i] = dryR + wetR;
    }
}

int main(void)
{
    patch.Init();
	int sampleRate = patch.AudioSampleRate();
	reverb.configure(sampleRate);
    patch.StartAudio(AudioCallback);
    while(1) {}
}
