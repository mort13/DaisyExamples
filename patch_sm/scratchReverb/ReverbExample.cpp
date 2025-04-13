#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <array>
#include "mix-matrix.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;

#define MAX_DELAY ((size_t)(10.0f * 48000.0f))

DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS globalDelay;

double randomInRange(double low, double high) {
	// There are better randoms than this, and you should use them instead 😛
	double unitRand = rand()/double(RAND_MAX);
	return low + unitRand*(high - low);
}


template<int channels=8>
struct MultiChannelMixedFeedback {
	using Array = std::array<double, channels>;
	double delayMs = 150;
	double decayGain = 0.85;

	std::array<int, channels> delaySamples;
	std::array<Delay, channels> delays;
	
	void configure(double sampleRate) {
		double delaySamplesBase = delayMs*0.001*sampleRate;
		for (int c = 0; c < channels; ++c) {
			double r = c*1.0/channels;
			delaySamples[c] = std::pow(2, r)*delaySamplesBase;
			delays[c].SetDelay(delaySamples[c] + 1);
			delays[c].Reset();
		}
	}
	
	Array process(Array input) {
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delayed[c] = delays[c].Read(delaySamples[c]);
		}
		
		// Mix using a Householder matrix
		Array mixed = delayed;
		Householder<double, channels>::inPlace(mixed.data());
		
		for (int c = 0; c < channels; ++c) {
			double sum = input[c] + mixed[c]*decayGain;
			delays[c].Write(sum);
		}
		
		return delayed;
	}
};

template<int channels=8>
struct DiffusionStep {
	using Array = std::array<double, channels>;
	double delayMsRange = 50;
	
	std::array<int, channels> delaySamples;
	std::array<Delay, channels> delays;
	std::array<bool, channels> flipPolarity;
	
	void configure(double sampleRate) {
		double delaySamplesRange = delayMsRange*0.001*sampleRate;
		for (int c = 0; c < channels; ++c) {
			double rangeLow = delaySamplesRange*c/channels;
			double rangeHigh = delaySamplesRange*(c + 1)/channels;
			delaySamples[c] = randomInRange(rangeLow, rangeHigh);
			delays[c].SetDelay(delaySamples[c] + 1);
			delays[c].Reset();
			flipPolarity[c] = rand()%2;
		}
	}
	
	Array process(Array input) {
		// Delay
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delays[c].Write(input[c]);
			delayed[c] = delays[c].Read(delaySamples[c]);
		}
		
		// Mix with a Hadamard matrix
		Array mixed = delayed;
		Hadamard<double, channels>::inPlace(mixed.data());

		// Flip some polarities
		for (int c = 0; c < channels; ++c) {
			if (flipPolarity[c]) mixed[c] *= -1;
		}

		return mixed;
	}
};


template<int channels=8, int stepCount=4>
struct DiffuserHalfLengths {
	using Array = std::array<double, channels>;

	using Step = DiffusionStep<channels>;
	std::array<Step, stepCount> steps;

	DiffuserHalfLengths(double diffusionMs) {
		for (auto &step : steps) {
			diffusionMs *= 0.5;
			step.delayMsRange = diffusionMs;
		}
	}
	
	void configure(double sampleRate) {
		for (auto &step : steps) step.configure(sampleRate);
	}
	
	Array process(Array samples) {
		for (auto &step : steps) {
			samples = step.process(samples);
		}
		return samples;
	}
};

template<int channels=8, int diffusionSteps=4>
struct BasicReverb {
	using Array = std::array<double, channels>;
	
	MultiChannelMixedFeedback<channels> feedback;
	DiffuserHalfLengths<channels, diffusionSteps> diffuser;
	double dry, wet;

	BasicReverb(double roomSizeMs, double rt60, double dry=0, double wet=1) : diffuser(roomSizeMs), dry(dry), wet(wet) {
		feedback.delayMs = roomSizeMs;

		// How long does our signal take to go around the feedback loop?
		double typicalLoopMs = roomSizeMs*1.5;
		// How many times will it do that during our RT60 period?
		double loopsPerRt60 = rt60/(typicalLoopMs*0.001);
		// This tells us how many dB to reduce per loop
		double dbPerCycle = -60/loopsPerRt60;

		feedback.decayGain = std::pow(10, dbPerCycle*0.05);
	}
	
	void configure(double sampleRate) {
		feedback.configure(sampleRate);
		diffuser.configure(sampleRate);
	}
	
	Array process(Array input) {
		Array diffuse = diffuser.process(input);
		Array longLasting = feedback.process(diffuse);
		Array output;
		for (int c = 0; c < channels; ++c) {
			output[c] = dry*input[c] + wet*longLasting[c];
		}
		return output;
	}
};

constexpr int channels = 2;
BasicReverb<channels> reverb(100.0, 2.0, 0.3, 0.7); // roomSizeMs=100ms, RT60=2s, dry=0.3, wet=0.7

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    float time_knob = patch.GetAdcValue(CV_1);
    float time      = fmap(time_knob, 0.3f, 0.99f);

    float damp_knob = patch.GetAdcValue(CV_2);
    float damp      = fmap(damp_knob, 1000.f, 19000.f, Mapping::LOG);

    float in_level = patch.GetAdcValue(CV_3);

    float send_level = patch.GetAdcValue(CV_4);

    //reverb.SetFeedback(time);
    //reverb.SetLpFreq(damp);


    for(size_t i = 0; i < size; i++)
    {
        float dryl  = IN_L[i] * in_level;
        float dryr  = IN_R[i] * in_level;
        std::array<double, 2> dry = { static_cast<double>(dryl), static_cast<double>(dryr) };
        
        float sendl = IN_L[i] * send_level;
        float sendr = IN_R[i] * send_level;
        float wetl, wetr;

        reverb.process(dry);

        OUT_L[i] = dryl + wetl;
        OUT_R[i] = dryr + wetr;
    }
}

int main(void)
{
    int sampleRate = patch.AudioSampleRate();
    patch.Init();
    patch.StartAudio(AudioCallback);
    while(1) {}
}
