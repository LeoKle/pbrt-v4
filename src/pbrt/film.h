// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_FILM_H
#define PBRT_FILM_H

// PhysLight code contributed by Anders Langlands and Luca Fascione
// Copyright (c) 2020, Weta Digital, Ltd.
// SPDX-License-Identifier: Apache-2.0

#include <pbrt/pbrt.h>

#include <pbrt/base/bxdf.h>
#include <pbrt/base/camera.h>
#include <pbrt/base/film.h>
#include <pbrt/bsdf.h>
#include <pbrt/util/color.h>
#include <pbrt/util/colorspace.h>
#include <pbrt/util/parallel.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/sampling.h>
#include <pbrt/util/spectrum.h>
#include <pbrt/util/transform.h>
#include <pbrt/util/vecmath.h>

#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

//Spectral sensitivities, werden angepasst wenn Prof Braun uns die Daten schickt
//https://github.com/butcherg/ssf-data/blob/master/
static constexpr int   H2_SSF_SAMPLES = 33;

static constexpr Float H2_SSF_BANDS[H2_SSF_SAMPLES] = {
    400.f, 410.f, 420.f, 430.f, 440.f, 450.f, 460.f, 470.f, 480.f, 490.f,
    500.f, 510.f, 520.f, 530.f, 540.f, 550.f, 560.f, 570.f, 580.f, 590.f,
    600.f, 610.f, 620.f, 630.f, 640.f, 650.f, 660.f, 670.f, 680.f, 690.f,
    700.f, 710.f, 720.f
};

static constexpr Float H2_red_ssf[H2_SSF_SAMPLES] = {
    0.011502f, 0.010943f, 0.009814f, 0.008881f, 0.009348f,
    0.010582f, 0.014842f, 0.023967f, 0.028472f, 0.029149f,
    0.037155f, 0.044464f, 0.050044f, 0.058435f, 0.064967f,
    0.069375f, 0.059423f, 0.063703f, 0.107500f, 0.303100f,
    0.469380f, 0.517160f, 0.350020f, 0.323390f, 0.233930f,
    0.160610f, 0.107610f, 0.076421f, 0.047979f, 0.026946f,
    0.017900f, 0.011725f, 0.007116f
};

static constexpr Float H2_green_ssf[H2_SSF_SAMPLES] = {
    0.010796f, 0.014396f, 0.018709f, 0.025698f, 0.036083f,
    0.054893f, 0.095247f, 0.185540f, 0.267700f, 0.326000f,
    0.474500f, 0.685860f, 0.867270f, 1.000000f, 0.957900f,
    0.938680f, 0.783280f, 0.671550f, 0.476000f, 0.310970f,
    0.133910f, 0.059444f, 0.022136f, 0.018125f, 0.012011f,
    0.008557f, 0.006527f, 0.006095f, 0.005276f, 0.004191f,
    0.003890f, 0.002988f, 0.001873f
};

static constexpr Float H2_blue_ssf[H2_SSF_SAMPLES] = {
    0.260100f, 0.383000f, 0.442320f, 0.520190f, 0.584060f,
    0.710920f, 0.759570f, 0.827430f, 0.857600f, 0.806730f,
    0.702560f, 0.560780f, 0.384890f, 0.251890f, 0.149100f,
    0.090586f, 0.053560f, 0.042300f, 0.031948f, 0.026033f,
    0.018909f, 0.015748f, 0.010347f, 0.010878f, 0.008875f,
    0.007382f, 0.005871f, 0.004988f, 0.003601f, 0.002395f,
    0.001931f, 0.001396f, 0.000892f
};

// Canon 60D spectral sensitivity (normalized)
// Wavelength range: 400–720 nm, step 10 nm

static constexpr int   C60D_SSF_SAMPLES = 33;

static constexpr Float C60D_SSF_BANDS[C60D_SSF_SAMPLES] = {
    400.f, 410.f, 420.f, 430.f, 440.f, 450.f, 460.f, 470.f, 480.f, 490.f,
    500.f, 510.f, 520.f, 530.f, 540.f, 550.f, 560.f, 570.f, 580.f, 590.f,
    600.f, 610.f, 620.f, 630.f, 640.f, 650.f, 660.f, 670.f, 680.f, 690.f,
    700.f, 710.f, 720.f
};

