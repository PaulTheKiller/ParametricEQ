/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//malowanie wygl¹du sliderów
void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto b = Rectangle<float>(x, y, width, height);
    auto c = b.getCentre();

    g.setColour(Colours::black); //wnêtrze pokrêt³a 
    g.fillEllipse(b);

    g.setColour(Colours::white);//obwód pokrêt³a
    g.drawEllipse(b,1.f);

    Path p; //œcie¿ka - pozycja pokrêt³a

    Rectangle<float> r;
    r.setLeft(c.getX()-1); //gruboœæ œcie¿ki
    r.setRight(c.getX()+1);
    r.setTop(b.getY()); //wysokoœæ œcie¿ki
    r.setBottom(c.getY()); //œrodek pokrêt³a

    p.addRectangle(r); //dodanie prostok¹ta r do œcie¿ki

    jassert(rotaryStartAngle < rotaryEndAngle);
    auto angle = jmap(sliderPosProportional, rotaryStartAngle, rotaryEndAngle);
    p.applyTransform(AffineTransform().rotated(angle, c.getX(), c.getY()));

    g.fillPath(p);

    if (auto* knob = dynamic_cast<KnobWithText*>(&slider))
    {
        juce::String text;

        //napisy przed stringami
        if (knob->getUnit() == "Hz")
        {
            text << "f: ";
        }
        else if (knob->getUnit() == "")
        {
            text << "Q: ";
        }
        else if (knob->getUnit() == "dB")
        {
            text << "G: ";
        }
        
        else
        {
            text << "Sl:";
        }

        g.setFont(14);
        text << knob->getString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        Rectangle<float> t;
        t.setSize(strWidth + 4, 16);
        t.setCentre(b.getCentreX(), 14 + b.getHeight());
        g.setColour(Colours::black);
        g.fillRect(t);
        g.setColour(Colours::white);
        g.drawFittedText(text, t.toNearestInt(), Justification::centred, 1);

    }
}


