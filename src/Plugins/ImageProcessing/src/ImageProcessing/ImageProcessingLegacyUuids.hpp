#pragma once

#include "simplnx/Common/Uuid.hpp"

/**
 * @file ImageProcessingLegacyUuids.hpp
 * @brief Holds the filter UUIDs of the removed ITKImageProcessing plugin.
 *
 * A stored pipeline can name one of these UUIDs. ImageProcessingLegacyUUIDMapping.hpp maps each one to
 * the equivalent ImageProcessing filter. The benchmark and the tests use the same constants, so one UUID
 * has one definition.
 *
 * Uuid::FromString is constexpr. A malformed UUID therefore stops the build instead of the test run.
 */
namespace nx::core::ImageProcessingLegacyUuids
{

inline constexpr Uuid k_ITKBinaryContourImageFilter = *Uuid::FromString("ed214e76-6954-49b4-817b-13f92315e722");
inline constexpr Uuid k_ITKBinaryOpeningByReconstructionImageFilter = *Uuid::FromString("02c15392-382c-406d-a174-07ea6fa11b67");
inline constexpr Uuid k_ITKClosingByReconstructionImageFilter = *Uuid::FromString("b5ff32a8-e799-4f72-8d13-e2581f748562");
inline constexpr Uuid k_ITKGrayscaleFillholeImageFilter = *Uuid::FromString("1f1dd9e4-d361-432b-a22b-5535664ee545");
inline constexpr Uuid k_ITKGrayscaleGrindPeakImageFilter = *Uuid::FromString("6aa5b193-c290-4fe3-a409-7759a62d48ea");
inline constexpr Uuid k_ITKHConvexImageFilter = *Uuid::FromString("620240a1-0b04-4bc7-a4c3-531917de4bc0");
inline constexpr Uuid k_ITKHMaximaImageFilter = *Uuid::FromString("40039f72-30b0-4a3f-8ea4-2f76c5f65bc1");
inline constexpr Uuid k_ITKHMinimaImageFilter = *Uuid::FromString("9b2eb24b-90e5-41c0-9230-1044170ee8ea");
inline constexpr Uuid k_ITKLabelContourImageFilter = *Uuid::FromString("b64ff45d-3661-4926-9e87-5dd7b379b261");
inline constexpr Uuid k_ITKMorphologicalGradientImageFilter = *Uuid::FromString("9103009a-8884-4097-8c34-aec7019589ea");
inline constexpr Uuid k_ITKMorphologicalWatershedImageFilter = *Uuid::FromString("f70337e5-4435-41f7-aecc-d79b4b1faccd");
inline constexpr Uuid k_ITKOpeningByReconstructionImageFilter = *Uuid::FromString("c4225a23-0b23-4782-b509-296fb39a672b");
inline constexpr Uuid k_ITKSignedMaurerDistanceMapImageFilter = *Uuid::FromString("e81f72d3-e806-4afe-ab4c-795c6a3f526f");
inline constexpr Uuid k_ITKValuedRegionalMaximaImageFilter = *Uuid::FromString("2c0bb4f6-69fe-4c43-a32e-21b4b11efcff");
inline constexpr Uuid k_ITKValuedRegionalMinimaImageFilter = *Uuid::FromString("38548e01-6a3c-49fb-b7b6-489f965cc61e");
inline constexpr Uuid k_ITKImageReader = *Uuid::FromString("d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2");
inline constexpr Uuid k_ITKMhaFileReader = *Uuid::FromString("41c33a08-0052-4915-8d53-d503f85f30d9");
inline constexpr Uuid k_ITKImportImageStack = *Uuid::FromString("dcf980b7-ecca-46d1-af31-ac65f6e3b6bb");
inline constexpr Uuid k_ITKImportFijiMontage = *Uuid::FromString("4c48ea16-13ef-4281-89cf-315be5fb857d");
inline constexpr Uuid k_ITKImageWriterFilter = *Uuid::FromString("a181ee3e-1678-4133-b9c5-a9dd7bfec62f");

#ifndef ImageProcessing_LEAN_AND_MEAN

inline constexpr Uuid k_ITKAbsImageFilter = *Uuid::FromString("e9dd12bc-f7fa-4ba2-98b0-fec3326bf620");
inline constexpr Uuid k_ITKAcosImageFilter = *Uuid::FromString("e7411c44-95ab-4623-8bf4-59b63d2d08c5");
inline constexpr Uuid k_ITKAdaptiveHistogramEqualizationImageFilter = *Uuid::FromString("ea3e7439-8327-4190-8ff7-49ecc321718f");
inline constexpr Uuid k_ITKApproximateSignedDistanceMapImageFilter = *Uuid::FromString("87ed0d3a-c394-4bb5-ac7f-6cc746984b09");
inline constexpr Uuid k_ITKAsinImageFilter = *Uuid::FromString("1b463492-041f-4680-abb1-0b94a3019063");
inline constexpr Uuid k_ITKAtanImageFilter = *Uuid::FromString("39933f50-088c-46ac-a421-d238f1b178fd");
inline constexpr Uuid k_ITKBinaryDilateImageFilter = *Uuid::FromString("d4e973cb-c501-4c64-af26-fcf791c0f36d");
inline constexpr Uuid k_ITKBinaryErodeImageFilter = *Uuid::FromString("243dd30b-d1f0-42ad-8b47-77d57f9fc262");
inline constexpr Uuid k_ITKBinaryMorphologicalClosingImageFilter = *Uuid::FromString("abb27e0c-b049-4f60-8355-178d86bb1de4");
inline constexpr Uuid k_ITKBinaryMorphologicalOpeningImageFilter = *Uuid::FromString("861ccb46-dbce-41bf-a66f-25cc18cd1073");
inline constexpr Uuid k_ITKBinaryProjectionImageFilter = *Uuid::FromString("04ea495e-2cf0-4dba-8d29-cf33a38c094d");
inline constexpr Uuid k_ITKBinaryThinningImageFilter = *Uuid::FromString("8fcd24cb-769d-400f-97cf-9b4dad1b8cd2");
inline constexpr Uuid k_ITKBinaryThresholdImageFilter = *Uuid::FromString("ba2494b0-c4f0-43ff-9d08-900395900e0c");
inline constexpr Uuid k_ITKBlackTopHatImageFilter = *Uuid::FromString("b7471b64-2282-449b-82b4-3ce359e9dda0");
inline constexpr Uuid k_ITKBoundedReciprocalImageFilter = *Uuid::FromString("da72b2ae-74ef-4198-820f-6a381b3fff05");
inline constexpr Uuid k_ITKConnectedComponentImageFilter = *Uuid::FromString("905354c1-d55b-4436-b9f7-f4a6e80e5c0f");
inline constexpr Uuid k_ITKCosImageFilter = *Uuid::FromString("6fe37f77-ceae-4839-9cf6-3ca7a70e14d0");
inline constexpr Uuid k_ITKCurvatureAnisotropicDiffusionImageFilter = *Uuid::FromString("ada68f29-b1f2-44a2-86dc-f0cd28f54633");
inline constexpr Uuid k_ITKCurvatureFlowImageFilter = *Uuid::FromString("fe5b2ed3-54dd-4207-ad88-48a95134684a");
inline constexpr Uuid k_ITKDanielssonDistanceMapImageFilter = *Uuid::FromString("f0cd4faf-a676-41ed-9ea5-859035f94836");
inline constexpr Uuid k_ITKDilateObjectMorphologyImageFilter = *Uuid::FromString("e3f7c642-4c16-47f6-ac5c-cd276d61bfa6");
inline constexpr Uuid k_ITKDiscreteGaussianImageFilter = *Uuid::FromString("025edc1a-986d-4005-92d1-545dfdc13abd");
inline constexpr Uuid k_ITKDoubleThresholdImageFilter = *Uuid::FromString("e268a65f-33f1-493f-a6c7-4635e57df3c4");
inline constexpr Uuid k_ITKErodeObjectMorphologyImageFilter = *Uuid::FromString("db3fe379-6ce4-4be3-baca-1cbff00aca6a");
inline constexpr Uuid k_ITKExpImageFilter = *Uuid::FromString("264977e7-cc0d-4d2b-ba9c-a30765b498b2");
inline constexpr Uuid k_ITKExpNegativeImageFilter = *Uuid::FromString("2c84cc7c-01ab-4550-9ba5-b9fa58b74599");
inline constexpr Uuid k_ITKGradientAnisotropicDiffusionImageFilter = *Uuid::FromString("9dcef77b-e7d2-4a2a-b310-bfa80e8ea7c5");
inline constexpr Uuid k_ITKGradientMagnitudeImageFilter = *Uuid::FromString("719df7b2-8db2-43eb-a40c-a015982eec08");
inline constexpr Uuid k_ITKGradientMagnitudeRecursiveGaussianImageFilter = *Uuid::FromString("32db4ae4-4087-4688-874a-b1d725188f18");
inline constexpr Uuid k_ITKGrayscaleDilateImageFilter = *Uuid::FromString("944f4d1a-adb1-401a-9c0f-e2085ef2f6dc");
inline constexpr Uuid k_ITKGrayscaleErodeImageFilter = *Uuid::FromString("3ceb1e6f-f4a4-4d8f-82d5-ecd20d4fc285");
inline constexpr Uuid k_ITKGrayscaleMorphologicalClosingImageFilter = *Uuid::FromString("8b859b54-93d4-4341-8fbf-85e1e461d5b5");
inline constexpr Uuid k_ITKGrayscaleMorphologicalOpeningImageFilter = *Uuid::FromString("54433e92-fb7d-40f4-95bf-eb3db76d5caa");
inline constexpr Uuid k_ITKIntensityWindowingImageFilter = *Uuid::FromString("ee317bf6-79aa-4dcc-b59e-b0246dc3fcfa");
inline constexpr Uuid k_ITKInvertIntensityImageFilter = *Uuid::FromString("9958d587-5698-4ea5-b8ea-fb71428b5d02");
inline constexpr Uuid k_ITKIsoContourDistanceImageFilter = *Uuid::FromString("e82fa143-7ac2-4c09-a88c-9ea71d47d594");
inline constexpr Uuid k_ITKLaplacianRecursiveGaussianImageFilter = *Uuid::FromString("782d76a4-e3f6-4c2a-a1b0-7456a3e77f24");
inline constexpr Uuid k_ITKLog10ImageFilter = *Uuid::FromString("900ca377-e79d-4b54-b298-33d518238099");
inline constexpr Uuid k_ITKLogImageFilter = *Uuid::FromString("4b6655ad-4e6c-4e68-a771-55ca0ae40915");
inline constexpr Uuid k_ITKMaskImageFilter = *Uuid::FromString("d3138266-3f34-4d6e-8e21-904c94351293");
inline constexpr Uuid k_ITKMaximumProjectionImageFilter = *Uuid::FromString("6dfe9167-d77d-41c7-aebf-569d6190645d");
inline constexpr Uuid k_ITKMeanProjectionImageFilter = *Uuid::FromString("62ffddba-cc57-45fc-a93a-27914eea11ad");
inline constexpr Uuid k_ITKMedianImageFilter = *Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8");
inline constexpr Uuid k_ITKMedianProjectionImageFilter = *Uuid::FromString("00e48f6b-8a00-414f-b3d9-49d48a3f9a00");
inline constexpr Uuid k_ITKMinimumProjectionImageFilter = *Uuid::FromString("86898336-8680-4c4e-b166-3f8de9e3d4f2");
inline constexpr Uuid k_ITKMinMaxCurvatureFlowImageFilter = *Uuid::FromString("b836c081-6692-411d-81d0-a50afce6b288");
inline constexpr Uuid k_ITKMorphologicalWatershedFromMarkersImageFilter = *Uuid::FromString("9bfcf09b-b510-4d46-982c-d2e35dedefdf");
inline constexpr Uuid k_ITKNormalizeImageFilter = *Uuid::FromString("9d8ce30e-c75e-4ca8-b6be-0b11baa7e6ce");
inline constexpr Uuid k_ITKNormalizeToConstantImageFilter = *Uuid::FromString("eb2ab30f-698c-44b2-a70a-e8a0961bca1c");
inline constexpr Uuid k_ITKNotImageFilter = *Uuid::FromString("6d67ad50-7e89-4678-85db-0b3247a2cfb9");
inline constexpr Uuid k_ITKOtsuMultipleThresholdsImageFilter = *Uuid::FromString("30f37bcd-701f-4e64-aa9d-1181469d3fb5");
inline constexpr Uuid k_ITKRegionalMaximaImageFilter = *Uuid::FromString("a2b8a295-5730-477a-b97b-8a0b0400397c");
inline constexpr Uuid k_ITKRegionalMinimaImageFilter = *Uuid::FromString("7ec0883e-ac48-40e9-8b97-11bdfde721e2");
inline constexpr Uuid k_ITKRelabelComponentImageFilter = *Uuid::FromString("37e29d16-1020-478c-a506-c121e8f670ad");
inline constexpr Uuid k_ITKRescaleIntensityImageFilter = *Uuid::FromString("f08ea34d-9ad8-456c-a81b-9b3790b29379");
inline constexpr Uuid k_ITKSigmoidImageFilter = *Uuid::FromString("cb9ec2b6-80d9-42e6-807b-d908bea6daea");
inline constexpr Uuid k_ITKSignedDanielssonDistanceMapImageFilter = *Uuid::FromString("2f42e771-1d84-4468-8991-9a1fad7eb740");
inline constexpr Uuid k_ITKSinImageFilter = *Uuid::FromString("06c76c7a-c384-44be-bd01-6fd58070cd65");
inline constexpr Uuid k_ITKSmoothingRecursiveGaussianImageFilter = *Uuid::FromString("74859077-69ba-40ad-965f-8699dde1c22d");
inline constexpr Uuid k_ITKSqrtImageFilter = *Uuid::FromString("05c7c812-4e33-4e9a-bf27-d4c17f5dff68");
inline constexpr Uuid k_ITKSquareImageFilter = *Uuid::FromString("385ca853-626c-43bb-ae86-db8d8b72693b");
inline constexpr Uuid k_ITKStandardDeviationProjectionImageFilter = *Uuid::FromString("8e022ad4-cbbe-4b08-a89f-8f529d799db1");
inline constexpr Uuid k_ITKSumProjectionImageFilter = *Uuid::FromString("f784bd72-f89f-4f2b-8fec-274c9c3e2394");
inline constexpr Uuid k_ITKTanImageFilter = *Uuid::FromString("7cf3c08e-1af1-4540-aa08-4488a74923fc");
inline constexpr Uuid k_ITKThresholdImageFilter = *Uuid::FromString("ddf222f3-4af2-4583-967d-3eb9b86e77b4");
inline constexpr Uuid k_ITKThresholdMaximumConnectedComponentsImageFilter = *Uuid::FromString("dc0f6771-87bf-457f-8430-2d943e039a24");
inline constexpr Uuid k_ITKWhiteTopHatImageFilter = *Uuid::FromString("2f377682-d0a8-4dea-8c68-60c2c523a074");
inline constexpr Uuid k_ITKZeroCrossingImageFilter = *Uuid::FromString("89a14057-776a-4e35-80b6-69361e078394");
#endif
} // namespace nx::core::ImageProcessingLegacyUuids
