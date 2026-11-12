#pragma once
#include <cmath>
#include <array>
#include <juce_core/juce_core.h>

// Klangbild Wah – Wah-Pedal mit resonantem Filter (Zustandsvariablen-Filter, TPT),
// Pedalstellung von Hand, über die Hüllkurve (Auto-Wah) oder über einen LFO.
namespace wah
{
struct Type
{
    const char* name;
    double fLow, fHigh;   // Fersen- und Spitzenstellung
    double q;             // Resonanz
    double lowKeep;       // Anteil Tiefpass (Bass bleibt erhalten)
    double drive;         // Transistor-Biss vor dem Filter
};

inline const std::array<Type, 4>& types()
{
    static const std::array<Type, 4> t { {
        { "Klassisch", 420.0, 2200.0, 5.2, 0.18, 1.4 },   // amerikanisches Standard-Wah
        { "Vintage",   350.0, 1900.0, 7.0, 0.12, 1.8 },   // italienisches 60er-Wah: enger, vokaler
        { "Bass",      170.0, 1150.0, 5.5, 0.45, 1.2 },   // Bass-Wah: tiefer Bereich, Fundament bleibt
        { "Breit",     300.0, 3600.0, 3.4, 0.22, 1.1 },   // weiter Weg, weicher – auch für Keys
    } };
    return t;
}

// Zustandsvariablen-Filter (Zavalishin), stabil auch bei schnellen Frequenzänderungen
struct SVF
{
    double ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    // liefert Bandpass (auf Spitzenpegel 1 normiert) und Tiefpass
    inline void process (double x, double g, double k, double& bp, double& lp)
    {
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1, a3 = g * a2;
        const double v3 = x - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        bp = v1 * k;
        lp = v2;
    }
};

struct Envelope
{
    double env = 0, att = 0.01, rel = 0.001;
    void set (double sr, double attMs, double relMs)
    {
        att = 1.0 - std::exp (-1.0 / (attMs * 0.001 * sr));
        rel = 1.0 - std::exp (-1.0 / (relMs * 0.001 * sr));
    }
    inline double process (double x)
    {
        const double a = std::abs (x);
        env += (a - env) * (a > env ? att : rel);
        return env;
    }
};

// Pedalweg 0..1 → Frequenz (logarithmisch wie das Audio-Poti im Pedal)
inline double frequencyFor (const Type& t, double pos)
{
    pos = juce::jlimit (0.0, 1.0, pos);
    return t.fLow * std::pow (t.fHigh / t.fLow, pos);
}

inline float bite (float x, float drive)
{
    // leichte, unsymmetrische Transistor-Sättigung
    const float d = drive;
    return (std::tanh (d * x + 0.15f) - std::tanh (0.15f)) / d;
}
} // namespace wah
