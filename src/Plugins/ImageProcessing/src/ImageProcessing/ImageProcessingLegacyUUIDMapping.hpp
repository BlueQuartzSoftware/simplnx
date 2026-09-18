#pragma once

#include "ImageProcessing/Filters/AbsImageFilter.hpp"
#include "ImageProcessing/Filters/AcosImageFilter.hpp"
#include "ImageProcessing/Filters/AdaptiveHistogramEqualizationImageFilter.hpp"
#include "ImageProcessing/Filters/ApproximateSignedDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/AsinImageFilter.hpp"
#include "ImageProcessing/Filters/AtanImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryContourImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryDilateImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryErodeImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryOpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThinningImageFilter.hpp"
#include "ImageProcessing/Filters/BinaryThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/BlackTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/BoundedReciprocalImageFilter.hpp"
#include "ImageProcessing/Filters/ClosingByReconstructionImageFilter.hpp"
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
#include "ImageProcessing/Filters/GrayscaleFillholeImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleGrindPeakImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalClosingImageFilter.hpp"
#include "ImageProcessing/Filters/GrayscaleMorphologicalOpeningImageFilter.hpp"
#include "ImageProcessing/Filters/HConvexImageFilter.hpp"
#include "ImageProcessing/Filters/HMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/HMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/ImportFijiMontageFilter.hpp"
#include "ImageProcessing/Filters/IntensityWindowingImageFilter.hpp"
#include "ImageProcessing/Filters/InvertIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/IsoContourDistanceImageFilter.hpp"
#include "ImageProcessing/Filters/LabelContourImageFilter.hpp"
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
#include "ImageProcessing/Filters/MorphologicalGradientImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"
#include "ImageProcessing/Filters/MorphologicalWatershedImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ImageProcessing/Filters/NormalizeToConstantImageFilter.hpp"
#include "ImageProcessing/Filters/NotImageFilter.hpp"
#include "ImageProcessing/Filters/OpeningByReconstructionImageFilter.hpp"
#include "ImageProcessing/Filters/OtsuMultipleThresholdsImageFilter.hpp"
#include "ImageProcessing/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/Filters/ReadImageStackFilter.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "ImageProcessing/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "ImageProcessing/Filters/RegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/RegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/RelabelComponentImageFilter.hpp"
#include "ImageProcessing/Filters/RescaleIntensityImageFilter.hpp"
#include "ImageProcessing/Filters/SigmoidImageFilter.hpp"
#include "ImageProcessing/Filters/SignedDanielssonDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/SignedMaurerDistanceMapImageFilter.hpp"
#include "ImageProcessing/Filters/SinImageFilter.hpp"
#include "ImageProcessing/Filters/SmoothingRecursiveGaussianImageFilter.hpp"
#include "ImageProcessing/Filters/SqrtImageFilter.hpp"
#include "ImageProcessing/Filters/SquareImageFilter.hpp"
#include "ImageProcessing/Filters/StandardDeviationProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/SumProjectionImageFilter.hpp"
#include "ImageProcessing/Filters/TanImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdImageFilter.hpp"
#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMaximaImageFilter.hpp"
#include "ImageProcessing/Filters/ValuedRegionalMinimaImageFilter.hpp"
#include "ImageProcessing/Filters/WhiteTopHatImageFilter.hpp"
#include "ImageProcessing/Filters/WriteImageFilter.hpp"
#include "ImageProcessing/Filters/ZeroCrossingImageFilter.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"

