#pragma once

#include "ImageProcessing/ImageProcessingLegacyUuids.hpp"

#include "ImageProcessing/Filters/BinaryContourImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryOpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/ClosingByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleFillholeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleGrindPeakImageFilter.hpp"
#include "ImageProcessing/Filters/HConvexImageFilter.hpp"
#include "ImageProcessing/Filters/HMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/HMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/ImportFijiMontageFilter.hpp"
#include "ImageProcessing/Filters/LabelContourImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalGradientImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedImageFilter.hpp"
#include "ImageProcessing/Filters/OpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "ImageProcessing/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "ImageProcessing/Filters/SignedMaurerDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/WriteImageFilter.hpp"

#ifndef ImageProcessing_LEAN_AND_MEAN

#include "ImageProcessing/Filters/AbsImageFilter.hpp"
#include "ImageProcessing/Filters/AcosImageFilter.hpp"
#include "ImageProcessing/Filters/AdaptiveHistogramEqualizationImageFilter.hpp"
#include "ImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/AsinImageFilter.hpp"
#include "ImageProcessing/Filters/AtanImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryDilateImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryErodeImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThinningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/BlackTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/BoundedReciprocalImageFilter.hpp"
#include "ImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "ImageProcessing/Filters/CosImageFilter.hpp"
#include "ImageProcessing/Filters/CurvatureAnisotropicDiffusionImageFilter.hpp"
#include "ImageProcessing/Filters/CurvatureFlowImageFilter.hpp"
#include "ImageProcessing/Filters/DanielssonDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/DilateObjectMorphologyImageFilter.hpp"
#include "ImageProcessing/Filters/DiscreteGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/DoubleThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/ErodeObjectMorphologyImageFilter.hpp"
#include "ImageProcessing/Filters/ExpImageFilter.hpp"
#include "ImageProcessing/Filters/ExpNegativeImageFilter.hpp"
#include "ImageProcessing/Filters/GradientAnisotropicDiffusionImageFilter.hpp"
#include "ImageProcessing/Filters/GradientMagnitudeImageFilter.hpp"
#include "ImageProcessing/Filters/GradientMagnitudeRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleDilateImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleErodeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/IntensityWindowingImageFilter.hpp"
#include "ImageProcessing/Filters/InvertIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"
#include "ImageProcessing/Filters/LaplacianRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/Log10ImageFilter.hpp"
#include "ImageProcessing/Filters/LogImageFilter.hpp"
#include "ImageProcessing/Filters/MaskImageFilter.hpp"
#include "ImageProcessing/Filters/MaximumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MeanProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MedianImageFilter.hpp"
#include "ImageProcessing/Filters/MedianProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MinMaxCurvatureFlowImageFilter.hpp"
#include "ImageProcessing/Filters/MinimumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeToConstantImageFilter.hpp"
#include "ImageProcessing/Filters/NotImageFilter.hpp"
#include "ImageProcessing/Filters/OtsuMultipleThresholdsImageFilter.hpp"
#include "ImageProcessing/Filters/RegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/RegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/RelabelComponentImageFilter.hpp"
#include "ImageProcessing/Filters/RescaleIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/SigmoidImageFilter.hpp"
#include "ImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/SinImageFilter.hpp"
#include "ImageProcessing/Filters/SmoothingRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/SqrtImageFilter.hpp"
#include "ImageProcessing/Filters/SquareImageFilter.hpp"
#include "ImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/SumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/TanImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"
#include "ImageProcessing/Filters/WhiteTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#endif

#include "simplnx/Plugin/AbstractPlugin.hpp"

