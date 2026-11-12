#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
WahAudioProcessorEditor::WahAudioProcessorEditor (WahAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    pedalKnob = &addKnob ("pedal", "PEDAL", true);
    resKnob   = &addKnob ("resonance", "RESONANZ", true);
    modeKnob  = &addKnob ("mode", "MODUS", false);
    typeKnob  = &addKnob ("type", "TYP", false);
    intKnob   = &addKnob ("intensity", u8 ("INTENSIT\xc3\x84T"), false);
    tempoKnob = &addKnob ("tempo", "TEMPO", false);
    outKnob   = &addKnob ("out", "AUSGANG", false);
    pedalKnob->slider.getProperties().set ("stepLabels", "FERSE||||||SPITZE");
    resKnob->slider.getProperties().set ("stepLabels", "WEICH||||SCHARF");
    modeKnob->slider.getProperties().set ("stepLabels", "PEDAL|AUTO|LFO");
    typeKnob->slider.getProperties().set ("stepLabels", "KLASS.|VINT.|BASS|BREIT");
    tempoKnob->slider.getProperties().set ("stepLabels", "LANGSAM||||SCHNELL");
    outKnob->slider.getProperties().set ("stepLabels", "-12|-6|0|+6|+12");
    modeKnob->slider.setMouseDragSensitivity (110);
    typeKnob->slider.setMouseDragSensitivity (130);

    setupSwitch (dirSwitch, "up", "AUF", "AB", lampAmber);
    setupSwitch (onSwitch, "on", "AN", "AUS", lampRed);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    pedal.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (pedal);
    freqLabel.setJustificationType (juce::Justification::centred);
    freqLabel.setFont (lnf.labelFont (15.0f, 0.15f));
    freqLabel.setColour (juce::Label::textColourId, ink);
    freqLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (freqLabel);

    setSize (1180, 640);
    startTimerHz (60);
}

WahAudioProcessorEditor::~WahAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

WahAudioProcessorEditor::Knob& WahAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void WahAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lampColour)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lampColour.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

//==============================================================================
void WahAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void WahAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

