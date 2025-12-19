#include <gtest/gtest.h>

#include <pbrt/film.h>
#include <pbrt/filters.h>
#include <pbrt/quantum_efficiency.h>
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

    EXPECT_EQ(film->GetMosaicType(0,0), MosaicType::R);
    EXPECT_EQ(film->GetMosaicType(1,0), MosaicType::G);
    EXPECT_EQ(film->GetMosaicType(0,1), MosaicType::G);
    EXPECT_EQ(film->GetMosaicType(1,1), MosaicType::B);

    alloc.delete_object(film);
}