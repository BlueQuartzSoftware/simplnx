#include "ComputeGBPDMetricBasedFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeGBPDMetricBased.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeGBPDMetricBasedFilter::name() const
{
  return FilterTraits<ComputeGBPDMetricBasedFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeGBPDMetricBasedFilter::className() const
{
  return FilterTraits<ComputeGBPDMetricBasedFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeGBPDMetricBasedFilter::uuid() const
{
  return FilterTraits<ComputeGBPDMetricBasedFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeGBPDMetricBasedFilter::humanName() const
{
  return "Compute GBPD (Metric-Based Approach)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeGBPDMetricBasedFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters ComputeGBPDMetricBasedFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_PhaseOfInterest_Key, "Phase of Interest",
                                                 "Index of the Ensemble for which to compute GBPD; boundaries having grains of this phase on both its sides will only be taken into account", 1));
  params.insert(std::make_unique<Float32Parameter>(k_LimitDist_Key, "Limiting Distance [deg.]", "The max angle from within which mesh segments are selected", 7.0f));
  params.insert(std::make_unique<Int32Parameter>(k_NumSamplPts_Key, "Number of Sampling Points (on a Hemisphere)", "The approximate number of sampling directions", 3000));
  params.insert(std::make_unique<BoolParameter>(k_ExcludeTripleLines_Key, "Exclude Triangles Directly Neighboring Triple Lines", "If checked, only interiors of Faces are included in GBCD", false));
  params.insert(std::make_unique<BoolParameter>(k_SaveRelativeErr_Key, "Save Relative Errors Instead of Their Absolute Values",
                                                "Whether or not to save the distribution errors as relative (if exceeds 100%, then rounded down) or absolute", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The complete path to the triangle geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry Vertex Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_NodeTypesArrayPath_Key, "Node Types", "Specifies the type of node in the Geometry", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "Specifies the normal of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceAreasArrayPath_Key, "Face Areas", "Specifies the area of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry Face Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, "Feature Face Labels",
                                                          "Specifies which original Features are on either side of each boundary Feature", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Average Euler Angles", "Three angles defining the orientation of the Feature in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Specifies to which phase each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_DistOutputFile_Key, "Output Distribution File", "The output distribution file path (extension .dat, GMT format)",
                                                          fs::path("distribution.dat"), FileSystemPathParameter::ExtensionsType{".dat"}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<FileSystemPathParameter>(k_ErrOutputFile_Key, "Output Distribution Errors File", "The output distribution errors file path (extension .dat, GMT format)",
                                                          fs::path("distribution_errors.dat"), FileSystemPathParameter::ExtensionsType{".dat"}, FileSystemPathParameter::PathType::OutputFile));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeGBPDMetricBasedFilter::clone() const
{
  return std::make_unique<ComputeGBPDMetricBasedFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeGBPDMetricBasedFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pPhaseOfInterestValue = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  auto pNumSamplePtsValue = filterArgs.value<int32>(k_NumSamplPts_Key);
  auto pDistOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_DistOutputFile_Key);
  auto pErrOutputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_ErrOutputFile_Key);
  auto pNodeTypesArrayPathValue = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshFaceAreasArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  auto pSurfaceMeshFeatureFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key);
  auto pFeatureEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pNumSamplePtsValue < 1)
  {
    return MakePreflightErrorResult(-8320, "The number of sampling points must be greater than zero");
  }

  // Set some reasonable value, but allow user to use more if they know what they're doing
  if(pNumSamplePtsValue > 5000)
  {
    resultOutputActions.warnings().push_back({-8321, "The number of sampling points is greater than 5000, but it is unlikely that many are needed"});
  }

  if(pDistOutputFileValue == pErrOutputFileValue)
  {
    return MakePreflightErrorResult(-8322, "The output files must be different");
  }

  if(pPhaseOfInterestValue <= 0)
  {
    return MakePreflightErrorResult(-8323, "The phase of interest must be greater than 0");
  }

  const auto numEnsembles = static_cast<int>(dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue).getNumberOfTuples());
  if(pPhaseOfInterestValue >= numEnsembles)
  {
    return MakePreflightErrorResult(-8324, fmt::format("The phase of interest index ({}) is larger than the number of Ensembles ({})", pPhaseOfInterestValue, numEnsembles));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeGBPDMetricBasedFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  ComputeGBPDMetricBasedInputValues inputValues;

  inputValues.PhaseOfInterest = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  inputValues.LimitDist = filterArgs.value<float32>(k_LimitDist_Key);
  inputValues.NumSamplPts = filterArgs.value<int32>(k_NumSamplPts_Key);
  inputValues.ExcludeTripleLines = filterArgs.value<bool>(k_ExcludeTripleLines_Key);
  inputValues.DistOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_DistOutputFile_Key);
  inputValues.ErrOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_ErrOutputFile_Key);
  inputValues.SaveRelativeErr = filterArgs.value<bool>(k_SaveRelativeErr_Key);
  inputValues.NodeTypesArrayPath = filterArgs.value<DataPath>(k_NodeTypesArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.SurfaceMeshFaceNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  inputValues.SurfaceMeshFaceAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceAreasArrayPath_Key);
  inputValues.SurfaceMeshFeatureFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceLabelsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);

  return ComputeGBPDMetricBased(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_PhaseOfInterestKey = "PhaseOfInterest";
constexpr StringLiteral k_LimitDistKey = "LimitDist";
constexpr StringLiteral k_NumSamplPtsKey = "NumSamplPts";
constexpr StringLiteral k_ExcludeTripleLinesKey = "ExcludeTripleLines";
constexpr StringLiteral k_DistOutputFileKey = "DistOutputFile";
constexpr StringLiteral k_ErrOutputFileKey = "ErrOutputFile";
constexpr StringLiteral k_SaveRelativeErrKey = "SaveRelativeErr";
constexpr StringLiteral k_NodeTypesArrayPathKey = "NodeTypesArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceNormalsArrayPathKey = "SurfaceMeshFaceNormalsArrayPath";
constexpr StringLiteral k_SurfaceMeshFaceAreasArrayPathKey = "SurfaceMeshFaceAreasArrayPath";
constexpr StringLiteral k_SurfaceMeshFeatureFaceLabelsArrayPathKey = "SurfaceMeshFeatureFaceLabelsArrayPath";
constexpr StringLiteral k_FeatureEulerAnglesArrayPathKey = "FeatureEulerAnglesArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeGBPDMetricBasedFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeGBPDMetricBasedFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_PhaseOfInterestKey, k_PhaseOfInterest_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_LimitDistKey, k_LimitDist_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumSamplPtsKey, k_NumSamplPts_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ExcludeTripleLinesKey, k_ExcludeTripleLines_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_DistOutputFileKey, k_DistOutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_ErrOutputFileKey, k_ErrOutputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SaveRelativeErrKey, k_SaveRelativeErr_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_NodeTypesArrayPathKey, k_NodeTypesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceNormalsArrayPathKey, k_SurfaceMeshFaceNormalsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceAreasArrayPathKey, k_SurfaceMeshFaceAreasArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFeatureFaceLabelsArrayPathKey,
                                                                                                                   k_SurfaceMeshFeatureFaceLabelsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureEulerAnglesArrayPathKey, k_FeatureEulerAnglesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
