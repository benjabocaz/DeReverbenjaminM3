#include "PluginEditor.h"
#include <array>
#include <cmath>

void VintageRackLAF::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, const float rotaryStartAngle,
                                      const float rotaryEndAngle, juce::Slider& slider)
{
    const bool isModeSelector = (slider.getName() == "MODE_KNOB");
    const float radius = juce::jmin(width / 2, height / 2) - 9.0f;
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const int numTicks = isModeSelector ? 4 : 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(numTicks - 1);
        const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float r1 = radius + 3.5f;
        const float r2 = radius + (isModeSelector ? 8.5f : (i % 5 == 0 ? 8.0f : 5.0f));

        const bool active = isModeSelector ? (std::abs(t - sliderPos) < 0.15f) : (t <= sliderPos + 0.001f);
        g.setColour(active ? juce::Colour(0xFFF2BE6B) : juce::Colour(0xFF5D564C));
        g.drawLine(cx + std::sin(a) * r1, cy - std::cos(a) * r1,
                   cx + std::sin(a) * r2, cy - std::cos(a) * r2, isModeSelector ? 2.4f : 1.5f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.fillEllipse(cx - radius + 2.5f, cy - radius + 5.0f, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient skirtGrad(juce::Colour(0xFF4A484F), cx - radius * 0.6f, cy - radius * 0.6f,
                                   juce::Colour(0xFF121215), cx + radius * 0.6f, cy + radius * 0.6f, false);
    g.setGradientFill(skirtGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    const int flutes = 12;
    const float fluteR = radius * 0.88f;
    for (int i = 0; i < flutes; ++i)
    {
        const float a = angle + (static_cast<float>(i) / flutes) * juce::MathConstants<float>::twoPi;
        const float fx = cx + std::sin(a) * fluteR;
        const float fy = cy - std::cos(a) * fluteR;
        g.setColour(juce::Colour(0xFF0E0E10).withAlpha(0.75f));
        g.fillEllipse(fx - 2.4f, fy - 2.4f, 4.8f, 4.8f);
    }

    const float capR = radius * 0.74f;
    juce::ColourGradient capGrad(isModeSelector ? juce::Colour(0xFF5A3E24) : juce::Colour(0xFF34333A),
                                 cx - capR * 0.5f, cy - capR * 0.5f,
                                 isModeSelector ? juce::Colour(0xFF291A0D) : juce::Colour(0xFF17171B),
                                 cx + capR * 0.6f, cy + capR * 0.6f, true);
    g.setGradientFill(capGrad);
    g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    g.setColour(isModeSelector ? juce::Colour(0xFFE6B972) : juce::Colour(0xFF756C5E));
    g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.3f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.8f, -capR + 2.5f, 3.6f, capR * 0.68f, 1.5f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
    g.setColour(juce::Colour(0xFFF6D28B));
    g.fillPath(pointer);
}

DeReverbenjaminM3Editor::DeReverbenjaminM3Editor(DeReverbenjaminM3Processor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&vintageLAF);

    modeSlider.setName("MODE_KNOB");
    modeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    modeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 16);
    modeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFF2BE6B));
    modeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xFF3E352B));
    modeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF101012));
    addAndMakeVisible(modeSlider);

    modeLabel.setText("SOURCE", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE6B972));
    modeLabel.setFont(10.5f);
    addAndMakeVisible(modeLabel);

    setupKnob(roomAmountSlider, roomAmountLabel, "DE-ROOM");
    setupKnob(focusSlider, focusLabel, "AMBIANCE");
    setupKnob(decaySlider, decayLabel, "DECAY");
    setupKnob(smoothSlider, smoothLabel, "SMOOTH");

    setupKnob(tailAmountSlider, tailAmountLabel, "DE-TAIL");
    setupKnob(sensSlider, sensLabel, "SENSITIVITY");
    setupKnob(releaseSlider, releaseLabel, "RELEASE", " ms");
    setupKnob(outputSlider, outputLabel, "OUTPUT", " dB");

    setupKnob(res1Slider, res1Label, "PUNCH");
    setupKnob(res2Slider, res2Label, "THUMP");
    setupKnob(res3Slider, res3Label, "SNAP");
    setupKnob(mixSlider, mixLabel, "MIX");

    modeAtt       = std::make_unique<Attachment>(audioProcessor.apvts, "mode", modeSlider);
    roomAmountAtt = std::make_unique<Attachment>(audioProcessor.apvts, "roomAmount", roomAmountSlider);
    focusAtt      = std::make_unique<Attachment>(audioProcessor.apvts, "focus", focusSlider);
    decayAtt      = std::make_unique<Attachment>(audioProcessor.apvts, "decay", decaySlider);
    smoothAtt     = std::make_unique<Attachment>(audioProcessor.apvts, "smooth", smoothSlider);

    tailAmountAtt = std::make_unique<Attachment>(audioProcessor.apvts, "tailAmount", tailAmountSlider);
    sensAtt       = std::make_unique<Attachment>(audioProcessor.apvts, "sensitivity", sensSlider);
    releaseAtt    = std::make_unique<Attachment>(audioProcessor.apvts, "release", releaseSlider);
    outputAtt     = std::make_unique<Attachment>(audioProcessor.apvts, "output", outputSlider);

    res1Att       = std::make_unique<Attachment>(audioProcessor.apvts, "res1", res1Slider);
    res2Att       = std::make_unique<Attachment>(audioProcessor.apvts, "res2", res2Slider);
    res3Att       = std::make_unique<Attachment>(audioProcessor.apvts, "res3", res3Slider);
    mixAtt        = std::make_unique<Attachment>(audioProcessor.apvts, "mix", mixSlider);

    updateStage3Labels(static_cast<int>(audioProcessor.apvts.getRawParameterValue("mode")->load()));

    setSize(684, 536);
    startTimerHz(30);
}

