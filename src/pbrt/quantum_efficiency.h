#ifndef QE_H
#define QE_H

#include <pbrt/util/error.h>

// Quantum efficiency curves for CMOS colors (RGB + CMY)
// Data imported from QE_*.csv (wavelength in nm, QE in [0,1])

constexpr int QE_SAMPLES = 321;
extern const float QE_BANDS[QE_SAMPLES];
extern const float QE_RED[QE_SAMPLES];
extern const float QE_GREEN[QE_SAMPLES];
extern const float QE_BLUE[QE_SAMPLES];
extern const float QE_CYAN[QE_SAMPLES];
extern const float QE_MAGENTA[QE_SAMPLES];
extern const float QE_YELLOW[QE_SAMPLES];
extern const float QE_MONO[QE_SAMPLES];

namespace pbrt {
    enum class MosaicType : uint8_t {
    R = 0,
    G,
    B,
    C,
    Y,
    M,
    MONO,
    Count
};

static_assert(int(MosaicType::Count) == 7);

struct SpectralCurve {
    const Float* bands; // wavelengths in nanometers
    const Float* values; // qe values [0,1]
    int n; // number of values
};

PBRT_CPU_GPU inline constexpr SpectralCurve QECurves[] = {
    { QE_BANDS, QE_RED,     QE_SAMPLES }, // R
    { QE_BANDS, QE_GREEN,   QE_SAMPLES }, // G
    { QE_BANDS, QE_BLUE,    QE_SAMPLES }, // B
    { QE_BANDS, QE_CYAN,    QE_SAMPLES }, // C
    { QE_BANDS, QE_YELLOW,  QE_SAMPLES }, // Y
    { QE_BANDS, QE_MAGENTA, QE_SAMPLES }, // M
    { QE_BANDS, QE_MONO,    QE_SAMPLES }  // MONO
};

static_assert(int(MosaicType::Count) ==
              int(sizeof(QECurves) / sizeof(QECurves[0])));

PBRT_CPU_GPU inline Float SampleSpectralCurve(
    const Float* __restrict bands,
    const Float* __restrict values,
    int n,
    Float lambda)
{
    if (n <= 1)
        return (n == 1) ? values[0] : Float(0);

    // Below range
    if (lambda <= bands[0]) {
        Float x0 = bands[0], x1 = bands[1];
        Float y0 = values[0], y1 = values[1];
        Float v = y0 + (y1 - y0) * (lambda - x0) / (x1 - x0);
        return v > 0 ? v : Float(0);
    }

    // Above range
    if (lambda >= bands[n - 1]) {
        Float x0 = bands[n - 2], x1 = bands[n - 1];
        Float y0 = values[n - 2], y1 = values[n - 1];
        Float v = y1 + (y1 - y0) * (lambda - x1) / (x1 - x0);
        return v > 0 ? v : Float(0);
    }

    // Linear search (n is small; branch-predictable)
    int i = 0;
    while (lambda > bands[i + 1])
        ++i;

    Float t = (lambda - bands[i]) / (bands[i + 1] - bands[i]);
    return values[i] + t * (values[i + 1] - values[i]);
};


PBRT_CPU_GPU inline const SpectralCurve& GetQECurve(MosaicType t)
{
    return QECurves[static_cast<int>(t)];
};

PBRT_CPU_GPU inline Float SampleMosaicQE(
    MosaicType mosaic,
    Float lambda)
{
    const SpectralCurve& c = GetQECurve(mosaic);
    return SampleSpectralCurve(c.bands, c.values, c.n, lambda);
}

PBRT_CPU_GPU
inline MosaicType CharToMosaic(char c) {
    switch (c) {
    case 'R': return MosaicType::R;
    case 'G': return MosaicType::G;
    case 'B': return MosaicType::B;
    case 'C': return MosaicType::C;
    case 'Y': return MosaicType::Y;
    case 'M': return MosaicType::M;
    case 'W': return MosaicType::MONO;
    default:
        ErrorExit("Unknown CFA pattern character '%c'", c);
        return MosaicType::MONO;
    }
};
}

#endif