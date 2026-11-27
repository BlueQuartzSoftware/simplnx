#include "ComputeEuclideanDistMapFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeEuclideanDistMapFilter::name() const
{
  return FilterTraits<ComputeEuclideanDistMapFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeEuclideanDistMapFilter::className() const
{
  return FilterTraits<ComputeEuclideanDistMapFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeEuclideanDistMapFilter::uuid() const
{
  return FilterTraits<ComputeEuclideanDistMapFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeEuclideanDistMapFilter::humanName() const
{
  return "Compute Euclidean Distance Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeEuclideanDistMapFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeEuclideanDistMapFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<BoolParameter>(k_CalcManhattanDist_Key, "Output arrays are Manhattan distance (int32)",
                                                "If Manhattan distance is used then results are stored as int32 otherwise results are stored as float32", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_DoBoundaries_Key, "Calculate Distance to Boundaries", "Whether the distance of each Cell to a Feature boundary is calculated", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_DoTripleLines_Key, "Calculate Distance to Triple Lines", "Whether the distance of each Cell to a triple line between Features is calculated", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_DoQuadPoints_Key, "Calculate Distance to Quadruple Points", "Whether the distance of each Cell to a quadruple point between Features is calculated", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_GBDistancesArrayName_Key, "Boundary Distances",
                                                          "The name of the array with the distance the cells are from the boundary of the Feature they belong to.", "GBManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_TJDistancesArrayName_Key, "Triple Line Distances",
                                                          "The name of the array with the distance the cells are from a triple junction of Features.", "TJManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_QPDistancesArrayName_Key, "Quadruple Point Distances",
                                                          "The name of the array with the distance the cells are from a quadruple point of Features.", "QPManhattanDistances"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DoBoundaries_Key, k_GBDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoTripleLines_Key, k_TJDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoQuadPoints_Key, k_QPDistancesArrayName_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeEuclideanDistMapFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeEuclideanDistMapFilter::clone() const
{
  return std::make_unique<ComputeEuclideanDistMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeEuclideanDistMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                      const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pCalculateDistToBoundaries = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pCalculateDistToTripleLines = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pCalculateDistToQuadPoints = filterArgs.value<bool>(k_DoQuadPoints_Key);

  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto tupleCountResult = ValidateCellTupleCount(dataStructure, filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key), {pFeatureIdsArrayPathValue}, -12805);
  if(tupleCountResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleCountResult), {})};
  }

  DataPath parentGroup = pFeatureIdsArrayPathValue.getParent();

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  // Get the Cell Data Array, so we get the tuple shape correct
  const auto* cellDataArrayPtr = dataStructure.getDataAs<Int32ArrayType>(pFeatureIdsArrayPathValue);
  if(nullptr == cellDataArrayPtr)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-12801, fmt::format("{} Data Array is not of the correct type. Select a DataArray object.", pFeatureIdsArrayPathValue.toString()))};
  }

  ShapeType tupleShape = cellDataArrayPtr->getIDataStore()->getTupleShape();
  DataType outputDataType = DataType::int32;
  if(!pCalcManhattanDistValue)
  {
    outputDataType = DataType::float32;
  }

  if(!pCalculateDistToBoundaries && !pCalculateDistToTripleLines && !pCalculateDistToQuadPoints)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                -12802, fmt::format("There is no output.  One of the following options must be selected: Calculate Distance To Boundaries, Calculate Distance to Triple Lines, or Calculate "
                                    "Distance to Quadruple Points.",
                                    pFeatureIdsArrayPathValue.toString()))};
  }

  // Create the GBDistancesArray
  if(pCalculateDistToBoundaries)
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the TJDistancesArray
  if(pCalculateDistToTripleLines)
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the QPDistancesArray
  if(pCalculateDistToQuadPoints)
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeEuclideanDistMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeEuclideanDistMapInputValues inputValues;

  inputValues.calc_manhattan_dist = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  inputValues.do_boundaries = filterArgs.value<bool>(k_DoBoundaries_Key);
  inputValues.do_triple_lines = filterArgs.value<bool>(k_DoTripleLines_Key);
  inputValues.do_quad_points = filterArgs.value<bool>(k_DoQuadPoints_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  DataPath parentGroupPath = inputValues.feature_ids_array_path.getParent();
  inputValues.gb_distances_array_path = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
  inputValues.tj_distances_array_path = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
  inputValues.qp_distances_array_path = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
  inputValues.input_image_geometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeEuclideanDistMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_CalcManhattanDistKey = "CalcManhattanDist";
constexpr StringLiteralType k_DoBoundariesKey = "DoBoundaries";
constexpr StringLiteralType k_DoTripleLinesKey = "DoTripleLines";
constexpr StringLiteralType k_DoQuadPointsKey = "DoQuadPoints";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_GBDistancesArrayNameKey = "GBDistancesArrayName";
constexpr StringLiteralType k_TJDistancesArrayNameKey = "TJDistancesArrayName";
constexpr StringLiteralType k_QPDistancesArrayNameKey = "QPDistancesArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeEuclideanDistMapFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeEuclideanDistMapFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_CalcManhattanDistKey, k_CalcManhattanDist_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_DoBoundariesKey, k_DoBoundaries_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_DoTripleLinesKey, k_DoTripleLines_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_DoQuadPointsKey, k_DoQuadPoints_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey,
                                                                                                                                        k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_GBDistancesArrayNameKey, k_GBDistancesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_TJDistancesArrayNameKey, k_TJDistancesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_QPDistancesArrayNameKey, k_QPDistancesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
