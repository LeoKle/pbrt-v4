#include <gtest/gtest.h>

#include <pbrt/base/filter.h>
#include <pbrt/film.h>
#include <pbrt/filters.h>
#include <pbrt/quantum_efficiency.h>
#include <pbrt/util/colorspace.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/spectrum.h>
#include <pbrt/util/vecmath.h>

using namespace pbrt;

ParameterDictionary createParameterDictionary(int pattern_width, int pattern_height,
                                              const std::string &pattern,
                                              int xresolution = 1920,
                                              int yresolution = 1080) {
    ParsedParameterVector paramsVec;
    Allocator alloc;

    ParsedParameter *xres = alloc.new_object<ParsedParameter>(FileLoc());
    xres->type = "integer";
    xres->name = "xresolution";
    xres->AddInt(xresolution);
    paramsVec.push_back(xres);

    ParsedParameter *yres = alloc.new_object<ParsedParameter>(FileLoc());
    yres->type = "integer";
    yres->name = "yresolution";
    yres->AddInt(yresolution);
    paramsVec.push_back(yres);

    ParsedParameter *filename = alloc.new_object<ParsedParameter>(FileLoc());
    filename->type = "string";
    filename->name = "filename";
    filename->AddString("test.exr");
    paramsVec.push_back(filename);

    ParsedParameter *pattern_width_param = alloc.new_object<ParsedParameter>(FileLoc());
    pattern_width_param->type = "integer";
    pattern_width_param->name = "pattern_width";
    pattern_width_param->AddInt(pattern_width);
    paramsVec.push_back(pattern_width_param);

    ParsedParameter *pattern_height_param = alloc.new_object<ParsedParameter>(FileLoc());
    pattern_height_param->type = "integer";
    pattern_height_param->name = "pattern_height";
    pattern_height_param->AddInt(pattern_height);
    paramsVec.push_back(pattern_height_param);

    ParsedParameter *pattern_type = alloc.new_object<ParsedParameter>(FileLoc());
    pattern_type->type = "string";
    pattern_type->name = "pattern";
    pattern_type->AddString(pattern);
    paramsVec.push_back(pattern_type);

    return ParameterDictionary(paramsVec, RGBColorSpace::sRGB);
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

TEST(CFAFilm, RGGB) {
    const auto params = createParameterDictionary(2, 2, "RGGB");

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

    EXPECT_EQ(film->GetMosaicType(0, 0), MosaicType::R);
    EXPECT_EQ(film->GetMosaicType(1, 0), MosaicType::G);
    EXPECT_EQ(film->GetMosaicType(0, 1), MosaicType::G);
    EXPECT_EQ(film->GetMosaicType(1, 1), MosaicType::B);
    EXPECT_EQ(film->GetMosaicType(2, 0), MosaicType::R);
    EXPECT_EQ(film->GetMosaicType(3, 3), MosaicType::B);

    alloc.delete_object(film);
}

TEST(CFAFilm, CYYM) {
    const auto params = createParameterDictionary(2, 2, "CYYM");

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

    EXPECT_EQ(film->GetMosaicType(0, 0), MosaicType::C);
    EXPECT_EQ(film->GetMosaicType(1, 0), MosaicType::Y);
    EXPECT_EQ(film->GetMosaicType(0, 1), MosaicType::Y);
    EXPECT_EQ(film->GetMosaicType(1, 1), MosaicType::M);
    EXPECT_EQ(film->GetMosaicType(2, 0), MosaicType::C);

    alloc.delete_object(film);
}