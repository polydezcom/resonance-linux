#include "core/dsp/biquad.h"

#include <QtTest>

#include <cmath>
#include <numbers>

using resonance::Biquad;

class TestBiquad : public QObject
{
    Q_OBJECT

private slots:
    void flatWhenGainIsZero();
    void boostAtCenterFrequency();
    void cutAtCenterFrequency();
    void processIsFinite();
    void resetClearsState();
};

void TestBiquad::flatWhenGainIsZero()
{
    Biquad filter;
    filter.setPeaking(48000.0, 1000.0, 0.707, 0.0);

    for (double frequency : {50.0, 200.0, 1000.0, 5000.0, 15000.0}) {
        const double db = Biquad::magnitudeDb(filter.coefficients(), 48000.0,
                                              frequency);
        QVERIFY2(std::abs(db) < 0.001,
                 qPrintable(QStringLiteral("gain at %1 Hz was %2 dB")
                                .arg(frequency)
                                .arg(db)));
    }
}

void TestBiquad::boostAtCenterFrequency()
{
    Biquad filter;
    filter.setPeaking(48000.0, 1000.0, 0.8, 6.0);

    const double center = Biquad::magnitudeDb(filter.coefficients(), 48000.0,
                                              1000.0);
    QVERIFY2(std::abs(center - 6.0) < 0.05,
             qPrintable(QStringLiteral("center response was %1 dB").arg(center)));

    const double low = Biquad::magnitudeDb(filter.coefficients(), 48000.0, 200.0);
    const double high = Biquad::magnitudeDb(filter.coefficients(), 48000.0, 5000.0);
    QVERIFY(low < center);
    QVERIFY(high < center);
}

void TestBiquad::cutAtCenterFrequency()
{
    Biquad filter;
    filter.setPeaking(44100.0, 4000.0, 0.9, -9.0);

    const double center = Biquad::magnitudeDb(filter.coefficients(), 44100.0,
                                              4000.0);
    QVERIFY2(std::abs(center + 9.0) < 0.05,
             qPrintable(QStringLiteral("center response was %1 dB").arg(center)));
}

void TestBiquad::processIsFinite()
{
    Biquad filter;
    filter.setPeaking(48000.0, 12000.0, 1.2, 12.0);

    for (int i = 0; i < 48000; ++i) {
        const double t = static_cast<double>(i) / 48000.0;
        const float input = static_cast<float>(
            std::sin(2.0 * std::numbers::pi * 440.0 * t));
        const float output = filter.process(input);
        QVERIFY(std::isfinite(output));
    }
}

void TestBiquad::resetClearsState()
{
    Biquad filter;
    filter.setPeaking(48000.0, 1000.0, 0.8, 6.0);

    for (int i = 0; i < 100; ++i)
        filter.process(1.0f);

    filter.reset();
    QCOMPARE(filter.process(0.0f), 0.0f);
}

QTEST_APPLESS_MAIN(TestBiquad)
#include "test_biquad.moc"
