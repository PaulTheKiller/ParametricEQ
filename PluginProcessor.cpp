/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PJKParametricEQAudioProcessor::PJKParametricEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

PJKParametricEQAudioProcessor::~PJKParametricEQAudioProcessor()
{
}

//==============================================================================
const juce::String PJKParametricEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PJKParametricEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PJKParametricEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PJKParametricEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PJKParametricEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PJKParametricEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PJKParametricEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PJKParametricEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PJKParametricEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void PJKParametricEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
//Przygotowanie programu
void PJKParametricEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    getLatencySamples();
    
    juce::dsp::ProcessSpec ps;
    ps.numChannels = 1; //jeden kana³ mono
    ps.sampleRate = sampleRate;
    ps.maximumBlockSize = samplesPerBlock;
    
    leftChain.prepare(ps);
    rightChain.prepare(ps);

    //wzmocnienie
    gain.prepare(ps);
    gain.setRampDurationSeconds(0.01);

    //aktualizacja filtrów
    updateAllFilters();

    //reset miernika
    leftRMSLevel.reset(sampleRate, 0.4f);
    rightRMSLevel.reset(sampleRate, 0.4f);

    leftRMSLevel.setCurrentAndTargetValue(-100.f);
    rightRMSLevel.setCurrentAndTargetValue(-100.f);
}

void PJKParametricEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PJKParametricEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//Blok przetwarzania
void PJKParametricEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateAllFilters();
    
    //blok przejmuje bufor
    juce::dsp::AudioBlock<float> block(buffer);

    //kontekst przetwarzania przejmuje blok
    juce::dsp::ProcessContextReplacing<float> leftContext(block.getSingleChannelBlock(0)); //lewy
    juce::dsp::ProcessContextReplacing<float> rightContext(block.getSingleChannelBlock(1)); //prawy

    //przetwarzanie kontekstu
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    //wzmocnienie koñcowe
    gain.setGainDecibels(getSettings(state).gain);
    gain.process(leftContext);
    gain.process(rightContext);

    //miernik RMS
   leftRMSLevel.skip(buffer.getNumSamples());
    rightRMSLevel.skip(buffer.getNumSamples());

    const auto leftLevel = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
    if (leftLevel < leftRMSLevel.getCurrentValue())
        leftRMSLevel.setTargetValue(leftLevel);
    else
        leftRMSLevel.setCurrentAndTargetValue(leftLevel);

    const auto rightLevel = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
    if (rightLevel < rightRMSLevel.getCurrentValue())
        rightRMSLevel.setTargetValue(rightLevel);
    else
        rightRMSLevel.setCurrentAndTargetValue(rightLevel);
}
//getter do miernika
float PJKParametricEQAudioProcessor::getRMSValue(const int channel) const
{
    if (channel == 0)
        return leftRMSLevel.getCurrentValue();
    if (channel == 1)
        return rightRMSLevel.getCurrentValue();
    return 0.f;
}

//==============================================================================
bool PJKParametricEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PJKParametricEQAudioProcessor::createEditor()
{
    //PPCPP Prosty Editor 
    return new PJKParametricEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
//
void PJKParametricEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream stream(destData, true);
    state.state.writeToStream(stream);
}

void PJKParametricEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid())
    {
        state.replaceState(tree);
        updateAllFilters();
    }
}

//wczytywanie ustawieñ parametrów do struktury
Settings getSettings(juce::AudioProcessorValueTreeState& state)
{
    Settings settings;
    settings.highPassFreq = state.getRawParameterValue("HighPass Freq")->load();
    settings.highPassSlope = state.getRawParameterValue("HighPass Slope")->load();

    settings.filter1Type = state.getRawParameterValue("Filter1 Type")->load();
    settings.filter1Freq = state.getRawParameterValue("Filter1 Freq")->load();
    settings.filter1Gain = state.getRawParameterValue("Filter1 Gain")->load();
    settings.filter1Quality = state.getRawParameterValue("Filter1 Quality")->load();

    settings.filter2Type = state.getRawParameterValue("Filter2 Type")->load();
    settings.filter2Freq = state.getRawParameterValue("Filter2 Freq")->load();
    settings.filter2Gain = state.getRawParameterValue("Filter2 Gain")->load();
    settings.filter2Quality = state.getRawParameterValue("Filter2 Quality")->load();

    settings.filter3Type = state.getRawParameterValue("Filter3 Type")->load();
    settings.filter3Freq = state.getRawParameterValue("Filter3 Freq")->load();
    settings.filter3Gain = state.getRawParameterValue("Filter3 Gain")->load();
    settings.filter3Quality = state.getRawParameterValue("Filter3 Quality")->load();

    settings.filter4Type = state.getRawParameterValue("Filter4 Type")->load();
    settings.filter4Freq = state.getRawParameterValue("Filter4 Freq")->load();
    settings.filter4Gain = state.getRawParameterValue("Filter4 Gain")->load();
    settings.filter4Quality = state.getRawParameterValue("Filter4 Quality")->load();


    settings.lowPassFreq = state.getRawParameterValue("LowPass Freq")->load();
    settings.lowPassSlope = state.getRawParameterValue("LowPass Slope")->load();

    settings.gain = state.getRawParameterValue("Gain")->load();

    settings.highPassOff = state.getRawParameterValue("HighPass Off")->load();
    settings.lowPassOff = state.getRawParameterValue("LowPass Off")->load();
    settings.filter1Off = state.getRawParameterValue("Filter1 Off")->load();
    settings.filter2Off = state.getRawParameterValue("Filter2 Off")->load();
    settings.filter3Off = state.getRawParameterValue("Filter3 Off")->load();
    settings.filter4Off = state.getRawParameterValue("Filter4 Off")->load();

    return settings;
}