/* clang-format off */
namespace nx::core
{
static const AbstractPlugin::SIMPLMapType k_SIMPL_to_ImageProcessing
{
  // The entries below come from the retired ITKImageProcessing plugin. That plugin owned these SIMPL
  // Uuids, so this map could not claim them while both plugins loaded. ImageProcessing now owns them.
  // The comment on each line names the DREAM3D 6.x filter class the Uuid belongs to.
  {nx::core::Uuid::FromString("f2259481-5011-5f22-9fcb-c92fb6f8be10").value(), {nx::core::FilterTraits<ReadBinaryCTNorthstarFilter>::uuid, &ReadBinaryCTNorthstarFilter::FromSIMPLJson}}, // ImportBinaryCTNorthstarFilter
  {nx::core::Uuid::FromString("5fa10d81-94b4-582b-833f-8eabe659069e").value(), {nx::core::FilterTraits<ReadVolumeGraphicsFileFilter>::uuid, &ReadVolumeGraphicsFileFilter::FromSIMPLJson}}, // ImportVolumeGraphicsFileFilter
  {nx::core::Uuid::FromString("653b7b5c-03cb-5b32-8c3e-3637745e5ff6").value(), {nx::core::FilterTraits<ReadImageFilter>::uuid, &ReadImageFilter::FromSIMPLJson}}, // ITKImageReaderFilter
  {nx::core::Uuid::FromString("11473711-f94d-5d96-b749-ec36a81ad338").value(), {nx::core::FilterTraits<WriteImageFilter>::uuid, &WriteImageFilter::FromSIMPLJson}}, // ITKImageWriter
  {nx::core::Uuid::FromString("cf7d7497-9573-5102-bedd-38f86a6cdfd4").value(), {nx::core::FilterTraits<ReadImageStackFilter>::uuid, &ReadImageStackFilter::FromSIMPLJson}}, // ITKImportImageStack
  {nx::core::Uuid::FromString("3c451ac9-bfef-5e41-bae9-3957a0fc26a1").value(), {nx::core::FilterTraits<BinaryContourImageFilter>::uuid, &BinaryContourImageFilter::FromSIMPLJson}}, // ITKBinaryContourImage
  {nx::core::Uuid::FromString("bd1c2353-0a39-52c0-902b-ee64721994c7").value(), {nx::core::FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid, &BinaryOpeningByReconstructionImageFilter::FromSIMPLJson}}, // ITKBinaryOpeningByReconstructionImage
  {nx::core::Uuid::FromString("99a7aa3c-f945-5e77-875a-23b5231ab3f4").value(), {nx::core::FilterTraits<ClosingByReconstructionImageFilter>::uuid, &ClosingByReconstructionImageFilter::FromSIMPLJson}}, // ITKClosingByReconstructionImage
  {nx::core::Uuid::FromString("54c8dd45-88c4-5d4b-8a39-e3cc595e1cf8").value(), {nx::core::FilterTraits<GrayscaleFillholeImageFilter>::uuid, &GrayscaleFillholeImageFilter::FromSIMPLJson}}, // ITKGrayscaleFillholeImage
  {nx::core::Uuid::FromString("d910551f-4eec-55c9-b0ce-69c2277e61bd").value(), {nx::core::FilterTraits<GrayscaleGrindPeakImageFilter>::uuid, &GrayscaleGrindPeakImageFilter::FromSIMPLJson}}, // ITKGrayscaleGrindPeakImage
  {nx::core::Uuid::FromString("8bc34707-04c0-5e83-8583-48ee19306a1d").value(), {nx::core::FilterTraits<HConvexImageFilter>::uuid, &HConvexImageFilter::FromSIMPLJson}}, // ITKHConvexImage
  {nx::core::Uuid::FromString("932a6df4-212e-53a1-a2ab-c29bd376bb7b").value(), {nx::core::FilterTraits<HMaximaImageFilter>::uuid, &HMaximaImageFilter::FromSIMPLJson}}, // ITKHMaximaImage
  {nx::core::Uuid::FromString("f1d7cf59-9b7c-53cb-b71a-76cf91c86e8f").value(), {nx::core::FilterTraits<HMinimaImageFilter>::uuid, &HMinimaImageFilter::FromSIMPLJson}}, // ITKHMinimaImage
  {nx::core::Uuid::FromString("668f0b90-b504-5fba-b648-7c9677e1f452").value(), {nx::core::FilterTraits<LabelContourImageFilter>::uuid, &LabelContourImageFilter::FromSIMPLJson}}, // ITKLabelContourImage
  {nx::core::Uuid::FromString("12c83608-c4c5-5c72-b22f-a7696e3f5448").value(), {nx::core::FilterTraits<MorphologicalGradientImageFilter>::uuid, &MorphologicalGradientImageFilter::FromSIMPLJson}}, // ITKMorphologicalGradientImage
  {nx::core::Uuid::FromString("b2248340-a371-5899-90a2-86047950f0a2").value(), {nx::core::FilterTraits<MorphologicalWatershedImageFilter>::uuid, &MorphologicalWatershedImageFilter::FromSIMPLJson}}, // ITKMorphologicalWatershedImage
  {nx::core::Uuid::FromString("ca04004f-fb11-588d-9f77-d00b3ee9ad2a").value(), {nx::core::FilterTraits<OpeningByReconstructionImageFilter>::uuid, &OpeningByReconstructionImageFilter::FromSIMPLJson}}, // ITKOpeningByReconstructionImage
  {nx::core::Uuid::FromString("bb15d42a-3077-582a-be1a-76b2bae172e9").value(), {nx::core::FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid, &SignedMaurerDistanceMapImageFilter::FromSIMPLJson}}, // ITKSignedMaurerDistanceMapImage
  {nx::core::Uuid::FromString("10aff542-81c5-5f09-9797-c7171c40b6a0").value(), {nx::core::FilterTraits<ValuedRegionalMaximaImageFilter>::uuid, &ValuedRegionalMaximaImageFilter::FromSIMPLJson}}, // ITKValuedRegionalMaximaImage
  {nx::core::Uuid::FromString("739a0908-cb60-50f7-a484-b2157d023093").value(), {nx::core::FilterTraits<ValuedRegionalMinimaImageFilter>::uuid, &ValuedRegionalMinimaImageFilter::FromSIMPLJson}}, // ITKValuedRegionalMinimaImage

  #ifndef ImageProcessing_LEAN_AND_MEAN
  {nx::core::Uuid::FromString("53df5340-f632-598f-8a9b-802296b3a95c").value(), {nx::core::FilterTraits<DiscreteGaussianImageFilter>::uuid, &DiscreteGaussianImageFilter::FromSIMPLJson}}, // ITKDiscreteGaussianImage
  {nx::core::Uuid::FromString("cc27ee9a-9946-56ad-afd4-6e98b71f417d").value(), {nx::core::FilterTraits<MedianImageFilter>::uuid, &MedianImageFilter::FromSIMPLJson}}, // ITKMedianImage
  {nx::core::Uuid::FromString("77bf2192-851d-5127-9add-634c1ef4f67f").value(), {nx::core::FilterTraits<RescaleIntensityImageFilter>::uuid, &RescaleIntensityImageFilter::FromSIMPLJson}}, // ITKRescaleIntensityImage
  {nx::core::Uuid::FromString("09f45c29-1cfb-566c-b3ae-d832b4f95905").value(), {nx::core::FilterTraits<AbsImageFilter>::uuid, &AbsImageFilter::FromSIMPLJson}}, // ITKAbsImage
  {nx::core::Uuid::FromString("b09ec654-87a5-5dfa-9949-aa69f1fbfdd1").value(), {nx::core::FilterTraits<AcosImageFilter>::uuid, &AcosImageFilter::FromSIMPLJson}}, // ITKAcosImage
  {nx::core::Uuid::FromString("2d5a7599-5e01-5489-a107-23b704d2b5eb").value(), {nx::core::FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid, &AdaptiveHistogramEqualizationImageFilter::FromSIMPLJson}}, // ITKAdaptiveHistogramEqualizationImage
  {nx::core::Uuid::FromString("79509ab1-24e1-50e4-9351-c5ce7cd87a72").value(), {nx::core::FilterTraits<AsinImageFilter>::uuid, &AsinImageFilter::FromSIMPLJson}}, // ITKAsinImage
  {nx::core::Uuid::FromString("e938569d-3644-5d00-a4e0-ab327937457d").value(), {nx::core::FilterTraits<AtanImageFilter>::uuid, &AtanImageFilter::FromSIMPLJson}}, // ITKAtanImage
  {nx::core::Uuid::FromString("ba8a3f2e-3963-57c0-a8da-239e25de0526").value(), {nx::core::FilterTraits<BinaryThresholdImageFilter>::uuid, &BinaryThresholdImageFilter::FromSIMPLJson}}, // ITKBinaryThresholdImage
  {nx::core::Uuid::FromString("2c2d7bf6-1e78-52e6-80aa-58b504ce0912").value(), {nx::core::FilterTraits<CosImageFilter>::uuid, &CosImageFilter::FromSIMPLJson}}, // ITKCosImage
  {nx::core::Uuid::FromString("3aa99151-e722-51a0-90ba-71e93347ab09").value(), {nx::core::FilterTraits<GradientMagnitudeImageFilter>::uuid, &GradientMagnitudeImageFilter::FromSIMPLJson}}, // ITKGradientMagnitudeImage
  {nx::core::Uuid::FromString("c6e10fa5-5462-546b-b34b-0f0ea75a7e43").value(), {nx::core::FilterTraits<InvertIntensityImageFilter>::uuid, &InvertIntensityImageFilter::FromSIMPLJson}}, // ITKInvertIntensityImage
  {nx::core::Uuid::FromString("dbfd1a57-2a17-572d-93a7-8fd2f8e92eb0").value(), {nx::core::FilterTraits<Log10ImageFilter>::uuid, &Log10ImageFilter::FromSIMPLJson}}, // ITKLog10Image
  {nx::core::Uuid::FromString("69aba77c-9a35-5251-a18a-e3728ddd2963").value(), {nx::core::FilterTraits<LogImageFilter>::uuid, &LogImageFilter::FromSIMPLJson}}, // ITKLogImage
  {nx::core::Uuid::FromString("97102d65-9c32-576a-9177-c59d958bad10").value(), {nx::core::FilterTraits<MaskImageFilter>::uuid, &MaskImageFilter::FromSIMPLJson}}, // ITKMaskImage
  {nx::core::Uuid::FromString("5b905619-c46b-5690-b6fa-8e97cf4537b8").value(), {nx::core::FilterTraits<NormalizeImageFilter>::uuid, &NormalizeImageFilter::FromSIMPLJson}}, // ITKNormalizeImage
  {nx::core::Uuid::FromString("6e66563a-edcf-5e11-bc1d-ceed36d8493f").value(), {nx::core::FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid, &OtsuMultipleThresholdsImageFilter::FromSIMPLJson}}, // ITKOtsuMultipleThresholdsImage
  {nx::core::Uuid::FromString("1eb4b4f7-1704-58e4-9f78-8726a5c8c302").value(), {nx::core::FilterTraits<SinImageFilter>::uuid, &SinImageFilter::FromSIMPLJson}}, // ITKSinImage
  {nx::core::Uuid::FromString("8087dcad-68f2-598b-9670-d0f57647a445").value(), {nx::core::FilterTraits<SqrtImageFilter>::uuid, &SqrtImageFilter::FromSIMPLJson}}, // ITKSqrtImage
  {nx::core::Uuid::FromString("672810d9-5ec0-59c1-a209-8fb56c7a018a").value(), {nx::core::FilterTraits<TanImageFilter>::uuid, &TanImageFilter::FromSIMPLJson}}, // ITKTanImage
  {nx::core::Uuid::FromString("f86167ad-a1a1-557b-97ea-92a3618baa8f").value(), {nx::core::FilterTraits<BinaryDilateImageFilter>::uuid, &BinaryDilateImageFilter::FromSIMPLJson}}, // ITKBinaryDilateImage
  {nx::core::Uuid::FromString("522c5249-c048-579a-98dd-f7aadafc5578").value(), {nx::core::FilterTraits<BinaryErodeImageFilter>::uuid, &BinaryErodeImageFilter::FromSIMPLJson}}, // ITKBinaryErodeImage
  {nx::core::Uuid::FromString("704c801a-7549-54c4-9def-c4bb58d07fd1").value(), {nx::core::FilterTraits<BinaryMorphologicalOpeningImageFilter>::uuid, &BinaryMorphologicalOpeningImageFilter::FromSIMPLJson}}, // ITKBinaryMorphologicalOpeningImage
  {nx::core::Uuid::FromString("606c3700-f793-5852-9a0f-3123bd212447").value(), {nx::core::FilterTraits<BinaryProjectionImageFilter>::uuid, &BinaryProjectionImageFilter::FromSIMPLJson}}, // ITKBinaryProjectionImage
  {nx::core::Uuid::FromString("dcceeb50-5924-5eae-88ea-34793cf545a9").value(), {nx::core::FilterTraits<BinaryThinningImageFilter>::uuid, &BinaryThinningImageFilter::FromSIMPLJson}}, // ITKBinaryThinningImage
  {nx::core::Uuid::FromString("e26e7359-f72c-5924-b42e-dd5dd454a794").value(), {nx::core::FilterTraits<BlackTopHatImageFilter>::uuid, &BlackTopHatImageFilter::FromSIMPLJson}}, // ITKBlackTopHatImage
  {nx::core::Uuid::FromString("dbf29c6d-461c-55e7-a6c4-56477d9da55b").value(), {nx::core::FilterTraits<DilateObjectMorphologyImageFilter>::uuid, &DilateObjectMorphologyImageFilter::FromSIMPLJson}}, // ITKDilateObjectMorphologyImage
  {nx::core::Uuid::FromString("caea0698-4253-518b-ab3f-8ebc140d92ea").value(), {nx::core::FilterTraits<ErodeObjectMorphologyImageFilter>::uuid, &ErodeObjectMorphologyImageFilter::FromSIMPLJson}}, // ITKErodeObjectMorphologyImage
  {nx::core::Uuid::FromString("a6fb3f3a-6c7a-5dfc-a4f1-75ff1d62c32f").value(), {nx::core::FilterTraits<ExpImageFilter>::uuid, &ExpImageFilter::FromSIMPLJson}}, // ITKExpImage
  {nx::core::Uuid::FromString("634c2306-c1ee-5a45-a55c-f8286e36999a").value(), {nx::core::FilterTraits<ExpNegativeImageFilter>::uuid, &ExpNegativeImageFilter::FromSIMPLJson}}, // ITKExpNegativeImage
  {nx::core::Uuid::FromString("66cec151-2950-51f8-8a02-47d3516d8721").value(), {nx::core::FilterTraits<GrayscaleDilateImageFilter>::uuid, &GrayscaleDilateImageFilter::FromSIMPLJson}}, // ITKGrayscaleDilateImage
  {nx::core::Uuid::FromString("aef4e804-3f7a-5dc0-911c-b1f16a393a69").value(), {nx::core::FilterTraits<GrayscaleErodeImageFilter>::uuid, &GrayscaleErodeImageFilter::FromSIMPLJson}}, // ITKGrayscaleErodeImage
  {nx::core::Uuid::FromString("849a1903-5595-5029-bbde-6f4b68b2a25c").value(), {nx::core::FilterTraits<GrayscaleMorphologicalClosingImageFilter>::uuid, &GrayscaleMorphologicalClosingImageFilter::FromSIMPLJson}}, // ITKGrayscaleMorphologicalClosingImage
  {nx::core::Uuid::FromString("c88ac42b-9477-5088-9ec0-862af1e0bb56").value(), {nx::core::FilterTraits<GrayscaleMorphologicalOpeningImageFilter>::uuid, &GrayscaleMorphologicalOpeningImageFilter::FromSIMPLJson}}, // ITKGrayscaleMorphologicalOpeningImage
  {nx::core::Uuid::FromString("4faf4c59-6f29-53af-bc78-5aecffce0e37").value(), {nx::core::FilterTraits<IntensityWindowingImageFilter>::uuid, &IntensityWindowingImageFilter::FromSIMPLJson}}, // ITKIntensityWindowingImage
  {nx::core::Uuid::FromString("c8362fb9-d3ab-55c0-902b-274cc27d9bb8").value(), {nx::core::FilterTraits<NotImageFilter>::uuid, &NotImageFilter::FromSIMPLJson}}, // ITKNotImage
  {nx::core::Uuid::FromString("4398d76d-c9aa-5161-bb48-92dd9daaa352").value(), {nx::core::FilterTraits<RelabelComponentImageFilter>::uuid, &RelabelComponentImageFilter::FromSIMPLJson}}, // ITKRelabelComponentImage
  {nx::core::Uuid::FromString("e6675be7-e98d-5e0f-a088-ba15cc301038").value(), {nx::core::FilterTraits<SigmoidImageFilter>::uuid, &SigmoidImageFilter::FromSIMPLJson}}, // ITKSigmoidImage
  {nx::core::Uuid::FromString("f092420e-14a0-5dc0-91f8-de0082103aef").value(), {nx::core::FilterTraits<SquareImageFilter>::uuid, &SquareImageFilter::FromSIMPLJson}}, // ITKSquareImage
  {nx::core::Uuid::FromString("5845ee06-5c8a-5a74-80fb-c820bd8dfb75").value(), {nx::core::FilterTraits<ThresholdImageFilter>::uuid, &ThresholdImageFilter::FromSIMPLJson}}, // ITKThresholdImage
  {nx::core::Uuid::FromString("02e059f7-8055-52b4-9d48-915b67d1e39a").value(), {nx::core::FilterTraits<WhiteTopHatImageFilter>::uuid, &WhiteTopHatImageFilter::FromSIMPLJson}}, // ITKWhiteTopHatImage

#endif

};

// Old ITKImageProcessing simplnx (NX) filter Uuid -> new ImageProcessing filter Uuid.
// Consulted by PipelineFilter::FromJson only when a stored NX filter Uuid fails to resolve (i.e.
// after ITKImageProcessing is removed). Harmless during coexistence: while ITKImageProcessing is
// loaded, the old Uuid resolves directly and this map is never consulted.
static const AbstractPlugin::FilterReplacementMapType k_ITK_to_ImageProcessing_Replacements
{

  {ImageProcessingLegacyUuids::k_ITKBinaryContourImageFilter, nx::core::FilterTraits<BinaryContourImageFilter>::uuid}, // ITKBinaryContourImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryOpeningByReconstructionImageFilter, nx::core::FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid}, // ITKBinaryOpeningByReconstructionImageFilter
  {ImageProcessingLegacyUuids::k_ITKClosingByReconstructionImageFilter, nx::core::FilterTraits<ClosingByReconstructionImageFilter>::uuid}, // ITKClosingByReconstructionImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleFillholeImageFilter, nx::core::FilterTraits<GrayscaleFillholeImageFilter>::uuid}, // ITKGrayscaleFillholeImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleGrindPeakImageFilter, nx::core::FilterTraits<GrayscaleGrindPeakImageFilter>::uuid}, // ITKGrayscaleGrindPeakImageFilter
  {ImageProcessingLegacyUuids::k_ITKHConvexImageFilter, nx::core::FilterTraits<HConvexImageFilter>::uuid}, // ITKHConvexImageFilter
  {ImageProcessingLegacyUuids::k_ITKHMaximaImageFilter, nx::core::FilterTraits<HMaximaImageFilter>::uuid}, // ITKHMaximaImageFilter
  {ImageProcessingLegacyUuids::k_ITKHMinimaImageFilter, nx::core::FilterTraits<HMinimaImageFilter>::uuid}, // ITKHMinimaImageFilter
  {ImageProcessingLegacyUuids::k_ITKLabelContourImageFilter, nx::core::FilterTraits<LabelContourImageFilter>::uuid}, // ITKLabelContourImageFilter
  {ImageProcessingLegacyUuids::k_ITKMorphologicalGradientImageFilter, nx::core::FilterTraits<MorphologicalGradientImageFilter>::uuid}, // ITKMorphologicalGradientImageFilter
  {ImageProcessingLegacyUuids::k_ITKMorphologicalWatershedImageFilter, nx::core::FilterTraits<MorphologicalWatershedImageFilter>::uuid}, // ITKMorphologicalWatershedImageFilter
  {ImageProcessingLegacyUuids::k_ITKOpeningByReconstructionImageFilter, nx::core::FilterTraits<OpeningByReconstructionImageFilter>::uuid}, // ITKOpeningByReconstructionImageFilter
  {ImageProcessingLegacyUuids::k_ITKSignedMaurerDistanceMapImageFilter, nx::core::FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid}, // ITKSignedMaurerDistanceMapImageFilter
  {ImageProcessingLegacyUuids::k_ITKValuedRegionalMaximaImageFilter, nx::core::FilterTraits<ValuedRegionalMaximaImageFilter>::uuid}, // ITKValuedRegionalMaximaImageFilter
  {ImageProcessingLegacyUuids::k_ITKValuedRegionalMinimaImageFilter, nx::core::FilterTraits<ValuedRegionalMinimaImageFilter>::uuid}, // ITKValuedRegionalMinimaImageFilter
  {ImageProcessingLegacyUuids::k_ITKImageReader, nx::core::FilterTraits<ReadImageFilter>::uuid}, // ITKImageReader
  {ImageProcessingLegacyUuids::k_ITKMhaFileReader, nx::core::FilterTraits<ReadMhaFileFilter>::uuid}, // ITKMhaFileReader
  {ImageProcessingLegacyUuids::k_ITKImportImageStack, nx::core::FilterTraits<ReadImageStackFilter>::uuid}, // ITKImportImageStack
  {ImageProcessingLegacyUuids::k_ITKImportFijiMontage, nx::core::FilterTraits<ImportFijiMontageFilter>::uuid}, // ITKImportFijiMontage
  {ImageProcessingLegacyUuids::k_ITKImageWriterFilter, nx::core::FilterTraits<WriteImageFilter>::uuid}, // ITKImageWriterFilter

#ifndef ImageProcessing_LEAN_AND_MEAN
  {ImageProcessingLegacyUuids::k_ITKAbsImageFilter, nx::core::FilterTraits<AbsImageFilter>::uuid}, // ITKAbsImageFilter
  {ImageProcessingLegacyUuids::k_ITKAcosImageFilter, nx::core::FilterTraits<AcosImageFilter>::uuid}, // ITKAcosImageFilter
  {ImageProcessingLegacyUuids::k_ITKAdaptiveHistogramEqualizationImageFilter, nx::core::FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid}, // ITKAdaptiveHistogramEqualizationImageFilter
  {ImageProcessingLegacyUuids::k_ITKApproximateSignedDistanceMapImageFilter, nx::core::FilterTraits<ApproximateSignedDistanceMapImageFilter>::uuid}, // ITKApproximateSignedDistanceMapImageFilter
  {ImageProcessingLegacyUuids::k_ITKAsinImageFilter, nx::core::FilterTraits<AsinImageFilter>::uuid}, // ITKAsinImageFilter
  {ImageProcessingLegacyUuids::k_ITKAtanImageFilter, nx::core::FilterTraits<AtanImageFilter>::uuid}, // ITKAtanImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryDilateImageFilter, nx::core::FilterTraits<BinaryDilateImageFilter>::uuid}, // ITKBinaryDilateImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryErodeImageFilter, nx::core::FilterTraits<BinaryErodeImageFilter>::uuid}, // ITKBinaryErodeImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryMorphologicalClosingImageFilter, nx::core::FilterTraits<BinaryMorphologicalClosingImageFilter>::uuid}, // ITKBinaryMorphologicalClosingImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryMorphologicalOpeningImageFilter, nx::core::FilterTraits<BinaryMorphologicalOpeningImageFilter>::uuid}, // ITKBinaryMorphologicalOpeningImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryProjectionImageFilter, nx::core::FilterTraits<BinaryProjectionImageFilter>::uuid}, // ITKBinaryProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryThinningImageFilter, nx::core::FilterTraits<BinaryThinningImageFilter>::uuid}, // ITKBinaryThinningImageFilter
  {ImageProcessingLegacyUuids::k_ITKBinaryThresholdImageFilter, nx::core::FilterTraits<BinaryThresholdImageFilter>::uuid}, // ITKBinaryThresholdImageFilter
  {ImageProcessingLegacyUuids::k_ITKBlackTopHatImageFilter, nx::core::FilterTraits<BlackTopHatImageFilter>::uuid}, // ITKBlackTopHatImageFilter
  {ImageProcessingLegacyUuids::k_ITKBoundedReciprocalImageFilter, nx::core::FilterTraits<BoundedReciprocalImageFilter>::uuid}, // ITKBoundedReciprocalImageFilter
  {ImageProcessingLegacyUuids::k_ITKConnectedComponentImageFilter, nx::core::FilterTraits<ConnectedComponentImageFilter>::uuid}, // ITKConnectedComponentImageFilter
  {ImageProcessingLegacyUuids::k_ITKCosImageFilter, nx::core::FilterTraits<CosImageFilter>::uuid}, // ITKCosImageFilter
  {ImageProcessingLegacyUuids::k_ITKCurvatureAnisotropicDiffusionImageFilter, nx::core::FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid}, // ITKCurvatureAnisotropicDiffusionImageFilter
  {ImageProcessingLegacyUuids::k_ITKCurvatureFlowImageFilter, nx::core::FilterTraits<CurvatureFlowImageFilter>::uuid}, // ITKCurvatureFlowImageFilter
  {ImageProcessingLegacyUuids::k_ITKDanielssonDistanceMapImageFilter, nx::core::FilterTraits<DanielssonDistanceMapImageFilter>::uuid}, // ITKDanielssonDistanceMapImageFilter
  {ImageProcessingLegacyUuids::k_ITKDilateObjectMorphologyImageFilter, nx::core::FilterTraits<DilateObjectMorphologyImageFilter>::uuid}, // ITKDilateObjectMorphologyImageFilter
  {ImageProcessingLegacyUuids::k_ITKDiscreteGaussianImageFilter, nx::core::FilterTraits<DiscreteGaussianImageFilter>::uuid}, // ITKDiscreteGaussianImageFilter
  {ImageProcessingLegacyUuids::k_ITKDoubleThresholdImageFilter, nx::core::FilterTraits<DoubleThresholdImageFilter>::uuid}, // ITKDoubleThresholdImageFilter
  {ImageProcessingLegacyUuids::k_ITKErodeObjectMorphologyImageFilter, nx::core::FilterTraits<ErodeObjectMorphologyImageFilter>::uuid}, // ITKErodeObjectMorphologyImageFilter
  {ImageProcessingLegacyUuids::k_ITKExpImageFilter, nx::core::FilterTraits<ExpImageFilter>::uuid}, // ITKExpImageFilter
  {ImageProcessingLegacyUuids::k_ITKExpNegativeImageFilter, nx::core::FilterTraits<ExpNegativeImageFilter>::uuid}, // ITKExpNegativeImageFilter
  {ImageProcessingLegacyUuids::k_ITKGradientAnisotropicDiffusionImageFilter, nx::core::FilterTraits<GradientAnisotropicDiffusionImageFilter>::uuid}, // ITKGradientAnisotropicDiffusionImageFilter
  {ImageProcessingLegacyUuids::k_ITKGradientMagnitudeImageFilter, nx::core::FilterTraits<GradientMagnitudeImageFilter>::uuid}, // ITKGradientMagnitudeImageFilter
  {ImageProcessingLegacyUuids::k_ITKGradientMagnitudeRecursiveGaussianImageFilter, nx::core::FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::uuid}, // ITKGradientMagnitudeRecursiveGaussianImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleDilateImageFilter, nx::core::FilterTraits<GrayscaleDilateImageFilter>::uuid}, // ITKGrayscaleDilateImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleErodeImageFilter, nx::core::FilterTraits<GrayscaleErodeImageFilter>::uuid}, // ITKGrayscaleErodeImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleMorphologicalClosingImageFilter, nx::core::FilterTraits<GrayscaleMorphologicalClosingImageFilter>::uuid}, // ITKGrayscaleMorphologicalClosingImageFilter
  {ImageProcessingLegacyUuids::k_ITKGrayscaleMorphologicalOpeningImageFilter, nx::core::FilterTraits<GrayscaleMorphologicalOpeningImageFilter>::uuid}, // ITKGrayscaleMorphologicalOpeningImageFilter
  {ImageProcessingLegacyUuids::k_ITKIntensityWindowingImageFilter, nx::core::FilterTraits<IntensityWindowingImageFilter>::uuid}, // ITKIntensityWindowingImageFilter
  {ImageProcessingLegacyUuids::k_ITKInvertIntensityImageFilter, nx::core::FilterTraits<InvertIntensityImageFilter>::uuid}, // ITKInvertIntensityImageFilter
  {ImageProcessingLegacyUuids::k_ITKIsoContourDistanceImageFilter, nx::core::FilterTraits<IsoContourDistanceImageFilter>::uuid}, // ITKIsoContourDistanceImageFilter
  {ImageProcessingLegacyUuids::k_ITKLaplacianRecursiveGaussianImageFilter, nx::core::FilterTraits<LaplacianRecursiveGaussianImageFilter>::uuid}, // ITKLaplacianRecursiveGaussianImageFilter
  {ImageProcessingLegacyUuids::k_ITKLog10ImageFilter, nx::core::FilterTraits<Log10ImageFilter>::uuid}, // ITKLog10ImageFilter
  {ImageProcessingLegacyUuids::k_ITKLogImageFilter, nx::core::FilterTraits<LogImageFilter>::uuid}, // ITKLogImageFilter
  {ImageProcessingLegacyUuids::k_ITKMaskImageFilter, nx::core::FilterTraits<MaskImageFilter>::uuid}, // ITKMaskImageFilter
  {ImageProcessingLegacyUuids::k_ITKMaximumProjectionImageFilter, nx::core::FilterTraits<MaximumProjectionImageFilter>::uuid}, // ITKMaximumProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKMeanProjectionImageFilter, nx::core::FilterTraits<MeanProjectionImageFilter>::uuid}, // ITKMeanProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKMedianImageFilter, nx::core::FilterTraits<MedianImageFilter>::uuid}, // ITKMedianImageFilter
  {ImageProcessingLegacyUuids::k_ITKMedianProjectionImageFilter, nx::core::FilterTraits<MedianProjectionImageFilter>::uuid}, // ITKMedianProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKMinimumProjectionImageFilter, nx::core::FilterTraits<MinimumProjectionImageFilter>::uuid}, // ITKMinimumProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKMinMaxCurvatureFlowImageFilter, nx::core::FilterTraits<MinMaxCurvatureFlowImageFilter>::uuid}, // ITKMinMaxCurvatureFlowImageFilter
  {ImageProcessingLegacyUuids::k_ITKMorphologicalWatershedFromMarkersImageFilter, nx::core::FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::uuid}, // ITKMorphologicalWatershedFromMarkersImageFilter
  {ImageProcessingLegacyUuids::k_ITKNormalizeImageFilter, nx::core::FilterTraits<NormalizeImageFilter>::uuid}, // ITKNormalizeImageFilter
  {ImageProcessingLegacyUuids::k_ITKNormalizeToConstantImageFilter, nx::core::FilterTraits<NormalizeToConstantImageFilter>::uuid}, // ITKNormalizeToConstantImageFilter
  {ImageProcessingLegacyUuids::k_ITKNotImageFilter, nx::core::FilterTraits<NotImageFilter>::uuid}, // ITKNotImageFilter
  {ImageProcessingLegacyUuids::k_ITKOtsuMultipleThresholdsImageFilter, nx::core::FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid}, // ITKOtsuMultipleThresholdsImageFilter
  {ImageProcessingLegacyUuids::k_ITKRegionalMaximaImageFilter, nx::core::FilterTraits<RegionalMaximaImageFilter>::uuid}, // ITKRegionalMaximaImageFilter
  {ImageProcessingLegacyUuids::k_ITKRegionalMinimaImageFilter, nx::core::FilterTraits<RegionalMinimaImageFilter>::uuid}, // ITKRegionalMinimaImageFilter
  {ImageProcessingLegacyUuids::k_ITKRelabelComponentImageFilter, nx::core::FilterTraits<RelabelComponentImageFilter>::uuid}, // ITKRelabelComponentImageFilter
  {ImageProcessingLegacyUuids::k_ITKRescaleIntensityImageFilter, nx::core::FilterTraits<RescaleIntensityImageFilter>::uuid}, // ITKRescaleIntensityImageFilter
  {ImageProcessingLegacyUuids::k_ITKSigmoidImageFilter, nx::core::FilterTraits<SigmoidImageFilter>::uuid}, // ITKSigmoidImageFilter
  {ImageProcessingLegacyUuids::k_ITKSignedDanielssonDistanceMapImageFilter, nx::core::FilterTraits<SignedDanielssonDistanceMapImageFilter>::uuid}, // ITKSignedDanielssonDistanceMapImageFilter
  {ImageProcessingLegacyUuids::k_ITKSinImageFilter, nx::core::FilterTraits<SinImageFilter>::uuid}, // ITKSinImageFilter
  {ImageProcessingLegacyUuids::k_ITKSmoothingRecursiveGaussianImageFilter, nx::core::FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid}, // ITKSmoothingRecursiveGaussianImageFilter
  {ImageProcessingLegacyUuids::k_ITKSqrtImageFilter, nx::core::FilterTraits<SqrtImageFilter>::uuid}, // ITKSqrtImageFilter
  {ImageProcessingLegacyUuids::k_ITKSquareImageFilter, nx::core::FilterTraits<SquareImageFilter>::uuid}, // ITKSquareImageFilter
  {ImageProcessingLegacyUuids::k_ITKStandardDeviationProjectionImageFilter, nx::core::FilterTraits<StandardDeviationProjectionImageFilter>::uuid}, // ITKStandardDeviationProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKSumProjectionImageFilter, nx::core::FilterTraits<SumProjectionImageFilter>::uuid}, // ITKSumProjectionImageFilter
  {ImageProcessingLegacyUuids::k_ITKTanImageFilter, nx::core::FilterTraits<TanImageFilter>::uuid}, // ITKTanImageFilter
  {ImageProcessingLegacyUuids::k_ITKThresholdImageFilter, nx::core::FilterTraits<ThresholdImageFilter>::uuid}, // ITKThresholdImageFilter
  {ImageProcessingLegacyUuids::k_ITKThresholdMaximumConnectedComponentsImageFilter, nx::core::FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::uuid}, // ITKThresholdMaximumConnectedComponentsImageFilter
  {ImageProcessingLegacyUuids::k_ITKWhiteTopHatImageFilter, nx::core::FilterTraits<WhiteTopHatImageFilter>::uuid}, // ITKWhiteTopHatImageFilter
  {ImageProcessingLegacyUuids::k_ITKZeroCrossingImageFilter, nx::core::FilterTraits<ZeroCrossingImageFilter>::uuid}, // ITKZeroCrossingImageFilter

#endif
};

// The SIMPL map claims every Uuid the retired ITKImageProcessing plugin owned. Two loaded plugins cannot
// claim the same SIMPL Uuid, so these entries waited until that plugin was deleted. It is deleted, so the
// entries are live. Add a new SIMPL Uuid here whenever this plugin gains a filter that DREAM3D 6.x shipped.
} // namespace nx::core
/* clang-format on */
