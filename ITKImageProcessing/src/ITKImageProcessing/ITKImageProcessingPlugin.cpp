#include "ITKImageProcessingPlugin.hpp"

#include <itkBMPImageIOFactory.h>
#include <itkBioRadImageIOFactory.h>
#include <itkGE4ImageIOFactory.h>
#include <itkGE5ImageIOFactory.h>
#include <itkGiplImageIOFactory.h>
#include <itkJPEGImageIOFactory.h>
#include <itkMRCImageIOFactory.h>
#include <itkMetaImageIOFactory.h>
#include <itkNiftiImageIOFactory.h>
#include <itkNrrdImageIOFactory.h>
#include <itkPNGImageIOFactory.h>
#include <itkStimulateImageIOFactory.h>
#include <itkTIFFImageIOFactory.h>
#include <itkVTKImageIOFactory.h>

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{7c2a7c4e-4582-52a6-92de-16b626d43fbf}").value(), Uuid::FromString("6b0a7eb8-72fa-57ab-a349-54a29243361a").value()}, /* ApplyDewarpParameters */
    {Uuid::FromString("{30687f44-9c10-5617-bcb8-4109cbd6e55e}").value(), Uuid::FromString("605faa74-cddd-57b0-b896-29efff0ae42b").value()}, /* AxioVisionV4ToTileConfiguration */
    {Uuid::FromString("{5394f60c-b0f0-5f98-85da-3b5730109060}").value(), Uuid::FromString("ccf8ac91-6e7e-5dcb-b277-5dc0db152583").value()}, /* CalcDewarpParameters */
    {Uuid::FromString("{09f45c29-1cfb-566c-b3ae-d832b4f95905}").value(), Uuid::FromString("1b7965a1-a338-5934-aef1-0c6384f27f78").value()}, /* ITKAbsImage */
    {Uuid::FromString("{b09ec654-87a5-5dfa-9949-aa69f1fbfdd1}").value(), Uuid::FromString("30969d24-d450-537b-b01c-e6d55170bfc5").value()}, /* ITKAcosImage */
    {Uuid::FromString("{2d5a7599-5e01-5489-a107-23b704d2b5eb}").value(), Uuid::FromString("86813ed4-3250-5614-bc46-62924ae36447").value()}, /* ITKAdaptiveHistogramEqualizationImage */
    {Uuid::FromString("{066712e3-0378-566e-8236-1796c88d5e02}").value(), Uuid::FromString("a95c60a9-3c8c-5a8a-9198-acecba8059af").value()}, /* ITKApproximateSignedDistanceMapImage */
    {Uuid::FromString("{79509ab1-24e1-50e4-9351-c5ce7cd87a72}").value(), Uuid::FromString("56c9a254-75b8-5469-a2f2-4083caa887f9").value()}, /* ITKAsinImage */
    {Uuid::FromString("{e938569d-3644-5d00-a4e0-ab327937457d}").value(), Uuid::FromString("fa838868-3875-5e08-89e9-26b139d1e32c").value()}, /* ITKAtanImage */
    {Uuid::FromString("{18ab754c-3219-59c8-928e-5fb4a09174e0}").value(), Uuid::FromString("df77e07b-af59-5aa8-9847-a8fe6a49efe9").value()}, /* ITKBilateralImage */
    {Uuid::FromString("{0cf0698d-65eb-58bb-9f89-51e875432197}").value(), Uuid::FromString("52b153d0-2a0f-5709-986e-54bbbf639dab").value()}, /* ITKBinaryClosingByReconstructionImage */
    {Uuid::FromString("{3c451ac9-bfef-5e41-bae9-3957a0fc26a1}").value(), Uuid::FromString("c21515fc-9d55-516b-9550-1ef82687a997").value()}, /* ITKBinaryContourImage */
    {Uuid::FromString("{f86167ad-a1a1-557b-97ea-92a3618baa8f}").value(), Uuid::FromString("99d34cf4-5a1b-5ffa-9f98-71bdb0d89f2f").value()}, /* ITKBinaryDilateImage */
    {Uuid::FromString("{522c5249-c048-579a-98dd-f7aadafc5578}").value(), Uuid::FromString("79d5592c-ecd6-5877-ac4d-8029c823ec0c").value()}, /* ITKBinaryErodeImage */
    {Uuid::FromString("{1d8deea7-c6d0-5fa1-95cb-b14f19df97e8}").value(), Uuid::FromString("cd137ed9-e172-5870-8455-ce0c79b40ad6").value()}, /* ITKBinaryMorphologicalClosingImage */
    {Uuid::FromString("{704c801a-7549-54c4-9def-c4bb58d07fd1}").value(), Uuid::FromString("3f9dc10d-8996-5448-bfeb-43d3fe3d114e").value()}, /* ITKBinaryMorphologicalOpeningImage */
    {Uuid::FromString("{bd1c2353-0a39-52c0-902b-ee64721994c7}").value(), Uuid::FromString("6f3b1215-f7f1-571d-a4f6-5c83fc2bb3bf").value()}, /* ITKBinaryOpeningByReconstructionImage */
    {Uuid::FromString("{606c3700-f793-5852-9a0f-3123bd212447}").value(), Uuid::FromString("01757a68-fb95-5e14-a341-0ad3888e9c8d").value()}, /* ITKBinaryProjectionImage */
    {Uuid::FromString("{dcceeb50-5924-5eae-88ea-34793cf545a9}").value(), Uuid::FromString("bd2ec326-253e-5c71-9e9e-78e005bb12ee").value()}, /* ITKBinaryThinningImage */
    {Uuid::FromString("{ba8a3f2e-3963-57c0-a8da-239e25de0526}").value(), Uuid::FromString("1afb779c-a3b0-5e71-a240-da816628e0c4").value()}, /* ITKBinaryThresholdImage */
    {Uuid::FromString("{4f51765f-ee36-52d0-96b6-f2aca3c24e7c}").value(), Uuid::FromString("93743db7-d68f-5a22-b68f-38b2b7e53027").value()}, /* ITKBinomialBlurImage */
    {Uuid::FromString("{e26e7359-f72c-5924-b42e-dd5dd454a794}").value(), Uuid::FromString("50797fba-3c19-505d-8fb6-b960a851a36d").value()}, /* ITKBlackTopHatImage */
    {Uuid::FromString("{17f60a64-de93-59aa-a5e2-063e0aadafb7}").value(), Uuid::FromString("c291a624-85b0-55d8-a97a-5fadc8d70c0e").value()}, /* ITKBoundedReciprocalImage */
    {Uuid::FromString("{6138f949-e363-5ca3-bc31-4f3daed65da5}").value(), Uuid::FromString("5e20c6fb-cfc3-52f9-8c79-f7a7e6cef5af").value()}, /* ITKBoxMeanImage */
    {Uuid::FromString("{3ad55a9b-98ec-549a-a6a8-c8967366260b}").value(), Uuid::FromString("aa333a2b-dadb-5401-b40a-136d4d57253e").value()}, /* ITKCastImage */
    {Uuid::FromString("{99a7aa3c-f945-5e77-875a-23b5231ab3f4}").value(), Uuid::FromString("5475c26a-00f7-59a4-99b1-70d34fda0633").value()}, /* ITKClosingByReconstructionImage */
    {Uuid::FromString("{bf554dd5-a927-5969-b651-1c47d386afce}").value(), Uuid::FromString("96325950-27cf-5491-a164-1bcd6f5a64b5").value()}, /* ITKConnectedComponentImage */
    {Uuid::FromString("{2c2d7bf6-1e78-52e6-80aa-58b504ce0912}").value(), Uuid::FromString("537405d3-59ba-5f66-a291-db7dd11421d8").value()}, /* ITKCosImage */
    {Uuid::FromString("{009fb2d0-6f65-5406-bb2a-4a883d0bc18c}").value(), Uuid::FromString("60387baa-1ab0-5b32-ada4-f0e99d554117").value()}, /* ITKCurvatureAnisotropicDiffusionImage */
    {Uuid::FromString("{653f26dd-a5ef-5c75-b6f6-bc096268f25e}").value(), Uuid::FromString("fd634520-0f50-5d90-a314-e3463c377552").value()}, /* ITKCurvatureFlowImage */
    {Uuid::FromString("{53d5b289-a716-559b-89d9-5ebb34f714ca}").value(), Uuid::FromString("0602e360-4255-5744-8ade-8761d2c87143").value()}, /* ITKDanielssonDistanceMapImage */
    {Uuid::FromString("{dbf29c6d-461c-55e7-a6c4-56477d9da55b}").value(), Uuid::FromString("966d7652-a93d-5b38-9046-499a7d8655cd").value()}, /* ITKDilateObjectMorphologyImage */
    {Uuid::FromString("{53df5340-f632-598f-8a9b-802296b3a95c}").value(), Uuid::FromString("65a00e45-15fe-548a-a761-6e85b6e049f7").value()}, /* ITKDiscreteGaussianImage */
    {Uuid::FromString("{7fcfa231-519e-58da-bf8f-ad0f728bf0fe}").value(), Uuid::FromString("31feb4d2-2d4e-55da-a296-776fe58f0f5b").value()}, /* ITKDoubleThresholdImage */
    {Uuid::FromString("{caea0698-4253-518b-ab3f-8ebc140d92ea}").value(), Uuid::FromString("b376041b-c8f6-5664-a5be-6d95daf6c8a2").value()}, /* ITKErodeObjectMorphologyImage */
    {Uuid::FromString("{a6fb3f3a-6c7a-5dfc-a4f1-75ff1d62c32f}").value(), Uuid::FromString("6b2c4391-d360-5279-bc58-59ded8ba549b").value()}, /* ITKExpImage */
    {Uuid::FromString("{634c2306-c1ee-5a45-a55c-f8286e36999a}").value(), Uuid::FromString("219a4879-422b-5b0b-9404-84fd59fa8359").value()}, /* ITKExpNegativeImage */
    {Uuid::FromString("{a0d962b7-9d5c-5abc-a078-1fe795df4663}").value(), Uuid::FromString("2731ffd1-49fb-51a7-b899-e6c9910df516").value()}, /* ITKFFTNormalizedCorrelationImage */
    {Uuid::FromString("{98d0b50b-9639-53a0-9e30-2fae6f7ab869}").value(), Uuid::FromString("0278d40d-91b9-5e0b-8169-4a44298cdc39").value()}, /* ITKGradientAnisotropicDiffusionImage */
    {Uuid::FromString("{3aa99151-e722-51a0-90ba-71e93347ab09}").value(), Uuid::FromString("a7a403d5-3b9c-5872-b3c1-6c1a38f4ca5b").value()}, /* ITKGradientMagnitudeImage */
    {Uuid::FromString("{fd688b32-d90e-5945-905b-2b7187b46265}").value(), Uuid::FromString("1a852e31-bfd0-58e1-8ccc-026b81a97d42").value()}, /* ITKGradientMagnitudeRecursiveGaussianImage */
    {Uuid::FromString("{66cec151-2950-51f8-8a02-47d3516d8721}").value(), Uuid::FromString("37d108a0-f802-5bfd-860f-38bfeea2fa6d").value()}, /* ITKGrayscaleDilateImage */
    {Uuid::FromString("{aef4e804-3f7a-5dc0-911c-b1f16a393a69}").value(), Uuid::FromString("77d708ee-acec-5fb8-95b4-0e60f5246f85").value()}, /* ITKGrayscaleErodeImage */
    {Uuid::FromString("{54c8dd45-88c4-5d4b-8a39-e3cc595e1cf8}").value(), Uuid::FromString("ad63c408-23e7-5fdb-bd68-e1d66f987893").value()}, /* ITKGrayscaleFillholeImage */
    {Uuid::FromString("{d910551f-4eec-55c9-b0ce-69c2277e61bd}").value(), Uuid::FromString("d87ade03-07d5-505d-8a44-ccec822ccebf").value()}, /* ITKGrayscaleGrindPeakImage */
    {Uuid::FromString("{849a1903-5595-5029-bbde-6f4b68b2a25c}").value(), Uuid::FromString("41a1aefe-ffc3-52dc-8259-c0ed3928a4d0").value()}, /* ITKGrayscaleMorphologicalClosingImage */
    {Uuid::FromString("{c88ac42b-9477-5088-9ec0-862af1e0bb56}").value(), Uuid::FromString("866f4e17-494f-5ffb-bea7-781489c22e48").value()}, /* ITKGrayscaleMorphologicalOpeningImage */
    {Uuid::FromString("{8bc34707-04c0-5e83-8583-48ee19306a1d}").value(), Uuid::FromString("2bdbd16c-6085-5d17-9673-57a4ef336ad5").value()}, /* ITKHConvexImage */
    {Uuid::FromString("{932a6df4-212e-53a1-a2ab-c29bd376bb7b}").value(), Uuid::FromString("c41fccc8-e0df-50d9-8692-c253dfb7094d").value()}, /* ITKHMaximaImage */
    {Uuid::FromString("{f1d7cf59-9b7c-53cb-b71a-76cf91c86e8f}").value(), Uuid::FromString("2f3a1212-ba67-5797-b3d3-e1dd23199573").value()}, /* ITKHMinimaImage */
    {Uuid::FromString("{33ca886c-42b9-524a-984a-dab24f8bb74c}").value(), Uuid::FromString("b13a3b4d-3a9f-557a-908e-ce989ea7af9d").value()}, /* ITKHistogramMatchingImage */
    {Uuid::FromString("{653b7b5c-03cb-5b32-8c3e-3637745e5ff6}").value(), Uuid::FromString("c434dee5-0ae3-5a0f-86f6-9dba7f8c360f").value()}, /* ITKImageReader */
    {Uuid::FromString("{11473711-f94d-5d96-b749-ec36a81ad338}").value(), Uuid::FromString("c9ee33da-3962-5dda-9091-513902d6c3f7").value()}, /* ITKImageWriter */
    {Uuid::FromString("{5878723b-cc16-5486-ac5f-ff0107107e74}").value(), Uuid::FromString("edd9c7f3-aed0-51db-b7e8-97571ea409e8").value()}, /* ITKImportFijiMontage */
    {Uuid::FromString("{cf7d7497-9573-5102-bedd-38f86a6cdfd4}").value(), Uuid::FromString("63f5addd-cb67-503e-ae22-c8ef0542ec37").value()}, /* ITKImportImageStack */
    {Uuid::FromString("{cdb130af-3616-57b1-be59-fe18113b2621}").value(), Uuid::FromString("1c3f0124-1f39-58a5-bc8c-3439e2fda0ac").value()}, /* ITKImportRoboMetMontage */
    {Uuid::FromString("{4faf4c59-6f29-53af-bc78-5aecffce0e37}").value(), Uuid::FromString("0dabd1ef-3d2d-5388-a0ed-982bbf775296").value()}, /* ITKIntensityWindowingImage */
    {Uuid::FromString("{c6e10fa5-5462-546b-b34b-0f0ea75a7e43}").value(), Uuid::FromString("a85ddca8-3870-5f56-bc3c-62e8c9f7fa2e").value()}, /* ITKInvertIntensityImage */
    {Uuid::FromString("{e9952e0e-97e4-53aa-8c6c-115fd18f0b89}").value(), Uuid::FromString("b017d59f-1117-5259-973f-76592321b9c6").value()}, /* ITKIsoContourDistanceImage */
    {Uuid::FromString("{668f0b90-b504-5fba-b648-7c9677e1f452}").value(), Uuid::FromString("22c176b5-0e94-5b7a-a633-845ebb97267d").value()}, /* ITKLabelContourImage */
    {Uuid::FromString("{9677659d-b08c-58a4-ac4d-fba7d9093454}").value(), Uuid::FromString("04f741b2-b44c-5175-8a6e-2887afc76a5e").value()}, /* ITKLaplacianRecursiveGaussianImage */
    {Uuid::FromString("{c4963181-c788-5efc-8560-d005a5e01eea}").value(), Uuid::FromString("d03cc15b-e06c-5ba6-a296-5a99f3baabad").value()}, /* ITKLaplacianSharpeningImage */
    {Uuid::FromString("{dbfd1a57-2a17-572d-93a7-8fd2f8e92eb0}").value(), Uuid::FromString("c446e71c-f591-5419-ba4e-4d39c909ac83").value()}, /* ITKLog10Image */
    {Uuid::FromString("{69aba77c-9a35-5251-a18a-e3728ddd2963}").value(), Uuid::FromString("973b1e0f-eb81-5a08-9c57-98ad5d82e7d3").value()}, /* ITKLogImage */
    {Uuid::FromString("{97102d65-9c32-576a-9177-c59d958bad10}").value(), Uuid::FromString("872bcc76-a3cb-5ae8-b2da-e0323c608e17").value()}, /* ITKMaskImage */
    {Uuid::FromString("{b2cb7ad7-6f62-51c4-943d-54d19c64e7be}").value(), Uuid::FromString("0d3cbb5e-cc05-5b02-bc9d-08f72a8f3973").value()}, /* ITKMaximumProjectionImage */
    {Uuid::FromString("{85c061bc-3ad7-5abc-8fc7-140678311706}").value(), Uuid::FromString("d3d21fb5-b153-5415-bb95-0b5f42a101de").value()}, /* ITKMeanProjectionImage */
    {Uuid::FromString("{cc27ee9a-9946-56ad-afd4-6e98b71f417d}").value(), Uuid::FromString("9f4e213c-ba3c-5988-9dc1-5554aac5770f").value()}, /* ITKMedianImage */
    {Uuid::FromString("{1a7e8483-5838-585c-8d71-e79f836133c4}").value(), Uuid::FromString("89538825-7456-5fa9-a34f-0f0601492edd").value()}, /* ITKMedianProjectionImage */
    {Uuid::FromString("{bd9bdf46-a229-544a-b158-151920261a63}").value(), Uuid::FromString("21f5d49f-53f1-5a86-a184-d90f6aa4b849").value()}, /* ITKMinMaxCurvatureFlowImage */
    {Uuid::FromString("{6394a737-4a31-5593-9bb5-28492129ccf7}").value(), Uuid::FromString("7afdb663-3a05-5cfa-b9af-d5c299568fdc").value()}, /* ITKMinimumProjectionImage */
    {Uuid::FromString("{12c83608-c4c5-5c72-b22f-a7696e3f5448}").value(), Uuid::FromString("246f1889-4ad8-5750-ba10-63232abc53f6").value()}, /* ITKMorphologicalGradientImage */
    {Uuid::FromString("{81308863-094b-511d-9aa9-865e02e2b8d5}").value(), Uuid::FromString("0af42200-3cf9-5871-bbcc-d0245587bc08").value()}, /* ITKMorphologicalWatershedFromMarkersImage */
    {Uuid::FromString("{b2248340-a371-5899-90a2-86047950f0a2}").value(), Uuid::FromString("9838447d-a492-5165-a0b9-a1672b63aa49").value()}, /* ITKMorphologicalWatershedImage */
    {Uuid::FromString("{c080e143-1895-5f71-9799-06b8c2d58faf}").value(), Uuid::FromString("1f368789-8927-52b0-9121-0cfda4e4c9f0").value()}, /* ITKMultiScaleHessianBasedObjectnessImage */
    {Uuid::FromString("{5b905619-c46b-5690-b6fa-8e97cf4537b8}").value(), Uuid::FromString("f8656962-fbd9-50f2-9097-b650b46800d8").value()}, /* ITKNormalizeImage */
    {Uuid::FromString("{001dd629-7032-56a9-99ec-ffaf2caf2ab0}").value(), Uuid::FromString("a84fe404-7c6f-5666-89ee-ab197391227c").value()}, /* ITKNormalizeToConstantImage */
    {Uuid::FromString("{c8362fb9-d3ab-55c0-902b-274cc27d9bb8}").value(), Uuid::FromString("1cf8d778-2c88-5bd1-b2a7-8a980b2413e2").value()}, /* ITKNotImage */
    {Uuid::FromString("{ca04004f-fb11-588d-9f77-d00b3ee9ad2a}").value(), Uuid::FromString("038fd2d1-4ce4-5d92-9706-b8559e0889c6").value()}, /* ITKOpeningByReconstructionImage */
    {Uuid::FromString("{6e66563a-edcf-5e11-bc1d-ceed36d8493f}").value(), Uuid::FromString("3b784c44-549a-547b-aabc-ffe9fde3b3bd").value()}, /* ITKOtsuMultipleThresholdsImage */
    {Uuid::FromString("{4388723b-cc16-3477-ac6f-fe0107107e74}").value(), Uuid::FromString("06e9aabd-aaeb-514b-bd53-954aea0befc9").value()}, /* ITKPCMTileRegistration */
    {Uuid::FromString("{ed61aebd-3a47-5ee1-8c9e-4ce205111b76}").value(), Uuid::FromString("25681bf3-31aa-552a-9319-99f4a75c7240").value()}, /* ITKPatchBasedDenoisingImage */
    {Uuid::FromString("{d3856d4c-5651-5eab-8740-489a87fa8bdd}").value(), Uuid::FromString("5d59eecd-7c23-5cdf-9922-d4ab4b907534").value()}, /* ITKProxTVImage */
    {Uuid::FromString("{bae507d6-4d0a-5ad2-8279-c674f1c90db8}").value(), Uuid::FromString("884242a9-93ca-57cc-906d-d39cf032cd2c").value()}, /* ITKRGBToLuminanceImage */
    {Uuid::FromString("{49b5feb1-ec05-5a26-af25-00053151d944}").value(), Uuid::FromString("13f22db0-d406-5ebd-ab59-0ccc0cfa62ba").value()}, /* ITKRefineTileCoordinates */
    {Uuid::FromString("{9af89118-2d15-54ca-9590-75df8be33317}").value(), Uuid::FromString("884d1eda-bfb1-5c43-8162-3fc6c771135e").value()}, /* ITKRegionalMaximaImage */
    {Uuid::FromString("{f8ed09ae-1f84-5668-a4ad-d770e264f01e}").value(), Uuid::FromString("766f8e03-2d87-51f2-80d0-6a4b45e1febc").value()}, /* ITKRegionalMinimaImage */
    {Uuid::FromString("{4398d76d-c9aa-5161-bb48-92dd9daaa352}").value(), Uuid::FromString("27dedcb7-b8c3-5b6b-a69f-377fe8fe0361").value()}, /* ITKRelabelComponentImage */
    {Uuid::FromString("{77bf2192-851d-5127-9add-634c1ef4f67f}").value(), Uuid::FromString("1ced378c-8abe-5801-b41b-305929bb6641").value()}, /* ITKRescaleIntensityImage */
    {Uuid::FromString("{602c270d-ec55-580c-9108-785ba8530964}").value(), Uuid::FromString("3ce99bca-d424-5955-871a-7c5762215adc").value()}, /* ITKSaltAndPepperNoiseImage */
    {Uuid::FromString("{31d325fa-e605-5415-85ab-ab93e8cbf32f}").value(), Uuid::FromString("ba054dd0-a851-52a0-9ef8-fb58635bba10").value()}, /* ITKShiftScaleImage */
    {Uuid::FromString("{97f20f54-276b-54f3-87c9-5eaf16e6c4df}").value(), Uuid::FromString("ba053c20-4ae5-5257-bf06-d584202b1a16").value()}, /* ITKShotNoiseImage */
    {Uuid::FromString("{e6675be7-e98d-5e0f-a088-ba15cc301038}").value(), Uuid::FromString("ef0c0080-21ca-5526-a15e-23546f2a7454").value()}, /* ITKSigmoidImage */
    {Uuid::FromString("{fc192fa8-b6b7-539c-9763-f683724da626}").value(), Uuid::FromString("8a689c33-9c20-5562-9c3d-cb616e66104b").value()}, /* ITKSignedDanielssonDistanceMapImage */
    {Uuid::FromString("{bb15d42a-3077-582a-be1a-76b2bae172e9}").value(), Uuid::FromString("4a70c6bd-ca68-5234-9a7b-e2b5202ca0f6").value()}, /* ITKSignedMaurerDistanceMapImage */
    {Uuid::FromString("{1eb4b4f7-1704-58e4-9f78-8726a5c8c302}").value(), Uuid::FromString("ff7b833f-bc50-5570-9429-0a4ac031d1ae").value()}, /* ITKSinImage */
    {Uuid::FromString("{0fd06492-06b1-5044-964c-e0555c556327}").value(), Uuid::FromString("5589a536-86a2-5552-9edb-aab8b12cd624").value()}, /* ITKSmoothingRecursiveGaussianImage */
    {Uuid::FromString("{764085a4-6ecb-5fb7-891d-2fda208ba5d8}").value(), Uuid::FromString("dc8483fb-640b-52c3-8f67-ff95109a04b6").value()}, /* ITKSpeckleNoiseImage */
    {Uuid::FromString("{8087dcad-68f2-598b-9670-d0f57647a445}").value(), Uuid::FromString("20143318-9561-515f-90fa-9b0f6e4d3dad").value()}, /* ITKSqrtImage */
    {Uuid::FromString("{f092420e-14a0-5dc0-91f8-de0082103aef}").value(), Uuid::FromString("c39a00b9-5045-53c5-b9a7-7840483b1089").value()}, /* ITKSquareImage */
    {Uuid::FromString("{89b327a7-c6a0-5965-b8aa-9d8bfcedcc76}").value(), Uuid::FromString("f6695137-f547-5e0c-b458-3bf36577a1c4").value()}, /* ITKStandardDeviationProjectionImage */
    {Uuid::FromString("{fa4efd40-f4a6-5524-9fc6-e1f8bbb2c42f}").value(), Uuid::FromString("fa4efd40-f4a6-5524-9fc6-e1f8bbb2c42f").value()}, /* ITKStitchMontage */
    {Uuid::FromString("{40714670-b3bd-554c-badb-787d8bab7568}").value(), Uuid::FromString("06633145-024a-5846-b503-82b83b890bc2").value()}, /* ITKSumProjectionImage */
    {Uuid::FromString("{672810d9-5ec0-59c1-a209-8fb56c7a018a}").value(), Uuid::FromString("bfad4166-03d8-5258-bcbb-e69a6ca9a734").value()}, /* ITKTanImage */
    {Uuid::FromString("{5845ee06-5c8a-5a74-80fb-c820bd8dfb75}").value(), Uuid::FromString("9f27bade-d674-585f-8e74-a58fbef78286").value()}, /* ITKThresholdImage */
    {Uuid::FromString("{2a531add-79fd-5c07-98a6-f875d2ecef4e}").value(), Uuid::FromString("64f5dc49-4720-5a65-8e73-6a3db7212eb8").value()}, /* ITKThresholdMaximumConnectedComponentsImage */
    {Uuid::FromString("{10aff542-81c5-5f09-9797-c7171c40b6a0}").value(), Uuid::FromString("6515d2c3-9635-5cc1-a3a6-e9de53b3e4ef").value()}, /* ITKValuedRegionalMaximaImage */
    {Uuid::FromString("{739a0908-cb60-50f7-a484-b2157d023093}").value(), Uuid::FromString("a7a93aed-3639-55fd-8c88-d79c5fd2583e").value()}, /* ITKValuedRegionalMinimaImage */
    {Uuid::FromString("{9d42ede4-fd4b-57fe-9595-50c16deaa3a2}").value(), Uuid::FromString("f36d8296-71c1-56b3-940b-8a6859d457e3").value()}, /* ITKVectorConnectedComponentImage */
    {Uuid::FromString("{bc1051ba-6c67-5391-809b-48627ed47fa7}").value(), Uuid::FromString("40448555-a4f7-576a-9b8d-1a48c6191e92").value()}, /* ITKVectorRescaleIntensityImage */
    {Uuid::FromString("{02e059f7-8055-52b4-9d48-915b67d1e39a}").value(), Uuid::FromString("ad5a5d18-4c53-5698-9ba5-01584d7e13a0").value()}, /* ITKWhiteTopHatImage */
    {Uuid::FromString("{0259fa1a-4706-5df1-8418-95ffc7b932dd}").value(), Uuid::FromString("e9e99d5b-25df-5e53-a7e7-024ed548e8a6").value()}, /* ITKZeroCrossingImage */
    {Uuid::FromString("{a48f7a51-0ca9-584f-a0ca-4bfebdc41d7c}").value(), Uuid::FromString("340ff86c-a203-5ad9-8c87-aa6227ff2009").value()}, /* IlluminationCorrection */
    {Uuid::FromString("{411b008c-006f-51b2-ba05-99e51a01af3c}").value(), Uuid::FromString("f793164c-d8a4-576a-8cd1-846f8f835a6a").value()}, /* ImportAxioVisionV4Montage */
    {Uuid::FromString("{c5474cd1-bea9-5a33-a0df-516e5735bab4}").value(), Uuid::FromString("0c8cf5ce-1166-5cbc-8349-c4cd87af7498").value()}, /* ImportVectorImageStack */
    {Uuid::FromString("{774ec947-eed6-5eb2-a01b-ee6470e61521}").value(), Uuid::FromString("59d54885-256e-5860-b37d-70b318e6321b").value()}, /* ImportZenInfoMontage */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
} // namespace

