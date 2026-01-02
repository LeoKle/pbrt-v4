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
    for(int x = 0; x < xresolution; ++x) {
        for (int y = 0; y < yresolution;++y) {
            const auto rgb = film->GetPixelRGB(Point2i(x, y), 1);
            ASSERT_EQ(rgb.r, 0);
            ASSERT_EQ(rgb.g, 0);
            ASSERT_EQ(rgb.b, 0);

            const auto intensity = film->GetIntensity(Point2i(x, y));
            ASSERT_EQ(intensity, 0);
        }
    }

    constexpr Float lambda = 550.f;
    constexpr Float eps = 1e-3f; // 0.001 nm

    SampledWavelengths swl =
        SampledWavelengths::SampleUniform(0.5f, lambda - eps, lambda + eps);

    printf("%s", swl.ToString().c_str());

    SampledSpectrum L;
    for (int i = 0; i < NSpectrumSamples; ++i)
        L[i] = 1.f;

    film->AddSample(
        Point2i(0, 0),   // pixel
        L,               // spectrum
        swl,             // wavelengths
        nullptr,         // visible surface (unused)
        1.f              // weight
    );

    const auto rgb1 = film->GetPixelRGB(Point2i(0, 0), 1);
    const auto rgb2 = film->GetPixelRGB(Point2i(1, 1), 1);

    printf("RGB = (%f, %f, %f)\n", rgb1.r, rgb1.g, rgb1.b);
    printf("RGB = (%f, %f, %f)\n", rgb2.r, rgb2.g, rgb2.b);

    alloc.delete_object(film);
}

TEST(CFAFilmRender, AddSample550nm) {
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

    constexpr Float lambda = 550.f;
    constexpr Float eps = 1e-3f; // 0.001 nm

    SampledWavelengths swl =
        SampledWavelengths::SampleUniform(0.5f, lambda - eps, lambda + eps);


    SampledSpectrum L;
    for (int i = 0; i < NSpectrumSamples; ++i)
        L[i] = 1.f;

    const auto sample_point = Point2i(0, 0);
    film->AddSample(
        sample_point,   // pixel
        L,               // spectrum
        swl,             // wavelengths
        nullptr,         // visible surface (unused)
        1.f              // weight
    );

    const auto rgb1 = film->GetPixelRGB(sample_point, 1);
    const auto rgb2 = film->GetPixelRGB(Point2i(1, 1), 1);

    printf("RGB = (%f, %f, %f)\n", rgb1.r, rgb1.g, rgb1.b);
    printf("RGB = (%f, %f, %f)\n", rgb2.r, rgb2.g, rgb2.b);

    alloc.delete_object(film);
}