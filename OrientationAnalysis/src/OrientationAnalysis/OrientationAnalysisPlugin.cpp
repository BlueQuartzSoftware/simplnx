#include "OrientationAnalysisPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{f4a7c2df-e9b0-5da9-b745-a862666d6c99}").value(), Uuid::FromString("5c7e5d98-8cfa-5d06-bbe5-f53b3e6817c7").value()}, /* BadDataNeighborOrientationCheck */
    {Uuid::FromString("{f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792}").value(), Uuid::FromString("c373b370-ddf5-532c-b37f-d5aa57bdf884").value()}, /* ChangeAngleRepresentation */
    {Uuid::FromString("{e1343abe-e5ad-5eb1-a89d-c209e620e4de}").value(), Uuid::FromString("53b4980c-5c5f-5eb0-a02a-9f1c6858cec6").value()}, /* ConvertHexGridToSquareGrid */
    {Uuid::FromString("{e5629880-98c4-5656-82b8-c9fe2b9744de}").value(), Uuid::FromString("f2426580-cd33-56ce-8aa7-31ac1cf9ae72").value()}, /* ConvertOrientations */
    {Uuid::FromString("{439e31b7-3198-5d0d-aef6-65a9e9c1a016}").value(), Uuid::FromString("2caf96e4-dfce-521d-8560-d0f470759e03").value()}, /* ConvertQuaternion */
    {Uuid::FromString("{2a0bfcd3-2517-5117-b164-964dfac8fe22}").value(), Uuid::FromString("b56368c3-8ddb-5dc2-ae45-6d7f23eb51e9").value()}, /* CreateEnsembleInfo */
    {Uuid::FromString("{c4398303-db7d-506e-81ea-08f253895ccb}").value(), Uuid::FromString("36174e6b-7d4c-5388-b65a-d139332c5c07").value()}, /* CreateLambertSphere */
    {Uuid::FromString("{b78d8825-d3ac-5351-be20-172f07fd2aec}").value(), Uuid::FromString("98d7e2a0-ab15-5a3a-ae5d-9926c7fe74b7").value()}, /* EMsoftSO3Sampler */
    {Uuid::FromString("{6e332fca-0475-5fec-821e-e01f668ec1d3}").value(), Uuid::FromString("37c6098a-d05c-5fb9-8730-1440f075351a").value()}, /* EbsdToH5Ebsd */
    {Uuid::FromString("{33a37a47-d002-5c18-b270-86025881fe1e}").value(), Uuid::FromString("eaa20ce4-b078-5097-b074-138c085d20b8").value()}, /* EnsembleInfoReader */
    {Uuid::FromString("{c5a9a96c-7570-5279-b383-cc25ebae0046}").value(), Uuid::FromString("6c49e72c-614d-5b1f-8cf7-320c92d17c8a").value()}, /* FindAvgCAxes */
    {Uuid::FromString("{bf7036d8-25bd-540e-b6de-3a5ab0e42c5f}").value(), Uuid::FromString("5d403c0c-d11b-53ab-8702-38207b5545e4").value()}, /* FindAvgOrientations */
    {Uuid::FromString("{8071facb-8905-5699-b345-105ae4ac33ff}").value(), Uuid::FromString("1fe81a18-79fd-5236-8f20-b0a93d2de9a4").value()}, /* FindBoundaryStrengths */
    {Uuid::FromString("{68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e}").value(), Uuid::FromString("2fdd35d7-ab59-5616-8db8-470da47f3a52").value()}, /* FindCAxisLocations */
    {Uuid::FromString("{94f986fc-1295-5e32-9808-752855fa658a}").value(), Uuid::FromString("cfcaf881-d90e-5e2b-ad6e-0265177107ca").value()}, /* FindDistsToCharactGBs */
    {Uuid::FromString("{cdd50b83-ea09-5499-b008-4b253cf4c246}").value(), Uuid::FromString("a8878e9f-0a64-5cae-a3cf-a73fdc5b0ecf").value()}, /* FindFeatureNeighborCAxisMisalignments */
    {Uuid::FromString("{1a0848da-2edd-52c0-b111-62a4dc6d2886}").value(), Uuid::FromString("92502590-0051-5d5a-8e3d-5ca7f2274d83").value()}, /* FindFeatureReferenceCAxisMisorientations */
    {Uuid::FromString("{428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c}").value(), Uuid::FromString("7fb3a9c0-0a43-5dd9-8eb9-914ce51c0136").value()}, /* FindFeatureReferenceMisorientations */
    {Uuid::FromString("{6e97ff50-48bf-5403-a049-1d271bd72df9}").value(), Uuid::FromString("963872e6-659c-5176-926b-1eafa127f759").value()}, /* FindGBCD */
    {Uuid::FromString("{d67e9f28-2fe5-5188-b0f8-323a7e603de6}").value(), Uuid::FromString("1cf0de20-4cdf-53a3-893d-4f9d3a70b62e").value()}, /* FindGBCDMetricBased */
    {Uuid::FromString("{00d20627-5b88-56ba-ac7a-fc2a4b337903}").value(), Uuid::FromString("aab9db9c-ed7e-5d19-b6e0-4564f7bc7295").value()}, /* FindGBPDMetricBased */
    {Uuid::FromString("{88d332c1-cf6c-52d3-a38d-22f6eae19fa6}").value(), Uuid::FromString("d0112925-2356-542d-8ac3-ab47d71f4f49").value()}, /* FindKernelAvgMisorientations */
    {Uuid::FromString("{286dd493-4fea-54f4-b59e-459dd13bbe57}").value(), Uuid::FromString("5eb8f75a-2e99-581e-b27e-13b795f3e93b").value()}, /* FindMisorientations */
    {Uuid::FromString("{e67ca06a-176f-58fc-a676-d6ee5553511a}").value(), Uuid::FromString("89c291a9-1c4d-59c2-ba69-b47beb4de594").value()}, /* FindSchmids */
    {Uuid::FromString("{97523038-5fb2-5e82-9177-ed3e8b24b4bd}").value(), Uuid::FromString("0683d013-45d3-5361-b137-60fbea65de07").value()}, /* FindSlipTransmissionMetrics */
    {Uuid::FromString("{a10124f3-05d0-5f49-93a0-e93926f5b48b}").value(), Uuid::FromString("dba17c3c-26a7-5390-8d8c-bacbb234d8cf").value()}, /* FindTwinBoundaries */
    {Uuid::FromString("{b0e30e6d-912d-5a7e-aeed-750134aba86b}").value(), Uuid::FromString("98d6940d-f000-5547-a955-94771aa770a8").value()}, /* FindTwinBoundarySchmidFactors */
    {Uuid::FromString("{9a6677a6-b9e5-5fee-afa2-27e868cab8ca}").value(), Uuid::FromString("6d052444-6c89-5d20-9b40-417636c2a906").value()}, /* GenerateFZQuaternions */
    {Uuid::FromString("{0a121e03-3922-5c29-962d-40d88653f4b6}").value(), Uuid::FromString("f5a92f32-b387-50f3-9d02-f54203b5b60c").value()}, /* GenerateFaceIPFColoring */
    {Uuid::FromString("{7cd30864-7bcf-5c10-aea7-d107373e2d40}").value(), Uuid::FromString("b4fccecd-d827-5d8e-aa45-5f5f50de11f8").value()}, /* GenerateFaceMisorientationColoring */
    {Uuid::FromString("{a50e6532-8075-5de5-ab63-945feb0de7f7}").value(), Uuid::FromString("50ba2f15-b6ab-58fa-aace-60edb910ee87").value()}, /* GenerateIPFColors */
    {Uuid::FromString("{ec58f4fe-8e51-527e-9536-8b6f185684be}").value(), Uuid::FromString("ec58f4fe-8e51-527e-9536-8b6f185684be").value()}, /* GenerateOrientationMatrixTranspose */
    {Uuid::FromString("{630d7486-75ea-5e04-874c-894460cd7c4d}").value(), Uuid::FromString("630d7486-75ea-5e04-874c-894460cd7c4d").value()}, /* GenerateQuaternionConjugate */
    {Uuid::FromString("{27c724cc-8b69-5ebe-b90e-29d33858a032}").value(), Uuid::FromString("d4e3aeb2-4d20-5c4a-8749-668ba4700c01").value()}, /* INLWriter */
    {Uuid::FromString("{179b0c7a-4e62-5070-ba49-ae58d5ccbfe8}").value(), Uuid::FromString("981792fc-152d-5b16-b6b4-6cae45b4c1f6").value()}, /* ImportEbsdMontage */
    {Uuid::FromString("{8abdea7d-f715-5a24-8165-7f946bbc2fe9}").value(), Uuid::FromString("8abdea7d-f715-5a24-8165-7f946bbc2fe9").value()}, /* ImportH5EspritData */
    {Uuid::FromString("{3ff4701b-3a0c-52e3-910a-fa927aa6584c}").value(), Uuid::FromString("0bffdd87-4aaf-573b-8bfa-3a004772cccb").value()}, /* ImportH5OimData */
    {Uuid::FromString("{6427cd5e-0ad2-5a24-8847-29f8e0720f4f}").value(), Uuid::FromString("b5eec343-6b30-5e06-a544-8bd8619c2593").value()}, /* NeighborOrientationCorrelation */
    {Uuid::FromString("{5af9c1e6-ed6f-5672-9ae0-2b931344d729}").value(), Uuid::FromString("9b30f229-d541-5039-a81f-b5371d4354df").value()}, /* OrientationUtility */
    {Uuid::FromString("{b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40}").value(), Uuid::FromString("b7d4690a-eeaa-501c-bca4-fbfb6f3cbfcd").value()}, /* ReadAngData */
    {Uuid::FromString("{d1df969c-0428-53c3-b61d-99ea2bb6da28}").value(), Uuid::FromString("2fcaf2c2-309c-5c7b-b938-a5e784540e91").value()}, /* ReadCtfData */
    {Uuid::FromString("{4ef7f56b-616e-5a80-9e68-1da8f35ad235}").value(), Uuid::FromString("a814dfba-f1b3-5020-949a-52528069f363").value()}, /* ReadH5Ebsd */
    {Uuid::FromString("{17410178-4e5f-58b9-900e-8194c69200ab}").value(), Uuid::FromString("24e515cd-f586-59db-86bd-6bc135aa27ef").value()}, /* ReplaceElementAttributesWithNeighborValues */
    {Uuid::FromString("{a2b62395-1a7d-5058-a840-752d8f8e2430}").value(), Uuid::FromString("a2b62395-1a7d-5058-a840-752d8f8e2430").value()}, /* RodriguesConvertor */
    {Uuid::FromString("{ef9420b2-8c46-55f3-8ae4-f53790639de4}").value(), Uuid::FromString("351a19eb-899b-5325-9d05-f43127219138").value()}, /* RotateEulerRefFrame */
    {Uuid::FromString("{3630623e-724b-5154-a060-a5fca4ecfff5}").value(), Uuid::FromString("7403472c-d468-5511-b28d-6b66fec56cf0").value()}, /* Stereographic3D */
    {Uuid::FromString("{a10bb78e-fcff-553d-97d6-830a43c85385}").value(), Uuid::FromString("15b8857a-a577-5bea-9cf2-37f12f450523").value()}, /* WritePoleFigure */
    {Uuid::FromString("{a4952f40-22dd-54ec-8c38-69c3fcd0e6f7}").value(), Uuid::FromString("40288ce5-92cd-5dcd-b136-3e61569b4b23").value()}, /* WriteStatsGenOdfAngleFile */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("c09cf01b-014e-5adb-84eb-ea76fc79eeb1");
} // namespace

OrientationAnalysisPlugin::OrientationAnalysisPlugin()
: AbstractPlugin(k_ID, "OrientationAnalysis", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

OrientationAnalysisPlugin::~OrientationAnalysisPlugin() = default;

std::vector<complex::H5::IDataFactory*> OrientationAnalysisPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(OrientationAnalysisPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "OrientationAnalysis/plugin_filter_registration.h"