static constexpr Float C60D_red_ssf[C60D_SSF_SAMPLES] = {
    0.003694f, 0.005789f, 0.011972f, 0.006010f, 0.003506f,
    0.003014f, 0.003971f, 0.006949f, 0.016477f, 0.024973f,
    0.033583f, 0.052786f, 0.063128f, 0.080597f, 0.099936f,
    0.158630f, 0.199190f, 0.292000f, 0.444020f, 0.547200f,
    0.518150f, 0.510120f, 0.402940f, 0.353470f, 0.276200f,
    0.234350f, 0.172640f, 0.131780f, 0.054995f, 0.009011f,
    0.002072f, 0.000538f, 0.000146f
};

static constexpr Float C60D_green_ssf[C60D_SSF_SAMPLES] = {
    0.004016f, 0.010547f, 0.044575f, 0.050839f, 0.070745f,
    0.091766f, 0.127860f, 0.270330f, 0.570340f, 0.621030f,
    0.811230f, 0.933340f, 0.879250f, 1.000000f, 0.900320f,
    0.898210f, 0.752410f, 0.718610f, 0.565570f, 0.439870f,
    0.273290f, 0.160990f, 0.077993f, 0.051902f, 0.034045f,
    0.025987f, 0.018665f, 0.018169f, 0.010452f, 0.002445f,
    0.000711f, 0.000218f, 0.000083f
};

static constexpr Float C60D_blue_ssf[C60D_SSF_SAMPLES] = {
    0.021702f, 0.097758f, 0.498980f, 0.659170f, 0.696250f,
    0.809490f, 0.854470f, 0.794600f, 0.720310f, 0.621000f,
    0.526680f, 0.397330f, 0.219160f, 0.155690f, 0.104950f,
    0.080773f, 0.050105f, 0.041103f, 0.033563f, 0.027940f,
    0.019429f, 0.014083f, 0.008888f, 0.007996f, 0.007504f,
    0.008546f, 0.008251f, 0.008116f, 0.003858f, 0.000747f,
    0.000202f, 0.000075f, 0.000050f
};

namespace pbrt {

// PixelSensor Definition
class PixelSensor {
  public:
    // PixelSensor Public Methods
    static PixelSensor *Create(const ParameterDictionary &parameters,
                               const RGBColorSpace *colorSpace, Float exposureTime,
                               const FileLoc *loc, Allocator alloc);

    static PixelSensor *CreateDefault(Allocator alloc = {});

    PixelSensor(Spectrum r, Spectrum g, Spectrum b, const RGBColorSpace *outputColorSpace,
                Spectrum sensorIllum, Float imagingRatio, Allocator alloc)
        : r_bar(r, alloc), g_bar(g, alloc), b_bar(b, alloc), imagingRatio(imagingRatio) {
        // Compute XYZ from camera RGB matrix
        // Compute _rgbCamera_ values for training swatches
        Float rgbCamera[nSwatchReflectances][3];
        for (int i = 0; i < nSwatchReflectances; ++i) {
            RGB rgb = ProjectReflectance<RGB>(swatchReflectances[i], sensorIllum, &r_bar,
                                              &g_bar, &b_bar);
            for (int c = 0; c < 3; ++c)
                rgbCamera[i][c] = rgb[c];
        }

        // Compute _xyzOutput_ values for training swatches
        Float xyzOutput[24][3];
        Float sensorWhiteG = InnerProduct(sensorIllum, &g_bar);
        Float sensorWhiteY = InnerProduct(sensorIllum, &Spectra::Y());
        for (size_t i = 0; i < nSwatchReflectances; ++i) {
            Spectrum s = swatchReflectances[i];
            XYZ xyz =
                ProjectReflectance<XYZ>(s, &outputColorSpace->illuminant, &Spectra::X(),
                                        &Spectra::Y(), &Spectra::Z()) *
                (sensorWhiteY / sensorWhiteG);
            for (int c = 0; c < 3; ++c)
                xyzOutput[i][c] = xyz[c];
        }

        // Initialize _XYZFromSensorRGB_ using linear least squares
        pstd::optional<SquareMatrix<3>> m =
            LinearLeastSquares(rgbCamera, xyzOutput, nSwatchReflectances);
        if (!m)
            ErrorExit("Sensor XYZ from RGB matrix could not be solved.");
        XYZFromSensorRGB = *m;
    }