//funkcje aktualizowania parametrów filtrów

void updateCoefficients(Filter::CoefficientsPtr& before, const Filter::CoefficientsPtr& after)
{
    *before = *after;
}

Filter::CoefficientsPtr createFilters1_4(const Settings& settings, double sampleRate, int filterID)
{
    switch (filterID)
    {
    case 0:
    {
        switch (settings.filter1Type)
        {
        case 0:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                settings.filter1Freq, settings.filter1Quality,
                juce::Decibels::decibelsToGain(settings.filter1Gain)); break;
        case 1:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,
                settings.filter1Freq, settings.filter1Quality,
                juce::Decibels::decibelsToGain(settings.filter1Gain)); break;
        case 2:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,
                settings.filter1Freq, settings.filter1Quality,
                juce::Decibels::decibelsToGain(settings.filter1Gain)); break;
        default:
            break;
        }break;
    }
    case 1:
    {
        switch (settings.filter2Type)
        {
        case 0:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                settings.filter2Freq, settings.filter2Quality,
                juce::Decibels::decibelsToGain(settings.filter2Gain)); break;
        case 1:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,
                settings.filter2Freq, settings.filter2Quality,
                juce::Decibels::decibelsToGain(settings.filter2Gain)); break;
        case 2:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,
                settings.filter2Freq, settings.filter2Quality,
                juce::Decibels::decibelsToGain(settings.filter2Gain)); break;
        default:
            break;
        }break;
    }
    case 2:
    {
        switch (settings.filter3Type)
        {
        case 0:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                settings.filter3Freq, settings.filter3Quality,
                juce::Decibels::decibelsToGain(settings.filter3Gain)); break;
        case 1:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,
                settings.filter3Freq, settings.filter3Quality,
                juce::Decibels::decibelsToGain(settings.filter3Gain)); break;
        case 2:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,
                settings.filter3Freq, settings.filter3Quality,
                juce::Decibels::decibelsToGain(settings.filter3Gain)); break;
        default:
            break;
        }break;
    }
    case 3:
    {
        switch (settings.filter4Type)
        {
        case 0:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                settings.filter4Freq, settings.filter4Quality,
                juce::Decibels::decibelsToGain(settings.filter4Gain)); break;
        case 1:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,
                settings.filter4Freq, settings.filter4Quality,
                juce::Decibels::decibelsToGain(settings.filter4Gain)); break;
        case 2:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,
                settings.filter4Freq, settings.filter4Quality,
                juce::Decibels::decibelsToGain(settings.filter4Gain)); break;
        default:
            break;
        }break;
    }
    default:
        break;
    }
    
    
}