juce::Rectangle<int> KnobWithText::getKnobBounds() const
{
    auto b = getLocalBounds();
    //œrednica pokrêt³a
    auto d = juce::jmin(b.getWidth(), b.getHeight()) - (14 * 2);
    juce::Rectangle<int> r;
    r.setSize(d, d);
    r.setCentre(b.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String KnobWithText::getString() const
{
    if (auto* choiceParameter = dynamic_cast<juce::AudioParameterChoice*>(rap))
        return choiceParameter->getCurrentChoiceName();
    
    juce::String s;

    if (auto* floatParameter = dynamic_cast<juce::AudioParameterFloat*>(rap))
    {
        float value = getValue();
        if (unit == "dB")
        {
            s = juce::String(value, 2);
            s << " ";
        }
        else if (unit == "")
        {
            s = juce::String(value, 2);
        }
        else if (value < 1000.f)
        {
            s = juce::String(value, 0);
            s << " ";
        }
        else if (value >= 1000.f)
        {
            //kHz
            value /= 1000.f;
            s = juce::String(value, 2);
            s << " k";
        }
        
    }
    if (unit.isNotEmpty())
    {
        s << unit;
    }
    return s;
}

void KnobWithText::paint(juce::Graphics& g)
{
    using namespace juce;

    auto knobBounds = getKnobBounds();
    
    auto startAngle = degreesToRadians(225.f); //180 + 45
    auto endAngle = degreesToRadians(495.f); //360 + 180 - 45

    getLookAndFeel().drawRotarySlider(g,
        knobBounds.getX(),
        knobBounds.getY(),
        knobBounds.getWidth(),
        knobBounds.getHeight(),
        jmap(getValue(), getRange().getStart(), getRange().getEnd(), 0.0, 1.0),
        startAngle,
        endAngle,
        *this);
}


//==============================================================================
PJKParametricEQAudioProcessorEditor::PJKParametricEQAudioProcessorEditor (PJKParametricEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    //Attachmenty do kontrolek w konstruktorze
    highPassFreqSlider(*audioProcessor.state.getParameter("HighPass Freq"), "Hz"),
    highPassSlopeSlider(*audioProcessor.state.getParameter("HighPass Slope"), "dB/oct"),

    lowPassFreqSlider(*audioProcessor.state.getParameter("LowPass Freq"), "Hz"),
    lowPassSlopeSlider(*audioProcessor.state.getParameter("LowPass Slope"), "dB/oct"),

    filter1FreqSlider(*audioProcessor.state.getParameter("Filter1 Freq"), "Hz"),
    filter2FreqSlider(*audioProcessor.state.getParameter("Filter2 Freq"), "Hz"),
    filter3FreqSlider(*audioProcessor.state.getParameter("Filter3 Freq"), "Hz"),
    filter4FreqSlider(*audioProcessor.state.getParameter("Filter4 Freq"), "Hz"),

    filter1GainSlider(*audioProcessor.state.getParameter("Filter1 Gain"), "dB"),
    filter2GainSlider(*audioProcessor.state.getParameter("Filter2 Gain"), "dB"),
    filter3GainSlider(*audioProcessor.state.getParameter("Filter3 Gain"), "dB"),
    filter4GainSlider(*audioProcessor.state.getParameter("Filter4 Gain"), "dB"),

    filter1QualitySlider(*audioProcessor.state.getParameter("Filter1 Quality"), ""),
    filter2QualitySlider(*audioProcessor.state.getParameter("Filter2 Quality"), ""),
    filter3QualitySlider(*audioProcessor.state.getParameter("Filter3 Quality"), ""),
    filter4QualitySlider(*audioProcessor.state.getParameter("Filter4 Quality"), ""),


    frequencyResponse(audioProcessor),
        
    highPassFreqSliderAttachment(audioProcessor.state, "HighPass Freq",highPassFreqSlider),
    highPassSlopeSliderAttachment(audioProcessor.state, "HighPass Slope", highPassSlopeSlider),

    lowPassFreqSliderAttachment(audioProcessor.state, "LowPass Freq", lowPassFreqSlider),
    lowPassSlopeSliderAttachment(audioProcessor.state, "LowPass Slope", lowPassSlopeSlider),

    filter1FreqSliderAttachment(audioProcessor.state, "Filter1 Freq", filter1FreqSlider),
    filter2FreqSliderAttachment(audioProcessor.state, "Filter2 Freq", filter2FreqSlider),
    filter3FreqSliderAttachment(audioProcessor.state, "Filter3 Freq", filter3FreqSlider),
    filter4FreqSliderAttachment(audioProcessor.state, "Filter4 Freq", filter4FreqSlider),

    filter1GainSliderAttachment(audioProcessor.state, "Filter1 Gain", filter1GainSlider),
    filter2GainSliderAttachment(audioProcessor.state, "Filter2 Gain", filter2GainSlider),
    filter3GainSliderAttachment(audioProcessor.state, "Filter3 Gain", filter3GainSlider),
    filter4GainSliderAttachment(audioProcessor.state, "Filter4 Gain", filter4GainSlider),

    filter1QualitySliderAttachment(audioProcessor.state, "Filter1 Quality", filter1QualitySlider),
    filter2QualitySliderAttachment(audioProcessor.state, "Filter2 Quality", filter2QualitySlider),
    filter3QualitySliderAttachment(audioProcessor.state, "Filter3 Quality", filter3QualitySlider),
    filter4QualitySliderAttachment(audioProcessor.state, "Filter4 Quality", filter4QualitySlider),

    filter1TypeSliderAttachment(audioProcessor.state, "Filter1 Type", filter1TypeSlider),
    filter2TypeSliderAttachment(audioProcessor.state, "Filter2 Type", filter2TypeSlider),
    filter3TypeSliderAttachment(audioProcessor.state, "Filter3 Type", filter3TypeSlider),
    filter4TypeSliderAttachment(audioProcessor.state, "Filter4 Type", filter4TypeSlider),
    
    gainSliderAttachment(audioProcessor.state, "Gain", gainSlider),

    highPassTextButtonAttachment(audioProcessor.state, "HighPass Off", highPassTextButton),
    lowPassTextButtonAttachment(audioProcessor.state, "LowPass Off", lowPassTextButton),
    filter1TextButtonAttachment(audioProcessor.state, "Filter1 Off", filter1TextButton),
    filter2TextButtonAttachment(audioProcessor.state, "Filter2 Off", filter2TextButton),
    filter3TextButtonAttachment(audioProcessor.state, "Filter3 Off", filter3TextButton),
    filter4TextButtonAttachment(audioProcessor.state, "Filter4 Off", filter4TextButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    //Dodawanie widocznych kontrolek
    addAndMakeVisible(highPassFreqSlider);
    addAndMakeVisible(highPassSlopeSlider);
    addAndMakeVisible(highPassTextButton);

    addAndMakeVisible(lowPassFreqSlider);
    addAndMakeVisible(lowPassSlopeSlider);
    addAndMakeVisible(lowPassTextButton);

    addAndMakeVisible(filter1FreqSlider);
    addAndMakeVisible(filter1GainSlider);
    addAndMakeVisible(filter1QualitySlider);
    addAndMakeVisible(filter1TextButton);
    addAndMakeVisible(filter1TypeSlider);

    addAndMakeVisible(filter2FreqSlider);
    addAndMakeVisible(filter2GainSlider);
    addAndMakeVisible(filter2QualitySlider);
    addAndMakeVisible(filter2TextButton);
    addAndMakeVisible(filter2TypeSlider);

    addAndMakeVisible(filter3FreqSlider);
    addAndMakeVisible(filter3GainSlider);
    addAndMakeVisible(filter3QualitySlider);
    addAndMakeVisible(filter3TextButton);
    addAndMakeVisible(filter3TypeSlider);

    addAndMakeVisible(filter4FreqSlider);
    addAndMakeVisible(filter4GainSlider);
    addAndMakeVisible(filter4QualitySlider);
    addAndMakeVisible(filter4TextButton);
    addAndMakeVisible(filter4TypeSlider);

    addAndMakeVisible(gainSlider);
    addAndMakeVisible(frequencyResponse);

    addAndMakeVisible(leftMeter);
    addAndMakeVisible(rightMeter);

    setSize (700, 500);

    startTimerHz(30);
}

PJKParametricEQAudioProcessorEditor::~PJKParametricEQAudioProcessorEditor()
{
   
}

//==============================================================================

void PJKParametricEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    //malowanie t³a komponentów
    g.fillAll(juce::Colours::black);
    auto b = getLocalBounds();
    auto gainBounds = b.removeFromRight(100);

    auto frequencyResponseBounds = b.removeFromTop(b.getHeight() * 0.5);

    frequencyResponse.setBounds(frequencyResponseBounds);

    auto highPassBounds = b.removeFromLeft(100);
    auto filter1Bounds = b.removeFromLeft(100);
    auto filter2Bounds = b.removeFromLeft(100);
    auto filter3Bounds = b.removeFromLeft(100);
    auto filter4Bounds = b.removeFromLeft(100);
    auto lowPassBounds = b.removeFromLeft(100);
    

    g.setColour(juce::Colour(49, 37, 9));
    g.fillRect(gainBounds);

    g.setColour(juce::Colour(14, 59, 67));
    g.fillRect(highPassBounds);
    
    g.setColour(juce::Colour(53, 114, 102));
    g.fillRect(filter1Bounds);

    g.setColour(juce::Colour(101, 83, 47));
    g.fillRect(filter2Bounds);

    g.setColour(juce::Colour(53, 114, 102));
    g.fillRect(filter3Bounds);

    g.setColour(juce::Colour(101, 83, 47));
    g.fillRect(filter4Bounds);

    g.setColour(juce::Colour(14, 59, 67));
    g.fillRect(lowPassBounds);
    

    //napisy, slidery type i gain
    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::Slider::backgroundColourId, juce::Colours::black);
    getLookAndFeel().setColour(juce::Slider::trackColourId, juce::Colours::white);
    
    getLookAndFeel().setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    getLookAndFeel().setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    getLookAndFeel().setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
   
    
    gainSlider.setTextValueSuffix("dB");
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 25);
   

    filter1TypeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    filter2TypeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    filter3TypeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    filter4TypeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);

    //wygl¹d buttonów 

    highPassTextButton.setButtonText("High-Pass");
    highPassTextButton.setClickingTogglesState(true);
    highPassTextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    highPassTextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    highPassTextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    highPassTextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(14, 59, 67));

    filter1TextButton.setButtonText("Filter 1");
    filter1TextButton.setClickingTogglesState(true);
    filter1TextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    filter1TextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    filter1TextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    filter1TextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(53, 114, 102));
    
    filter2TextButton.setButtonText("Filter 2");
    filter2TextButton.setClickingTogglesState(true);
    filter2TextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    filter2TextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    filter2TextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    filter2TextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(101, 83, 47));
    
    filter3TextButton.setButtonText("Filter 3");
    filter3TextButton.setClickingTogglesState(true);
    filter3TextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    filter3TextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    filter3TextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    filter3TextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(53, 114, 102));
    

    filter4TextButton.setButtonText("Filter 4");
    filter4TextButton.setClickingTogglesState(true);
    filter4TextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    filter4TextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    filter4TextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    filter4TextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(101, 83, 47));

    lowPassTextButton.setButtonText("Low-Pass");
    lowPassTextButton.setClickingTogglesState(true);
    lowPassTextButton.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::lightgrey);
    lowPassTextButton.setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colours::white);
    lowPassTextButton.setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    lowPassTextButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colour(14, 59, 67));

    
}