    PixelSensor(const RGBColorSpace *outputColorSpace, Spectrum sensorIllum,
                Float imagingRatio, Allocator alloc)
        : r_bar(&Spectra::X(), alloc),
          g_bar(&Spectra::Y(), alloc),
          b_bar(&Spectra::Z(), alloc),
          imagingRatio(imagingRatio) {
        // Compute white balancing matrix for XYZ _PixelSensor_
        if (sensorIllum) {
            Point2f sourceWhite = SpectrumToXYZ(sensorIllum).xy();
            Point2f targetWhite = outputColorSpace->w;
            XYZFromSensorRGB = WhiteBalance(sourceWhite, targetWhite);
        }
    }

    PBRT_CPU_GPU
    RGB ToSensorRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
        L = SafeDiv(L, lambda.PDF());
        return imagingRatio * RGB((r_bar.Sample(lambda) * L).Average(),
                                  (g_bar.Sample(lambda) * L).Average(),
                                  (b_bar.Sample(lambda) * L).Average());
    }

    // PixelSensor Public Members
    SquareMatrix<3> XYZFromSensorRGB;

  private:
    // PixelSensor Private Methods
    template <typename Triplet>
    static Triplet ProjectReflectance(Spectrum r, Spectrum illum, Spectrum b1,
                                      Spectrum b2, Spectrum b3);

    // PixelSensor Private Members
    DenselySampledSpectrum r_bar, g_bar, b_bar;
    Float imagingRatio;
    static constexpr int nSwatchReflectances = 24;
    static Spectrum swatchReflectances[nSwatchReflectances];
};

// PixelSensor Inline Methods
template <typename Triplet>
inline Triplet PixelSensor::ProjectReflectance(Spectrum refl, Spectrum illum, Spectrum b1,
                                               Spectrum b2, Spectrum b3) {
    Triplet result;
    Float g_integral = 0;
    for (Float lambda = Lambda_min; lambda <= Lambda_max; ++lambda) {
        g_integral += b2(lambda) * illum(lambda);
        result[0] += b1(lambda) * refl(lambda) * illum(lambda);
        result[1] += b2(lambda) * refl(lambda) * illum(lambda);
        result[2] += b3(lambda) * refl(lambda) * illum(lambda);
    }
    return result / g_integral;
}

// VisibleSurface Definition
class VisibleSurface {
  public:
    // VisibleSurface Public Methods
    PBRT_CPU_GPU
    VisibleSurface(const SurfaceInteraction &si, SampledSpectrum albedo,
                   const SampledWavelengths &lambda);

    PBRT_CPU_GPU
    operator bool() const { return set; }

    VisibleSurface() = default;

    std::string ToString() const;

    // VisibleSurface Public Members
    Point3f p;
    Normal3f n, ns;
    Point2f uv;
    Float time = 0;
    Vector3f dpdx, dpdy;
    SampledSpectrum albedo;
    bool set = false;
};

// FilmBaseParameters Definition
struct FilmBaseParameters {
    FilmBaseParameters(const ParameterDictionary &parameters, Filter filter,
                       const PixelSensor *sensor, const FileLoc *loc);
    FilmBaseParameters(Point2i fullResolution, Bounds2i pixelBounds, Filter filter,
                       Float diagonal, const PixelSensor *sensor, std::string filename)
        : fullResolution(fullResolution),
          pixelBounds(pixelBounds),
          filter(filter),
          diagonal(diagonal),
          sensor(sensor),
          filename(filename) {}

    Point2i fullResolution;
    Bounds2i pixelBounds;
    Filter filter;
    Float diagonal;
    const PixelSensor *sensor;
    std::string filename;
};

// FilmBase Definition
class FilmBase {
  public:
    // FilmBase Public Methods
    FilmBase(FilmBaseParameters p)
        : fullResolution(p.fullResolution),
          pixelBounds(p.pixelBounds),
          filter(p.filter),
          diagonal(p.diagonal * .001f),
          sensor(p.sensor),
          filename(p.filename) {
        CHECK(!pixelBounds.IsEmpty());
        CHECK_GE(pixelBounds.pMin.x, 0);
        CHECK_LE(pixelBounds.pMax.x, fullResolution.x);
        CHECK_GE(pixelBounds.pMin.y, 0);
        CHECK_LE(pixelBounds.pMax.y, fullResolution.y);
        LOG_VERBOSE("Created film with full resolution %s, pixelBounds %s",
                    fullResolution, pixelBounds);
    }

