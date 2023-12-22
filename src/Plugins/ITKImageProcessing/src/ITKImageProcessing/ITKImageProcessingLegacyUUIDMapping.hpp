#pragma once

#include "simplnx/Plugin/AbstractPlugin.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <string>

#include "ITKImageProcessing/ITKImageProcessingConfig.hpp"

#include "ITKImageProcessing/Filters/ITKDiscreteGaussianImage.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Filters/ITKImageWriter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"
#include "ITKImageProcessing/Filters/ITKMedianImage.hpp"
#include "ITKImageProcessing/Filters/ITKRescaleIntensityImage.hpp"

// clang-format off
#ifndef ITKIMAGEPROCESSING_LEAN_AND_MEAN
#ifndef SIMPLNX_CONDA_BUILD
#include "ITKImageProcessing/Filters/ITKBinaryContourImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryOpeningByReconstructionImage.hpp"
#include "ITKImageProcessing/Filters/ITKClosingByReconstructionImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleFillholeImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleGrindPeakImage.hpp"
#include "ITKImageProcessing/Filters/ITKHConvexImage.hpp"
#include "ITKImageProcessing/Filters/ITKHMaximaImage.hpp"
#include "ITKImageProcessing/Filters/ITKHMinimaImage.hpp"
#include "ITKImageProcessing/Filters/ITKLabelContourImage.hpp"
#include "ITKImageProcessing/Filters/ITKMorphologicalGradientImage.hpp"
#include "ITKImageProcessing/Filters/ITKMorphologicalWatershedImage.hpp"
#include "ITKImageProcessing/Filters/ITKOpeningByReconstructionImage.hpp"
#include "ITKImageProcessing/Filters/ITKSignedMaurerDistanceMapImage.hpp"
#include "ITKImageProcessing/Filters/ITKValuedRegionalMaximaImage.hpp"
#include "ITKImageProcessing/Filters/ITKValuedRegionalMinimaImage.hpp"
#endif
#include "ITKImageProcessing/Filters/ITKAbsImage.hpp"
#include "ITKImageProcessing/Filters/ITKAcosImage.hpp"
#include "ITKImageProcessing/Filters/ITKAdaptiveHistogramEqualizationImage.hpp"
#include "ITKImageProcessing/Filters/ITKAsinImage.hpp"
#include "ITKImageProcessing/Filters/ITKAtanImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryThresholdImage.hpp"
#include "ITKImageProcessing/Filters/ITKCosImage.hpp"
#include "ITKImageProcessing/Filters/ITKGradientMagnitudeImage.hpp"
#include "ITKImageProcessing/Filters/ITKInvertIntensityImage.hpp"
#include "ITKImageProcessing/Filters/ITKLog10Image.hpp"
#include "ITKImageProcessing/Filters/ITKLogImage.hpp"
#include "ITKImageProcessing/Filters/ITKMaskImage.hpp"
#include "ITKImageProcessing/Filters/ITKNormalizeImage.hpp"
#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp"
#include "ITKImageProcessing/Filters/ITKSinImage.hpp"
#include "ITKImageProcessing/Filters/ITKSqrtImage.hpp"
#include "ITKImageProcessing/Filters/ITKTanImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryDilateImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryErodeImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryMorphologicalOpeningImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryProjectionImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryThinningImage.hpp"
#include "ITKImageProcessing/Filters/ITKBlackTopHatImage.hpp"
#include "ITKImageProcessing/Filters/ITKDilateObjectMorphologyImage.hpp"
#include "ITKImageProcessing/Filters/ITKErodeObjectMorphologyImage.hpp"
#include "ITKImageProcessing/Filters/ITKExpImage.hpp"
#include "ITKImageProcessing/Filters/ITKExpNegativeImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleDilateImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleErodeImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleMorphologicalClosingImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleMorphologicalOpeningImage.hpp"
#include "ITKImageProcessing/Filters/ITKIntensityWindowingImage.hpp"
#include "ITKImageProcessing/Filters/ITKNotImage.hpp"
#include "ITKImageProcessing/Filters/ITKRelabelComponentImage.hpp"
#include "ITKImageProcessing/Filters/ITKSigmoidImage.hpp"
#include "ITKImageProcessing/Filters/ITKSquareImage.hpp"
#include "ITKImageProcessing/Filters/ITKThresholdImage.hpp"
#include "ITKImageProcessing/Filters/ITKWhiteTopHatImage.hpp"
// @@__HEADER__TOKEN__DO__NOT__DELETE__@@
#endif