//granice obiektów
void PJKParametricEQAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();
    auto gainBounds = b.removeFromRight(100);
    auto frequencyResponseBounds = b.removeFromTop(b.getHeight() * 0.5);

    frequencyResponse.setBounds(frequencyResponseBounds);

    auto highPassBounds = b.removeFromLeft(100);
    auto filter1Bounds = b.removeFromLeft(100);
    auto filter2Bounds = b.removeFromLeft(100);
    auto filter3Bounds = b.removeFromLeft(100);
    auto filter4Bounds = b.removeFromLeft(100);
    auto lowPassBounds = b.removeFromLeft(100);

    //zmiana granic buttonów 

    highPassTextButton.setBounds(highPassBounds.removeFromTop(20));
    highPassFreqSlider.setBounds(highPassBounds.removeFromTop(highPassBounds.getHeight()*0.5));
    highPassSlopeSlider.setBounds(highPassBounds);

    filter1TextButton.setBounds(filter1Bounds.removeFromTop(20));
    filter1FreqSlider.setBounds(filter1Bounds.removeFromTop(66));
    filter1GainSlider.setBounds(filter1Bounds.removeFromTop(66));
    filter1QualitySlider.setBounds(filter1Bounds.removeFromTop(66));
    filter1TypeSlider.setBounds(filter1Bounds);

    filter2TextButton.setBounds(filter2Bounds.removeFromTop(20));
    filter2FreqSlider.setBounds(filter2Bounds.removeFromTop(66));
    filter2GainSlider.setBounds(filter2Bounds.removeFromTop(66));
    filter2QualitySlider.setBounds(filter2Bounds.removeFromTop(66));
    filter2TypeSlider.setBounds(filter2Bounds);

    filter3TextButton.setBounds(filter3Bounds.removeFromTop(20));
    filter3FreqSlider.setBounds(filter3Bounds.removeFromTop(66));
    filter3GainSlider.setBounds(filter3Bounds.removeFromTop(66));
    filter3QualitySlider.setBounds(filter3Bounds.removeFromTop(66));
    filter3TypeSlider.setBounds(filter3Bounds);

    filter4TextButton.setBounds(filter4Bounds.removeFromTop(20));
    filter4FreqSlider.setBounds(filter4Bounds.removeFromTop(66));
    filter4GainSlider.setBounds(filter4Bounds.removeFromTop(66));
    filter4QualitySlider.setBounds(filter4Bounds.removeFromTop(66));
    filter4TypeSlider.setBounds(filter4Bounds);

    lowPassTextButton.setBounds(lowPassBounds.removeFromTop(20));
    lowPassFreqSlider.setBounds(lowPassBounds.removeFromTop(lowPassBounds.getHeight() * 0.5));
    lowPassSlopeSlider.setBounds(lowPassBounds);

    gainSlider.setBounds(gainBounds.removeFromLeft(gainBounds.getWidth() * 0.5));    
    
    leftMeter.setBounds(gainBounds.removeFromLeft(gainBounds.getWidth() * 0.5));
    rightMeter.setBounds(gainBounds);

   
}
//timer callback do miernika
void PJKParametricEQAudioProcessorEditor::timerCallback()
{
    leftMeter.setLevel(audioProcessor.getRMSValue(0));
    rightMeter.setLevel(audioProcessor.getRMSValue(1));

    leftMeter.repaint();
    rightMeter.repaint();
}


