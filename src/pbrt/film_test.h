#ifndef FILM_TEST_H
#define FILM_TEST_H

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

ParameterDictionary static createParameterDictionary(int pattern_width, int pattern_height,
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

#endif  // FILM_TEST_H