ITKImageProcessingPlugin::ITKImageProcessingPlugin()
: AbstractPlugin(k_ID, "ITKImageProcessing", "<<--Description was not read-->>", "BlueQuartz Software")
{
  registerFilters();
  RegisterITKImageIO();
}

ITKImageProcessingPlugin::~ITKImageProcessingPlugin() noexcept = default;

void ITKImageProcessingPlugin::RegisterITKImageIO()
{
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::NrrdImageIOFactory::RegisterOneFactory();
  itk::PNGImageIOFactory::RegisterOneFactory();
  itk::TIFFImageIOFactory::RegisterOneFactory();
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::BMPImageIOFactory::RegisterOneFactory();
  itk::MetaImageIOFactory::RegisterOneFactory();
  itk::NiftiImageIOFactory::RegisterOneFactory();
  itk::GiplImageIOFactory::RegisterOneFactory();
  itk::VTKImageIOFactory::RegisterOneFactory();
  itk::StimulateImageIOFactory::RegisterOneFactory();
  itk::BioRadImageIOFactory::RegisterOneFactory();
  itk::GE4ImageIOFactory::RegisterOneFactory();
  itk::GE5ImageIOFactory::RegisterOneFactory();
  itk::MRCImageIOFactory::RegisterOneFactory();
}

std::vector<complex::H5::IDataFactory*> ITKImageProcessingPlugin::getDataFactories() const
{
  return {};
}

std::vector<std::string> ITKImageProcessingPlugin::GetList2DSupportedFileExtensions()
{
  return {".png", ".tif", ".jpg", ".jpeg", ".bmp"};
}

COMPLEX_DEF_PLUGIN(ITKImageProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ITKImageProcessing/plugin_filter_registration.h"