//charakterystyka
FrequencyResponse::FrequencyResponse(PJKParametricEQAudioProcessor& p) :audioProcessor(p)
{
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters)
    {
        parameter->addListener(this);
    }
    updateFrequencyResponse();
    startTimerHz(60);
}


FrequencyResponse::~FrequencyResponse()
{
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters)
    {
        parameter->removeListener(this);
    }
}

void FrequencyResponse::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersValueChanged.set(true);
}
//aktualizacja parametrów przez krzyw¹
void FrequencyResponse::timerCallback()
{
    if (parametersValueChanged.compareAndSetBool(false, true))
    {
        updateFrequencyResponse();
        repaint();
    }

    
}

void FrequencyResponse::updateFrequencyResponse()
{
    auto settings = getSettings(audioProcessor.state);

    //buttony w editorze
    chain.setBypassed<Positions::HighPass>(settings.highPassOff);
    chain.setBypassed<Positions::LowPass>(settings.lowPassOff);
    chain.setBypassed<Positions::Filter1>(settings.filter1Off);
    chain.setBypassed<Positions::Filter2>(settings.filter2Off);
    chain.setBypassed<Positions::Filter3>(settings.filter3Off);
    chain.setBypassed<Positions::Filter4>(settings.filter4Off);

    auto highPassCoeff = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(settings.highPassFreq, audioProcessor.getSampleRate(), 2 * (settings.highPassSlope + 1));

    auto lowPassCoeff = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(settings.lowPassFreq, audioProcessor.getSampleRate(), 2 * (settings.lowPassSlope + 1));

    updatePassFilter(chain.get<Positions::HighPass>(), highPassCoeff, settings.highPassSlope);
    updatePassFilter(chain.get<Positions::LowPass>(), lowPassCoeff, settings.lowPassSlope);

    auto filter1Coeff = createFilters1_4(settings, audioProcessor.getSampleRate(), 0);
    auto filter2Coeff = createFilters1_4(settings, audioProcessor.getSampleRate(), 1);
    auto filter3Coeff = createFilters1_4(settings, audioProcessor.getSampleRate(), 2);
    auto filter4Coeff = createFilters1_4(settings, audioProcessor.getSampleRate(), 3);
    updateCoefficients(chain.get<Positions::Filter1>().coefficients, filter1Coeff);
    updateCoefficients(chain.get<Positions::Filter1>().coefficients, filter1Coeff);
    updateCoefficients(chain.get<Positions::Filter2>().coefficients, filter2Coeff);
    updateCoefficients(chain.get<Positions::Filter2>().coefficients, filter2Coeff);
    updateCoefficients(chain.get<Positions::Filter3>().coefficients, filter3Coeff);
    updateCoefficients(chain.get<Positions::Filter3>().coefficients, filter3Coeff);
    updateCoefficients(chain.get<Positions::Filter4>().coefficients, filter4Coeff);
    updateCoefficients(chain.get<Positions::Filter4>().coefficients, filter4Coeff);

}
//siatka
void FrequencyResponse::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);

    auto b = getResponseBounds();

    //siatka - czêstotliwoœci

    Array<float> frequencies
    {
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000, 20000
    };

    g.setColour(Colours::lightgrey);
    
    Array<float> xs;  
    for (int i = 0; i < frequencies.size(); ++i)
    {
        //skala logarytmiczna siatki, wybrane f z tablicy
        auto logX = mapFromLog10(frequencies[i], 20.f, 20000.f);
        xs.add(b.getX() + b.getWidth() * logX);


        //napisy do czêstotliwoœci
        auto value = frequencies[i];
        String freqLabel;
        if (frequencies[i] < 1000.f)
        {
            freqLabel << value;
            freqLabel << " ";
        }

        else if (frequencies[i] >= 1000.f)
        {
            //kHz
            value /= 1000.f;
            freqLabel << value;
            freqLabel << " k";
        }
        freqLabel << "Hz";
        Rectangle<int> r;

        g.setColour(Colour(208, 229, 98));
        g.setFont(10);

        r.setSize(g.getCurrentFont().getStringWidth(freqLabel), 10);
        r.setCentre(xs[i], b.getBottom() + 5);

        g.drawFittedText(freqLabel, r, Justification::centred, 1);

    }

    for (auto x : xs)
    {
        //dla ka¿dego x z przypisan¹ f, linia od góry do do³u
        g.setColour(Colours::lightgrey);
        g.drawVerticalLine(x, b.getY(), b.getBottom());
    }

    //siatka gain
    Array<float> gains
    {
        -18, -12, -6, 0, 6, 12, 18
    };
    
    for (auto gain : gains)
    {
        auto y = jmap(gain, -20.f, 20.f, float(b.getBottom()), float(b.getY()));
        if(gain==0.f)
            g.setColour(Colours::orange.brighter());
        else
            g.setColour(Colours::lightgrey);
        g.drawHorizontalLine(y, b.getX(), b.getRight());

        String gainLabel;
        if (gain > 0)
        {
            gainLabel << "+";
        }
        gainLabel << gain;

        g.setColour(Colour(208, 229, 98));
        g.setFont(10);

        Rectangle<int> r;

        r.setSize(g.getCurrentFont().getStringWidth(gainLabel), 10);
        //gainy po lewej
        r.setX(getWidth() - g.getCurrentFont().getStringWidth(gainLabel)-2);
        r.setCentre(r.getCentreX(), y);

        g.drawFittedText(gainLabel, r, Justification::centred, 1);

        //gainy po prawej
        r.setX(1);
        g.drawFittedText(gainLabel, r, Justification::centred, 1);

    }
    
}


