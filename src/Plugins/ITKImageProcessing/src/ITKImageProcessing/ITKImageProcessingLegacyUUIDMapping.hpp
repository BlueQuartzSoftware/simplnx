#pragma once

#include <map>
#include <string>

// clang-format off
#include "ITKImageProcessing/Filters/ITKAbsImage.hpp"
#include "ITKImageProcessing/Filters/ITKAcosImage.hpp"
#include "ITKImageProcessing/Filters/ITKAdaptiveHistogramEqualizationImage.hpp"
#include "ITKImageProcessing/Filters/ITKAsinImage.hpp"
#include "ITKImageProcessing/Filters/ITKAtanImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryContourImage.hpp"
#include "ITKImageProcessing/Filters/ITKBinaryThresholdImage.hpp"
#include "ITKImageProcessing/Filters/ITKClosingByReconstructionImage.hpp"
#include "ITKImageProcessing/Filters/ITKCosImage.hpp"
#include "ITKImageProcessing/Filters/ITKGradientMagnitudeImage.hpp"
//#include "ITKImageProcessing/Filters/ITKGradientMagnitudeRecursiveGaussianImage.hpp"
#include "ITKImageProcessing/Filters/ITKGrayscaleFillholeImage.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Filters/ITKImageWriter.hpp"
#include "ITKImageProcessing/Filters/ITKImportImageStack.hpp"
#include "ITKImageProcessing/Filters/ITKInvertIntensityImage.hpp"
#include "ITKImageProcessing/Filters/ITKLog10Image.hpp"
#include "ITKImageProcessing/Filters/ITKLogImage.hpp"
#include "ITKImageProcessing/Filters/ITKMaskImage.hpp"
#include "ITKImageProcessing/Filters/ITKMedianImage.hpp"
#include "ITKImageProcessing/Filters/ITKMorphologicalWatershedImage.hpp"
#include "ITKImageProcessing/Filters/ITKNormalizeImage.hpp"
#include "ITKImageProcessing/Filters/ITKOpeningByReconstructionImage.hpp"
#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp"
#include "ITKImageProcessing/Filters/ITKSignedMaurerDistanceMapImage.hpp"
#include "ITKImageProcessing/Filters/ITKSinImage.hpp"
#include "ITKImageProcessing/Filters/ITKSqrtImage.hpp"
#include "ITKImageProcessing/Filters/ITKTanImage.hpp"

// @@__HEADER__TOKEN__DO__NOT__DELETE__@@