DeReverbenjaminM3Editor::~DeReverbenjaminM3Editor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void DeReverbenjaminM3Editor::updateStage3Labels(int mode)
{
    if (mode == lastDisplayedMode)
        return;

    lastDisplayedMode = mode;

    if (mode == 0)
    {
        res1Label.setText("PUNCH", juce::dontSendNotification);
        res2Label.setText("THUMP", juce::dontSendNotification);
        res3Label.setText("SNAP",  juce::dontSendNotification);
    }
    else if (mode == 1)
    {
        res1Label.setText("BODY",     juce::dontSendNotification);
        res2Label.setText("PRESENCE", juce::dontSendNotification);
        res3Label.setText("SILK",     juce::dontSendNotification);
    }
    else if (mode == 2)
    {
        res1Label.setText("ATTACK", juce::dontSendNotification);
        res2Label.setText("WARMTH", juce::dontSendNotification);
        res3Label.setText("BITE",   juce::dontSendNotification);
    }
    else
    {
        res1Label.setText("HAMMER",   juce::dontSendNotification);
        res2Label.setText("RICHNESS", juce::dontSendNotification);
        res3Label.setText("SHIMMER",  juce::dontSendNotification);
    }

    repaint(40, 384, 604, 28);
}

void DeReverbenjaminM3Editor::setupKnob(juce::Slider& slider, juce::Label& label,
                                        const juce::String& name, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 16);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFE5D7C0));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xFF302C27));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF101012));
    if (suffix.isNotEmpty())
        slider.setTextValueSuffix(suffix);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFD9AA63));
    label.setFont(11.0f);
    addAndMakeVisible(label);
}

void DeReverbenjaminM3Editor::timerCallback()
{
    const int currentMode = static_cast<int>(audioProcessor.apvts.getRawParameterValue("mode")->load());
    updateStage3Labels(currentMode);

    const float target = audioProcessor.currentGainReduction.load();
    smoothedGr = 0.72f * smoothedGr + 0.28f * target;
    repaint(430, 14, 212, 80);
}

void DeReverbenjaminM3Editor::drawWoodPanel(juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::ColourGradient woodGrad(juce::Colour(0xFF5B361D), area.getX(), area.getY(),
                                  juce::Colour(0xFF2D180A), area.getRight(), area.getY(), false);
    g.setGradientFill(woodGrad);
    g.fillRect(area);

    g.setColour(juce::Colour(0xFF1D0E05).withAlpha(0.32f));
    for (float x = area.getX() + 3.0f; x < area.getRight(); x += 4.5f)
        g.drawLine(x, area.getY(), x + 1.5f, area.getBottom(), 1.1f);

    g.setColour(juce::Colour(0xFF7B4D2B).withAlpha(0.35f));
    g.drawLine(area.getX() + 1.0f, area.getY(), area.getX() + 1.0f, area.getBottom(), 1.0f);
}