    PBRT_CPU_GPU
    Point2i FullResolution() const { return fullResolution; }
    PBRT_CPU_GPU
    Bounds2i PixelBounds() const { return pixelBounds; }
    PBRT_CPU_GPU
    Float Diagonal() const { return diagonal; }
    PBRT_CPU_GPU
    Filter GetFilter() const { return filter; }
    PBRT_CPU_GPU
    const PixelSensor *GetPixelSensor() const { return sensor; }
    std::string GetFilename() const { return filename; }

    PBRT_CPU_GPU
    SampledWavelengths SampleWavelengths(Float u) const {
        return SampledWavelengths::SampleVisible(u);
    }

    PBRT_CPU_GPU
    Bounds2f SampleBounds() const;

    std::string BaseToString() const;

  protected:
    // FilmBase Protected Members
    Point2i fullResolution;
    Bounds2i pixelBounds;
    Filter filter;
    Float diagonal;
    const PixelSensor *sensor;
    std::string filename;
};

// RGBFilm Definition
class RGBFilm : public FilmBase {
  public:
    // RGBFilm Public Methods
    PBRT_CPU_GPU
    bool UsesVisibleSurface() const { return false; }

    PBRT_CPU_GPU
    void AddSample(Point2i pFilm, SampledSpectrum L, const SampledWavelengths &lambda,
                   const VisibleSurface *, Float weight) {
        // Convert sample radiance to _PixelSensor_ RGB
        RGB rgb = sensor->ToSensorRGB(L, lambda);

        // Optionally clamp sensor RGB value
        Float m = std::max({rgb.r, rgb.g, rgb.b});
        if (m > maxComponentValue)
            rgb *= maxComponentValue / m;

        DCHECK(InsideExclusive(pFilm, pixelBounds));
        // Update pixel values with filtered sample contribution
        Pixel &pixel = pixels[pFilm];
        for (int c = 0; c < 3; ++c)
            pixel.rgbSum[c] += weight * rgb[c];
        pixel.weightSum += weight;
    }

    PBRT_CPU_GPU
    RGB GetPixelRGB(Point2i p, Float splatScale = 1) const {
        const Pixel &pixel = pixels[p];
        RGB rgb(pixel.rgbSum[0], pixel.rgbSum[1], pixel.rgbSum[2]);
        // Normalize _rgb_ with weight sum
        Float weightSum = pixel.weightSum;
        if (weightSum != 0)
            rgb /= weightSum;

        // Add splat value at pixel
        for (int c = 0; c < 3; ++c)
            rgb[c] += splatScale * pixel.rgbSplat[c] / filterIntegral;

        // Convert _rgb_ to output RGB color space
        rgb = outputRGBFromSensorRGB * rgb;

        return rgb;
    }

    RGBFilm(FilmBaseParameters p, const RGBColorSpace *colorSpace,
            Float maxComponentValue = Infinity, bool writeFP16 = true,
            Allocator alloc = {});

    static RGBFilm *Create(const ParameterDictionary &parameters, Float exposureTime,
                           Filter filter, const RGBColorSpace *colorSpace,
                           const FileLoc *loc, Allocator alloc);

    PBRT_CPU_GPU
    void AddSplat(Point2f p, SampledSpectrum v, const SampledWavelengths &lambda);

    void WriteImage(ImageMetadata metadata, Float splatScale = 1);
    Image GetImage(ImageMetadata *metadata, Float splatScale = 1);

    std::string ToString() const;

    PBRT_CPU_GPU
    RGB ToOutputRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
        RGB sensorRGB = sensor->ToSensorRGB(L, lambda);
        return outputRGBFromSensorRGB * sensorRGB;
    }

    PBRT_CPU_GPU void ResetPixel(Point2i p) { memset(&pixels[p], 0, sizeof(Pixel)); }

  private:
    // RGBFilm::Pixel Definition
    struct Pixel {
        Pixel() = default;
        double rgbSum[3] = {0., 0., 0.};
        double weightSum = 0.;
        AtomicDouble rgbSplat[3];
    };

    // RGBFilm Private Members
    const RGBColorSpace *colorSpace;
    Float maxComponentValue;
    bool writeFP16;
    Float filterIntegral;
    SquareMatrix<3> outputRGBFromSensorRGB;
    Array2D<Pixel> pixels;
};