namespace nx::core
{
  static const AbstractPlugin::SIMPLMapType k_SIMPL_to_ITKImageProcessing
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID}, // dream3d-class-name
    {nx::core::Uuid::FromString("53df5340-f632-598f-8a9b-802296b3a95c").value(), {nx::core::FilterTraits<ITKDiscreteGaussianImage>::uuid, &ITKDiscreteGaussianImage::FromSIMPLJson}}, // ITKDiscreteGaussianImage
    {nx::core::Uuid::FromString("653b7b5c-03cb-5b32-8c3e-3637745e5ff6").value(), {nx::core::FilterTraits<ITKImageReader>::uuid, &ITKImageReader::FromSIMPLJson}}, // ITKImageReader
    {nx::core::Uuid::FromString("11473711-f94d-5d96-b749-ec36a81ad338").value(), {nx::core::FilterTraits<ITKImageWriter>::uuid, &ITKImageWriter::FromSIMPLJson}}, // ITKImageWriter
    {nx::core::Uuid::FromString("cf7d7497-9573-5102-bedd-38f86a6cdfd4").value(), {nx::core::FilterTraits<ITKImportImageStack>::uuid, &ITKImportImageStack::FromSIMPLJson}}, // ITKImportImageStack
    {nx::core::Uuid::FromString("cc27ee9a-9946-56ad-afd4-6e98b71f417d").value(), {nx::core::FilterTraits<ITKMedianImage>::uuid, &ITKMedianImage::FromSIMPLJson}}, // ITKMedianImage
    {nx::core::Uuid::FromString("77bf2192-851d-5127-9add-634c1ef4f67f").value(), {nx::core::FilterTraits<ITKRescaleIntensityImage>::uuid, &ITKRescaleIntensityImage::FromSIMPLJson}}, // ITKRescaleIntensityImage


#ifndef ITKIMAGEPROCESSING_LEAN_AND_MEAN

#ifndef SIMPLNX_CONDA_BUILD
    {nx::core::Uuid::FromString("3c451ac9-bfef-5e41-bae9-3957a0fc26a1").value(), {nx::core::FilterTraits<ITKBinaryContourImage>::uuid, &ITKBinaryContourImage::FromSIMPLJson}}, // ITKBinaryContourImage
    {nx::core::Uuid::FromString("bd1c2353-0a39-52c0-902b-ee64721994c7").value(), {nx::core::FilterTraits<ITKBinaryOpeningByReconstructionImage>::uuid, &ITKBinaryOpeningByReconstructionImage::FromSIMPLJson}}, // ITKBinaryOpeningByReconstructionImage
    {nx::core::Uuid::FromString("99a7aa3c-f945-5e77-875a-23b5231ab3f4").value(), {nx::core::FilterTraits<ITKClosingByReconstructionImage>::uuid, &ITKClosingByReconstructionImage::FromSIMPLJson}}, // ITKClosingByReconstructionImage
    {nx::core::Uuid::FromString("54c8dd45-88c4-5d4b-8a39-e3cc595e1cf8").value(), {nx::core::FilterTraits<ITKGrayscaleFillholeImage>::uuid, &ITKGrayscaleFillholeImage::FromSIMPLJson}}, // ITKGrayscaleFillholeImage
    {nx::core::Uuid::FromString("d910551f-4eec-55c9-b0ce-69c2277e61bd").value(), {nx::core::FilterTraits<ITKGrayscaleGrindPeakImage>::uuid, &ITKGrayscaleGrindPeakImage::FromSIMPLJson}}, // ITKGrayscaleGrindPeakImage
    {nx::core::Uuid::FromString("8bc34707-04c0-5e83-8583-48ee19306a1d").value(), {nx::core::FilterTraits<ITKHConvexImage>::uuid, &ITKHConvexImage::FromSIMPLJson}}, // ITKHConvexImage
    {nx::core::Uuid::FromString("932a6df4-212e-53a1-a2ab-c29bd376bb7b").value(), {nx::core::FilterTraits<ITKHMaximaImage>::uuid, &ITKHMaximaImage::FromSIMPLJson}}, // ITKHMaximaImage
    {nx::core::Uuid::FromString("f1d7cf59-9b7c-53cb-b71a-76cf91c86e8f").value(), {nx::core::FilterTraits<ITKHMinimaImage>::uuid, &ITKHMinimaImage::FromSIMPLJson}}, // ITKHMinimaImage
    {nx::core::Uuid::FromString("668f0b90-b504-5fba-b648-7c9677e1f452").value(), {nx::core::FilterTraits<ITKLabelContourImage>::uuid, &ITKLabelContourImage::FromSIMPLJson}}, // ITKLabelContourImage
    {nx::core::Uuid::FromString("12c83608-c4c5-5c72-b22f-a7696e3f5448").value(), {nx::core::FilterTraits<ITKMorphologicalGradientImage>::uuid, &ITKMorphologicalGradientImage::FromSIMPLJson}}, // ITKMorphologicalGradientImage
    {nx::core::Uuid::FromString("b2248340-a371-5899-90a2-86047950f0a2").value(), {nx::core::FilterTraits<ITKMorphologicalWatershedImage>::uuid, &ITKMorphologicalWatershedImage::FromSIMPLJson}}, // ITKMorphologicalWatershedImage
    {nx::core::Uuid::FromString("ca04004f-fb11-588d-9f77-d00b3ee9ad2a").value(), {nx::core::FilterTraits<ITKOpeningByReconstructionImage>::uuid, &ITKOpeningByReconstructionImage::FromSIMPLJson}}, // ITKOpeningByReconstructionImage
    {nx::core::Uuid::FromString("bb15d42a-3077-582a-be1a-76b2bae172e9").value(), {nx::core::FilterTraits<ITKSignedMaurerDistanceMapImage>::uuid, &ITKSignedMaurerDistanceMapImage::FromSIMPLJson}}, // ITKSignedMaurerDistanceMapImage
    {nx::core::Uuid::FromString("10aff542-81c5-5f09-9797-c7171c40b6a0").value(), {nx::core::FilterTraits<ITKValuedRegionalMaximaImage>::uuid, &ITKValuedRegionalMaximaImage::FromSIMPLJson}}, // ITKValuedRegionalMaximaImage
    {nx::core::Uuid::FromString("739a0908-cb60-50f7-a484-b2157d023093").value(), {nx::core::FilterTraits<ITKValuedRegionalMinimaImage>::uuid, &ITKValuedRegionalMinimaImage::FromSIMPLJson}}, // ITKValuedRegionalMinimaImage
