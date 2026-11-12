#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr percent() { return withText ([] (float v) { return juce::String (juce::roundToInt (v)) + " %"; }); }
}

juce::AudioProcessorValueTreeState::ParameterLayout WahAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "pedal", 1 }, "Pedal", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, percent()));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Modus",
        juce::StringArray { "Pedal", "Auto-Wah", "LFO" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Typ",
        juce::StringArray { "Klassisch", "Vintage", "Bass", "Breit" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "resonance", 1 }, "Resonanz", juce::NormalisableRange<float> (50.0f, 160.0f, 0.1f), 100.0f, percent()));
    l.add (std::make_unique<APF> (juce::ParameterID { "intensity", 1 }, juce::String (juce::CharPointer_UTF8 ("Intensit\xc3\xa4t")), juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f, percent()));
    l.add (std::make_unique<APF> (juce::ParameterID { "tempo", 1 }, "Tempo", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, percent()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "up", 1 }, "Richtung auf", true));
    l.add (std::make_unique<APF> (juce::ParameterID { "out", 1 }, "Ausgang", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        withText ([] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; })));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "Wah an", true));
    return l;
}

const std::vector<WahAudioProcessor::Preset>& WahAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                                 { { "pedal", 50 }, { "mode", 0 }, { "type", 0 }, { "resonance", 100 }, { "intensity", 60 }, { "tempo", 50 }, { "up", 1 }, { "out", 0 } } },
        { "Halb offen \xe2\x80\x93 nasaler Solo-Ton", { { "pedal", 38 }, { "mode", 0 }, { "type", 0 }, { "resonance", 110 }, { "intensity", 60 }, { "tempo", 50 }, { "up", 1 }, { "out", 0 } } },
        { "Vintage \xe2\x80\x93 vokal",              { { "pedal", 50 }, { "mode", 0 }, { "type", 1 }, { "resonance", 100 }, { "intensity", 60 }, { "tempo", 50 }, { "up", 1 }, { "out", 0 } } },
        { "Funk \xe2\x80\x93 Auto-Wah",              { { "pedal", 10 }, { "mode", 1 }, { "type", 0 }, { "resonance", 115 }, { "intensity", 80 }, { "tempo", 70 }, { "up", 1 }, { "out", 0 } } },
        { "Auto-Wah \xe2\x80\x93 weich",             { { "pedal", 15 }, { "mode", 1 }, { "type", 3 }, { "resonance", 90 },  { "intensity", 65 }, { "tempo", 35 }, { "up", 1 }, { "out", 0 } } },
        { "Auto-Wah abw\xc3\xa4rts",                 { { "pedal", 90 }, { "mode", 1 }, { "type", 0 }, { "resonance", 110 }, { "intensity", 75 }, { "tempo", 60 }, { "up", 0 }, { "out", 0 } } },
        { "Bass \xe2\x80\x93 Envelope-Funk",         { { "pedal", 5 },  { "mode", 1 }, { "type", 2 }, { "resonance", 120 }, { "intensity", 85 }, { "tempo", 65 }, { "up", 1 }, { "out", 0 } } },
        { "LFO \xe2\x80\x93 langsam pendelnd",       { { "pedal", 50 }, { "mode", 2 }, { "type", 0 }, { "resonance", 100 }, { "intensity", 80 }, { "tempo", 30 }, { "up", 1 }, { "out", 0 } } },
        { "Orgel/Keys \xe2\x80\x93 breit",           { { "pedal", 20 }, { "mode", 1 }, { "type", 3 }, { "resonance", 80 },  { "intensity", 60 }, { "tempo", 45 }, { "up", 1 }, { "out", 0 } } },
    };
    return p;
}

void WahAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String WahAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

WahAudioProcessor::WahAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildWah", createLayout())
{
}

bool WahAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void WahAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& f : filters) f.reset();
    env.env = 0;
    posState = apvts.getRawParameterValue ("pedal")->load() * 0.01;
    posCoef = 1.0 - std::exp (-1.0 / (0.012 * sr));   // Pedal bewegt sich nicht sprunghaft
    smOut.reset (sr, 0.03); smOn.reset (sr, 0.02); smRes.reset (sr, 0.03);
    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));
    smOn.setCurrentAndTargetValue (apvts.getRawParameterValue ("on")->load() > 0.5f ? 1.0f : 0.0f);
    smRes.setCurrentAndTargetValue (apvts.getRawParameterValue ("resonance")->load() * 0.01f);
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
    lfoPhase = 0;
}

void WahAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = juce::jmin (2, getTotalNumOutputChannels());
    if (n == 0 || outCh == 0)
        return;
    if (inCh == 1 && outCh == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    const int mode = (int) apvts.getRawParameterValue ("mode")->load();
    const int ti = juce::jlimit (0, 3, (int) apvts.getRawParameterValue ("type")->load());
    const auto& type = wah::types()[(size_t) ti];
    const double pedal = apvts.getRawParameterValue ("pedal")->load() * 0.01;
    const double inten = apvts.getRawParameterValue ("intensity")->load() * 0.01;
    const double tempo = apvts.getRawParameterValue ("tempo")->load() * 0.01;
    const bool up = apvts.getRawParameterValue ("up")->load() > 0.5f;
    smOut.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()) * makeup[(size_t) ti]);
    smOn.setTargetValue (apvts.getRawParameterValue ("on")->load() > 0.5f ? 1.0f : 0.0f);
    smRes.setTargetValue (apvts.getRawParameterValue ("resonance")->load() * 0.01f);

    // Tempo: Auto-Wah = Ansprache (Attack/Release), LFO = Geschwindigkeit
    env.set (sr, 30.0 * std::pow (0.12, tempo), 650.0 * std::pow (0.15, tempo));
    const double lfoHz = 0.1 * std::pow (80.0, tempo);
    const double lfoInc = lfoHz / sr;

    float* ch[2] = { buffer.getWritePointer (0), outCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float pk = 0;
    double f = 0;
    for (int i = 0; i < n; ++i)
    {
        double target = pedal;
        if (mode == 1)
        {
            const float mono = ch[1] != nullptr ? 0.5f * (ch[0][i] + ch[1][i]) : ch[0][i];
            const double e = env.process (mono);
            const double lev = juce::jlimit (0.0, 1.0, (20.0 * std::log10 (e + 1e-9) + 48.0) / 36.0);
            target = pedal + (up ? 1.0 : -1.0) * inten * lev;
        }
        else if (mode == 2)
        {
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            target = pedal + 0.5 * inten * std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
        }
        posState += (juce::jlimit (0.0, 1.0, target) - posState) * posCoef;

        f = wah::frequencyFor (type, posState);
        const double g = std::tan (juce::MathConstants<double>::pi * juce::jmin (f, sr * 0.45) / sr);
        const double k = 1.0 / (type.q * smRes.getNextValue());
        const float on = smOn.getNextValue(), og = smOut.getNextValue();
        for (int c = 0; c < outCh; ++c)
        {
            if (ch[c] == nullptr) continue;
            const float x = ch[c][i];
            double bp, lp;
            filters[(size_t) c].process (wah::bite (x, (float) type.drive), g, k, bp, lp);
            const float wet = (float) (bp + type.lowKeep * lp) * og;
            const float y = x + (wet - x) * on;
            ch[c][i] = y;
            pk = juce::jmax (pk, std::abs (y));
            if (c == 0) vuState += (std::abs (y) - vuState) * vuCoef;
        }
    }
    position.store ((float) posState);
    frequency.store ((float) f);
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void WahAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void WahAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* WahAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new WahAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WahAudioProcessor(); }