void WahAudioProcessorEditor::timerCallback()
{
    const float p = proc.position.load();
    const bool on = proc.apvts.getRawParameterValue ("on")->load() > 0.5f;
    if (std::abs (p - pedal.pos) > 0.002f || on != pedal.on) { pedal.pos = p; pedal.on = on; pedal.repaint(); }

    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    if (mode != lastMode)
    {
        // Regler, die gerade nichts bewirken, treten zurück
        intKnob->slider.setAlpha (mode == 0 ? 0.45f : 1.0f);
        tempoKnob->slider.setAlpha (mode == 0 ? 0.45f : 1.0f);
        dirSwitch.setAlpha (mode == 1 ? 1.0f : 0.45f);
        lastMode = mode;
        repaint();
    }

    const auto txt = juce::String (juce::roundToInt (proc.frequency.load())) + " Hz";
    if (freqLabel.getText() != txt)
        freqLabel.setText (txt, juce::dontSendNotification);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void WahAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("WAH", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 140.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("WAH-PEDAL \xc2\xb7 AUTO-WAH \xc2\xb7 LFO"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);
    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    // Frontplatte
    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    // Rahmen um das Lampenfenster
    const auto lb = lampBezel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (lb.translated (2.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.25f), 0, lb.getY(), gold.darker (0.55f), 0, lb.getBottom(), false));
    g.fillRoundedRectangle (lb, 12.0f);
    for (auto pt : { lb.getTopLeft().translated (11, 11), lb.getTopRight().translated (-11, 11),
                     lb.getBottomLeft().translated (11, -11), lb.getBottomRight().translated (-11, -11) })
        VintageLookAndFeel::drawScrew (g, pt, 4.0f, pt.x * 0.013f);

    // Trennlinie
    const auto dv = divider.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine ((int) dv.getY() + 1, dv.getX(), dv.getRight());
    g.setColour (ink.withAlpha (0.45f));
    g.drawHorizontalLine ((int) dv.getY(), dv.getX(), dv.getRight());

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 13.0f, k->big ? 0.2f : 0.14f), juce::Justification::centred);
    }
    drawEngraved (g, "AUTO-RICHTUNG", switchLabels[0].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    drawEngraved (g, "FUSSSCHALTER", switchLabels[1].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    {
        const char* hints[] = { "Pedal von Hand oder per MIDI-Pedal (Logic: Smart Controls)", "Anschlag steuert das Wah \xe2\x80\x93 Pedal = Grundstellung", "LFO pendelt um die Pedalstellung" };
        g.setColour (ink.withAlpha (0.6f));
        g.setFont (lnf.labelFontRegular (12.5f));
        g.drawText (u8 (hints[juce::jlimit (0, 2, lastMode)]), juce::Rectangle<float> ((float) divider.getX(), (float) divider.getY() - 22.0f, (float) divider.getWidth(), 16.0f), juce::Justification::centred);
    }

    const auto pl = pilotArea.toFloat();
    VintageLookAndFeel::drawJewel (g, juce::Rectangle<float> (28.0f, 28.0f).withCentre ({ pl.getX() + 16.0f, pl.getCentreY() }), lampRed, true);
    drawEngraved (g, "BETRIEB", { pl.getX() + 36.0f, pl.getCentreY() - 8.0f, 80.0f, 16.0f }, lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    const auto tag = juce::Rectangle<float> (120.0f, 24.0f).withCentre ({ pl.getRight() - 60.0f, pl.getCentreY() });
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-67", tag, juce::Justification::centred);
}

void WahAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);
    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (34, 22);

    // obere Reihe: Pedal | Pedalfenster | Resonanz
    pedalKnob->slider.setBounds (in.getX() + 20, in.getY() + 10, 230, 210);
    resKnob->slider.setBounds (in.getRight() - 250, in.getY() + 10, 230, 210);
    lampBezel = juce::Rectangle<int> (380, 196).withCentre ({ in.getCentreX(), in.getY() + 108 });
    pedal.setBounds (lampBezel.reduced (16).withTrimmedBottom (26));
    freqLabel.setBounds (lampBezel.getX() + 16, lampBezel.getBottom() - 38, lampBezel.getWidth() - 32, 22);

    divider = { in.getX() + 10, in.getY() + 246, in.getWidth() - 20, 2 };

    // untere Reihe: Modus, Typ, Intensität, Tempo, Ausgang | Schalter
    const int ry = in.getY() + 268;
    constexpr int kw = 112, kh = 112;
    Knob* row[] = { modeKnob, typeKnob, intKnob, tempoKnob, outKnob };
    const int rowW = 700;
    for (int i = 0; i < 5; ++i)
        row[i]->slider.setBounds (in.getX() + i * (rowW / 5) + (rowW / 5 - kw) / 2, ry, kw, kh);

    const int sx = in.getX() + rowW + 26;
    switchLabels[0] = { sx, ry + 2, 170, 16 };
    dirSwitch.setBounds (sx, ry + 22, 150, 40);
    switchLabels[1] = { sx + 170, ry + 2, 170, 16 };
    onSwitch.setBounds (sx + 170, ry + 22, 150, 40);
    pilotArea = { sx, ry + 84, in.getRight() - sx, 34 };
    renderTextures();
}

