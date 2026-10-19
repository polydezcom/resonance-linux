#include "core/dsp/biquad.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace resonance {
namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

void Biquad::setPeaking(double sampleRate, double frequency, double q,
                        double gainDb)
{
    if (sampleRate <= 0.0 || frequency <= 0.0 || q <= 0.0) {
        setCoefficients({});
        return;
    }

    const double nyquist = sampleRate * 0.5;
    frequency = std::clamp(frequency, 10.0, nyquist * 0.95);

    const double a = std::pow(10.0, gainDb / 40.0);
    const double omega = 2.0 * kPi * frequency / sampleRate;
    const double alpha = std::sin(omega) / (2.0 * q);
    const double cosOmega = std::cos(omega);

    const double b0 = 1.0 + alpha * a;
    const double b1 = -2.0 * cosOmega;
    const double b2 = 1.0 - alpha * a;
    const double a0 = 1.0 + alpha / a;
    const double a1 = -2.0 * cosOmega;
    const double a2 = 1.0 - alpha / a;

    BiquadCoefficients coefficients;
    coefficients.b0 = b0 / a0;
    coefficients.b1 = b1 / a0;
    coefficients.b2 = b2 / a0;
    coefficients.a1 = a1 / a0;
    coefficients.a2 = a2 / a0;
    setCoefficients(coefficients);
}

void Biquad::setCoefficients(const BiquadCoefficients &coefficients)
{
    m_coefficients = coefficients;
}

void Biquad::reset()
{
    m_z1 = 0.0f;
    m_z2 = 0.0f;
}

float Biquad::process(float input)
{
    const double output = m_coefficients.b0 * input + m_z1;
    m_z1 = static_cast<float>(
        m_coefficients.b1 * input - m_coefficients.a1 * output + m_z2);
    m_z2 = static_cast<float>(
        m_coefficients.b2 * input - m_coefficients.a2 * output);
    return static_cast<float>(output);
}

double Biquad::magnitudeDb(const BiquadCoefficients &coefficients,
                           double sampleRate, double frequency)
{
    if (sampleRate <= 0.0)
        return 0.0;

    const double omega = 2.0 * kPi * frequency / sampleRate;
    const std::complex<double> zInv = std::polar(1.0, -omega);
    const std::complex<double> numerator =
        coefficients.b0 + coefficients.b1 * zInv
        + coefficients.b2 * zInv * zInv;
    const std::complex<double> denominator =
        1.0 + coefficients.a1 * zInv + coefficients.a2 * zInv * zInv;
    const double magnitude = std::abs(numerator / denominator);
    return 20.0 * std::log10(std::max(magnitude, 1.0e-12));
}

} // namespace resonance
