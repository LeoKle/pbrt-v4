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

            EXPECT_EQ(film->GetMosaicType(x, y), CharToMosaic(expectedChar));
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