//==============================================================================
void WahAudioProcessorEditor::PedalWindow::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    // dunkle Bühne mit Bodenlicht
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1611), 0, b.getY(), juce::Colour (0xff070605), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x40ffd9a0), b.getCentreX(), b.getBottom() - 10.0f, juce::Colour (0x00ffd9a0), b.getCentreX() + b.getWidth() * 0.5f, b.getBottom() - 10.0f, true));
    g.fillRect (b);

    const float floorY = b.getBottom() - 22.0f;
    const float w = b.getWidth() * 0.72f, x0 = b.getCentreX() - w * 0.5f;
    // Boden-Schatten
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (x0 - 6.0f, floorY - 4.0f, w + 12.0f, 12.0f);

    // Gehäuse (Seitenansicht, Druckguss)
    juce::Path body;
    body.startNewSubPath (x0, floorY);
    body.lineTo (x0 + w, floorY);
    body.lineTo (x0 + w, floorY - 30.0f);
    body.lineTo (x0 + 8.0f, floorY - 42.0f);
    body.lineTo (x0, floorY - 36.0f);
    body.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3833), 0, floorY - 42.0f, juce::Colour (0xff121110), 0, floorY, false));
    g.fillPath (body);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawLine (x0 + 8.0f, floorY - 41.0f, x0 + w, floorY - 29.0f, 1.2f);
    // Schriftzug am Gehäuse
    g.setColour (juce::Colour (0xffd9c48f).withAlpha (0.75f));
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("KLANGBILD", juce::Rectangle<float> (x0 + 30.0f, floorY - 26.0f, 120.0f, 14.0f), juce::Justification::centredLeft);

    // Wippe: Drehpunkt hinten-mittig, kippt zwischen Ferse (hinten unten) und Spitze (vorn unten)
    const juce::Point<float> pivot (x0 + w * 0.58f, floorY - 40.0f);
    const float ang = juce::jmap (pos, 0.0f, 1.0f, -0.17f, 0.17f);   // Bogenmaß
    juce::Path treadle;
    treadle.addRoundedRectangle (-w * 0.60f, -14.0f, w * 1.02f, 14.0f, 4.0f);
    const auto tf = juce::AffineTransform::rotation (ang).translated (pivot);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4640), pivot.x, pivot.y - 20.0f, juce::Colour (0xff1b1a18), pivot.x, pivot.y + 4.0f, false));
    g.fillPath (treadle, tf);
    // Gummibelag mit Rillen
    juce::Path rubber;
    rubber.addRoundedRectangle (-w * 0.56f, -18.0f, w * 0.94f, 5.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0d0c));
    g.fillPath (rubber, tf);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    for (int k = 0; k < 22; ++k)
    {
        juce::Line<float> ln (-w * 0.54f + (float) k * w * 0.042f, -18.0f, -w * 0.54f + (float) k * w * 0.042f, -13.5f);
        g.drawLine (ln.withShortenedStart (0).getStart().transformedBy (tf).x, ln.getStart().transformedBy (tf).y,
                    ln.getEnd().transformedBy (tf).x, ln.getEnd().transformedBy (tf).y, 1.0f);
    }
    // Zahnstange zum Poti
    g.setColour (juce::Colour (0xff8d8678));
    g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (pivot));

    // Kontroll-LED vorne
    const auto led = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ x0 + w - 16.0f, floorY - 16.0f });
    if (on)
    {
        g.setColour (juce::Colour (0x66ff3020));
        g.fillEllipse (led.expanded (6.0f));
    }
    g.setColour (on ? juce::Colour (0xffff4a30) : juce::Colour (0xff4a1a14));
    g.fillEllipse (led);

    // Stellungsanzeige
    const auto bar = juce::Rectangle<float> (b.getX() + 18.0f, b.getY() + 12.0f, b.getWidth() - 36.0f, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (bar, 2.5f);
    g.setColour (juce::Colour (0xffd9c48f));
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * pos), 2.5f);
    g.setColour (juce::Colour (0xffd9c48f).withAlpha (0.6f));
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText ("FERSE", bar.translated (0, 7.0f).withHeight (12.0f), juce::Justification::centredLeft);
    g.drawText ("SPITZE", bar.translated (0, 7.0f).withHeight (12.0f), juce::Justification::centredRight);

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 2.0f);
}