void DeReverbenjaminM3Editor::drawScrew(juce::Graphics& g, float cx, float cy) const
{
    const float r = 6.5f;
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(cx - r + 1.0f, cy - r + 1.5f, r * 2.0f, r * 2.0f);

    juce::ColourGradient screwGrad(juce::Colour(0xFF6B6970), cx - r * 0.5f, cy - r * 0.5f,
                                   juce::Colour(0xFF252429), cx + r * 0.5f, cy + r * 0.5f, true);
    g.setGradientFill(screwGrad);
    g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

    g.setColour(juce::Colour(0xFF141416));
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.1f);
    g.drawLine(cx - 4.0f, cy - 2.3f, cx + 4.0f, cy + 2.3f, 1.5f);
}

void DeReverbenjaminM3Editor::drawLed(juce::Graphics& g, float cx, float cy, bool isOn, juce::Colour colour) const
{
    if (isOn)
    {
        g.setColour(colour.withAlpha(0.28f));
        g.fillEllipse(cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
    }

    g.setColour(juce::Colour(0xFF111113));
    g.fillEllipse(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);

    g.setColour(isOn ? colour : colour.darker(2.4f));
    g.fillEllipse(cx - 3.2f, cy - 3.2f, 6.4f, 6.4f);

    g.setColour(juce::Colours::white.withAlpha(isOn ? 0.55f : 0.12f));
    g.fillEllipse(cx - 1.6f, cy - 2.2f, 2.0f, 2.0f);
}

void DeReverbenjaminM3Editor::drawVuMeter(juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setColour(juce::Colour(0xFF0A0A0C));
    g.fillRoundedRectangle(area.expanded(4.0f), 7.0f);
    g.setColour(juce::Colour(0xFF4D463D));
    g.drawRoundedRectangle(area.expanded(4.0f), 7.0f, 1.2f);

    juce::ColourGradient vuGrad(juce::Colour(0xFFFFF0C8), area.getCentreX(), area.getY(),
                                juce::Colour(0xFFD6B067), area.getCentreX(), area.getBottom(), false);
    g.setGradientFill(vuGrad);
    g.fillRoundedRectangle(area, 4.0f);

    const float pivotX = area.getCentreX();
    const float pivotY = area.getBottom() + 18.0f;
    const float arcRadius = area.getHeight() + 6.0f;

    g.setColour(juce::Colour(0xFF2B2114));
    g.setFont(9.5f);
    g.drawText("GAIN REDUCTION (dB)", area.withTrimmedTop(4.0f).withHeight(14.0f), juce::Justification::centred);

    const float startAngle = -0.68f;
    const float endAngle   =  0.68f;

    for (int i = 0; i <= 6; ++i)
    {
        const float t = static_cast<float>(i) / 6.0f;
        const float a = startAngle + t * (endAngle - startAngle);
        g.setColour(i < 2 ? juce::Colour(0xFFB52B1E) : juce::Colour(0xFF2B2114));
        g.drawLine(pivotX + std::sin(a) * (arcRadius - 8.5f), pivotY - std::cos(a) * (arcRadius - 8.5f),
                   pivotX + std::sin(a) * arcRadius,          pivotY - std::cos(a) * arcRadius, 1.5f);
    }

    const float grDb = juce::jlimit(-18.0f, 0.0f, juce::Decibels::gainToDecibels(smoothedGr, -18.0f));
    const float norm = (grDb + 18.0f) / 18.0f;
    const float needleAngle = startAngle + norm * (endAngle - startAngle);

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());
    g.setColour(juce::Colour(0xFF1A140E));
    g.drawLine(pivotX, pivotY,
               pivotX + std::sin(needleAngle) * (arcRadius + 2.0f),
               pivotY - std::cos(needleAngle) * (arcRadius + 2.0f), 2.1f);

    juce::ColourGradient glassGrad(juce::Colours::white.withAlpha(0.18f), area.getX(), area.getY(),
                                   juce::Colours::transparentWhite, area.getX(), area.getCentreY(), false);
    g.setGradientFill(glassGrad);
    g.fillRect(area.withHeight(area.getHeight() * 0.45f));
    g.restoreState();

    g.setColour(juce::Colour(0xFF4A443C));
    g.drawRoundedRectangle(area, 4.0f, 1.5f);
}