#endif

    {nx::core::Uuid::FromString("09f45c29-1cfb-566c-b3ae-d832b4f95905").value(), {nx::core::FilterTraits<ITKAbsImage>::uuid, &ITKAbsImage::FromSIMPLJson}}, // ITKAbsImage
    {nx::core::Uuid::FromString("b09ec654-87a5-5dfa-9949-aa69f1fbfdd1").value(), {nx::core::FilterTraits<ITKAcosImage>::uuid, &ITKAcosImage::FromSIMPLJson}}, // ITKAcosImage
    {nx::core::Uuid::FromString("2d5a7599-5e01-5489-a107-23b704d2b5eb").value(), {nx::core::FilterTraits<ITKAdaptiveHistogramEqualizationImage>::uuid, &ITKAdaptiveHistogramEqualizationImage::FromSIMPLJson}}, // ITKAdaptiveHistogramEqualizationImage
    {nx::core::Uuid::FromString("79509ab1-24e1-50e4-9351-c5ce7cd87a72").value(), {nx::core::FilterTraits<ITKAsinImage>::uuid, &ITKAsinImage::FromSIMPLJson}}, // ITKAsinImage
    {nx::core::Uuid::FromString("e938569d-3644-5d00-a4e0-ab327937457d").value(), {nx::core::FilterTraits<ITKAtanImage>::uuid, &ITKAtanImage::FromSIMPLJson}}, // ITKAtanImage
    {nx::core::Uuid::FromString("ba8a3f2e-3963-57c0-a8da-239e25de0526").value(), {nx::core::FilterTraits<ITKBinaryThresholdImage>::uuid, &ITKBinaryThresholdImage::FromSIMPLJson}}, // ITKBinaryThresholdImage
    {nx::core::Uuid::FromString("2c2d7bf6-1e78-52e6-80aa-58b504ce0912").value(), {nx::core::FilterTraits<ITKCosImage>::uuid, &ITKCosImage::FromSIMPLJson}}, // ITKCosImage
    {nx::core::Uuid::FromString("3aa99151-e722-51a0-90ba-71e93347ab09").value(), {nx::core::FilterTraits<ITKGradientMagnitudeImage>::uuid, &ITKGradientMagnitudeImage::FromSIMPLJson}}, // ITKGradientMagnitudeImage
    {nx::core::Uuid::FromString("c6e10fa5-5462-546b-b34b-0f0ea75a7e43").value(), {nx::core::FilterTraits<ITKInvertIntensityImage>::uuid, &ITKInvertIntensityImage::FromSIMPLJson}}, // ITKInvertIntensityImage
    {nx::core::Uuid::FromString("dbfd1a57-2a17-572d-93a7-8fd2f8e92eb0").value(), {nx::core::FilterTraits<ITKLog10Image>::uuid, &ITKLog10Image::FromSIMPLJson}}, // ITKLog10Image
    {nx::core::Uuid::FromString("69aba77c-9a35-5251-a18a-e3728ddd2963").value(), {nx::core::FilterTraits<ITKLogImage>::uuid, &ITKLogImage::FromSIMPLJson}}, // ITKLogImage
    {nx::core::Uuid::FromString("97102d65-9c32-576a-9177-c59d958bad10").value(), {nx::core::FilterTraits<ITKMaskImage>::uuid, &ITKMaskImage::FromSIMPLJson}}, // ITKMaskImage
    {nx::core::Uuid::FromString("5b905619-c46b-5690-b6fa-8e97cf4537b8").value(), {nx::core::FilterTraits<ITKNormalizeImage>::uuid, &ITKNormalizeImage::FromSIMPLJson}}, // ITKNormalizeImage
    {nx::core::Uuid::FromString("6e66563a-edcf-5e11-bc1d-ceed36d8493f").value(), {nx::core::FilterTraits<ITKOtsuMultipleThresholdsImage>::uuid, &ITKOtsuMultipleThresholdsImage::FromSIMPLJson}}, // ITKOtsuMultipleThresholdsImage
    {nx::core::Uuid::FromString("1eb4b4f7-1704-58e4-9f78-8726a5c8c302").value(), {nx::core::FilterTraits<ITKSinImage>::uuid, &ITKSinImage::FromSIMPLJson}}, // ITKSinImage
    {nx::core::Uuid::FromString("8087dcad-68f2-598b-9670-d0f57647a445").value(), {nx::core::FilterTraits<ITKSqrtImage>::uuid, &ITKSqrtImage::FromSIMPLJson}}, // ITKSqrtImage
    {nx::core::Uuid::FromString("672810d9-5ec0-59c1-a209-8fb56c7a018a").value(), {nx::core::FilterTraits<ITKTanImage>::uuid, &ITKTanImage::FromSIMPLJson}}, // ITKTanImage
    {nx::core::Uuid::FromString("f86167ad-a1a1-557b-97ea-92a3618baa8f").value(), {nx::core::FilterTraits<ITKBinaryDilateImage>::uuid, &ITKBinaryDilateImage::FromSIMPLJson}}, // ITKBinaryDilateImage
    {nx::core::Uuid::FromString("522c5249-c048-579a-98dd-f7aadafc5578").value(), {nx::core::FilterTraits<ITKBinaryErodeImage>::uuid, &ITKBinaryErodeImage::FromSIMPLJson}}, // ITKBinaryErodeImage
    {nx::core::Uuid::FromString("704c801a-7549-54c4-9def-c4bb58d07fd1").value(), {nx::core::FilterTraits<ITKBinaryMorphologicalOpeningImage>::uuid, &ITKBinaryMorphologicalOpeningImage::FromSIMPLJson}}, // ITKBinaryMorphologicalOpeningImage
    {nx::core::Uuid::FromString("606c3700-f793-5852-9a0f-3123bd212447").value(), {nx::core::FilterTraits<ITKBinaryProjectionImage>::uuid, &ITKBinaryProjectionImage::FromSIMPLJson}}, // ITKBinaryProjectionImage
    {nx::core::Uuid::FromString("dcceeb50-5924-5eae-88ea-34793cf545a9").value(), {nx::core::FilterTraits<ITKBinaryThinningImage>::uuid, &ITKBinaryThinningImage::FromSIMPLJson}}, // ITKBinaryThinningImage
    {nx::core::Uuid::FromString("e26e7359-f72c-5924-b42e-dd5dd454a794").value(), {nx::core::FilterTraits<ITKBlackTopHatImage>::uuid, &ITKBlackTopHatImage::FromSIMPLJson}}, // ITKBlackTopHatImage
    {nx::core::Uuid::FromString("dbf29c6d-461c-55e7-a6c4-56477d9da55b").value(), {nx::core::FilterTraits<ITKDilateObjectMorphologyImage>::uuid, &ITKDilateObjectMorphologyImage::FromSIMPLJson}}, // ITKDilateObjectMorphologyImage
    {nx::core::Uuid::FromString("caea0698-4253-518b-ab3f-8ebc140d92ea").value(), {nx::core::FilterTraits<ITKErodeObjectMorphologyImage>::uuid, &ITKErodeObjectMorphologyImage::FromSIMPLJson}}, // ITKErodeObjectMorphologyImage
    {nx::core::Uuid::FromString("a6fb3f3a-6c7a-5dfc-a4f1-75ff1d62c32f").value(), {nx::core::FilterTraits<ITKExpImage>::uuid, &ITKExpImage::FromSIMPLJson}}, // ITKExpImage
    {nx::core::Uuid::FromString("634c2306-c1ee-5a45-a55c-f8286e36999a").value(), {nx::core::FilterTraits<ITKExpNegativeImage>::uuid, &ITKExpNegativeImage::FromSIMPLJson}}, // ITKExpNegativeImage
    {nx::core::Uuid::FromString("66cec151-2950-51f8-8a02-47d3516d8721").value(), {nx::core::FilterTraits<ITKGrayscaleDilateImage>::uuid, &ITKGrayscaleDilateImage::FromSIMPLJson}}, // ITKGrayscaleDilateImage
    {nx::core::Uuid::FromString("aef4e804-3f7a-5dc0-911c-b1f16a393a69").value(), {nx::core::FilterTraits<ITKGrayscaleErodeImage>::uuid, &ITKGrayscaleErodeImage::FromSIMPLJson}}, // ITKGrayscaleErodeImage
    {nx::core::Uuid::FromString("849a1903-5595-5029-bbde-6f4b68b2a25c").value(), {nx::core::FilterTraits<ITKGrayscaleMorphologicalClosingImage>::uuid, &ITKGrayscaleMorphologicalClosingImage::FromSIMPLJson}}, // ITKGrayscaleMorphologicalClosingImage
    {nx::core::Uuid::FromString("c88ac42b-9477-5088-9ec0-862af1e0bb56").value(), {nx::core::FilterTraits<ITKGrayscaleMorphologicalOpeningImage>::uuid, &ITKGrayscaleMorphologicalOpeningImage::FromSIMPLJson}}, // ITKGrayscaleMorphologicalOpeningImage
    {nx::core::Uuid::FromString("4faf4c59-6f29-53af-bc78-5aecffce0e37").value(), {nx::core::FilterTraits<ITKIntensityWindowingImage>::uuid, &ITKIntensityWindowingImage::FromSIMPLJson}}, // ITKIntensityWindowingImage
    {nx::core::Uuid::FromString("c8362fb9-d3ab-55c0-902b-274cc27d9bb8").value(), {nx::core::FilterTraits<ITKNotImage>::uuid, &ITKNotImage::FromSIMPLJson}}, // ITKNotImage
    {nx::core::Uuid::FromString("4398d76d-c9aa-5161-bb48-92dd9daaa352").value(), {nx::core::FilterTraits<ITKRelabelComponentImage>::uuid, &ITKRelabelComponentImage::FromSIMPLJson}}, // ITKRelabelComponentImage
    {nx::core::Uuid::FromString("e6675be7-e98d-5e0f-a088-ba15cc301038").value(), {nx::core::FilterTraits<ITKSigmoidImage>::uuid, &ITKSigmoidImage::FromSIMPLJson}}, // ITKSigmoidImage
    {nx::core::Uuid::FromString("f092420e-14a0-5dc0-91f8-de0082103aef").value(), {nx::core::FilterTraits<ITKSquareImage>::uuid, &ITKSquareImage::FromSIMPLJson}}, // ITKSquareImage
    {nx::core::Uuid::FromString("5845ee06-5c8a-5a74-80fb-c820bd8dfb75").value(), {nx::core::FilterTraits<ITKThresholdImage>::uuid, &ITKThresholdImage::FromSIMPLJson}}, // ITKThresholdImage
    {nx::core::Uuid::FromString("02e059f7-8055-52b4-9d48-915b67d1e39a").value(), {nx::core::FilterTraits<ITKWhiteTopHatImage>::uuid, &ITKWhiteTopHatImage::FromSIMPLJson}}, // ITKWhiteTopHatImage
    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
#endif
  };
} // namespace nx::core
  // clang-format on
