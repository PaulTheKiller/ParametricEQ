/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//Struktura do przechowania ustawieñ parametrów
struct Settings
{
    float highPassFreq{ 0 }, lowPassFreq{ 0 };
    int highPassSlope{ 0 }, lowPassSlope{ 0 };
    int filter1Type{ 0 }, filter2Type{ 0 }, filter3Type{ 0 }, filter4Type{ 0 };
    float filter1Freq{ 0 }, filter1Gain{ 0 }, filter1Quality{ 1.f },
        filter2Freq{ 0 }, filter2Gain{ 0 }, filter2Quality{ 1.f }, 
        filter3Freq{ 0 }, filter3Gain{ 0 }, filter3Quality{ 1.f }, 
        filter4Freq{ 0 }, filter4Gain{ 0 }, filter4Quality{ 1.f };
    float gain{ 0 };
    bool highPassOff{ true }, lowPassOff{ true },
        filter1Off{ false }, filter2Off{ false }, filter3Off{ false }, filter4Off{ false };
};

//funkcja do wczytywania parametrów z drzewa do struktury
Settings getSettings(juce::AudioProcessorValueTreeState& state);

//aliasy do filtrów i torów przetwarzania
using Filter = juce::dsp::IIR::Filter<float>;

using HLPassFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using Chain = juce::dsp::ProcessorChain<HLPassFilter, Filter, Filter, Filter, Filter, HLPassFilter>;

enum Positions
{
    HighPass, Filter1, Filter2, Filter3, Filter4, LowPass
};

void updateCoefficients(Filter::CoefficientsPtr& before, const Filter::CoefficientsPtr& after);

//filterID: 0, 1, 2, 3; tworzenie filtrów

Filter::CoefficientsPtr createFilters1_4(const Settings& settings, double sampleRate, int filterID);

template<typename ChainType, typename CoefficientType>
void updatePassFilter(ChainType& passFilter,
    const CoefficientType& coeffs,
    const int& slope)
{
    //0 - 12 dB/oct, 1 - 24 dB/oct, 2 - 36dB/oct, 3 - 48dB/oct
    passFilter.template setBypassed<0>(true);
    passFilter.template setBypassed<1>(true);
    passFilter.template setBypassed<2>(true);
    passFilter.template setBypassed<3>(true);

    switch (slope)
    {
    case 3:
    {
        updateCoefficients(passFilter.template get<3>().coefficients, coeffs[3]);
        passFilter.template setBypassed<3>(false);
    }

    case 2:
    {
        updateCoefficients(passFilter.template get<2>().coefficients, coeffs[2]);
        passFilter.template setBypassed<2>(false);
    }
    case 1:
    {
        updateCoefficients(passFilter.template get<1>().coefficients, coeffs[1]);
        passFilter.template setBypassed<1>(false);
    }
    case 0:
    {
        updateCoefficients(passFilter.template get<0>().coefficients, coeffs[0]);
        passFilter.template setBypassed<0>(false);
    }
    default:
        break;
    }
}

//==============================================================================
/**
*/
class PJKParametricEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    PJKParametricEQAudioProcessor();
    ~PJKParametricEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
     
    //Drzewo parametrów
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); //funkcja do tworzenia layoutu
    juce::AudioProcessorValueTreeState state{ *this, nullptr, "Parameters", createParameterLayout() }; //drzewo parametrów
    
    //getter do miernika
    float getRMSValue(const int channel) const;
private:  
    //tor przetwarzania
    Chain leftChain, rightChain;
    //wzmocnienie
    juce::dsp::Gain<float> gain;
    
    void updateHighPass(const Settings& settings);
    void updateLowPass(const Settings& settings);

    void updateAllFilters();
   
    void updateFilters1_4(const Settings& settings);

    //miernik RMS
    juce::LinearSmoothedValue<float> rightRMSLevel, leftRMSLevel;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PJKParametricEQAudioProcessor)
};
