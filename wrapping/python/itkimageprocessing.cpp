#include <CxPybind/CxPybind.hpp>

#include <ITKImageProcessing/ITKImageProcessingConfig.hpp>
#include <ITKImageProcessing/ITKImageProcessingPlugin.hpp>

#include <ITKImageProcessing/Filters/ITKDiscreteGaussianImage.hpp>
#include <ITKImageProcessing/Filters/ITKImageReader.hpp>
#include <ITKImageProcessing/Filters/ITKImageWriter.hpp>
#include <ITKImageProcessing/Filters/ITKImportImageStack.hpp>
#include <ITKImageProcessing/Filters/ITKMedianImage.hpp>
#include <ITKImageProcessing/Filters/ITKMhaFileReader.hpp>
#include <ITKImageProcessing/Filters/ITKRescaleIntensityImage.hpp>

#ifndef ITKIMAGEPROCESSING_LEAN_AND_MEAN
#include <ITKImageProcessing/Filters/ITKAbsImage.hpp>
#include <ITKImageProcessing/Filters/ITKAcosImage.hpp>
#include <ITKImageProcessing/Filters/ITKAdaptiveHistogramEqualizationImage.hpp>
#include <ITKImageProcessing/Filters/ITKAsinImage.hpp>
#include <ITKImageProcessing/Filters/ITKAtanImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryContourImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryDilateImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryErodeImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryMorphologicalClosingImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryMorphologicalOpeningImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryOpeningByReconstructionImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryProjectionImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryThinningImage.hpp>
#include <ITKImageProcessing/Filters/ITKBinaryThresholdImage.hpp>
#include <ITKImageProcessing/Filters/ITKBlackTopHatImage.hpp>
#include <ITKImageProcessing/Filters/ITKClosingByReconstructionImage.hpp>
#include <ITKImageProcessing/Filters/ITKCosImage.hpp>
#include <ITKImageProcessing/Filters/ITKDilateObjectMorphologyImage.hpp>
#include <ITKImageProcessing/Filters/ITKErodeObjectMorphologyImage.hpp>
#include <ITKImageProcessing/Filters/ITKExpImage.hpp>
#include <ITKImageProcessing/Filters/ITKExpNegativeImage.hpp>
#include <ITKImageProcessing/Filters/ITKGradientMagnitudeImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleDilateImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleErodeImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleFillholeImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleGrindPeakImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleMorphologicalClosingImage.hpp>
#include <ITKImageProcessing/Filters/ITKGrayscaleMorphologicalOpeningImage.hpp>
#include <ITKImageProcessing/Filters/ITKHConvexImage.hpp>
#include <ITKImageProcessing/Filters/ITKHMaximaImage.hpp>
#include <ITKImageProcessing/Filters/ITKHMinimaImage.hpp>
#include <ITKImageProcessing/Filters/ITKIntensityWindowingImage.hpp>
#include <ITKImageProcessing/Filters/ITKInvertIntensityImage.hpp>
#include <ITKImageProcessing/Filters/ITKLabelContourImage.hpp>
#include <ITKImageProcessing/Filters/ITKLog10Image.hpp>
#include <ITKImageProcessing/Filters/ITKLogImage.hpp>
#include <ITKImageProcessing/Filters/ITKMaskImage.hpp>
#include <ITKImageProcessing/Filters/ITKMorphologicalGradientImage.hpp>
#include <ITKImageProcessing/Filters/ITKMorphologicalWatershedImage.hpp>
#include <ITKImageProcessing/Filters/ITKNormalizeImage.hpp>
#include <ITKImageProcessing/Filters/ITKNotImage.hpp>
#include <ITKImageProcessing/Filters/ITKOpeningByReconstructionImage.hpp>
#include <ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp>
#include <ITKImageProcessing/Filters/ITKRelabelComponentImage.hpp>
#include <ITKImageProcessing/Filters/ITKSigmoidImage.hpp>
#include <ITKImageProcessing/Filters/ITKSignedMaurerDistanceMapImage.hpp>
#include <ITKImageProcessing/Filters/ITKSinImage.hpp>
#include <ITKImageProcessing/Filters/ITKSqrtImage.hpp>
#include <ITKImageProcessing/Filters/ITKSquareImage.hpp>
#include <ITKImageProcessing/Filters/ITKTanImage.hpp>
#include <ITKImageProcessing/Filters/ITKThresholdImage.hpp>
#include <ITKImageProcessing/Filters/ITKValuedRegionalMaximaImage.hpp>
#include <ITKImageProcessing/Filters/ITKValuedRegionalMinimaImage.hpp>
#include <ITKImageProcessing/Filters/ITKWhiteTopHatImage.hpp>
#endif

