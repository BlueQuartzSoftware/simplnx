#pragma once

#include "complex/Plugin/AbstractPlugin.hpp"

#include <nlohmann/json.hpp>

// clang-format off
// #include "OrientationAnalysis/Filters/ConvertHexGridToSquareGrid.hpp"
// #include "OrientationAnalysis/Filters/CreateLambertSphere.hpp"
// #include "OrientationAnalysis/Filters/EbsdToH5Ebsd.hpp"
// #include "OrientationAnalysis/Filters/EMsoftSO3Sampler.hpp"
// #include "OrientationAnalysis/Filters/FindAvgCAxes.hpp"
#include "OrientationAnalysis/Filters/FindBoundaryStrengthsFilter.hpp"
// #include "OrientationAnalysis/Filters/FindCAxisLocations.hpp"
// #include "OrientationAnalysis/Filters/FindDistsToCharactGBs.hpp"
// #include "OrientationAnalysis/Filters/FindFeatureNeighborCAxisMisalignments.hpp"
#include "OrientationAnalysis/Filters/FindFeatureReferenceCAxisMisorientationsFilter.hpp"
// #include "OrientationAnalysis/Filters/FindGBCDMetricBased.hpp"
// #include "OrientationAnalysis/Filters/FindGBPDMetricBased.hpp"
#include "OrientationAnalysis/Filters/FindSlipTransmissionMetricsFilter.hpp"
// #include "OrientationAnalysis/Filters/FindTwinBoundaries.hpp"
// #include "OrientationAnalysis/Filters/FindTwinBoundarySchmidFactors.hpp"
// #include "OrientationAnalysis/Filters/GenerateOrientationMatrixTranspose.hpp"
// #include "OrientationAnalysis/Filters/ImportEbsdMontage.hpp"
// #include "OrientationAnalysis/Filters/ReadH5EspritData.hpp"
// #include "OrientationAnalysis/Filters/ReadH5OimData.hpp"
// #include "OrientationAnalysis/Filters/WriteINLFile.hpp"
// #include "OrientationAnalysis/Filters/OrientationUtility.hpp"
// #include "OrientationAnalysis/Filters/ReplaceElementAttributesWithNeighborValues.hpp"
// #include "OrientationAnalysis/Filters/Stereographic3D.hpp"
// #include "OrientationAnalysis/Filters/WritePoleFigure.hpp"
// #include "OrientationAnalysis/Filters/WriteStatsGenOdfAngleFile.hpp"
#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/AlignSectionsMutualInformationFilter.hpp"
#include "OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/ConvertOrientations.hpp"
#include "OrientationAnalysis/Filters/ConvertQuaternionFilter.hpp"
#include "OrientationAnalysis/Filters/CreateEnsembleInfoFilter.hpp"
#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/Filters/ReadEnsembleInfoFilter.hpp"
#include "OrientationAnalysis/Filters/WriteGBCDGMTFileFilter.hpp"
#include "OrientationAnalysis/Filters/WriteGBCDTriangleDataFilter.hpp"
#include "OrientationAnalysis/Filters/FindAvgCAxesFilter.hpp"
#include "OrientationAnalysis/Filters/FindAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindFeatureReferenceCAxisMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindFeatureReferenceMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindGBCDFilter.hpp"
#include "OrientationAnalysis/Filters/FindKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindSchmidsFilter.hpp"
#include "OrientationAnalysis/Filters/FindShapesFilter.hpp"
#include "OrientationAnalysis/Filters/GenerateFaceIPFColoringFilter.hpp"
#include "OrientationAnalysis/Filters/GenerateFaceMisorientationColoringFilter.hpp"
#include "OrientationAnalysis/Filters/GenerateFZQuaternions.hpp"
#include "OrientationAnalysis/Filters/GenerateGBCDPoleFigureFilter.hpp"
#include "OrientationAnalysis/Filters/GenerateIPFColorsFilter.hpp"
#include "OrientationAnalysis/Filters/GenerateQuaternionConjugateFilter.hpp"
#include "OrientationAnalysis/Filters/ReadH5EspritDataFilter.hpp"
#include "OrientationAnalysis/Filters/ReadH5OimDataFilter.hpp"
#include "OrientationAnalysis/Filters/MergeTwinsFilter.hpp"
#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/Filters/ReadAngDataFilter.hpp"
#include "OrientationAnalysis/Filters/ReadCtfDataFilter.hpp"
#include "OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp"
#include "OrientationAnalysis/Filters/RodriguesConvertorFilter.hpp"
#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/Filters/FindCAxisLocationsFilter.hpp"
#include "OrientationAnalysis/Filters/FindFeatureNeighborCAxisMisalignmentsFilter.hpp"
#include "OrientationAnalysis/Filters/FindGBCDMetricBasedFilter.hpp"
#include "OrientationAnalysis/Filters/FindGBPDMetricBasedFilter.hpp"
#include "OrientationAnalysis/Filters/FindSlipTransmissionMetricsFilter.hpp"
#include "OrientationAnalysis/Filters/WriteStatsGenOdfAngleFileFilter.hpp"
#include "OrientationAnalysis/Filters/WriteINLFileFilter.hpp"
// @@__HEADER__TOKEN__DO__NOT__DELETE__@@