// GBufferFilm Definition
class GBufferFilm : public FilmBase {
  public:
    // GBufferFilm Public Methods
    GBufferFilm(FilmBaseParameters p, const AnimatedTransform &outputFromRender,
                bool applyInverse, const RGBColorSpace *colorSpace,
                Float maxComponentValue = Infinity, bool writeFP16 = true,
                Allocator alloc = {});

    static GBufferFilm *Create(const ParameterDictionary &parameters, Float exposureTime,
                               const CameraTransform &cameraTransform, Filter filter,
                               const RGBColorSpace *colorSpace, const FileLoc *loc,
                               Allocator alloc);

    PBRT_CPU_GPU
    void AddSample(Point2i pFilm, SampledSpectrum L, const SampledWavelengths &lambda,
                   const VisibleSurface *visibleSurface, Float weight);

    PBRT_CPU_GPU
    void AddSplat(Point2f p, SampledSpectrum v, const SampledWavelengths &lambda);

    PBRT_CPU_GPU
    RGB ToOutputRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
        RGB cameraRGB = sensor->ToSensorRGB(L, lambda);
        return outputRGBFromSensorRGB * cameraRGB;
    }

    PBRT_CPU_GPU
    bool UsesVisibleSurface() const { return true; }

    PBRT_CPU_GPU
    RGB GetPixelRGB(Point2i p, Float splatScale = 1) const {
        const Pixel &pixel = pixels[p];
        RGB rgb(pixel.rgbSum[0], pixel.rgbSum[1], pixel.rgbSum[2]);

        // Normalize pixel with weight sum
        Float weightSum = pixel.weightSum;
        if (weightSum != 0)
            rgb /= weightSum;

        // Add splat value at pixel
        for (int c = 0; c < 3; ++c)
            rgb[c] += splatScale * pixel.rgbSplat[c] / filterIntegral;

        rgb = outputRGBFromSensorRGB * rgb;

        return rgb;
    }

    void WriteImage(ImageMetadata metadata, Float splatScale = 1);
    Image GetImage(ImageMetadata *metadata, Float splatScale = 1);

    std::string ToString() const;

    PBRT_CPU_GPU void ResetPixel(Point2i p) { memset(&pixels[p], 0, sizeof(Pixel)); }

  private:
    // GBufferFilm::Pixel Definition
    struct Pixel {
        Pixel() = default;
        double rgbSum[3] = {0., 0., 0.};
        double weightSum = 0., gBufferWeightSum = 0.;
        AtomicDouble rgbSplat[3];
        Point3f pSum;
        Float dzdxSum = 0, dzdySum = 0;
        Normal3f nSum, nsSum;
        Point2f uvSum;
        double rgbAlbedoSum[3] = {0., 0., 0.};
        VarianceEstimator<Float> rgbVariance[3];
    };

    // GBufferFilm Private Members
    AnimatedTransform outputFromRender;
    bool applyInverse;
    Array2D<Pixel> pixels;
    const RGBColorSpace *colorSpace;
    Float maxComponentValue;
    bool writeFP16;
    Float filterIntegral;
    SquareMatrix<3> outputRGBFromSensorRGB;
};

// SpectralFilm Definition
class SpectralFilm : public FilmBase {
  public:
    // SpectralFilm Public Methods
    PBRT_CPU_GPU
    bool UsesVisibleSurface() const { return false; }

    PBRT_CPU_GPU
    SampledWavelengths SampleWavelengths(Float u) const {
        return SampledWavelengths::SampleUniform(u, lambdaMin, lambdaMax);
    }

