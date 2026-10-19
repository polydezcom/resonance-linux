#pragma once

namespace resonance {

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

/**
 * RBJ cookbook biquad, direct form II transposed. Used by the graphic EQ.
 * Coefficients are deliberately double precision; audio state is float.
 */
class Biquad
{
public:
    void setPeaking(double sampleRate, double frequency, double q,
                    double gainDb);
    void setCoefficients(const BiquadCoefficients &coefficients);
    void reset();

    float process(float input);

    const BiquadCoefficients &coefficients() const { return m_coefficients; }

    /// Magnitude response in dB at @p frequency, for tests and UI curves.
    static double magnitudeDb(const BiquadCoefficients &coefficients,
                              double sampleRate, double frequency);

private:
    BiquadCoefficients m_coefficients;
    float m_z1 = 0.0f;
    float m_z2 = 0.0f;
};

} // namespace resonance
