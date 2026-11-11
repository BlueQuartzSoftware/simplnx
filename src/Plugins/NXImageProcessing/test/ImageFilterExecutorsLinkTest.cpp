#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("NXImageProcessing::ImageFilterExecutors: Exported functions link", "[NXImageProcessing][ImageFilterExecutors]")
{
  auto* volatile maskImageFilter = &ImageProcessing::ExecuteMaskImageFilter;
  REQUIRE(maskImageFilter != nullptr);
  auto* volatile adaptiveHistogramEqualizationImageFilter = &ImageProcessing::ExecuteAdaptiveHistogramEqualizationImageFilter;
  REQUIRE(adaptiveHistogramEqualizationImageFilter != nullptr);
  auto* volatile morphologyImageFilter = &ImageProcessing::ExecuteMorphologyImageFilter;
  REQUIRE(morphologyImageFilter != nullptr);
  auto* volatile objectMorphologyImageFilter = &ImageProcessing::ExecuteObjectMorphologyImageFilter;
  REQUIRE(objectMorphologyImageFilter != nullptr);
  auto* volatile binaryMorphologyImageFilter = &ImageProcessing::ExecuteBinaryMorphologyImageFilter;
  REQUIRE(binaryMorphologyImageFilter != nullptr);
  auto* volatile morphologyCompositeImageFilter = &ImageProcessing::ExecuteMorphologyCompositeImageFilter;
  REQUIRE(morphologyCompositeImageFilter != nullptr);
  auto* volatile binaryMorphologyCompositeImageFilter = &ImageProcessing::ExecuteBinaryMorphologyCompositeImageFilter;
  REQUIRE(binaryMorphologyCompositeImageFilter != nullptr);
  auto* volatile morphologicalReconstructionImageFilter = &ImageProcessing::ExecuteMorphologicalReconstructionImageFilter;
  REQUIRE(morphologicalReconstructionImageFilter != nullptr);
  auto* volatile reconstructionOpenCloseImageFilter = &ImageProcessing::ExecuteReconstructionOpenCloseImageFilter;
  REQUIRE(reconstructionOpenCloseImageFilter != nullptr);
  auto* volatile binaryOpeningByReconstructionImageFilter = &ImageProcessing::ExecuteBinaryOpeningByReconstructionImageFilter;
  REQUIRE(binaryOpeningByReconstructionImageFilter != nullptr);
  auto* volatile valuedRegionalExtremaImageFilter = &ImageProcessing::ExecuteValuedRegionalExtremaImageFilter;
  REQUIRE(valuedRegionalExtremaImageFilter != nullptr);
  auto* volatile regionalExtremaImageFilter = &ImageProcessing::ExecuteRegionalExtremaImageFilter;
  REQUIRE(regionalExtremaImageFilter != nullptr);
  auto* volatile signedMaurerDistanceMapImageFilter = &ImageProcessing::ExecuteSignedMaurerDistanceMapImageFilter;
  REQUIRE(signedMaurerDistanceMapImageFilter != nullptr);
  auto* volatile danielssonDistanceMapImageFilter = &ImageProcessing::ExecuteDanielssonDistanceMapImageFilter;
  REQUIRE(danielssonDistanceMapImageFilter != nullptr);
  auto* volatile signedDanielssonDistanceMapImageFilter = &ImageProcessing::ExecuteSignedDanielssonDistanceMapImageFilter;
  REQUIRE(signedDanielssonDistanceMapImageFilter != nullptr);
  auto* volatile isoContourDistanceImageFilter = &ImageProcessing::ExecuteIsoContourDistanceImageFilter;
  REQUIRE(isoContourDistanceImageFilter != nullptr);
  auto* volatile gradientMagnitudeRecursiveGaussianImageFilter = &ImageProcessing::ExecuteGradientMagnitudeRecursiveGaussianImageFilter;
  REQUIRE(gradientMagnitudeRecursiveGaussianImageFilter != nullptr);
  auto* volatile gradientMagnitudeImageFilter = &ImageProcessing::ExecuteGradientMagnitudeImageFilter;
  REQUIRE(gradientMagnitudeImageFilter != nullptr);
  auto* volatile laplacianRecursiveGaussianImageFilter = &ImageProcessing::ExecuteLaplacianRecursiveGaussianImageFilter;
  REQUIRE(laplacianRecursiveGaussianImageFilter != nullptr);
  auto* volatile smoothingRecursiveGaussianImageFilter = &ImageProcessing::ExecuteSmoothingRecursiveGaussianImageFilter;
  REQUIRE(smoothingRecursiveGaussianImageFilter != nullptr);
  auto* volatile discreteGaussianImageFilter = &ImageProcessing::ExecuteDiscreteGaussianImageFilter;
  REQUIRE(discreteGaussianImageFilter != nullptr);
  auto* volatile approximateSignedDistanceMapImageFilter = &ImageProcessing::ExecuteApproximateSignedDistanceMapImageFilter;
  REQUIRE(approximateSignedDistanceMapImageFilter != nullptr);
  auto* volatile connectedComponentImageFilter = &ImageProcessing::ExecuteConnectedComponentImageFilter;
  REQUIRE(connectedComponentImageFilter != nullptr);
  auto* volatile zeroCrossingImageFilter = &ImageProcessing::ExecuteZeroCrossingImageFilter;
  REQUIRE(zeroCrossingImageFilter != nullptr);
  auto* volatile doubleThresholdImageFilter = &ImageProcessing::ExecuteDoubleThresholdImageFilter;
  REQUIRE(doubleThresholdImageFilter != nullptr);
  auto* volatile relabelComponentImageFilter = &ImageProcessing::ExecuteRelabelComponentImageFilter;
  REQUIRE(relabelComponentImageFilter != nullptr);
  auto* volatile binaryThinningImageFilter = &ImageProcessing::ExecuteBinaryThinningImageFilter;
  REQUIRE(binaryThinningImageFilter != nullptr);
  auto* volatile thresholdMaximumConnectedComponentsImageFilter = &ImageProcessing::ExecuteThresholdMaximumConnectedComponentsImageFilter;
  REQUIRE(thresholdMaximumConnectedComponentsImageFilter != nullptr);
  auto* volatile morphologicalWatershedFromMarkersImageFilter = &ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter;
  REQUIRE(morphologicalWatershedFromMarkersImageFilter != nullptr);
  auto* volatile morphologicalWatershedImageFilter = &ImageProcessing::ExecuteMorphologicalWatershedImageFilter;
  REQUIRE(morphologicalWatershedImageFilter != nullptr);
}