    PBRT_CPU_GPU
    void AddSample(Point2i pFilm, SampledSpectrum L, const SampledWavelengths &lambda,
                   const VisibleSurface *, Float weight) {
        // Start by doing more or less what RGBFilm::AddSample() does so
        // that we can maintain accurate RGB values.

        // Convert sample radiance to _PixelSensor_ RGB
        RGB rgb = sensor->ToSensorRGB(L, lambda);

        // Optionally clamp sensor RGB value
        Float m = std::max({rgb.r, rgb.g, rgb.b});
        if (m > maxComponentValue)
            rgb *= maxComponentValue / m;

        DCHECK(InsideExclusive(pFilm, pixelBounds));
        // Update RGB fields in Pixel structure.
        Pixel &pixel = pixels[pFilm];
        for (int c = 0; c < 3; ++c)
            pixel.rgbSum[c] += weight * rgb[c];
        pixel.rgbWeightSum += weight;

        // Spectral processing starts here.
        // Optionally clamp spectral value. (TODO: for spectral should we
        // just clamp channels individually?)
        Float lm = L.MaxComponentValue();
        if (lm > maxComponentValue)
            L *= maxComponentValue / lm;

        // The CIE_Y_integral factor effectively cancels out the effect of
        // the conversion of light sources to use photometric units for
        // specification.  We then do *not* divide by the PDF in |lambda|
        // but take advantage of the fact that we know that it is uniform
        // in SampleWavelengths(), the fact that the buckets all have the
        // same extend, and can then just average radiance in buckets
        // below.
        L *= weight * CIE_Y_integral;

        // Accumulate contributions in spectral buckets.
        for (int i = 0; i < NSpectrumSamples; ++i) {
            int b = LambdaToBucket(lambda[i]);
            pixel.bucketSums[b] += L[i];
            pixel.weightSums[b] += weight;
        }
    }

    PBRT_CPU_GPU
    RGB GetPixelRGB(Point2i p, Float splatScale = 1) const;

    SpectralFilm(FilmBaseParameters p, Float lambdaMin, Float lambdaMax, int nBuckets,
                 const RGBColorSpace *colorSpace, Float maxComponentValue = Infinity,
                 bool writeFP16 = true, Allocator alloc = {});

    static SpectralFilm *Create(const ParameterDictionary &parameters, Float exposureTime,
                                Filter filter, const RGBColorSpace *colorSpace,
                                const FileLoc *loc, Allocator alloc);

    PBRT_CPU_GPU
    void AddSplat(Point2f p, SampledSpectrum v, const SampledWavelengths &lambda);

    void WriteImage(ImageMetadata metadata, Float splatScale = 1);

    // Returns an image with both RGB and spectral components, following
    // the layout proposed in "An OpenEXR Layout for Sepctral Images" by
    // Fichet et al., https://jcgt.org/published/0010/03/01/.
    Image GetImage(ImageMetadata *metadata, Float splatScale = 1);

    std::string ToString() const;

    PBRT_CPU_GPU
    RGB ToOutputRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
        LOG_FATAL("ToOutputRGB() is unimplemented. But that's ok since it's only used "
                  "in the SPPM integrator, which is inherently very much based on "
                  "RGB output.");
        return {};
    }

    PBRT_CPU_GPU void ResetPixel(Point2i p) {
        Pixel &pix = pixels[p];
        pix.rgbSum[0] = pix.rgbSum[1] = pix.rgbSum[2] = 0.;
        pix.rgbWeightSum = 0.;
        pix.rgbSplat[0] = pix.rgbSplat[1] = pix.rgbSplat[2] = 0.;
        memset(pix.bucketSums, 0, nBuckets * sizeof(double));
        memset(pix.weightSums, 0, nBuckets * sizeof(double));
        memset(pix.bucketSplats, 0, nBuckets * sizeof(AtomicDouble));
    }

  private:
    PBRT_CPU_GPU
    int LambdaToBucket(Float lambda) const {
        DCHECK_RARE(1e6f, lambda < lambdaMin || lambda > lambdaMax);
        int bucket = nBuckets * (lambda - lambdaMin) / (lambdaMax - lambdaMin);
        return Clamp(bucket, 0, nBuckets - 1);
    }

    // SpectralFilm::Pixel Definition
    struct Pixel {
        Pixel() = default;
        // Continue to store RGB, both to include in the final image as
        // well as for previews during rendering.
        double rgbSum[3] = {0., 0., 0.};
        double rgbWeightSum = 0.;
        AtomicDouble rgbSplat[3];
        // The following will all have nBuckets entries.
        double *bucketSums, *weightSums;
        AtomicDouble *bucketSplats;
    };

    // SpectralFilm Private Members
    const RGBColorSpace *colorSpace;
    Float lambdaMin, lambdaMax;
    int nBuckets;
    Float maxComponentValue;
    bool writeFP16;
    Float filterIntegral;
    Array2D<Pixel> pixels;
    SquareMatrix<3> outputRGBFromSensorRGB;
};

