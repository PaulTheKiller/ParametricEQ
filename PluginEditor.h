/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
//miernik RMS
struct LevelMeter : juce::Component
{
public:
    void paint(juce::Graphics& g) override;
    void setLevel(const float value) { level = value; }
private:
    float level = -60.f;
};

struct LookAndFeel :juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;
    
};
//wygl¹d pokrête³
struct KnobWithText :juce::Slider
{
    KnobWithText(juce::RangedAudioParameter& parameter, const juce::String& unitString):
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox), rap(&parameter), unit(unitString)
    {
        setLookAndFeel(&lookAndFeel);
    }

    ~KnobWithText()
    {
        setLookAndFeel(nullptr);
    }
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getKnobBounds() const;
    juce::String getString() const;
    juce::String getUnit() { return unit; }
private:
    LookAndFeel lookAndFeel; 
    juce::RangedAudioParameter* rap;
    juce::String unit;
};

//Charakterystyka
struct FrequencyResponse : juce::Component,
    juce::AudioProcessorParameter::Listener, 
    juce::Timer
{
    FrequencyResponse(PJKParametricEQAudioProcessor&);
    ~FrequencyResponse();
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {};
    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PJKParametricEQAudioProcessor& audioProcessor;
    //czy parametry siê zmieni³y
    juce::Atomic<bool> parametersValueChanged{ false };
    Chain chain;
    void updateFrequencyResponse();

    //siatka
    juce::Image background;
    juce::Rectangle<int> getResponseBounds();

};

//==============================================================================



class PJKParametricEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    PJKParametricEQAudioProcessorEditor (PJKParametricEQAudioProcessor&);
    ~PJKParametricEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //zegar do miernika
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PJKParametricEQAudioProcessor& audioProcessor;

    //pokrêt³a
    KnobWithText highPassFreqSlider,
        highPassSlopeSlider,
        lowPassFreqSlider,
        lowPassSlopeSlider,
        filter1FreqSlider,
        filter2FreqSlider,
        filter3FreqSlider,
        filter4FreqSlider,
        filter1GainSlider,
        filter2GainSlider,
        filter3GainSlider,
        filter4GainSlider,
        filter1QualitySlider,
        filter2QualitySlider,
        filter3QualitySlider,
        filter4QualitySlider;

    //suwaki
    juce::Slider filter1TypeSlider{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow },
        filter2TypeSlider{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow },
        filter3TypeSlider{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow },
        filter4TypeSlider{ juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };

    juce::Slider gainSlider{ juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };

    //Attachmenty do kontrolek
    juce::AudioProcessorValueTreeState::SliderAttachment highPassFreqSliderAttachment,
        highPassSlopeSliderAttachment,
        lowPassFreqSliderAttachment,
        lowPassSlopeSliderAttachment,
        filter1FreqSliderAttachment,
        filter2FreqSliderAttachment,
        filter3FreqSliderAttachment,
        filter4FreqSliderAttachment,
        filter1GainSliderAttachment,
        filter2GainSliderAttachment,
        filter3GainSliderAttachment,
        filter4GainSliderAttachment,
        filter1QualitySliderAttachment,
        filter2QualitySliderAttachment,
        filter3QualitySliderAttachment,
        filter4QualitySliderAttachment,
        filter1TypeSliderAttachment,
        filter2TypeSliderAttachment,
        filter3TypeSliderAttachment,
        filter4TypeSliderAttachment,
        gainSliderAttachment;

    //wy³¹czniki
    juce::TextButton highPassTextButton,
        lowPassTextButton,
        filter1TextButton,
        filter2TextButton,
        filter3TextButton,
        filter4TextButton;


    juce::AudioProcessorValueTreeState::ButtonAttachment highPassTextButtonAttachment,
        lowPassTextButtonAttachment,
        filter1TextButtonAttachment,
        filter2TextButtonAttachment,
        filter3TextButtonAttachment,
        filter4TextButtonAttachment;

    //Chain chain;
    FrequencyResponse frequencyResponse;
    LookAndFeel lookAndFeel;

    //miernik RMS lewy i prawy
    LevelMeter leftMeter, rightMeter;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PJKParametricEQAudioProcessorEditor)
};
