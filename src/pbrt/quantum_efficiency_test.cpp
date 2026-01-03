#include <gtest/gtest.h>

#include <pbrt/pbrt.h>
#include <pbrt/quantum_efficiency.h>

using namespace pbrt;

TEST(QESamplingTest, BasicRange) {
    for (int i = 0; i < int(FilterType::Count); ++i) {
        FilterType m = static_cast<FilterType>(i);
        const SpectralCurve& curve = GetQECurve(m);

        Float first = GetFilterQE(m, curve.bands[0]);
        Float mid   = GetFilterQE(m, curve.bands[curve.n / 2]);
        Float last  = GetFilterQE(m, curve.bands[curve.n - 1]);

        EXPECT_GE(first, 0.f);
        EXPECT_LE(first, 1.f);

        EXPECT_GE(mid, 0.f);
        EXPECT_LE(mid, 1.f);

        EXPECT_GE(last, 0.f);
        EXPECT_LE(last, 1.f);
    }
}

TEST(QESamplingTest, Interpolation) {
    FilterType m = FilterType::R;
    const SpectralCurve& curve = GetQECurve(m);

    // Pick two adjacent bands
    int i = curve.n / 3;
    Float lambda0 = curve.bands[i];
    Float lambda1 = curve.bands[i + 1];
    Float val0 = curve.values[i];
    Float val1 = curve.values[i + 1];

    // Midpoint should be roughly average of val0 and val1
    Float midLambda = 0.5f * (lambda0 + lambda1);
    Float midVal = GetFilterQE(m, midLambda);
    EXPECT_NEAR(midVal, 0.5f * (val0 + val1), 1e-6f);
}

TEST(QESamplingTest, Extrapolation) {
    FilterType m = FilterType::G;
    const SpectralCurve& curve = GetQECurve(m);

    // below first band
    Float below = GetFilterQE(m, curve.bands[0] - 10.f);
    EXPECT_GE(below, 0.f);

    // above last band
    Float above = GetFilterQE(m, curve.bands[curve.n - 1] + 10.f);
    EXPECT_GE(above, 0.f);
}