//malowanie krzywej
void FrequencyResponse::paint(juce::Graphics& g)
{
    //Rysowanie siatki
    g.fillAll(juce::Colours::black);
    g.drawImage(background, getLocalBounds().toFloat());

    auto b = getLocalBounds();
    auto gainBounds = b.removeFromRight(100);

    auto frequencyResponseBounds = getResponseBounds();

    auto width = frequencyResponseBounds.getWidth();

    auto& highpass = chain.get<Positions::HighPass>();
    auto& filter1 = chain.get<Positions::Filter1>();
    auto& filter2 = chain.get<Positions::Filter2>();
    auto& filter3 = chain.get<Positions::Filter3>();
    auto& filter4 = chain.get<Positions::Filter4>();
    auto& lowpass = chain.get<Positions::LowPass>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> amplitudeValues;
    amplitudeValues.resize(width);

    for (int i = 0; i < width; ++i)
    {
        double amplitude = 1.f;
        auto frequency = juce::mapToLog10(double(i) / double(width), 20.0, 20000.0);

            if (!chain.isBypassed<Positions::Filter1>())
                amplitude *= filter1.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!chain.isBypassed<Positions::Filter2>())
                amplitude *= filter2.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!chain.isBypassed<Positions::Filter3>())
                amplitude *= filter3.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!chain.isBypassed<Positions::Filter4>())
                amplitude *= filter4.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        
        if (!chain.isBypassed<Positions::HighPass>())
        {
            if (!highpass.isBypassed<0>())
                amplitude *= highpass.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!highpass.isBypassed<1>())
                amplitude *= highpass.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!highpass.isBypassed<2>())
                amplitude *= highpass.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!highpass.isBypassed<3>())
                amplitude *= highpass.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!chain.isBypassed<Positions::LowPass>())
        {
            if (!lowpass.isBypassed<0>())
                amplitude *= lowpass.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!lowpass.isBypassed<1>())
                amplitude *= lowpass.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!lowpass.isBypassed<2>())
                amplitude *= lowpass.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
            if (!lowpass.isBypassed<3>())
                amplitude *= lowpass.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        amplitudeValues[i] = juce::Decibels::gainToDecibels(amplitude);

    }

    juce::Path frequencyResponse;
    const double bottom = frequencyResponseBounds.getBottom();
    const double top = frequencyResponseBounds.getY();

    frequencyResponse.startNewSubPath(frequencyResponseBounds.getX(), juce::jmap(amplitudeValues.front(), -20.0, 20.0, bottom, top));

    for (int i = 0; i < amplitudeValues.size(); ++i)
    {
        frequencyResponse.lineTo(frequencyResponseBounds.getX() + i, juce::jmap(amplitudeValues[i], -20.0, 20.0, bottom, top));
       
    }


    g.setColour(juce::Colours::white);
    g.strokePath(frequencyResponse, juce::PathStrokeType(2.f));

}

