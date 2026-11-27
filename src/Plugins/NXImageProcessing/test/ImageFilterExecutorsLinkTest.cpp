#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <type_traits>

using namespace nx::core;

TEST_CASE("NXImageProcessing::ImageFilterExecutors: Exported functions link", "[NXImageProcessing][ImageFilterExecutors]")
{
  auto* volatile maskImageFilterPtr = &ImageProcessing::ExecuteMaskImageFilter;
  REQUIRE(maskImageFilterPtr != nullptr);
  auto* volatile adaptiveHistogramEqualizationImageFilterPtr = &ImageProcessing::ExecuteAdaptiveHistogramEqualizationImageFilter;
  REQUIRE(adaptiveHistogramEqualizationImageFilterPtr != nullptr);
  auto* volatile morphologyImageFilterPtr = &ImageProcessing::ExecuteMorphologyImageFilter;
  REQUIRE(morphologyImageFilterPtr != nullptr);
  auto* volatile objectMorphologyImageFilterPtr = &ImageProcessing::ExecuteObjectMorphologyImageFilter;
  REQUIRE(objectMorphologyImageFilterPtr != nullptr);
  auto* volatile binaryMorphologyImageFilterPtr = &ImageProcessing::ExecuteBinaryMorphologyImageFilter;
  REQUIRE(binaryMorphologyImageFilterPtr != nullptr);
  auto* volatile morphologyCompositeImageFilterPtr = &ImageProcessing::ExecuteMorphologyCompositeImageFilter;
  REQUIRE(morphologyCompositeImageFilterPtr != nullptr);
  auto* volatile binaryMorphologyCompositeImageFilterPtr = &ImageProcessing::ExecuteBinaryMorphologyCompositeImageFilter;
  REQUIRE(binaryMorphologyCompositeImageFilterPtr != nullptr);
  auto* volatile morphologicalReconstructionImageFilterPtr = &ImageProcessing::ExecuteMorphologicalReconstructionImageFilter;
  REQUIRE(morphologicalReconstructionImageFilterPtr != nullptr);
  auto* volatile reconstructionOpenCloseImageFilterPtr = &ImageProcessing::ExecuteReconstructionOpenCloseImageFilter;
  REQUIRE(reconstructionOpenCloseImageFilterPtr != nullptr);
  auto* volatile binaryOpeningByReconstructionImageFilterPtr = &ImageProcessing::ExecuteBinaryOpeningByReconstructionImageFilter;
  REQUIRE(binaryOpeningByReconstructionImageFilterPtr != nullptr);
  auto* volatile valuedRegionalExtremaImageFilterPtr = &ImageProcessing::ExecuteValuedRegionalExtremaImageFilter;
  REQUIRE(valuedRegionalExtremaImageFilterPtr != nullptr);
  auto* volatile regionalExtremaImageFilterPtr = &ImageProcessing::ExecuteRegionalExtremaImageFilter;
  REQUIRE(regionalExtremaImageFilterPtr != nullptr);
  auto* volatile signedMaurerDistanceMapImageFilterPtr = &ImageProcessing::ExecuteSignedMaurerDistanceMapImageFilter;
  REQUIRE(signedMaurerDistanceMapImageFilterPtr != nullptr);
  auto* volatile danielssonDistanceMapImageFilterPtr = &ImageProcessing::ExecuteDanielssonDistanceMapImageFilter;
  REQUIRE(danielssonDistanceMapImageFilterPtr != nullptr);
  auto* volatile signedDanielssonDistanceMapImageFilterPtr = &ImageProcessing::ExecuteSignedDanielssonDistanceMapImageFilter;
  REQUIRE(signedDanielssonDistanceMapImageFilterPtr != nullptr);
  auto* volatile isoContourDistanceImageFilterPtr = &ImageProcessing::ExecuteIsoContourDistanceImageFilter;
  REQUIRE(isoContourDistanceImageFilterPtr != nullptr);
  auto* volatile gradientMagnitudeRecursiveGaussianImageFilterPtr = &ImageProcessing::ExecuteGradientMagnitudeRecursiveGaussianImageFilter;
  REQUIRE(gradientMagnitudeRecursiveGaussianImageFilterPtr != nullptr);
  auto* volatile gradientMagnitudeImageFilterPtr = &ImageProcessing::ExecuteGradientMagnitudeImageFilter;
  REQUIRE(gradientMagnitudeImageFilterPtr != nullptr);
  auto* volatile laplacianRecursiveGaussianImageFilterPtr = &ImageProcessing::ExecuteLaplacianRecursiveGaussianImageFilter;
  REQUIRE(laplacianRecursiveGaussianImageFilterPtr != nullptr);
  auto* volatile smoothingRecursiveGaussianImageFilterPtr = &ImageProcessing::ExecuteSmoothingRecursiveGaussianImageFilter;
  REQUIRE(smoothingRecursiveGaussianImageFilterPtr != nullptr);
  auto* volatile discreteGaussianImageFilterPtr = &ImageProcessing::ExecuteDiscreteGaussianImageFilter;
  REQUIRE(discreteGaussianImageFilterPtr != nullptr);
  auto* volatile approximateSignedDistanceMapImageFilterPtr = &ImageProcessing::ExecuteApproximateSignedDistanceMapImageFilter;
  REQUIRE(approximateSignedDistanceMapImageFilterPtr != nullptr);
  auto* volatile connectedComponentImageFilterPtr = &ImageProcessing::ExecuteConnectedComponentImageFilter;
  REQUIRE(connectedComponentImageFilterPtr != nullptr);
  auto* volatile zeroCrossingImageFilterPtr = &ImageProcessing::ExecuteZeroCrossingImageFilter;
  REQUIRE(zeroCrossingImageFilterPtr != nullptr);
  auto* volatile doubleThresholdImageFilterPtr = &ImageProcessing::ExecuteDoubleThresholdImageFilter;
  REQUIRE(doubleThresholdImageFilterPtr != nullptr);
  auto* volatile relabelComponentImageFilterPtr = &ImageProcessing::ExecuteRelabelComponentImageFilter;
  REQUIRE(relabelComponentImageFilterPtr != nullptr);
  auto* volatile binaryThinningImageFilterPtr = &ImageProcessing::ExecuteBinaryThinningImageFilter;
  REQUIRE(binaryThinningImageFilterPtr != nullptr);
  auto* volatile thresholdMaximumConnectedComponentsImageFilterPtr = &ImageProcessing::ExecuteThresholdMaximumConnectedComponentsImageFilter;
  REQUIRE(thresholdMaximumConnectedComponentsImageFilterPtr != nullptr);
  using MarkerExecutor = Result<> (*)(DataStructure&, const DataPath&, const DataPath&, const DataPath&, const DataPath&, bool, bool, const std::atomic_bool&, const IFilter::MessageHandler&,
                                      ImageProcessing::ConnectedComponentLabelLimitHandling);
  static_assert(std::is_same_v<decltype(&ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter), MarkerExecutor>);
  MarkerExecutor volatile morphologicalWatershedFromMarkersImageFilterPtr = &ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter;
  REQUIRE(morphologicalWatershedFromMarkersImageFilterPtr != nullptr);
  auto* volatile morphologicalWatershedImageFilterPtr = &ImageProcessing::ExecuteMorphologicalWatershedImageFilter;
  REQUIRE(morphologicalWatershedImageFilterPtr != nullptr);
}
