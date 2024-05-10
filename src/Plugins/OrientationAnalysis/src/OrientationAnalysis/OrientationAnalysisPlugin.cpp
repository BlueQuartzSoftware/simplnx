#include "OrientationAnalysisPlugin.hpp"
#include "OrientationAnalysisLegacyUUIDMapping.hpp"

#include "OrientationAnalysis/OrientationAnalysis_filter_registration.hpp"

using namespace nx::core;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<nx::core::Uuid, nx::core::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{f4a7c2df-e9b0-5da9-b745-a862666d6c99}").value(), Uuid::FromString("f4a7c2df-e9b0-5da9-b745-a862666d6c99").value()}, /* BadDataNeighborOrientationCheck */
    {Uuid::FromString("{f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792}").value(), Uuid::FromString("f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792").value()}, /* ChangeAngleRepresentationFilter */
    {Uuid::FromString("{e1343abe-e5ad-5eb1-a89d-c209e620e4de}").value(), Uuid::FromString("e1343abe-e5ad-5eb1-a89d-c209e620e4de").value()}, /* ConvertHexGridToSquareGrid */
    {Uuid::FromString("{e5629880-98c4-5656-82b8-c9fe2b9744de}").value(), Uuid::FromString("e5629880-98c4-5656-82b8-c9fe2b9744de").value()}, /* ConvertOrientations */
    {Uuid::FromString("{439e31b7-3198-5d0d-aef6-65a9e9c1a016}").value(), Uuid::FromString("439e31b7-3198-5d0d-aef6-65a9e9c1a016").value()}, /* ConvertQuaternion */
    {Uuid::FromString("{2a0bfcd3-2517-5117-b164-964dfac8fe22}").value(), Uuid::FromString("2a0bfcd3-2517-5117-b164-964dfac8fe22").value()}, /* CreateEnsembleInfo */
    {Uuid::FromString("{c4398303-db7d-506e-81ea-08f253895ccb}").value(), Uuid::FromString("c4398303-db7d-506e-81ea-08f253895ccb").value()}, /* CreateLambertSphere */
    {Uuid::FromString("{b78d8825-d3ac-5351-be20-172f07fd2aec}").value(), Uuid::FromString("b78d8825-d3ac-5351-be20-172f07fd2aec").value()}, /* EMsoftSO3Sampler */
    {Uuid::FromString("{6e332fca-0475-5fec-821e-e01f668ec1d3}").value(), Uuid::FromString("6e332fca-0475-5fec-821e-e01f668ec1d3").value()}, /* EbsdToH5Ebsd */
    {Uuid::FromString("{33a37a47-d002-5c18-b270-86025881fe1e}").value(), Uuid::FromString("33a37a47-d002-5c18-b270-86025881fe1e").value()}, /* ReadEnsembleInfo */
    {Uuid::FromString("{c5a9a96c-7570-5279-b383-cc25ebae0046}").value(), Uuid::FromString("c5a9a96c-7570-5279-b383-cc25ebae0046").value()}, /* FindAvgCAxes */
    {Uuid::FromString("{bf7036d8-25bd-540e-b6de-3a5ab0e42c5f}").value(), Uuid::FromString("bf7036d8-25bd-540e-b6de-3a5ab0e42c5f").value()}, /* FindAvgOrientations */
    {Uuid::FromString("{8071facb-8905-5699-b345-105ae4ac33ff}").value(), Uuid::FromString("8071facb-8905-5699-b345-105ae4ac33ff").value()}, /* ComputeBoundaryStrengths */
    {Uuid::FromString("{68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e}").value(), Uuid::FromString("68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e").value()}, /* ComputeCAxisLocations */
    {Uuid::FromString("{94f986fc-1295-5e32-9808-752855fa658a}").value(), Uuid::FromString("94f986fc-1295-5e32-9808-752855fa658a").value()}, /* FindDistsToCharactGBs */
    {Uuid::FromString("{cdd50b83-ea09-5499-b008-4b253cf4c246}").value(), Uuid::FromString("cdd50b83-ea09-5499-b008-4b253cf4c246").value()}, /* ComputeFeatureNeighborCAxisMisalignments */
    {Uuid::FromString("{1a0848da-2edd-52c0-b111-62a4dc6d2886}").value(), Uuid::FromString("1a0848da-2edd-52c0-b111-62a4dc6d2886").value()}, /* ComputeFeatureReferenceCAxisMisorientations */
    {Uuid::FromString("{428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c}").value(), Uuid::FromString("428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c").value()}, /* ComputeFeatureReferenceMisorientations */
    {Uuid::FromString("{6e97ff50-48bf-5403-a049-1d271bd72df9}").value(), Uuid::FromString("6e97ff50-48bf-5403-a049-1d271bd72df9").value()}, /* ComputeGBCD */
    {Uuid::FromString("{d67e9f28-2fe5-5188-b0f8-323a7e603de6}").value(), Uuid::FromString("d67e9f28-2fe5-5188-b0f8-323a7e603de6").value()}, /* ComputeGBCDMetricBased */
    {Uuid::FromString("{00d20627-5b88-56ba-ac7a-fc2a4b337903}").value(), Uuid::FromString("00d20627-5b88-56ba-ac7a-fc2a4b337903").value()}, /* ComputeGBPDMetricBased */
    {Uuid::FromString("{88d332c1-cf6c-52d3-a38d-22f6eae19fa6}").value(), Uuid::FromString("88d332c1-cf6c-52d3-a38d-22f6eae19fa6").value()}, /* ComputeKernelAvgMisorientations */
    {Uuid::FromString("{286dd493-4fea-54f4-b59e-459dd13bbe57}").value(), Uuid::FromString("286dd493-4fea-54f4-b59e-459dd13bbe57").value()}, /* ComputeMisorientations */
    {Uuid::FromString("{e67ca06a-176f-58fc-a676-d6ee5553511a}").value(), Uuid::FromString("e67ca06a-176f-58fc-a676-d6ee5553511a").value()}, /* ComputeSchmids */
    {Uuid::FromString("{97523038-5fb2-5e82-9177-ed3e8b24b4bd}").value(), Uuid::FromString("97523038-5fb2-5e82-9177-ed3e8b24b4bd").value()}, /* ComputeSlipTransmissionMetrics */
    {Uuid::FromString("{a10124f3-05d0-5f49-93a0-e93926f5b48b}").value(), Uuid::FromString("a10124f3-05d0-5f49-93a0-e93926f5b48b").value()}, /* FindTwinBoundaries */
    {Uuid::FromString("{b0e30e6d-912d-5a7e-aeed-750134aba86b}").value(), Uuid::FromString("b0e30e6d-912d-5a7e-aeed-750134aba86b").value()}, /* FindTwinBoundarySchmidFactors */
    {Uuid::FromString("{9a6677a6-b9e5-5fee-afa2-27e868cab8ca}").value(), Uuid::FromString("9a6677a6-b9e5-5fee-afa2-27e868cab8ca").value()}, /* GenerateFZQuaternions */
    {Uuid::FromString("{0a121e03-3922-5c29-962d-40d88653f4b6}").value(), Uuid::FromString("0a121e03-3922-5c29-962d-40d88653f4b6").value()}, /* GenerateFaceIPFColoring */
    {Uuid::FromString("{a50e6532-8075-5de5-ab63-945feb0de7f7}").value(), Uuid::FromString("a50e6532-8075-5de5-ab63-945feb0de7f7").value()}, /* GenerateIPFColors */
    {Uuid::FromString("{ec58f4fe-8e51-527e-9536-8b6f185684be}").value(), Uuid::FromString("ec58f4fe-8e51-527e-9536-8b6f185684be").value()}, /* GenerateOrientationMatrixTranspose */
    {Uuid::FromString("{630d7486-75ea-5e04-874c-894460cd7c4d}").value(), Uuid::FromString("630d7486-75ea-5e04-874c-894460cd7c4d").value()}, /* ComputeQuaternionConjugate */
    {Uuid::FromString("{27c724cc-8b69-5ebe-b90e-29d33858a032}").value(), Uuid::FromString("27c724cc-8b69-5ebe-b90e-29d33858a032").value()}, /* WriteINLFile */
    {Uuid::FromString("{179b0c7a-4e62-5070-ba49-ae58d5ccbfe8}").value(), Uuid::FromString("179b0c7a-4e62-5070-ba49-ae58d5ccbfe8").value()}, /* ImportEbsdMontage */
    {Uuid::FromString("{8abdea7d-f715-5a24-8165-7f946bbc2fe9}").value(), Uuid::FromString("8abdea7d-f715-5a24-8165-7f946bbc2fe9").value()}, /* ReadH5EspritData */
    {Uuid::FromString("{3ff4701b-3a0c-52e3-910a-fa927aa6584c}").value(), Uuid::FromString("3ff4701b-3a0c-52e3-910a-fa927aa6584c").value()}, /* ReadH5OimData */
    {Uuid::FromString("{6427cd5e-0ad2-5a24-8847-29f8e0720f4f}").value(), Uuid::FromString("6427cd5e-0ad2-5a24-8847-29f8e0720f4f").value()}, /* NeighborOrientationCorrelation */
    {Uuid::FromString("{5af9c1e6-ed6f-5672-9ae0-2b931344d729}").value(), Uuid::FromString("5af9c1e6-ed6f-5672-9ae0-2b931344d729").value()}, /* OrientationUtility */
    {Uuid::FromString("{b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40}").value(), Uuid::FromString("b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40").value()}, /* ReadAngData */
    {Uuid::FromString("{d1df969c-0428-53c3-b61d-99ea2bb6da28}").value(), Uuid::FromString("d1df969c-0428-53c3-b61d-99ea2bb6da28").value()}, /* ReadCtfData */
    {Uuid::FromString("{4ef7f56b-616e-5a80-9e68-1da8f35ad235}").value(), Uuid::FromString("4ef7f56b-616e-5a80-9e68-1da8f35ad235").value()}, /* ReadH5Ebsd */
    {Uuid::FromString("{17410178-4e5f-58b9-900e-8194c69200ab}").value(), Uuid::FromString("17410178-4e5f-58b9-900e-8194c69200ab").value()}, /* ReplaceElementAttributesWithNeighborValues */
    {Uuid::FromString("{a2b62395-1a7d-5058-a840-752d8f8e2430}").value(), Uuid::FromString("a2b62395-1a7d-5058-a840-752d8f8e2430").value()}, /* RodriguesConvertor */
    {Uuid::FromString("{ef9420b2-8c46-55f3-8ae4-f53790639de4}").value(), Uuid::FromString("ef9420b2-8c46-55f3-8ae4-f53790639de4").value()}, /* RotateEulerRefFrame */
    {Uuid::FromString("{3630623e-724b-5154-a060-a5fca4ecfff5}").value(), Uuid::FromString("3630623e-724b-5154-a060-a5fca4ecfff5").value()}, /* Stereographic3D */
    {Uuid::FromString("{a10bb78e-fcff-553d-97d6-830a43c85385}").value(), Uuid::FromString("a10bb78e-fcff-553d-97d6-830a43c85385").value()}, /* WritePoleFigure */
    {Uuid::FromString("{a4952f40-22dd-54ec-8c38-69c3fcd0e6f7}").value(), Uuid::FromString("a4952f40-22dd-54ec-8c38-69c3fcd0e6f7").value()}, /* WriteStatsGenOdfAngleFile */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("c09cf01b-014e-5adb-84eb-ea76fc79eeb1");
} // namespace

OrientationAnalysisPlugin::OrientationAnalysisPlugin()
: AbstractPlugin(k_ID, "OrientationAnalysis", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

OrientationAnalysisPlugin::~OrientationAnalysisPlugin() = default;

AbstractPlugin::SIMPLMapType OrientationAnalysisPlugin::getSimplToSimplnxMap() const
{
  return nx::core::k_SIMPL_to_OrientationAnalysis;
}

SIMPLNX_DEF_PLUGIN(OrientationAnalysisPlugin)