void DeReverbenjaminM3Editor::paint(juce::Graphics& g)
{
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const float woodW = 22.0f;

    drawWoodPanel(g, { 0.0f, 0.0f, woodW, h });
    drawWoodPanel(g, { w - woodW, 0.0f, woodW, h });

    juce::Rectangle<float> chassis(woodW, 0.0f, w - woodW * 2.0f, h);
    juce::ColourGradient bgGrad(juce::Colour(0xFF2B2B31), 0.0f, 0.0f,
                                juce::Colour(0xFF151519), 0.0f, h, false);
    g.setGradientFill(bgGrad);
    g.fillRect(chassis);

    g.setColour(juce::Colours::white.withAlpha(0.018f));
    for (float y = 2.0f; y < h; y += 3.0f)
        g.drawHorizontalLine(static_cast<int>(y), woodW, w - woodW);

    g.setColour(juce::Colour(0xFF47423B));
    g.drawRect(chassis.reduced(1.0f), 1.5f);

    drawScrew(g, woodW + 14.0f, 16.0f);
    drawScrew(g, w - woodW - 14.0f, 16.0f);
    drawScrew(g, woodW + 14.0f, h - 16.0f);
    drawScrew(g, w - woodW - 14.0f, h - 16.0f);

    g.setColour(juce::Colour(0xFFF0E4D0));
    g.setFont(23.0f);
    g.drawText("DeReverbenjamin M3", static_cast<int>(woodW) + 32, 22, 265, 28, juce::Justification::left);

    g.setColour(juce::Colour(0xFFE0B063));
    g.setFont(9.6f);
    g.drawText("ANALOG RACK // 4096-FFT // 2X RESYNTH", static_cast<int>(woodW) + 34, 52, 265, 16, juce::Justification::left);

    drawVuMeter(g, juce::Rectangle<float>(woodW + 416.0f, 18.0f, 190.0f, 64.0f));

    auto drawPanel = [&](juce::Rectangle<int> bounds, const juce::String& title, bool ledOn)
    {
        g.setColour(juce::Colour(0xFF18181C));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(juce::Colour(0xFF3E3830));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.2f);

        drawLed(g, static_cast<float>(bounds.getX() + 16), static_cast<float>(bounds.getY() + 14),
                ledOn, juce::Colour(0xFFFFB44C));

        g.setColour(juce::Colour(0xFFB5A690));
        g.setFont(10.5f);
        g.drawText(title, bounds.getX() + 28, bounds.getY() + 6, bounds.getWidth() - 40, 16, juce::Justification::left);
    };

    const juce::String modeNames[4] = {
        "DRUMS RESYNTH (2X ANALOG)",
        "VOCAL STUDIO RESYNTH (2X ANALOG)",
        "GTR/BASS RESYNTH (2X ANALOG)",
        "KEYS RESYNTH (2X ANALOG)"
    };
    const int m = juce::jlimit(0, 3, lastDisplayedMode < 0 ? 0 : lastDisplayedMode);
    const int px = static_cast<int>(woodW) + 24;

    drawPanel({ px, 102, 592, 130 }, "STAGE 1  //  ULTRA-HD 4096-FFT DE-ROOM", true);
    drawPanel({ px, 244, 592, 130 }, "STAGE 2  //  TAIL DE-REVERB & OUTPUT", true);
    drawPanel({ px, 386, 592, 130 }, "STAGE 3  //  " + modeNames[m], true);
}

void DeReverbenjaminM3Editor::resized()
{
    const int offsetX = 22;
    modeLabel.setBounds(offsetX + 302, 10, 96, 14);
    modeSlider.setBounds(offsetX + 302, 22, 96, 68);

    auto layoutRow = [offsetX](int y, std::array<juce::Slider*, 4> sliders, std::array<juce::Label*, 4> labels)
    {
        const int startX = offsetX + 48;
        const int spacing = 142;
        const int knobW = 114;
        const int knobH = 82;

        for (size_t i = 0; i < 4; ++i)
        {
            const int x = startX + static_cast<int>(i) * spacing;
            labels[i]->setBounds(x, y, knobW, 16);
            sliders[i]->setBounds(x, y + 16, knobW, knobH);
        }
    };

    layoutRow(125,
              { &roomAmountSlider, &focusSlider, &decaySlider, &smoothSlider },
              { &roomAmountLabel,  &focusLabel,  &decayLabel,  &smoothLabel });

    layoutRow(267,
              { &tailAmountSlider, &sensSlider, &releaseSlider, &outputSlider },
              { &tailAmountLabel,  &sensLabel,  &releaseLabel,  &outputLabel });

    layoutRow(409,
              { &res1Slider, &res2Slider, &res3Slider, &mixSlider },
              { &res1Label,  &res2Label,  &res3Label,  &mixLabel });
}
