#include <gtest/gtest.h>

#include <pbrt/base/filter.h>
#include <pbrt/film.h>
#include <pbrt/film_test.h>
#include <pbrt/filters.h>
#include <pbrt/quantum_efficiency.h>
#include <pbrt/util/colorspace.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/spectrum.h>
#include <pbrt/util/vecmath.h>

using namespace pbrt;

struct CFATestCase {
    int width;
    int height;
    std::string pattern;
};

class CFAFilmParameterizedTest : public ::testing::TestWithParam<CFATestCase> {};

TEST_P(CFAFilmParameterizedTest, ArbitraryPatternTiling) {
    const auto &tc = GetParam();
    const auto params = createParameterDictionary(tc.width, tc.height, tc.pattern);

    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    ColorFilterArrayFilm *film =
        ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc);

    ASSERT_NE(film, nullptr);

    // verify pattern correctness in multiple repetitions of the pattern
    for (int y = 0; y < tc.height * 3; ++y) {
        for (int x = 0; x < tc.width * 3; ++x) {
            int px = x % tc.width;
            int py = y % tc.height;
            char expectedChar = tc.pattern[py * tc.width + px];

            EXPECT_EQ(film->GetMosaicType(x, y), CharToFilter(expectedChar));
        }
    }

    alloc.delete_object(film);
}

INSTANTIATE_TEST_CASE_P(CFAFilmPatterns, CFAFilmParameterizedTest,
                        ::testing::Values(CFATestCase{2, 2, "RGGB"},
                                          CFATestCase{2, 2, "CYYM"},
                                          CFATestCase{3, 2, "RGBRGB"},
                                          CFATestCase{4, 1, "RGBG"},
                                          CFATestCase{1, 4, "RGBG"}));

TEST(CFAFilm, InvalidPatternCharacter) {
    const auto params = createParameterDictionary(2, 2, "RGXG");

    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    ASSERT_DEATH(
        { ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc); },
        "Unknown CFA pattern character");
}

TEST(CFAFilm, PatternSizeValidation) {
    const auto params = createParameterDictionary(
        2, 2, "RGB");  // pattern size 2x2, but pattern is only 3 chars

    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    // as PBRT uses ErrorExit() which terminates it, we cannot check for == nullptr or
    // similiar
    ASSERT_DEATH(
        { ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc); },
        "CFA pattern size mismatch");
}

TEST(CFAFilmRender, DefaultZeroCheck) {
    const int xresolution = 500;
    const int yresolution = 500;
    const auto params = createParameterDictionary(1, 1, "R", xresolution, yresolution);

    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    ColorFilterArrayFilm *film =
        ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc);

    ASSERT_NE(film, nullptr) << "Film construction failed.";

    // assert film pixels rgb and intensity are 0 by default
    for (int x = 0; x < xresolution; ++x) {
        for (int y = 0; y < yresolution; ++y) {
            const auto rgb = film->GetPixelRGB(Point2i(x, y), 1);
            ASSERT_EQ(rgb.r, 0);
            ASSERT_EQ(rgb.g, 0);
            ASSERT_EQ(rgb.b, 0);

            const auto intensity = film->GetIntensity(Point2i(x, y));
            ASSERT_EQ(intensity, 0);
        }
    }

    constexpr Float lambda = 550.f;
    constexpr Float eps = 1e-3f;  // 0.001 nm

    SampledWavelengths swl =
        SampledWavelengths::SampleUniform(0.5f, lambda - eps, lambda + eps);

    printf("%s", swl.ToString().c_str());

    SampledSpectrum L;
    for (int i = 0; i < NSpectrumSamples; ++i)
        L[i] = 1.f;

    film->AddSample(Point2i(0, 0),  // pixel
                    L,              // spectrum
                    swl,            // wavelengths
                    nullptr,        // visible surface (unused)
                    1.f             // weight
    );

    const auto rgb1 = film->GetPixelRGB(Point2i(0, 0), 1);
    const auto rgb2 = film->GetPixelRGB(Point2i(1, 1), 1);

    printf("RGB = (%f, %f, %f)\n", rgb1.r, rgb1.g, rgb1.b);
    printf("RGB = (%f, %f, %f)\n", rgb2.r, rgb2.g, rgb2.b);

    alloc.delete_object(film);
}