juce::Rectangle<int> FrequencyResponse::getResponseBounds()
{
    auto b = getLocalBounds();
    b.removeFromTop(2);
    b.removeFromBottom(12);
    b.removeFromLeft(20);
    b.removeFromRight(20);

    return b;
}

//Malowanie miernika
void LevelMeter::paint(juce::Graphics& g)
{
    using namespace juce;
    auto b = getLocalBounds().toFloat();
    auto t = getLocalBounds().toFloat();
    g.setColour(Colours::darkgrey);
    g.fillRoundedRectangle(b, 4.f);

    auto height = jmap(level, -60.f, 0.f, 0.f, b.getHeight());
    auto gradient = ColourGradient{
        Colours::green,
        b.getBottomLeft(),
        Colours::red,
        b.getTopLeft(),
        false
    };

    gradient.addColour(0.5, Colours::yellow);

    if (level >= 0)
    {
        g.setColour(Colours::red);
    }
    else
    {
        g.setGradientFill(gradient);
    }
    g.fillRoundedRectangle(b.removeFromBottom(height), 4.f);

    Array<float> gains
    {
        -48, -36, -24, -18, -12, -9, -6, -3,
    };

    for (auto gain : gains)
    {
        auto y = jmap(gain, -60.f, 0.f, float(t.getBottom()), float(t.getY()));

        String meterText;
        meterText << gain;

        g.setColour(Colours::black);
        g.setFont(13); 

        Rectangle<int> r;

        r.setSize(g.getCurrentFont().getStringWidth(meterText), 10);
        r.setCentre(r.getCentreX(), y);

        r.setX(getWidth() - g.getCurrentFont().getStringWidth(meterText) - 5);
        g.drawFittedText(meterText, r, Justification::centred, 1);

    }
}