/* clang-format off */
namespace nx::core
{
static const AbstractPlugin::SIMPLMapType k_SIMPL_to_ImageProcessing
{
  {nx::core::Uuid::FromString("f2259481-5011-5f22-9fcb-c92fb6f8be10").value(), {nx::core::FilterTraits<ReadBinaryCTNorthstarFilter>::uuid, &ReadBinaryCTNorthstarFilter::FromSIMPLJson}}, // ImportBinaryCTNorthstarFilter
  {nx::core::Uuid::FromString("5fa10d81-94b4-582b-833f-8eabe659069e").value(), {nx::core::FilterTraits<ReadVolumeGraphicsFileFilter>::uuid, &ReadVolumeGraphicsFileFilter::FromSIMPLJson}}, // ImportVolumeGraphicsFileFilter
};

// Old ITKImageProcessing simplnx (NX) filter Uuid -> new ImageProcessing filter Uuid.
// Consulted by PipelineFilter::FromJson only when a stored NX filter Uuid fails to resolve (i.e.
// after ITKImageProcessing is removed). Harmless during coexistence: while ITKImageProcessing is
// loaded, the old Uuid resolves directly and this map is never consulted.
static const AbstractPlugin::FilterReplacementMapType k_ITK_to_ImageProcessing_Replacements
{
  {*nx::core::Uuid::FromString("d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2"), nx::core::FilterTraits<ReadImageFilter>::uuid}, // ITKImageReader
  {*nx::core::Uuid::FromString("41c33a08-0052-4915-8d53-d503f85f30d9"), nx::core::FilterTraits<ReadMhaFileFilter>::uuid}, // ITKMhaFileReader
  {*nx::core::Uuid::FromString("dcf980b7-ecca-46d1-af31-ac65f6e3b6bb"), nx::core::FilterTraits<ReadImageStackFilter>::uuid}, // ITKImportImageStack
  {*nx::core::Uuid::FromString("4c48ea16-13ef-4281-89cf-315be5fb857d"), nx::core::FilterTraits<ImportFijiMontageFilter>::uuid}, // ITKImportFijiMontage
  {*nx::core::Uuid::FromString("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620"), nx::core::FilterTraits<AbsImageFilter>::uuid}, // ITKAbsImageFilter
  {*nx::core::Uuid::FromString("e7411c44-95ab-4623-8bf4-59b63d2d08c5"), nx::core::FilterTraits<AcosImageFilter>::uuid}, // ITKAcosImageFilter
  {*nx::core::Uuid::FromString("ea3e7439-8327-4190-8ff7-49ecc321718f"), nx::core::FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid}, // ITKAdaptiveHistogramEqualizationImageFilter
  {*nx::core::Uuid::FromString("87ed0d3a-c394-4bb5-ac7f-6cc746984b09"), nx::core::FilterTraits<ApproximateSignedDistanceMapImageFilter>::uuid}, // ITKApproximateSignedDistanceMapImageFilter
  {*nx::core::Uuid::FromString("1b463492-041f-4680-abb1-0b94a3019063"), nx::core::FilterTraits<AsinImageFilter>::uuid}, // ITKAsinImageFilter
  {*nx::core::Uuid::FromString("39933f50-088c-46ac-a421-d238f1b178fd"), nx::core::FilterTraits<AtanImageFilter>::uuid}, // ITKAtanImageFilter
  {*nx::core::Uuid::FromString("ed214e76-6954-49b4-817b-13f92315e722"), nx::core::FilterTraits<BinaryContourImageFilter>::uuid}, // ITKBinaryContourImageFilter
  {*nx::core::Uuid::FromString("d4e973cb-c501-4c64-af26-fcf791c0f36d"), nx::core::FilterTraits<BinaryDilateImageFilter>::uuid}, // ITKBinaryDilateImageFilter
  {*nx::core::Uuid::FromString("243dd30b-d1f0-42ad-8b47-77d57f9fc262"), nx::core::FilterTraits<BinaryErodeImageFilter>::uuid}, // ITKBinaryErodeImageFilter
  {*nx::core::Uuid::FromString("abb27e0c-b049-4f60-8355-178d86bb1de4"), nx::core::FilterTraits<BinaryMorphologicalClosingImageFilter>::uuid}, // ITKBinaryMorphologicalClosingImageFilter
  {*nx::core::Uuid::FromString("861ccb46-dbce-41bf-a66f-25cc18cd1073"), nx::core::FilterTraits<BinaryMorphologicalOpeningImageFilter>::uuid}, // ITKBinaryMorphologicalOpeningImageFilter
  {*nx::core::Uuid::FromString("02c15392-382c-406d-a174-07ea6fa11b67"), nx::core::FilterTraits<BinaryOpeningByReconstructionImageFilter>::uuid}, // ITKBinaryOpeningByReconstructionImageFilter
  {*nx::core::Uuid::FromString("04ea495e-2cf0-4dba-8d29-cf33a38c094d"), nx::core::FilterTraits<BinaryProjectionImageFilter>::uuid}, // ITKBinaryProjectionImageFilter
  {*nx::core::Uuid::FromString("8fcd24cb-769d-400f-97cf-9b4dad1b8cd2"), nx::core::FilterTraits<BinaryThinningImageFilter>::uuid}, // ITKBinaryThinningImageFilter
  {*nx::core::Uuid::FromString("ba2494b0-c4f0-43ff-9d08-900395900e0c"), nx::core::FilterTraits<BinaryThresholdImageFilter>::uuid}, // ITKBinaryThresholdImageFilter
  {*nx::core::Uuid::FromString("b7471b64-2282-449b-82b4-3ce359e9dda0"), nx::core::FilterTraits<BlackTopHatImageFilter>::uuid}, // ITKBlackTopHatImageFilter
  {*nx::core::Uuid::FromString("da72b2ae-74ef-4198-820f-6a381b3fff05"), nx::core::FilterTraits<BoundedReciprocalImageFilter>::uuid}, // ITKBoundedReciprocalImageFilter
  {*nx::core::Uuid::FromString("b5ff32a8-e799-4f72-8d13-e2581f748562"), nx::core::FilterTraits<ClosingByReconstructionImageFilter>::uuid}, // ITKClosingByReconstructionImageFilter
  {*nx::core::Uuid::FromString("905354c1-d55b-4436-b9f7-f4a6e80e5c0f"), nx::core::FilterTraits<ConnectedComponentImageFilter>::uuid}, // ITKConnectedComponentImageFilter
  {*nx::core::Uuid::FromString("6fe37f77-ceae-4839-9cf6-3ca7a70e14d0"), nx::core::FilterTraits<CosImageFilter>::uuid}, // ITKCosImageFilter
  {*nx::core::Uuid::FromString("ada68f29-b1f2-44a2-86dc-f0cd28f54633"), nx::core::FilterTraits<CurvatureAnisotropicDiffusionImageFilter>::uuid}, // ITKCurvatureAnisotropicDiffusionImageFilter
  {*nx::core::Uuid::FromString("fe5b2ed3-54dd-4207-ad88-48a95134684a"), nx::core::FilterTraits<CurvatureFlowImageFilter>::uuid}, // ITKCurvatureFlowImageFilter
  {*nx::core::Uuid::FromString("f0cd4faf-a676-41ed-9ea5-859035f94836"), nx::core::FilterTraits<DanielssonDistanceMapImageFilter>::uuid}, // ITKDanielssonDistanceMapImageFilter
  {*nx::core::Uuid::FromString("e3f7c642-4c16-47f6-ac5c-cd276d61bfa6"), nx::core::FilterTraits<DilateObjectMorphologyImageFilter>::uuid}, // ITKDilateObjectMorphologyImageFilter
  {*nx::core::Uuid::FromString("025edc1a-986d-4005-92d1-545dfdc13abd"), nx::core::FilterTraits<DiscreteGaussianImageFilter>::uuid}, // ITKDiscreteGaussianImageFilter
  {*nx::core::Uuid::FromString("e268a65f-33f1-493f-a6c7-4635e57df3c4"), nx::core::FilterTraits<DoubleThresholdImageFilter>::uuid}, // ITKDoubleThresholdImageFilter
  {*nx::core::Uuid::FromString("db3fe379-6ce4-4be3-baca-1cbff00aca6a"), nx::core::FilterTraits<ErodeObjectMorphologyImageFilter>::uuid}, // ITKErodeObjectMorphologyImageFilter
  {*nx::core::Uuid::FromString("264977e7-cc0d-4d2b-ba9c-a30765b498b2"), nx::core::FilterTraits<ExpImageFilter>::uuid}, // ITKExpImageFilter
  {*nx::core::Uuid::FromString("2c84cc7c-01ab-4550-9ba5-b9fa58b74599"), nx::core::FilterTraits<ExpNegativeImageFilter>::uuid}, // ITKExpNegativeImageFilter
  {*nx::core::Uuid::FromString("9dcef77b-e7d2-4a2a-b310-bfa80e8ea7c5"), nx::core::FilterTraits<GradientAnisotropicDiffusionImageFilter>::uuid}, // ITKGradientAnisotropicDiffusionImageFilter
  {*nx::core::Uuid::FromString("719df7b2-8db2-43eb-a40c-a015982eec08"), nx::core::FilterTraits<GradientMagnitudeImageFilter>::uuid}, // ITKGradientMagnitudeImageFilter
  {*nx::core::Uuid::FromString("32db4ae4-4087-4688-874a-b1d725188f18"), nx::core::FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::uuid}, // ITKGradientMagnitudeRecursiveGaussianImageFilter
  {*nx::core::Uuid::FromString("944f4d1a-adb1-401a-9c0f-e2085ef2f6dc"), nx::core::FilterTraits<GrayscaleDilateImageFilter>::uuid}, // ITKGrayscaleDilateImageFilter
  {*nx::core::Uuid::FromString("3ceb1e6f-f4a4-4d8f-82d5-ecd20d4fc285"), nx::core::FilterTraits<GrayscaleErodeImageFilter>::uuid}, // ITKGrayscaleErodeImageFilter
  {*nx::core::Uuid::FromString("1f1dd9e4-d361-432b-a22b-5535664ee545"), nx::core::FilterTraits<GrayscaleFillholeImageFilter>::uuid}, // ITKGrayscaleFillholeImageFilter
  {*nx::core::Uuid::FromString("6aa5b193-c290-4fe3-a409-7759a62d48ea"), nx::core::FilterTraits<GrayscaleGrindPeakImageFilter>::uuid}, // ITKGrayscaleGrindPeakImageFilter
  {*nx::core::Uuid::FromString("8b859b54-93d4-4341-8fbf-85e1e461d5b5"), nx::core::FilterTraits<GrayscaleMorphologicalClosingImageFilter>::uuid}, // ITKGrayscaleMorphologicalClosingImageFilter
  {*nx::core::Uuid::FromString("54433e92-fb7d-40f4-95bf-eb3db76d5caa"), nx::core::FilterTraits<GrayscaleMorphologicalOpeningImageFilter>::uuid}, // ITKGrayscaleMorphologicalOpeningImageFilter
  {*nx::core::Uuid::FromString("620240a1-0b04-4bc7-a4c3-531917de4bc0"), nx::core::FilterTraits<HConvexImageFilter>::uuid}, // ITKHConvexImageFilter
  {*nx::core::Uuid::FromString("40039f72-30b0-4a3f-8ea4-2f76c5f65bc1"), nx::core::FilterTraits<HMaximaImageFilter>::uuid}, // ITKHMaximaImageFilter
  {*nx::core::Uuid::FromString("9b2eb24b-90e5-41c0-9230-1044170ee8ea"), nx::core::FilterTraits<HMinimaImageFilter>::uuid}, // ITKHMinimaImageFilter
  {*nx::core::Uuid::FromString("ee317bf6-79aa-4dcc-b59e-b0246dc3fcfa"), nx::core::FilterTraits<IntensityWindowingImageFilter>::uuid}, // ITKIntensityWindowingImageFilter
  {*nx::core::Uuid::FromString("9958d587-5698-4ea5-b8ea-fb71428b5d02"), nx::core::FilterTraits<InvertIntensityImageFilter>::uuid}, // ITKInvertIntensityImageFilter
  {*nx::core::Uuid::FromString("e82fa143-7ac2-4c09-a88c-9ea71d47d594"), nx::core::FilterTraits<IsoContourDistanceImageFilter>::uuid}, // ITKIsoContourDistanceImageFilter
  {*nx::core::Uuid::FromString("b64ff45d-3661-4926-9e87-5dd7b379b261"), nx::core::FilterTraits<LabelContourImageFilter>::uuid}, // ITKLabelContourImageFilter
  {*nx::core::Uuid::FromString("782d76a4-e3f6-4c2a-a1b0-7456a3e77f24"), nx::core::FilterTraits<LaplacianRecursiveGaussianImageFilter>::uuid}, // ITKLaplacianRecursiveGaussianImageFilter
  {*nx::core::Uuid::FromString("900ca377-e79d-4b54-b298-33d518238099"), nx::core::FilterTraits<Log10ImageFilter>::uuid}, // ITKLog10ImageFilter
  {*nx::core::Uuid::FromString("4b6655ad-4e6c-4e68-a771-55ca0ae40915"), nx::core::FilterTraits<LogImageFilter>::uuid}, // ITKLogImageFilter
  {*nx::core::Uuid::FromString("d3138266-3f34-4d6e-8e21-904c94351293"), nx::core::FilterTraits<MaskImageFilter>::uuid}, // ITKMaskImageFilter
  {*nx::core::Uuid::FromString("6dfe9167-d77d-41c7-aebf-569d6190645d"), nx::core::FilterTraits<MaximumProjectionImageFilter>::uuid}, // ITKMaximumProjectionImageFilter
  {*nx::core::Uuid::FromString("62ffddba-cc57-45fc-a93a-27914eea11ad"), nx::core::FilterTraits<MeanProjectionImageFilter>::uuid}, // ITKMeanProjectionImageFilter
  {*nx::core::Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8"), nx::core::FilterTraits<MedianImageFilter>::uuid}, // ITKMedianImageFilter
  {*nx::core::Uuid::FromString("00e48f6b-8a00-414f-b3d9-49d48a3f9a00"), nx::core::FilterTraits<MedianProjectionImageFilter>::uuid}, // ITKMedianProjectionImageFilter
  {*nx::core::Uuid::FromString("86898336-8680-4c4e-b166-3f8de9e3d4f2"), nx::core::FilterTraits<MinimumProjectionImageFilter>::uuid}, // ITKMinimumProjectionImageFilter
  {*nx::core::Uuid::FromString("b836c081-6692-411d-81d0-a50afce6b288"), nx::core::FilterTraits<MinMaxCurvatureFlowImageFilter>::uuid}, // ITKMinMaxCurvatureFlowImageFilter
  {*nx::core::Uuid::FromString("9103009a-8884-4097-8c34-aec7019589ea"), nx::core::FilterTraits<MorphologicalGradientImageFilter>::uuid}, // ITKMorphologicalGradientImageFilter
  {*nx::core::Uuid::FromString("9bfcf09b-b510-4d46-982c-d2e35dedefdf"), nx::core::FilterTraits<MorphologicalWatershedFromMarkersImageFilter>::uuid}, // ITKMorphologicalWatershedFromMarkersImageFilter
  {*nx::core::Uuid::FromString("f70337e5-4435-41f7-aecc-d79b4b1faccd"), nx::core::FilterTraits<MorphologicalWatershedImageFilter>::uuid}, // ITKMorphologicalWatershedImageFilter
  {*nx::core::Uuid::FromString("9d8ce30e-c75e-4ca8-b6be-0b11baa7e6ce"), nx::core::FilterTraits<NormalizeImageFilter>::uuid}, // ITKNormalizeImageFilter
  {*nx::core::Uuid::FromString("eb2ab30f-698c-44b2-a70a-e8a0961bca1c"), nx::core::FilterTraits<NormalizeToConstantImageFilter>::uuid}, // ITKNormalizeToConstantImageFilter
  {*nx::core::Uuid::FromString("6d67ad50-7e89-4678-85db-0b3247a2cfb9"), nx::core::FilterTraits<NotImageFilter>::uuid}, // ITKNotImageFilter
  {*nx::core::Uuid::FromString("c4225a23-0b23-4782-b509-296fb39a672b"), nx::core::FilterTraits<OpeningByReconstructionImageFilter>::uuid}, // ITKOpeningByReconstructionImageFilter
  {*nx::core::Uuid::FromString("30f37bcd-701f-4e64-aa9d-1181469d3fb5"), nx::core::FilterTraits<OtsuMultipleThresholdsImageFilter>::uuid}, // ITKOtsuMultipleThresholdsImageFilter
  {*nx::core::Uuid::FromString("a2b8a295-5730-477a-b97b-8a0b0400397c"), nx::core::FilterTraits<RegionalMaximaImageFilter>::uuid}, // ITKRegionalMaximaImageFilter
  {*nx::core::Uuid::FromString("7ec0883e-ac48-40e9-8b97-11bdfde721e2"), nx::core::FilterTraits<RegionalMinimaImageFilter>::uuid}, // ITKRegionalMinimaImageFilter
  {*nx::core::Uuid::FromString("37e29d16-1020-478c-a506-c121e8f670ad"), nx::core::FilterTraits<RelabelComponentImageFilter>::uuid}, // ITKRelabelComponentImageFilter
  {*nx::core::Uuid::FromString("f08ea34d-9ad8-456c-a81b-9b3790b29379"), nx::core::FilterTraits<RescaleIntensityImageFilter>::uuid}, // ITKRescaleIntensityImageFilter
  {*nx::core::Uuid::FromString("cb9ec2b6-80d9-42e6-807b-d908bea6daea"), nx::core::FilterTraits<SigmoidImageFilter>::uuid}, // ITKSigmoidImageFilter
  {*nx::core::Uuid::FromString("2f42e771-1d84-4468-8991-9a1fad7eb740"), nx::core::FilterTraits<SignedDanielssonDistanceMapImageFilter>::uuid}, // ITKSignedDanielssonDistanceMapImageFilter
  {*nx::core::Uuid::FromString("e81f72d3-e806-4afe-ab4c-795c6a3f526f"), nx::core::FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid}, // ITKSignedMaurerDistanceMapImageFilter
  {*nx::core::Uuid::FromString("06c76c7a-c384-44be-bd01-6fd58070cd65"), nx::core::FilterTraits<SinImageFilter>::uuid}, // ITKSinImageFilter
  {*nx::core::Uuid::FromString("74859077-69ba-40ad-965f-8699dde1c22d"), nx::core::FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid}, // ITKSmoothingRecursiveGaussianImageFilter
  {*nx::core::Uuid::FromString("05c7c812-4e33-4e9a-bf27-d4c17f5dff68"), nx::core::FilterTraits<SqrtImageFilter>::uuid}, // ITKSqrtImageFilter
  {*nx::core::Uuid::FromString("385ca853-626c-43bb-ae86-db8d8b72693b"), nx::core::FilterTraits<SquareImageFilter>::uuid}, // ITKSquareImageFilter
  {*nx::core::Uuid::FromString("8e022ad4-cbbe-4b08-a89f-8f529d799db1"), nx::core::FilterTraits<StandardDeviationProjectionImageFilter>::uuid}, // ITKStandardDeviationProjectionImageFilter
  {*nx::core::Uuid::FromString("f784bd72-f89f-4f2b-8fec-274c9c3e2394"), nx::core::FilterTraits<SumProjectionImageFilter>::uuid}, // ITKSumProjectionImageFilter
  {*nx::core::Uuid::FromString("7cf3c08e-1af1-4540-aa08-4488a74923fc"), nx::core::FilterTraits<TanImageFilter>::uuid}, // ITKTanImageFilter
  {*nx::core::Uuid::FromString("ddf222f3-4af2-4583-967d-3eb9b86e77b4"), nx::core::FilterTraits<ThresholdImageFilter>::uuid}, // ITKThresholdImageFilter
  {*nx::core::Uuid::FromString("dc0f6771-87bf-457f-8430-2d943e039a24"), nx::core::FilterTraits<ThresholdMaximumConnectedComponentsImageFilter>::uuid}, // ITKThresholdMaximumConnectedComponentsImageFilter
  {*nx::core::Uuid::FromString("2c0bb4f6-69fe-4c43-a32e-21b4b11efcff"), nx::core::FilterTraits<ValuedRegionalMaximaImageFilter>::uuid}, // ITKValuedRegionalMaximaImageFilter
  {*nx::core::Uuid::FromString("38548e01-6a3c-49fb-b7b6-489f965cc61e"), nx::core::FilterTraits<ValuedRegionalMinimaImageFilter>::uuid}, // ITKValuedRegionalMinimaImageFilter
  {*nx::core::Uuid::FromString("2f377682-d0a8-4dea-8c68-60c2c523a074"), nx::core::FilterTraits<WhiteTopHatImageFilter>::uuid}, // ITKWhiteTopHatImageFilter
  {*nx::core::Uuid::FromString("a181ee3e-1678-4133-b9c5-a9dd7bfec62f"), nx::core::FilterTraits<WriteImageFilter>::uuid}, // ITKImageWriterFilter
  {*nx::core::Uuid::FromString("89a14057-776a-4e35-80b6-69361e078394"), nx::core::FilterTraits<ZeroCrossingImageFilter>::uuid}, // ITKZeroCrossingImageFilter
};

// The SIMPL map contains only UUIDs that ITKImageProcessing does not own.
// Two loaded plugins cannot claim the same SIMPL UUID. Add ITK-owned mappings only after ITKImageProcessing is removed.
} // namespace nx::core
/* clang-format on */
