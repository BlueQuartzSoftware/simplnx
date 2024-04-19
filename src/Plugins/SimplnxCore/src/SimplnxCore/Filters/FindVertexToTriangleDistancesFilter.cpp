#include "FindVertexToTriangleDistancesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FindVertexToTriangleDistances.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace
{
inline constexpr StringLiteral k_TriangleBounds("Triangle Bounds");
}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::name() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::className() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindVertexToTriangleDistancesFilter::uuid() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::humanName() const
{
  return "Find Vertex to Triangle Distances";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindVertexToTriangleDistancesFilter::defaultTags() const
{
  return {className(), "#Sampling", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindVertexToTriangleDistancesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Arrays"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Source Vertex Geometry", "The Vertex Geometry point cloud to map to triangles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Target Triangle Geometry", "The triangle geometry to compare against", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_TriangleNormalsArrayPath_Key, "Triangle Normals", "The triangle geometry's normals array", DataPath{}, std::set<DataType>{DataType::float64}));

  params.insertSeparator(Parameters::Separator{"Created Output Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_DistancesArrayName_Key, "Distances Array", "The array to store distance between vertex and triangle", ""));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ClosestTriangleIdArrayName_Key, "Closest Triangle Ids Array", "The array to store the ID of the closest triangle", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindVertexToTriangleDistancesFilter::clone() const
{
  return std::make_unique<FindVertexToTriangleDistancesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindVertexToTriangleDistancesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto pTriangleNormalsArrayDataPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesDataName = filterArgs.value<std::string>(k_DistancesArrayName_Key);
  auto pClosestTriangleIdDataName = filterArgs.value<std::string>(k_ClosestTriangleIdArrayName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(pVertexGeometryDataPath);
  if(vertexGeomPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-4530, fmt::format("The DataPath {} is not a valid VertexGeometry.", pVertexGeometryDataPath.toString()))};
  }

  auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  if(triangleGeomPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-4531, fmt::format("The DataPath {} is not a valid TriangleGeometry.", pTriangleGeometryDataPath.toString()))};
  }

  const DataPath vertexDataPath = pVertexGeometryDataPath.createChildPath(vertexGeomPtr->getVertexAttributeMatrix()->getName());

  auto createDistancesArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{vertexGeomPtr->getNumberOfVertices()}, std::vector<usize>{1},
                                                                        vertexDataPath.createChildPath(pDistancesDataName));
  resultOutputActions.value().appendAction(std::move(createDistancesArrayAction));

  auto createClosestTriangleIdArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int64, std::vector<usize>{vertexGeomPtr->getNumberOfVertices()}, std::vector<usize>{1},
                                                                                vertexDataPath.createChildPath(pClosestTriangleIdDataName));
  resultOutputActions.value().appendAction(std::move(createClosestTriangleIdArrayAction));

  // Create temp array then deferred delete
  auto tempTriBoundsDataPath = DataPath({::k_TriangleBounds});
  auto createTriBoundsArrayAction =
      std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{triangleGeomPtr->getNumberOfFaces()}, std::vector<usize>{6}, tempTriBoundsDataPath);
  resultOutputActions.value().appendAction(std::move(createTriBoundsArrayAction));

  auto removeTempArrayAction = std::make_unique<DeleteDataAction>(tempTriBoundsDataPath);
  resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindVertexToTriangleDistancesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  FindVertexToTriangleDistancesInputValues inputValues;

  inputValues.VertexDataContainer = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  inputValues.TriangleDataContainer = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  inputValues.TriangleNormalsArrayPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);

  auto vertexGeom = dataStructure.getDataRefAs<VertexGeom>(inputValues.VertexDataContainer);
  const DataPath vertexDataPath = inputValues.VertexDataContainer.createChildPath(vertexGeom.getVertexAttributeMatrix()->getName());
  inputValues.DistancesArrayPath = vertexDataPath.createChildPath(filterArgs.value<std::string>(k_DistancesArrayName_Key));
  inputValues.ClosestTriangleIdArrayPath = vertexDataPath.createChildPath(filterArgs.value<std::string>(k_ClosestTriangleIdArrayName_Key));

  inputValues.TriBoundsDataPath = DataPath({::k_TriangleBounds});

  return FindVertexToTriangleDistances(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_VertexDataContainerKey = "VertexDataContainer";
constexpr StringLiteral k_TriangleDataContainerKey = "TriangleDataContainer";
constexpr StringLiteral k_TriangleNormalsArrayPathKey = "TriangleNormalsArrayPath";
constexpr StringLiteral k_DistancesArrayPathKey = "DistancesArrayPath";
constexpr StringLiteral k_ClosestTriangleIdArrayPathKey = "ClosestTriangleIdArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> FindVertexToTriangleDistancesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindVertexToTriangleDistancesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_VertexDataContainerKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TriangleDataContainerKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_TriangleNormalsArrayPathKey, k_TriangleNormalsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_DistancesArrayPathKey, k_DistancesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_ClosestTriangleIdArrayPathKey, k_ClosestTriangleIdArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
