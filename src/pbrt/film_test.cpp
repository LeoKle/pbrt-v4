#include <gtest/gtest.h>

#include <pbrt/film.h>
#include <pbrt/filters.h>
#include <pbrt/base/filter.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/colorspace.h>
#include <pbrt/util/vecmath.h>
#include <pbrt/util/spectrum.h>


using namespace pbrt;

TEST(Film, Simple) {
    ParameterDictionary params;
    Float exposure = 1.0f;
    Filter filter = new BoxFilter(Vector2f(0.5, 0.5));
    const RGBColorSpace *cs = RGBColorSpace::sRGB;
    FileLoc loc;
    Allocator alloc;

    ColorFilterArrayFilm *film =
        ColorFilterArrayFilm::Create(params, exposure, filter, cs, &loc, alloc);

    ASSERT_NE(film, nullptr) << "Film construction failed.";

    EXPECT_GT(film->ToString().size(), 0u);
    EXPECT_TRUE(film->UsesVisibleSurface() == false);

    alloc.delete_object(film);
}