class ColorFilterArrayFilm : public FilmBase {
  public:
    // ColorFilterArrayFilm Public Methods
    PBRT_CPU_GPU
    bool UsesVisibleSurface() const { return false; }

    PBRT_CPU_GPU
    SampledWavelengths SampleWavelengths(Float u) const {
        return SampledWavelengths::SampleUniform(u, lambdaMin, lambdaMax);
    }

    PBRT_CPU_GPU
    void AddSample(Point2i pFilm, SampledSpectrum L, const SampledWavelengths &lambda,
                   const VisibleSurface *, Float weight) {
        // Start by doing more or less what RGBFilm::AddSample() does so
        // that we can maintain accurate RGB values.

        //Modular arithmetic and weighting curve application
        const bool isInBlueMosaic = (pFilm.x % 2 == 0) && (pFilm.y % 2 == 0);
        const bool isInRedMosaic = (pFilm.x % 2 == 1) && (pFilm.y % 2 == 1);
        const bool isInGreenMosaic = (pFilm.x % 2) != (pFilm.y % 2); //Grün ist eine exclusive XOR Schaltung

        // Apply general per-pixel spectral response
        if (isInRedMosaic) {
            ApplySpectralResponse(L, lambda,
                                C60D_SSF_BANDS,
                                C60D_red_ssf,
                                C60D_SSF_SAMPLES);
        } else if (isInGreenMosaic) {
            ApplySpectralResponse(L, lambda,
                                C60D_SSF_BANDS,
                                C60D_green_ssf,
                                C60D_SSF_SAMPLES);
        } else if (isInBlueMosaic) {
            ApplySpectralResponse(L, lambda,
                                C60D_SSF_BANDS,
                                C60D_blue_ssf,
                                C60D_SSF_SAMPLES);
        }
        // Convert sample radiance to _PixelSensor_ RGB
        RGB rgb = sensor->ToSensorRGB(L, lambda); 

        // Optionally clamp sensor RGB value
        Float m = std::max({rgb.r, rgb.g, rgb.b});
        if (m > maxComponentValue)
            rgb *= maxComponentValue / m;

        DCHECK(InsideExclusive(pFilm, pixelBounds));
        // Update RGB fields in Pixel structure.
        Pixel &pixel = pixels[pFilm];
        Float outR = 0, outG = 0, outB = 0;

        if (isInRedMosaic) {
            outR = rgb.r;          // red pixel
        }
        if (isInGreenMosaic) {
            outG = rgb.g;          // green pixel
        }
        if (isInBlueMosaic) {
            outB = rgb.b;          // blue pixel
        }

        // Accumulate only that one channel
        pixel.rgbSum[0] += weight * outR;
        pixel.rgbSum[1] += weight * outG;
        pixel.rgbSum[2] += weight * outB;
        pixel.weightSum += weight;
    }

    PBRT_CPU_GPU
    RGB GetPixelRGB(Point2i p, Float splatScale = 1) const;

    ColorFilterArrayFilm(FilmBaseParameters p, Float lambdaMin, Float lambdaMax, int nBuckets,
                 const RGBColorSpace *colorSpace, Float maxComponentValue = Infinity,
                 bool writeFP16 = true, Allocator alloc = {});

    static ColorFilterArrayFilm *Create(const ParameterDictionary &parameters, Float exposureTime,
                                Filter filter, const RGBColorSpace *colorSpace,
                                const FileLoc *loc, Allocator alloc);

    PBRT_CPU_GPU
    void AddSplat(Point2f p, SampledSpectrum v, const SampledWavelengths &lambda);

    void WriteImage(ImageMetadata metadata, Float splatScale = 1);

    // Returns an image with both RGB and spectral components, following
    // the layout proposed in "An OpenEXR Layout for Sepctral Images" by
    // Fichet et al., https://jcgt.org/published/0010/03/01/.
    Image GetImage(ImageMetadata *metadata, Float splatScale = 1);

    std::string ToString() const;

