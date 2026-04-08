#include "BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_InconsistentTupleCount = -6809;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::name() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::className() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::className;
}

//------------------------------------------------------------------------------
Uuid BadDataNeighborOrientationCheckFilter::uuid() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::humanName() const
{
  return "Neighbor Orientation Comparison (Bad Data)";
}

//------------------------------------------------------------------------------
std::vector<std::string> BadDataNeighborOrientationCheckFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters BadDataNeighborOrientationCheckFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "Angular tolerance used to compare with neighboring Cells", 5.0f));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfNeighbors_Key, "Required Number of Neighbors",
                                                 "Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed", 6));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The target geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input/Output Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Used to define Cells as good or bad", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each phase",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType BadDataNeighborOrientationCheckFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer BadDataNeighborOrientationCheckFilter::clone() const
{
  return std::make_unique<BadDataNeighborOrientationCheckFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult BadDataNeighborOrientationCheckFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<DataPath> dataArrayPaths;
  dataArrayPaths.push_back(pGoodVoxelsArrayPathValue);
  dataArrayPaths.push_back(pCellPhasesArrayPathValue);
  dataArrayPaths.push_back(pQuatsArrayPathValue);

  // validate the number of tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(k_InconsistentTupleCount, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  resultOutputActions.value().modifiedActions.emplace_back(
      DataObjectModification{pGoodVoxelsArrayPathValue, DataObjectModification::ModifiedType::Modified, dataStructure.getData(pGoodVoxelsArrayPathValue)->getDataObjectType()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheckFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  BadDataNeighborOrientationCheckInputValues inputValues;

  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.NumberOfNeighbors = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return BadDataNeighborOrientationCheck(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteral k_NumberOfNeighborsKey = "NumberOfNeighbors";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> BadDataNeighborOrientationCheckFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BadDataNeighborOrientationCheckFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumberOfNeighborsKey, k_NumberOfNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