void PJKParametricEQAudioProcessor::updateFilters1_4(const Settings& settings)
{
    auto filter1Coeff = createFilters1_4(settings, getSampleRate(), 0);
    auto filter2Coeff = createFilters1_4(settings, getSampleRate(), 1);
    auto filter3Coeff = createFilters1_4(settings, getSampleRate(), 2);
    auto filter4Coeff = createFilters1_4(settings, getSampleRate(), 3);

    //przyciski do w³¹czania filtrów
    leftChain.setBypassed<Positions::Filter1>(settings.filter1Off);
    rightChain.setBypassed<Positions::Filter1>(settings.filter1Off);
    leftChain.setBypassed<Positions::Filter2>(settings.filter2Off);
    rightChain.setBypassed<Positions::Filter2>(settings.filter2Off);
    leftChain.setBypassed<Positions::Filter3>(settings.filter3Off);
    rightChain.setBypassed<Positions::Filter3>(settings.filter3Off);
    leftChain.setBypassed<Positions::Filter4>(settings.filter4Off);
    rightChain.setBypassed<Positions::Filter4>(settings.filter4Off);

    updateCoefficients(leftChain.get<Positions::Filter1>().coefficients, filter1Coeff);
    updateCoefficients(rightChain.get<Positions::Filter1>().coefficients, filter1Coeff);
    updateCoefficients(leftChain.get<Positions::Filter2>().coefficients, filter2Coeff);
    updateCoefficients(rightChain.get<Positions::Filter2>().coefficients, filter2Coeff);
    updateCoefficients(leftChain.get<Positions::Filter3>().coefficients, filter3Coeff);
    updateCoefficients(rightChain.get<Positions::Filter3>().coefficients, filter3Coeff);
    updateCoefficients(leftChain.get<Positions::Filter4>().coefficients, filter4Coeff);
    updateCoefficients(rightChain.get<Positions::Filter4>().coefficients, filter4Coeff);
    
}
void PJKParametricEQAudioProcessor::updateHighPass(const Settings& settings)
{
    auto highPassCoeff = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(getSettings(state).highPassFreq, getSampleRate(), 2 * (getSettings(state).highPassSlope + 1));

    auto& leftHighPass = leftChain.get<Positions::HighPass>();
    auto& rightHighPass = rightChain.get<Positions::HighPass>();

    leftChain.setBypassed<Positions::HighPass>(settings.highPassOff);
    rightChain.setBypassed<Positions::HighPass>(settings.highPassOff);

    updatePassFilter(leftHighPass, highPassCoeff, getSettings(state).highPassSlope);
    updatePassFilter(rightHighPass, highPassCoeff, getSettings(state).highPassSlope);
    
}
void PJKParametricEQAudioProcessor::updateLowPass(const Settings& settings)
{
    auto lowPassCoeff = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(getSettings(state).lowPassFreq, getSampleRate(), 2 * (getSettings(state).lowPassSlope + 1));

    auto& leftLowPass = leftChain.get<Positions::LowPass>();
    auto& rightLowPass = rightChain.get<Positions::LowPass>();

    leftChain.setBypassed<Positions::LowPass>(settings.lowPassOff);
    rightChain.setBypassed<Positions::LowPass>(settings.lowPassOff);

    updatePassFilter(leftLowPass, lowPassCoeff, getSettings(state).lowPassSlope);
    updatePassFilter(rightLowPass, lowPassCoeff, getSettings(state).lowPassSlope);
}
void PJKParametricEQAudioProcessor::updateAllFilters()
{
    updateHighPass(getSettings(state));
    updateFilters1_4(getSettings(state));
    updateLowPass(getSettings(state));
}

//Layout parametrów
juce::AudioProcessorValueTreeState::ParameterLayout PJKParametricEQAudioProcessor::createParameterLayout()
{
    //tworzenie layoutu
    juce::AudioProcessorValueTreeState::ParameterLayout layout; 
    
    juce::StringArray filterTypes = {"Peak","Low Shelf","High Shelf"};
    juce::StringArray slopes = {"12 dB/oct","24 dB/oct","36 dB/oct","48 dB/oct"};
    
    //funkcja dodawania parametru (ID, nazwa, zakres(dó³, góra, krok, skala), domyœlna)
    //HighPass
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighPass Freq", "HighPass Freq", 
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    layout.add(std::make_unique <juce::AudioParameterChoice>("HighPass Slope", "HighPass Slope", slopes, 0));

    //LowPass
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowPass Freq", "LowPass Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowPass Slope", "LowPass Slope", slopes, 0));

    //Filtry 1-4
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter1 Freq", "Filter1 Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 100.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter1 Gain", "Filter1 Gain", juce::NormalisableRange<float>(-20.f, 20.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter1 Quality", "Filter1 Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Filter1 Type", "Filter1 Type", filterTypes, 0));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter2 Freq", "Filter2 Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 500.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter2 Gain", "Filter2 Gain", juce::NormalisableRange<float>(-20.f, 20.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter2 Quality", "Filter2 Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Filter2 Type", "Filter2 Type", filterTypes, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter3 Freq", "Filter3 Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter3 Gain", "Filter3 Gain", juce::NormalisableRange<float>(-20.f, 20.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter3 Quality", "Filter3 Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Filter3 Type", "Filter3 Type", filterTypes, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter4 Freq", "Filter4 Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 5000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter4 Gain", "Filter4 Gain", juce::NormalisableRange<float>(-20.f, 20.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Filter4 Quality", "Filter4 Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Filter4 Type", "Filter4 Type", filterTypes, 0));

    //Wzmocnienie wyjœciowe
    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", juce::NormalisableRange<float>(-40.f, 20.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>("HighPass Off", "HighPass Off", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("LowPass Off", "LowPass Off", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Filter1 Off", "Filter1 Off", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Filter2 Off", "Filter2 Off", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Filter3 Off", "Filter3 Off", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Filter4 Off", "Filter4 Off", false));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PJKParametricEQAudioProcessor();
}
