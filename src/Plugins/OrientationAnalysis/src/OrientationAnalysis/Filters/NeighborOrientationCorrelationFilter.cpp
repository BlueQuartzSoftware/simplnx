#include "NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidNumTuples = -580093;
constexpr int32 k_InvalidLevel = -580094;
constexpr int32 k_LevelIsNoOp = -580095;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::name() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::className() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid NeighborOrientationCorrelationFilter::uuid() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::humanName() const
{
  return "Neighbor Orientation Correlation";
}

//------------------------------------------------------------------------------
std::vector<std::string> NeighborOrientationCorrelationFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters NeighborOrientationCorrelationFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Minimum Confidence Index", "Sets the minimum value of 'confidence' a Cell must have", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "Angular tolerance used to compare with neighboring Cells", 5.0f));
  params.insert(std::make_unique<Int32Parameter>(
      k_Level_Key, "Cleanup Level",
      "Controls the number of cleanup passes: the filter runs (6 - Level) passes, so lower values clean more aggressively. At the default of 6 the filter performs zero passes and makes no changes",
      6));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "Path to the target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CorrelationArrayPath_Key, "Confidence Index", "Specifies the confidence in the orientation of the Cell (TSL data)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType NeighborOrientationCorrelationFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NeighborOrientationCorrelationFilter::clone() const
{
  return std::make_unique<NeighborOrientationCorrelationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NeighborOrientationCorrelationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pLevelValue = filterArgs.value<int32>(k_Level_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_CorrelationArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // The level loop runs (6 - Level) passes. Negative values are almost certainly user
  // error; 6 or above runs zero passes, which deserves a warning since the filter
  // silently does nothing.
  if(pLevelValue < 0)
  {
    return {MakeErrorResult<OutputActions>(k_InvalidLevel, fmt::format("Cleanup Level must be 0 or greater (the filter runs 6 - Level cleanup passes). Value supplied: {}", pLevelValue))};
  }
  if(pLevelValue >= 6)
  {
    resultOutputActions.warnings().push_back(
        {k_LevelIsNoOp, fmt::format("Cleanup Level {} results in zero cleanup passes - the filter will make no changes. Set the level below 6 for the filter to do anything.", pLevelValue)});
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPathValue);

  std::vector<DataPath> dataArrayPaths;

  dataArrayPaths.push_back(pConfidenceIndexArrayPathValue);
  dataArrayPaths.push_back(pCellPhasesArrayPathValue);
  dataArrayPaths.push_back(pQuatsArrayPathValue);

  // collect the rest of the geometry's arrays that aren't ignored to check the tuple count
  DataPath parentPath = pConfidenceIndexArrayPathValue.getParent();
  const auto& parent = dataStructure.getDataRefAs<BaseGroup>(parentPath);
  auto childArrays = parent.findAllChildrenOfType<IDataArray>();
  for(const auto& childArray : childArrays)
  {
    bool ignore = false;
    DataPath childArrayPath;
    for(const auto& childDataPath : childArray->getDataPaths())
    {
      if(parentPath == childDataPath.getParent())
      {
        childArrayPath = childDataPath;
      }
    }
    for(const auto& ignoredPath : pIgnoredDataArrayPathsValue)
    {
      if(childArrayPath == ignoredPath)
      {
        ignore = true;
        break;
      }
    }
    if(!ignore)
    {
      dataArrayPaths.push_back(childArrayPath);
    }
  }

  // validate the number of tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return {
        MakeErrorResult<OutputActions>(k_InvalidNumTuples, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, imageGeom.getCellDataPath(), {});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> NeighborOrientationCorrelationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  NeighborOrientationCorrelationInputValues inputValues;

  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.MinConfidence = filterArgs.value<float32>(k_MinConfidence_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.Level = filterArgs.value<int32>(k_Level_Key);
  inputValues.ConfidenceIndexArrayPath = filterArgs.value<DataPath>(k_CorrelationArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return NeighborOrientationCorrelation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinConfidenceKey = "MinConfidence";
constexpr StringLiteral k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteral k_LevelKey = "Level";
constexpr StringLiteral k_ConfidenceIndexArrayPathKey = "ConfidenceIndexArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> NeighborOrientationCorrelationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = NeighborOrientationCorrelationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MinConfidenceKey, k_MinConfidence_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_LevelKey, k_Level_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ConfidenceIndexArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ConfidenceIndexArrayPathKey, k_CorrelationArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  // IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
  if(json.contains(SIMPL::k_IgnoredDataArrayPathsKey))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