struct CFAFilmQEParam {
    Float lambda_qe_low;
    Float lambda_qe_high;
    std::string mosaic;
};

class CFAFilmRenderQE : public ::testing::TestWithParam<CFAFilmQEParam> {};

TEST_P(CFAFilmRenderQE, AddSampleCompareQE) {
    const auto &p = GetParam();

    const int xresolution = 500;
    const int yresolution = 500;

    const auto params =
        createParameterDictionary(1, 1, p.mosaic.c_str(), xresolution, yresolution);

    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    ColorFilterArrayFilm *film =
        ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc);

    ASSERT_NE(film, nullptr);

    constexpr Float eps = 1e-3f;

    SampledSpectrum L;
    for (int i = 0; i < NSpectrumSamples; ++i)
        L[i] = 1.f;

    // first wavelength
    Point2i p0(0, 0);
    auto swlLow =
        SampledWavelengths::SampleUniform(0.5f, p.lambda_qe_low - eps, p.lambda_qe_low + eps);

    film->AddSample(p0, L, swlLow, nullptr, 1.f);
    Float i0 = film->GetIntensity(p0);

    ASSERT_GT(i0, 0) << "Expected greater than 0 intensity, got " << p.lambda_qe_low;

    // second wavelength
    Point2i p1(0, 1);
    auto swlHigh =
        SampledWavelengths::SampleUniform(0.5f, p.lambda_qe_high - eps, p.lambda_qe_high + eps);

    film->AddSample(p1, L, swlHigh, nullptr, 1.f);
    Float i1 = film->GetIntensity(p1);

    EXPECT_GT(i1, i0) << "Expected " << p.lambda_qe_low << " nm to have lower intensity than "
                      << p.lambda_qe_high << "nm at " << p.mosaic;

    alloc.delete_object(film);
}

INSTANTIATE_TEST_CASE_P(CFAQuantumEfficiency, CFAFilmRenderQE,
                        ::testing::Values(CFAFilmQEParam{550.f, 610.f, "R"},
                                          CFAFilmQEParam{450.f, 525.f, "G"},
                                          CFAFilmQEParam{600.f, 450.f, "B"}));

// TEST(CFAFilmRender, Template) {
//     const int xresolution = 500;
//     const int yresolution = 500;
//     const auto params = createParameterDictionary(1, 1, "R", xresolution, yresolution);

//     Float exposure = 1.0f;
//     Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
//     const RGBColorSpace *cs = RGBColorSpace::sRGB;
//     FileLoc loc;
//     Allocator alloc;

//     ColorFilterArrayFilm *film =
//         ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc);

//     ASSERT_NE(film, nullptr) << "Film construction failed.";

//     constexpr Float lambda = 550.f;
//     constexpr Float eps = 1e-3f; // 0.001 nm

//     SampledWavelengths swl =
//         SampledWavelengths::SampleUniform(0.5f, lambda - eps, lambda + eps);

//     SampledSpectrum L;
//     for (int i = 0; i < NSpectrumSamples; ++i)
//         L[i] = 1.f;

//     const auto sample_point = Point2i(0, 0);
//     film->AddSample(
//         sample_point,   // pixel
//         L,               // spectrum
//         swl,             // wavelengths
//         nullptr,         // visible surface (unused)
//         1.f              // weight
//     );

//     const auto rgb1 = film->GetPixelRGB(sample_point, 1);
//     const auto rgb2 = film->GetPixelRGB(Point2i(1, 1), 1);

//     printf("RGB = (%f, %f, %f)\n", rgb1.r, rgb1.g, rgb1.b);
//     printf("RGB = (%f, %f, %f)\n", rgb2.r, rgb2.g, rgb2.b);

//     alloc.delete_object(film);
// }