    PBRT_CPU_GPU
    RGB ToOutputRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
        LOG_FATAL("ToOutputRGB() is unimplemented. But that's ok since it's only used "
                  "in the SPPM integrator, which is inherently very much based on "
                  "RGB output.");
        return {};
    }

    PBRT_CPU_GPU void ResetPixel(Point2i p) {
        Pixel &pix = pixels[p];
        pix.rgbSum[0] = pix.rgbSum[1] = pix.rgbSum[2] = 0.;
        pix.rgbWeightSum = 0.;
        pix.rgbSplat[0] = pix.rgbSplat[1] = pix.rgbSplat[2] = 0.;
        memset(pix.bucketSums, 0, nBuckets * sizeof(double));
        memset(pix.weightSums, 0, nBuckets * sizeof(double));
        memset(pix.bucketSplats, 0, nBuckets * sizeof(AtomicDouble));
    }

  private:
    PBRT_CPU_GPU
    int LambdaToBucket(Float lambda) const {
        DCHECK_RARE(1e6f, lambda < lambdaMin || lambda > lambdaMax);
        int bucket = nBuckets * (lambda - lambdaMin) / (lambdaMax - lambdaMin);
        return Clamp(bucket, 0, nBuckets - 1);
    }

    // ColorFilterArrayFilm::Pixel Definition
    struct Pixel {
        Pixel() = default;
        // Continue to store RGB, both to include in the final image as
        // well as for previews during rendering.
        double rgbSum[3] = {0., 0., 0.};
        double rgbWeightSum = 0.;
        AtomicDouble rgbSplat[3];
        // The following will all have nBuckets entries.
        double *bucketSums, *weightSums;
        AtomicDouble *bucketSplats;
    };

    // ColorFilterArrayFilm Private Members
    const RGBColorSpace *colorSpace;
    Float lambdaMin, lambdaMax;
    int nBuckets;
    Float maxComponentValue;
    bool writeFP16;
    Float filterIntegral;
    Array2D<Pixel> pixels;
    SquareMatrix<3> outputRGBFromSensorRGB;
};

PBRT_CPU_GPU
inline SampledWavelengths Film::SampleWavelengths(Float u) const {
    auto sample = [&](auto ptr) { return ptr->SampleWavelengths(u); };
    return Dispatch(sample);
}

PBRT_CPU_GPU
inline Bounds2f Film::SampleBounds() const {
    auto sb = [&](auto ptr) { return ptr->SampleBounds(); };
    return Dispatch(sb);
}

PBRT_CPU_GPU
inline Bounds2i Film::PixelBounds() const {
    auto pb = [&](auto ptr) { return ptr->PixelBounds(); };
    return Dispatch(pb);
}

PBRT_CPU_GPU
inline Point2i Film::FullResolution() const {
    auto fr = [&](auto ptr) { return ptr->FullResolution(); };
    return Dispatch(fr);
}

PBRT_CPU_GPU
inline Float Film::Diagonal() const {
    auto diag = [&](auto ptr) { return ptr->Diagonal(); };
    return Dispatch(diag);
}

PBRT_CPU_GPU
inline Filter Film::GetFilter() const {
    auto filter = [&](auto ptr) { return ptr->GetFilter(); };
    return Dispatch(filter);
}

PBRT_CPU_GPU
inline bool Film::UsesVisibleSurface() const {
    auto uses = [&](auto ptr) { return ptr->UsesVisibleSurface(); };
    return Dispatch(uses);
}

PBRT_CPU_GPU
inline RGB Film::GetPixelRGB(Point2i p, Float splatScale) const {
    auto get = [&](auto ptr) { return ptr->GetPixelRGB(p, splatScale); };
    return Dispatch(get);
}

PBRT_CPU_GPU
inline RGB Film::ToOutputRGB(SampledSpectrum L, const SampledWavelengths &lambda) const {
    auto out = [&](auto ptr) { return ptr->ToOutputRGB(L, lambda); };
    return Dispatch(out);
}

PBRT_CPU_GPU
inline void Film::AddSample(Point2i pFilm, SampledSpectrum L,
                            const SampledWavelengths &lambda,
                            const VisibleSurface *visibleSurface, Float weight) {
    auto add = [&](auto ptr) {
        return ptr->AddSample(pFilm, L, lambda, visibleSurface, weight);
    };
    return Dispatch(add);
}

PBRT_CPU_GPU
inline const PixelSensor *Film::GetPixelSensor() const {
    auto filter = [&](auto ptr) { return ptr->GetPixelSensor(); };
    return Dispatch(filter);
}

PBRT_CPU_GPU
inline void Film::ResetPixel(Point2i p) {
    auto rp = [&](auto ptr) { ptr->ResetPixel(p); };
    return Dispatch(rp);
}

}  // namespace pbrt

#endif  // PBRT_FILM_H