#include <map>
#include <string>

namespace complex
{
  static const AbstractPlugin::SIMPLMapType k_SIMPL_to_OrientationAnalysis
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID, {}}}, // dream3d-class-name
    {complex::Uuid::FromString("0a121e03-3922-5c29-962d-40d88653f4b6").value(), {complex::FilterTraits<GenerateFaceIPFColoringFilter>::uuid, &GenerateFaceIPFColoringFilter::FromSIMPLJson}}, // GenerateFaceIPFColoring
    {complex::Uuid::FromString("1a0848da-2edd-52c0-b111-62a4dc6d2886").value(), {complex::FilterTraits<FindFeatureReferenceCAxisMisorientationsFilter>::uuid, &FindFeatureReferenceCAxisMisorientationsFilter::FromSIMPLJson}}, // FindFeatureReferenceCAxisMisorientations
    {complex::Uuid::FromString("286dd493-4fea-54f4-b59e-459dd13bbe57").value(), {complex::FilterTraits<FindMisorientationsFilter>::uuid, &FindMisorientationsFilter::FromSIMPLJson}}, // FindMisorientations
    {complex::Uuid::FromString("2a0bfcd3-2517-5117-b164-964dfac8fe22").value(), {complex::FilterTraits<CreateEnsembleInfoFilter>::uuid, &CreateEnsembleInfoFilter::FromSIMPLJson}}, // CreateEnsembleInfoFilter
    {complex::Uuid::FromString("33a37a47-d002-5c18-b270-86025881fe1e").value(), {complex::FilterTraits<EnsembleInfoReaderFilter>::uuid, &EnsembleInfoReaderFilter::FromSIMPLJson}}, // EnsembleInfoReader
    {complex::Uuid::FromString("3b0ababf-9c8d-538d-96af-e40775c4f0ab").value(), {complex::FilterTraits<FindShapesFilter>::uuid, &FindShapesFilter::FromSIMPLJson}}, // FindShapes
    {complex::Uuid::FromString("3ff4701b-3a0c-52e3-910a-fa927aa6584c").value(), {complex::FilterTraits<ImportH5OimDataFilter>::uuid, &ImportH5OimDataFilter::FromSIMPLJson}}, // ImportH5OimData
    {complex::Uuid::FromString("428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c").value(), {complex::FilterTraits<FindFeatureReferenceMisorientationsFilter>::uuid, &FindFeatureReferenceMisorientationsFilter::FromSIMPLJson}}, // FindFeatureReferenceMisorientations
    {complex::Uuid::FromString("433976f0-710a-5dcc-938e-fcde49cd842f").value(), {complex::FilterTraits<ExportGBCDTriangleDataFilter>::uuid, &ExportGBCDTriangleDataFilter::FromSIMPLJson}}, // ExportGBCDTriangleDataFilter
    {complex::Uuid::FromString("439e31b7-3198-5d0d-aef6-65a9e9c1a016").value(), {complex::FilterTraits<ConvertQuaternionFilter>::uuid, &ConvertQuaternionFilter::FromSIMPLJson}}, // ConvertQuaternion
    {complex::Uuid::FromString("4ef7f56b-616e-5a80-9e68-1da8f35ad235").value(), {complex::FilterTraits<ReadH5EbsdFilter>::uuid, &ReadH5EbsdFilter::FromSIMPLJson}}, // ReadH5Ebsd
    {complex::Uuid::FromString("4fb2b9de-3124-534b-b914-dbbbdbc14604").value(), {complex::FilterTraits<AlignSectionsMisorientationFilter>::uuid, &AlignSectionsMisorientationFilter::FromSIMPLJson}}, // AlignSectionsMisorientation
    {complex::Uuid::FromString("61c5519b-5561-58b8-a522-2ce1324e244d").value(), {complex::FilterTraits<AlignSectionsMutualInformationFilter>::uuid, &AlignSectionsMutualInformationFilter::FromSIMPLJson}}, // AlignSectionsMutualInformationFilter
    {complex::Uuid::FromString("630d7486-75ea-5e04-874c-894460cd7c4d").value(), {complex::FilterTraits<GenerateQuaternionConjugateFilter>::uuid, &GenerateQuaternionConjugateFilter::FromSIMPLJson}}, // GenerateQuaternionConjugate
    {complex::Uuid::FromString("6427cd5e-0ad2-5a24-8847-29f8e0720f4f").value(), {complex::FilterTraits<NeighborOrientationCorrelationFilter>::uuid, &NeighborOrientationCorrelationFilter::FromSIMPLJson}}, // NeighborOrientationCorrelation
    {complex::Uuid::FromString("6e97ff50-48bf-5403-a049-1d271bd72df9").value(), {complex::FilterTraits<FindGBCDFilter>::uuid, &FindGBCDFilter::FromSIMPLJson}}, // FindGBCDFilter
    {complex::Uuid::FromString("7861c691-b821-537b-bd25-dc195578e0ea").value(), {complex::FilterTraits<EBSDSegmentFeaturesFilter>::uuid, &EBSDSegmentFeaturesFilter::FromSIMPLJson}}, // EBSDSegmentFeatures
    {complex::Uuid::FromString("7cd30864-7bcf-5c10-aea7-d107373e2d40").value(), {complex::FilterTraits<GenerateFaceMisorientationColoringFilter>::uuid, &GenerateFaceMisorientationColoringFilter::FromSIMPLJson}}, // GenerateFaceMisorientationColoring
    {complex::Uuid::FromString("85900eba-3da9-5985-ac71-1d9d290a5d31").value(), {complex::FilterTraits<GenerateGBCDPoleFigureFilter>::uuid, &GenerateGBCDPoleFigureFilter::FromSIMPLJson}}, // VisualizeGBCDPoleFigureFilter
    {complex::Uuid::FromString("88d332c1-cf6c-52d3-a38d-22f6eae19fa6").value(), {complex::FilterTraits<FindKernelAvgMisorientationsFilter>::uuid, &FindKernelAvgMisorientationsFilter::FromSIMPLJson}}, // FindKernelAvgMisorientations
    {complex::Uuid::FromString("8abdea7d-f715-5a24-8165-7f946bbc2fe9").value(), {complex::FilterTraits<ImportH5EspritDataFilter>::uuid, &ImportH5EspritDataFilter::FromSIMPLJson}}, // ImportH5EspritData
    {complex::Uuid::FromString("9a6677a6-b9e5-5fee-afa2-27e868cab8ca").value(), {complex::FilterTraits<GenerateFZQuaternions>::uuid, &GenerateFZQuaternions::FromSIMPLJson}}, // GenerateFZQuaternions
    {complex::Uuid::FromString("a10bb78e-fcff-553d-97d6-830a43c85385").value(), {complex::FilterTraits<WritePoleFigureFilter>::uuid, &WritePoleFigureFilter::FromSIMPLJson}}, // WritePoleFigure
    {complex::Uuid::FromString("a2b62395-1a7d-5058-a840-752d8f8e2430").value(), {complex::FilterTraits<RodriguesConvertorFilter>::uuid, &RodriguesConvertorFilter::FromSIMPLJson}}, // RodriguesConvertor
    {complex::Uuid::FromString("a50e6532-8075-5de5-ab63-945feb0de7f7").value(), {complex::FilterTraits<GenerateIPFColorsFilter>::uuid, &GenerateIPFColorsFilter::FromSIMPLJson}}, // GenerateIPFColors
    {complex::Uuid::FromString("b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40").value(), {complex::FilterTraits<ReadAngDataFilter>::uuid, &ReadAngDataFilter::FromSIMPLJson}}, // ReadAngData
    {complex::Uuid::FromString("bf7036d8-25bd-540e-b6de-3a5ab0e42c5f").value(), {complex::FilterTraits<FindAvgOrientationsFilter>::uuid, &FindAvgOrientationsFilter::FromSIMPLJson}}, // FindAvgOrientations
    {complex::Uuid::FromString("bff6be19-1219-5876-8838-1574ad29d965").value(), {complex::FilterTraits<CAxisSegmentFeaturesFilter>::uuid, &CAxisSegmentFeaturesFilter::FromSIMPLJson}}, // CAxisSegmentFeatures
    {complex::Uuid::FromString("c5a9a96c-7570-5279-b383-cc25ebae0046").value(), {complex::FilterTraits<FindAvgCAxesFilter>::uuid, &FindAvgCAxesFilter::FromSIMPLJson}}, // FindAvgCAxes
    {complex::Uuid::FromString("c9af506e-9ea1-5ff5-a882-fa561def5f52").value(), {complex::FilterTraits<MergeTwinsFilter>::uuid, &MergeTwinsFilter::FromSIMPLJson}}, // MergeTwins
    {complex::Uuid::FromString("d1df969c-0428-53c3-b61d-99ea2bb6da28").value(), {complex::FilterTraits<ReadCtfDataFilter>::uuid, &ReadCtfDataFilter::FromSIMPLJson}}, // ReadCtfData
    {complex::Uuid::FromString("e5629880-98c4-5656-82b8-c9fe2b9744de").value(), {complex::FilterTraits<ConvertOrientations>::uuid, &ConvertOrientations::FromSIMPLJson}}, // ConvertOrientations
    {complex::Uuid::FromString("e67ca06a-176f-58fc-a676-d6ee5553511a").value(), {complex::FilterTraits<FindSchmidsFilter>::uuid, &FindSchmidsFilter::FromSIMPLJson}}, // FindSchmids
    {complex::Uuid::FromString("ef9420b2-8c46-55f3-8ae4-f53790639de4").value(), {complex::FilterTraits<RotateEulerRefFrameFilter>::uuid, &RotateEulerRefFrameFilter::FromSIMPLJson}}, // RotateEulerRefFrame
    {complex::Uuid::FromString("f4a7c2df-e9b0-5da9-b745-a862666d6c99").value(), {complex::FilterTraits<BadDataNeighborOrientationCheckFilter>::uuid, &BadDataNeighborOrientationCheckFilter::FromSIMPLJson}}, // BadDataNeighborOrientationCheck
    {complex::Uuid::FromString("f62065b4-54e9-53b1-bed7-2178a57d3c7a").value(), {complex::FilterTraits<ExportGBCDGMTFileFilter>::uuid, &ExportGBCDGMTFileFilter::FromSIMPLJson}}, // ExportGBCDGMTFileFilter
    {complex::Uuid::FromString("00d20627-5b88-56ba-ac7a-fc2a4b337903").value(), {complex::FilterTraits<FindGBPDMetricBasedFilter>::uuid, &FindGBPDMetricBasedFilter::FromSIMPLJson}}, // FindGBPDMetricBased
    // {complex::Uuid::FromString("17410178-4e5f-58b9-900e-8194c69200ab").value(), {complex::FilterTraits<ReplaceElementAttributesWithNeighborValues>::uuid, {}}}, // ReplaceElementAttributesWithNeighborValues
    // {complex::Uuid::FromString("179b0c7a-4e62-5070-ba49-ae58d5ccbfe8").value(), {complex::FilterTraits<ImportEbsdMontage>::uuid, {}}}, // ImportEbsdMontage
    {complex::Uuid::FromString("1a0848da-2edd-52c0-b111-62a4dc6d2886").value(), {complex::FilterTraits<FindFeatureReferenceCAxisMisorientationsFilter>::uuid, &FindFeatureReferenceCAxisMisorientationsFilter::FromSIMPLJson}}, // FindFeatureReferenceCAxisMisorientations
    // {complex::Uuid::FromString("27c724cc-8b69-5ebe-b90e-29d33858a032").value(), {complex::FilterTraits<INLWriter>::uuid, &INLWriterFilter::FromSIMPLJson}}, // INLWriter
    // {complex::Uuid::FromString("3630623e-724b-5154-a060-a5fca4ecfff5").value(), {complex::FilterTraits<Stereographic3D>::uuid, {}}}, // Stereographic3D
    // {complex::Uuid::FromString("3ff4701b-3a0c-52e3-910a-fa927aa6584c").value(), {complex::FilterTraits<ImportH5OimData>::uuid, {}}}, // ImportH5OimData
    // {complex::Uuid::FromString("5af9c1e6-ed6f-5672-9ae0-2b931344d729").value(), {complex::FilterTraits<OrientationUtility>::uuid, {}}}, // OrientationUtility
    {complex::Uuid::FromString("68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e").value(), {complex::FilterTraits<FindCAxisLocationsFilter>::uuid, &FindCAxisLocationsFilter::FromSIMPLJson}}, // FindCAxisLocations
    // {complex::Uuid::FromString("6e332fca-0475-5fec-821e-e01f668ec1d3").value(), {complex::FilterTraits<EbsdToH5Ebsd>::uuid, {}}}, // EbsdToH5Ebsd
    {complex::Uuid::FromString("8071facb-8905-5699-b345-105ae4ac33ff").value(), {complex::FilterTraits<FindBoundaryStrengthsFilter>::uuid, &FindBoundaryStrengthsFilter::FromSIMPLJson}}, // FindBoundaryStrengths
    // {complex::Uuid::FromString("8abdea7d-f715-5a24-8165-7f946bbc2fe9").value(), {complex::FilterTraits<ImportH5EspritData>::uuid, {}}}, // ImportH5EspritData
    // {complex::Uuid::FromString("94f986fc-1295-5e32-9808-752855fa658a").value(), {complex::FilterTraits<FindDistsToCharactGBs>::uuid, {}}}, // FindDistsToCharactGBs
    {complex::Uuid::FromString("97523038-5fb2-5e82-9177-ed3e8b24b4bd").value(), {complex::FilterTraits<FindSlipTransmissionMetricsFilter>::uuid, &FindSlipTransmissionMetricsFilter::FromSIMPLJson}}, // FindSlipTransmissionMetrics
    // {complex::Uuid::FromString("a10124f3-05d0-5f49-93a0-e93926f5b48b").value(), {complex::FilterTraits<FindTwinBoundaries>::uuid, {}}}, // FindTwinBoundaries
    // {complex::Uuid::FromString("a10bb78e-fcff-553d-97d6-830a43c85385").value(), {complex::FilterTraits<WritePoleFigure>::uuid, {}}}, // WritePoleFigure
    // {complex::Uuid::FromString("a4952f40-22dd-54ec-8c38-69c3fcd0e6f7").value(), {complex::FilterTraits<WriteStatsGenOdfAngleFile>::uuid, &WriteStatsGenOdfAngleFileFilter::FromSIMPLJson}}, // WriteStatsGenOdfAngleFile
    // {complex::Uuid::FromString("b0e30e6d-912d-5a7e-aeed-750134aba86b").value(), {complex::FilterTraits<FindTwinBoundarySchmidFactors>::uuid, {}}}, // FindTwinBoundarySchmidFactors
    // {complex::Uuid::FromString("b78d8825-d3ac-5351-be20-172f07fd2aec").value(), {complex::FilterTraits<EMsoftSO3Sampler>::uuid, {}}}, // EMsoftSO3Sampler
    // {complex::Uuid::FromString("c4398303-db7d-506e-81ea-08f253895ccb").value(), {complex::FilterTraits<CreateLambertSphere>::uuid, {}}}, // CreateLambertSphere
    // {complex::Uuid::FromString("c5a9a96c-7570-5279-b383-cc25ebae0046").value(), {complex::FilterTraits<FindAvgCAxes>::uuid, {}}}, // FindAvgCAxes
    {complex::Uuid::FromString("cdd50b83-ea09-5499-b008-4b253cf4c246").value(), {complex::FilterTraits<FindFeatureNeighborCAxisMisalignmentsFilter>::uuid, &FindFeatureNeighborCAxisMisalignmentsFilter::FromSIMPLJson}}, // FindFeatureNeighborCAxisMisalignments
    //{complex::Uuid::FromString("d67e9f28-2fe5-5188-b0f8-323a7e603de6").value(), {complex::FilterTraits<FindGBCDMetricBased>::uuid, &FindGBCDMetricBasedFilter::FromSIMPLJson}}, // FindGBCDMetricBased
    // {complex::Uuid::FromString("e1343abe-e5ad-5eb1-a89d-c209e620e4de").value(), {complex::FilterTraits<ConvertHexGridToSquareGrid>::uuid, {}}}, // ConvertHexGridToSquareGrid
    // {complex::Uuid::FromString("ec58f4fe-8e51-527e-9536-8b6f185684be").value(), {complex::FilterTraits<GenerateOrientationMatrixTranspose>::uuid, {}}}, // GenerateOrientationMatrixTranspose
    {complex::Uuid::FromString("d67e9f28-2fe5-5188-b0f8-323a7e603de6").value(), {complex::FilterTraits<FindGBCDMetricBasedFilter>::uuid, &FindGBCDMetricBasedFilter::FromSIMPLJson}}, // FindGBCDMetricBased
    {complex::Uuid::FromString("a4952f40-22dd-54ec-8c38-69c3fcd0e6f7").value(), {complex::FilterTraits<WriteStatsGenOdfAngleFileFilter>::uuid, &WriteStatsGenOdfAngleFileFilter::FromSIMPLJson}}, // WriteStatsGenOdfAngleFile
    {complex::Uuid::FromString("27c724cc-8b69-5ebe-b90e-29d33858a032").value(), {complex::FilterTraits<INLWriterFilter>::uuid, &INLWriterFilter::FromSIMPLJson}}, // INLWriter
    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
  };

} // namespace complex
// clang-format on
