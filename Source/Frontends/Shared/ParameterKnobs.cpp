#include "ParameterKnobs.h"

using namespace Shared;

ParameterKnobs::ParameterKnobs()
{
}

void ParameterKnobs::addKnob(const KnobConfig& config)
{
    KnobControl control;
    
    // Create slider
    control.slider = std::make_unique<juce::Slider>(
        juce::Slider::RotaryHorizontalVerticalDrag, 
        juce::Slider::TextBoxBelow
    );
    control.slider->setRange(config.minValue, config.maxValue, config.interval);
    control.slider->setValue(config.defaultValue);
    if (config.suffix.isNotEmpty())
        control.slider->setTextValueSuffix(config.suffix);
    
    // Make text box smaller and more compact
    control.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    
    if (config.onChange)
    {
        control.slider->onValueChange = [slider = control.slider.get(), onChange = config.onChange]()
        {
            onChange(slider->getValue());
        };
    }
    
    // Create label with smaller font
    control.label = std::make_unique<juce::Label>("", config.label);
    control.label->setJustificationType(juce::Justification::centred);
    control.label->setFont(juce::FontOptions(11.0f));
    
    addAndMakeVisible(control.slider.get());
    addAndMakeVisible(control.label.get());
    
    knobs.push_back(std::move(control));
    
    resized();
}

double ParameterKnobs::getKnobValue(int index) const
{
    if (index >= 0 && index < static_cast<int>(knobs.size()))
        return knobs[index].slider->getValue();
    return 0.0;
}

void ParameterKnobs::setKnobValue(int index, double value, juce::NotificationType notification)
{
    if (index >= 0 && index < static_cast<int>(knobs.size()))
        knobs[index].slider->setValue(value, notification);
}

void ParameterKnobs::resized()
{
    if (knobs.empty())
        return;
    
    auto bounds = getLocalBounds();
    
    // Compact dimensions for labels and spacing
    const int knobLabelHeight = 12;    // Reduced from 15
    const int knobLabelSpacing = 2;    // Reduced from 5
    const int textBoxHeight = 16;      // Height of text box below knob
    
    // Preferred dimensions
    const int preferredKnobSize = 110;
    const int preferredKnobSpacing = 15;
    
    // Calculate how much space we actually have
    const int availableWidth = bounds.getWidth();
    const int numKnobs = static_cast<int>(knobs.size());
    
    // Calculate what fits
    int preferredTotalWidth = (preferredKnobSize * numKnobs) + (preferredKnobSpacing * (numKnobs - 1));
    
    int knobSize;
    int knobSpacing;
    
    if (preferredTotalWidth <= availableWidth)
    {
        // We have enough space, use preferred sizes
        knobSize = preferredKnobSize;
        knobSpacing = preferredKnobSpacing;
    }
    else
    {
        // Scale down to fit available width
        // Start with smaller spacing
        knobSpacing = juce::jmax(5, preferredKnobSpacing / 2);
        
        // Calculate knob size that fits
        int totalSpacing = knobSpacing * (numKnobs - 1);
        knobSize = (availableWidth - totalSpacing) / numKnobs;
        
        // Clamp to reasonable minimum
        knobSize = juce::jmax(70, knobSize);  // Increased minimum from 60 to 70
        
        // Recalculate spacing if knobs are now too small
        if (knobSize == 70)
        {
            totalSpacing = availableWidth - (knobSize * numKnobs);
            knobSpacing = (numKnobs > 1) ? totalSpacing / (numKnobs - 1) : 0;
        }
    }
    
    // Calculate total width and center knobs
    const int totalKnobWidth = (knobSize * numKnobs) + (knobSpacing * (numKnobs - 1));
    const int knobStartX = (bounds.getWidth() - totalKnobWidth) / 2;
    
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        int xPos = knobStartX + static_cast<int>(i) * (knobSize + knobSpacing);
        
        // Total area for this knob column
        auto knobArea = juce::Rectangle<int>(xPos, bounds.getY(), knobSize, bounds.getHeight());
        
        // Label at top (smaller)
        auto labelArea = knobArea.removeFromTop(knobLabelHeight);
        knobs[i].label->setBounds(labelArea);
        
        // Small spacing
        knobArea.removeFromTop(knobLabelSpacing);
        
        // Reserve space for text box at bottom
        knobArea.removeFromBottom(textBoxHeight);
        
        // Rest goes to the slider (which includes the knob widget and text box)
        // The slider will use its textBoxStyle settings to layout the text box
        knobs[i].slider->setBounds(knobArea.expanded(0, textBoxHeight));
    }
}