namespace complex
{
  static const std::map<complex::Uuid, complex::Uuid> k_SIMPL_to_ITKImageProcessing
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID}, // dream3d-class-name
    {complex::Uuid::FromString("09f45c29-1cfb-566c-b3ae-d832b4f95905").value(), complex::FilterTraits<ITKAbsImage>::uuid}, // ITKAbsImage
    {complex::Uuid::FromString("b09ec654-87a5-5dfa-9949-aa69f1fbfdd1").value(), complex::FilterTraits<ITKAcosImage>::uuid}, // ITKAcosImage
    {complex::Uuid::FromString("2d5a7599-5e01-5489-a107-23b704d2b5eb").value(), complex::FilterTraits<ITKAdaptiveHistogramEqualizationImage>::uuid}, // ITKAdaptiveHistogramEqualizationImage
    {complex::Uuid::FromString("79509ab1-24e1-50e4-9351-c5ce7cd87a72").value(), complex::FilterTraits<ITKAsinImage>::uuid}, // ITKAsinImage
    {complex::Uuid::FromString("e938569d-3644-5d00-a4e0-ab327937457d").value(), complex::FilterTraits<ITKAtanImage>::uuid}, // ITKAtanImage
    {complex::Uuid::FromString("3c451ac9-bfef-5e41-bae9-3957a0fc26a1").value(), complex::FilterTraits<ITKBinaryContourImage>::uuid}, // ITKBinaryContourImage
    {complex::Uuid::FromString("ba8a3f2e-3963-57c0-a8da-239e25de0526").value(), complex::FilterTraits<ITKBinaryThresholdImage>::uuid}, // ITKBinaryThresholdImage
    {complex::Uuid::FromString("99a7aa3c-f945-5e77-875a-23b5231ab3f4").value(), complex::FilterTraits<ITKClosingByReconstructionImage>::uuid}, // ITKClosingByReconstructionImage
    {complex::Uuid::FromString("2c2d7bf6-1e78-52e6-80aa-58b504ce0912").value(), complex::FilterTraits<ITKCosImage>::uuid}, // ITKCosImage
    {complex::Uuid::FromString("3aa99151-e722-51a0-90ba-71e93347ab09").value(), complex::FilterTraits<ITKGradientMagnitudeImage>::uuid}, // ITKGradientMagnitudeImage
    {complex::Uuid::FromString("54c8dd45-88c4-5d4b-8a39-e3cc595e1cf8").value(), complex::FilterTraits<ITKGrayscaleFillholeImage>::uuid}, // ITKGrayscaleFillholeImage
    {complex::Uuid::FromString("653b7b5c-03cb-5b32-8c3e-3637745e5ff6").value(), complex::FilterTraits<ITKImageReader>::uuid}, // ITKImageReader
    {complex::Uuid::FromString("11473711-f94d-5d96-b749-ec36a81ad338").value(), complex::FilterTraits<ITKImageWriter>::uuid}, // ITKImageWriter
    {complex::Uuid::FromString("cf7d7497-9573-5102-bedd-38f86a6cdfd4").value(), complex::FilterTraits<ITKImportImageStack>::uuid}, // ITKImportImageStack
    {complex::Uuid::FromString("c6e10fa5-5462-546b-b34b-0f0ea75a7e43").value(), complex::FilterTraits<ITKInvertIntensityImage>::uuid}, // ITKInvertIntensityImage
    {complex::Uuid::FromString("dbfd1a57-2a17-572d-93a7-8fd2f8e92eb0").value(), complex::FilterTraits<ITKLog10Image>::uuid}, // ITKLog10Image
    {complex::Uuid::FromString("69aba77c-9a35-5251-a18a-e3728ddd2963").value(), complex::FilterTraits<ITKLogImage>::uuid}, // ITKLogImage
    {complex::Uuid::FromString("97102d65-9c32-576a-9177-c59d958bad10").value(), complex::FilterTraits<ITKMaskImage>::uuid}, // ITKMaskImage
    {complex::Uuid::FromString("cc27ee9a-9946-56ad-afd4-6e98b71f417d").value(), complex::FilterTraits<ITKMedianImage>::uuid}, // ITKMedianImage
    {complex::Uuid::FromString("b2248340-a371-5899-90a2-86047950f0a2").value(), complex::FilterTraits<ITKMorphologicalWatershedImage>::uuid}, // ITKMorphologicalWatershedImage
    {complex::Uuid::FromString("5b905619-c46b-5690-b6fa-8e97cf4537b8").value(), complex::FilterTraits<ITKNormalizeImage>::uuid}, // ITKNormalizeImage
    {complex::Uuid::FromString("ca04004f-fb11-588d-9f77-d00b3ee9ad2a").value(), complex::FilterTraits<ITKOpeningByReconstructionImage>::uuid}, // ITKOpeningByReconstructionImage
    {complex::Uuid::FromString("6e66563a-edcf-5e11-bc1d-ceed36d8493f").value(), complex::FilterTraits<ITKOtsuMultipleThresholdsImage>::uuid}, // ITKOtsuMultipleThresholdsImage
    {complex::Uuid::FromString("bb15d42a-3077-582a-be1a-76b2bae172e9").value(), complex::FilterTraits<ITKSignedMaurerDistanceMapImage>::uuid}, // ITKSignedMaurerDistanceMapImage
    {complex::Uuid::FromString("1eb4b4f7-1704-58e4-9f78-8726a5c8c302").value(), complex::FilterTraits<ITKSinImage>::uuid}, // ITKSinImage
    {complex::Uuid::FromString("8087dcad-68f2-598b-9670-d0f57647a445").value(), complex::FilterTraits<ITKSqrtImage>::uuid}, // ITKSqrtImage
    {complex::Uuid::FromString("672810d9-5ec0-59c1-a209-8fb56c7a018a").value(), complex::FilterTraits<ITKTanImage>::uuid}, // ITKTanImage
    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
  };

} // namespace complex
// clang-format on