using namespace complex;
using namespace complex::CxPybind;
namespace py = pybind11;

PYBIND11_MODULE(itkimageprocessing, mod)
{
  py::module_::import("complex");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<ITKImageProcessingPlugin>();

  BindFilter<ITKDiscreteGaussianImage>(mod, internals);
  BindFilter<ITKImageReader>(mod, internals);
  BindFilter<ITKImageWriter>(mod, internals);
  BindFilter<ITKImportImageStack>(mod, internals);
  BindFilter<ITKMedianImage>(mod, internals);
  BindFilter<ITKMhaFileReader>(mod, internals);
  BindFilter<ITKRescaleIntensityImage>(mod, internals);

#ifndef ITKIMAGEPROCESSING_LEAN_AND_MEAN
  BindFilter<ITKAbsImage>(mod, internals);
  BindFilter<ITKAcosImage>(mod, internals);
  BindFilter<ITKAdaptiveHistogramEqualizationImage>(mod, internals);
  BindFilter<ITKAsinImage>(mod, internals);
  BindFilter<ITKAtanImage>(mod, internals);
  BindFilter<ITKBinaryContourImage>(mod, internals);
  BindFilter<ITKBinaryDilateImage>(mod, internals);
  BindFilter<ITKBinaryErodeImage>(mod, internals);
  BindFilter<ITKBinaryMorphologicalClosingImage>(mod, internals);
  BindFilter<ITKBinaryMorphologicalOpeningImage>(mod, internals);
  BindFilter<ITKBinaryOpeningByReconstructionImage>(mod, internals);
  BindFilter<ITKBinaryProjectionImage>(mod, internals);
  BindFilter<ITKBinaryThinningImage>(mod, internals);
  BindFilter<ITKBinaryThresholdImage>(mod, internals);
  BindFilter<ITKBlackTopHatImage>(mod, internals);
  BindFilter<ITKClosingByReconstructionImage>(mod, internals);
  BindFilter<ITKCosImage>(mod, internals);
  BindFilter<ITKDilateObjectMorphologyImage>(mod, internals);
  BindFilter<ITKErodeObjectMorphologyImage>(mod, internals);
  BindFilter<ITKExpImage>(mod, internals);
  BindFilter<ITKExpNegativeImage>(mod, internals);
  BindFilter<ITKGradientMagnitudeImage>(mod, internals);
  BindFilter<ITKGrayscaleDilateImage>(mod, internals);
  BindFilter<ITKGrayscaleErodeImage>(mod, internals);
  BindFilter<ITKGrayscaleFillholeImage>(mod, internals);
  BindFilter<ITKGrayscaleGrindPeakImage>(mod, internals);
  BindFilter<ITKGrayscaleMorphologicalClosingImage>(mod, internals);
  BindFilter<ITKGrayscaleMorphologicalOpeningImage>(mod, internals);
  BindFilter<ITKHConvexImage>(mod, internals);
  BindFilter<ITKHMaximaImage>(mod, internals);
  BindFilter<ITKHMinimaImage>(mod, internals);
  BindFilter<ITKIntensityWindowingImage>(mod, internals);
  BindFilter<ITKInvertIntensityImage>(mod, internals);
  BindFilter<ITKLabelContourImage>(mod, internals);
  BindFilter<ITKLog10Image>(mod, internals);
  BindFilter<ITKLogImage>(mod, internals);
  BindFilter<ITKMaskImage>(mod, internals);
  BindFilter<ITKMorphologicalGradientImage>(mod, internals);
  BindFilter<ITKMorphologicalWatershedImage>(mod, internals);
  BindFilter<ITKNormalizeImage>(mod, internals);
  BindFilter<ITKNotImage>(mod, internals);
  BindFilter<ITKOpeningByReconstructionImage>(mod, internals);
  BindFilter<ITKOtsuMultipleThresholdsImage>(mod, internals);
  BindFilter<ITKRelabelComponentImage>(mod, internals);
  BindFilter<ITKSigmoidImage>(mod, internals);
  BindFilter<ITKSignedMaurerDistanceMapImage>(mod, internals);
  BindFilter<ITKSinImage>(mod, internals);
  BindFilter<ITKSqrtImage>(mod, internals);
  BindFilter<ITKSquareImage>(mod, internals);
  BindFilter<ITKTanImage>(mod, internals);
  BindFilter<ITKThresholdImage>(mod, internals);
  BindFilter<ITKValuedRegionalMaximaImage>(mod, internals);
  BindFilter<ITKValuedRegionalMinimaImage>(mod, internals);
  BindFilter<ITKWhiteTopHatImage>(mod, internals);
#endif

  internals.registerPluginPyFilters(*plugin);
}