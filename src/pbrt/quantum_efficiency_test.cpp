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

TEST(QESamplingTest, ExactSampleHit) {
    Float lambda[] = {400.f, 500.f, 600.f};
    Float values[] = {0.1f, 0.5f, 0.9f};

    EXPECT_FLOAT_EQ(
        LinearInterpolateSpectralCurve(lambda, values, 3, 400.f),
        0.1f);

    EXPECT_FLOAT_EQ(
        LinearInterpolateSpectralCurve(lambda, values, 3, 500.f),
        0.5f);

    EXPECT_FLOAT_EQ(
        LinearInterpolateSpectralCurve(lambda, values, 3, 600.f),
        0.9f);
}

TEST(QESamplingTest, MidpointInterpolation) {
    Float lambda[] = {400.f, 500.f};
    Float values[] = {0.2f, 0.6f};

    Float result = LinearInterpolateSpectralCurve(
        lambda, values, 2, 450.f);

    EXPECT_FLOAT_EQ(result, 0.4f);
}

TEST(QESamplingTest, FractionalInterpolation) {
    Float lambda[] = {400.f, 500.f};
    Float values[] = {0.f, 1.f};

    EXPECT_FLOAT_EQ(
        LinearInterpolateSpectralCurve(lambda, values, 2, 425.f),
        0.25f);

    EXPECT_FLOAT_EQ(
        LinearInterpolateSpectralCurve(lambda, values, 2, 475.f),
        0.75f);
};

TEST(QESamplingTest, PreservesMidpoint) {
    Float lambda[] = {400.f, 450.f, 500.f};
    Float values[] = {0.2f, 0.4f, 0.8f};

    Float v1 = LinearInterpolateSpectralCurve(lambda, values, 3, 425.f);
    Float v2 = LinearInterpolateSpectralCurve(lambda, values, 3, 475.f);

    EXPECT_GT(v2, v1);
}