#include "FindEuclideanDistMapFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FindEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::name() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::className() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindEuclideanDistMapFilter::uuid() const
{
  return FilterTraits<FindEuclideanDistMapFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindEuclideanDistMapFilter::humanName() const
{
  return "Find Euclidean Distance Map";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindEuclideanDistMapFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindEuclideanDistMapFilter::parameters() const
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
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveNearestNeighbors_Key, "Store the Nearest Boundary Cells", "Whether to store the nearest neighbors of Cell", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_GBDistancesArrayName_Key, "Boundary Distances",
                                                          "The name of the array with the distance the cells are from the boundary of the Feature they belong to.", "GBManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_TJDistancesArrayName_Key, "Triple Line Distances",
                                                          "The name of the array with the distance the cells are from a triple junction of Features.", "TJManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_QPDistancesArrayName_Key, "Quadruple Point Distances",
                                                          "The name of the array with the distance the cells are from a quadruple point of Features.", "QPManhattanDistances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NearestNeighborsArrayName_Key, "Nearest Boundary Cells",
                                                          "The name of the array with the indices of the closest cell that touches a boundary, triple and quadruple point for each cell.",
                                                          "NearestNeighbors"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_DoBoundaries_Key, k_GBDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoTripleLines_Key, k_TJDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_DoQuadPoints_Key, k_QPDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_SaveNearestNeighbors_Key, k_NearestNeighborsArrayName_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindEuclideanDistMapFilter::clone() const
{
  return std::make_unique<FindEuclideanDistMapFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindEuclideanDistMapFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCalcManhattanDistValue = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  auto pDoBoundariesValue = filterArgs.value<bool>(k_DoBoundaries_Key);
  auto pDoTripleLinesValue = filterArgs.value<bool>(k_DoTripleLines_Key);
  auto pDoQuadPointsValue = filterArgs.value<bool>(k_DoQuadPoints_Key);
  auto pSaveNearestNeighborsValue = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);

  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  DataPath parentGroup = pFeatureIdsArrayPathValue.getParent();

  // Interesting thing about this parameter: The Default VALUE must be at the root level of the Data Structure. This is because the
  // user may not actually want to keep that created array in which case we then try to delete the array. The DeleteDataAction
  // will fail in preflight because the Array was never actually created at its default location and so if fails. If the user
  // does in fact want to keep this array, then the user would have actually set the DataPaths to something that will actually get created.
  auto pNearestNeighborsArrayNameValue = filterArgs.value<std::string>(k_NearestNeighborsArrayName_Key);
  DataPath pNearestNeighborsArrayPath = pSaveNearestNeighborsValue ? parentGroup.createChildPath(pNearestNeighborsArrayNameValue) : DataPath::FromString(pNearestNeighborsArrayNameValue).value();

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  // Get the Cell Data Array so we get the tuple shape correct
  const auto* cellDataArray = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
  if(nullptr == cellDataArray)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12801, fmt::format("{} Data Array is not of the correct type. Select a DataArray object.", pFeatureIdsArrayPathValue.toString())}})};
  }

  IDataStore::ShapeType tupleShape = cellDataArray->getIDataStore()->getTupleShape();
  DataType outputDataType = DataType::int32;
  if(!pCalcManhattanDistValue)
  {
    outputDataType = DataType::float32;
  }

  // Create the GBDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the TJDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the QPDistancesArray
  {
    auto arrayPath = parentGroup.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
    auto action = std::make_unique<CreateArrayAction>(outputDataType, tupleShape, std::vector<usize>{1ULL}, arrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }
  // Create the NearestNeighborsArray
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::int32, tupleShape, std::vector<usize>{3ULL}, pNearestNeighborsArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // If we are NOT saving the nearest neighbors then we need to delete this array that gets created.
  if(!pSaveNearestNeighborsValue)
  {
    auto action = std::make_unique<DeleteDataAction>(pNearestNeighborsArrayPath, DeleteDataAction::DeleteType::JustObject);
    resultOutputActions.value().appendDeferredAction(std::move(action));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindEuclideanDistMapFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  FindEuclideanDistMapInputValues inputValues;

  inputValues.CalcManhattanDist = filterArgs.value<bool>(k_CalcManhattanDist_Key);
  inputValues.DoBoundaries = filterArgs.value<bool>(k_DoBoundaries_Key);
  inputValues.DoTripleLines = filterArgs.value<bool>(k_DoTripleLines_Key);
  inputValues.DoQuadPoints = filterArgs.value<bool>(k_DoQuadPoints_Key);
  inputValues.SaveNearestNeighbors = filterArgs.value<bool>(k_SaveNearestNeighbors_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  DataPath parentGroupPath = inputValues.FeatureIdsArrayPath.getParent();
  inputValues.GBDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_GBDistancesArrayName_Key));
  inputValues.TJDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_TJDistancesArrayName_Key));
  inputValues.QPDistancesArrayName = parentGroupPath.createChildPath(filterArgs.value<std::string>(k_QPDistancesArrayName_Key));
  auto nearestNeighborName = filterArgs.value<std::string>(k_NearestNeighborsArrayName_Key);
  inputValues.NearestNeighborsArrayName = inputValues.SaveNearestNeighbors ? parentGroupPath.createChildPath(nearestNeighborName) : DataPath::FromString(nearestNeighborName).value();
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return FindEuclideanDistMap(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CalcManhattanDistKey = "CalcManhattanDist";
constexpr StringLiteral k_DoBoundariesKey = "DoBoundaries";
constexpr StringLiteral k_DoTripleLinesKey = "DoTripleLines";
constexpr StringLiteral k_DoQuadPointsKey = "DoQuadPoints";
constexpr StringLiteral k_SaveNearestNeighborsKey = "SaveNearestNeighbors";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_GBDistancesArrayNameKey = "GBDistancesArrayName";
constexpr StringLiteral k_TJDistancesArrayNameKey = "TJDistancesArrayName";
constexpr StringLiteral k_QPDistancesArrayNameKey = "QPDistancesArrayName";
constexpr StringLiteral k_NearestNeighborsArrayNameKey = "NearestNeighborsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> FindEuclideanDistMapFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindEuclideanDistMapFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_CalcManhattanDistKey, k_CalcManhattanDist_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_DoBoundariesKey, k_DoBoundaries_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_DoTripleLinesKey, k_DoTripleLines_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_DoQuadPointsKey, k_DoQuadPoints_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SaveNearestNeighborsKey, k_SaveNearestNeighbors_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_GBDistancesArrayNameKey, k_GBDistancesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_TJDistancesArrayNameKey, k_TJDistancesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_QPDistancesArrayNameKey, k_QPDistancesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NearestNeighborsArrayNameKey, k_NearestNeighborsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
