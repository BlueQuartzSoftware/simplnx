#include "ComputeVertexToTriangleDistancesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeVertexToTriangleDistances.hpp"

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
constexpr StringLiteralType k_TriangleBounds("Triangle Bounds");
}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeVertexToTriangleDistancesFilter::name() const
{
  return FilterTraits<ComputeVertexToTriangleDistancesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeVertexToTriangleDistancesFilter::className() const
{
  return FilterTraits<ComputeVertexToTriangleDistancesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeVertexToTriangleDistancesFilter::uuid() const
{
  return FilterTraits<ComputeVertexToTriangleDistancesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeVertexToTriangleDistancesFilter::humanName() const
{
  return "Compute Vertex to Triangle Distances";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeVertexToTriangleDistancesFilter::defaultTags() const
{
  return {className(), "#Sampling", "#Spatial", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeVertexToTriangleDistancesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Source Vertex Geometry", "The Vertex Geometry point cloud to map to triangles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedTriangleGeometryPath_Key, "Target Triangle Geometry", "The triangle geometry to compare against", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleNormalsArrayPath_Key, "Triangle Normals", "The triangle geometry's normals array", DataPath{},
                                                          std::set<DataType>{DataType::float64}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));

  params.insertSeparator(Parameters::Separator{"Output Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_DistancesArrayName_Key, "Distances Array", "The array to store distance between vertex and triangle", ""));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ClosestTriangleIdArrayName_Key, "Closest Triangle Ids Array", "The array to store the ID of the closest triangle", ""));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeVertexToTriangleDistancesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeVertexToTriangleDistancesFilter::clone() const
{
  return std::make_unique<ComputeVertexToTriangleDistancesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeVertexToTriangleDistancesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                               const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  auto pTriangleNormalsArrayDataPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesDataName = filterArgs.value<std::string>(k_DistancesArrayName_Key);
  auto pClosestTriangleIdDataName = filterArgs.value<std::string>(k_ClosestTriangleIdArrayName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(pVertexGeometryDataPath);
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);

  const DataPath vertexDataPath = pVertexGeometryDataPath.createChildPath(vertexGeom.getVertexAttributeMatrix()->getName());

  auto createDistancesArrayAction =
      std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{vertexGeom.getNumberOfVertices()}, std::vector<usize>{1}, vertexDataPath.createChildPath(pDistancesDataName));
  resultOutputActions.value().appendAction(std::move(createDistancesArrayAction));

  auto createClosestTriangleIdArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::int64, std::vector<usize>{vertexGeom.getNumberOfVertices()}, std::vector<usize>{1},
                                                                                vertexDataPath.createChildPath(pClosestTriangleIdDataName));
  resultOutputActions.value().appendAction(std::move(createClosestTriangleIdArrayAction));

  // Create temp array then deferred delete
  auto tempTriBoundsDataPath = DataPath({::k_TriangleBounds});
  auto createTriBoundsArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{triangleGeom.getNumberOfFaces()}, std::vector<usize>{6}, tempTriBoundsDataPath);
  resultOutputActions.value().appendAction(std::move(createTriBoundsArrayAction));

  auto removeTempArrayAction = std::make_unique<DeleteDataAction>(tempTriBoundsDataPath);
  resultOutputActions.value().appendDeferredAction(std::move(removeTempArrayAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeVertexToTriangleDistancesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeVertexToTriangleDistancesInputValues inputValues;

  inputValues.vertex_data_container = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  inputValues.triangle_data_container = filterArgs.value<DataPath>(k_SelectedTriangleGeometryPath_Key);
  inputValues.triangle_normals_array_path = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);

  auto vertexGeom = dataStructure.getDataRefAs<VertexGeom>(inputValues.vertex_data_container);
  const DataPath vertexDataPath = inputValues.vertex_data_container.createChildPath(vertexGeom.getVertexAttributeMatrix()->getName());
  inputValues.distances_array_path = vertexDataPath.createChildPath(filterArgs.value<std::string>(k_DistancesArrayName_Key));
  inputValues.closest_triangle_id_array_path = vertexDataPath.createChildPath(filterArgs.value<std::string>(k_ClosestTriangleIdArrayName_Key));

  inputValues.tri_bounds_data_path = DataPath({::k_TriangleBounds});

  return ComputeVertexToTriangleDistances(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_VertexDataContainerKey = "VertexDataContainer";
constexpr StringLiteralType k_TriangleDataContainerKey = "TriangleDataContainer";
constexpr StringLiteralType k_TriangleNormalsArrayPathKey = "TriangleNormalsArrayPath";
constexpr StringLiteralType k_DistancesArrayPathKey = "DistancesArrayPath";
constexpr StringLiteralType k_ClosestTriangleIdArrayPathKey = "ClosestTriangleIdArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ComputeVertexToTriangleDistancesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeVertexToTriangleDistancesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_VertexDataContainerKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_TriangleDataContainerKey, k_SelectedTriangleGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_TriangleNormalsArrayPathKey, k_TriangleNormalsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_DistancesArrayPathKey, k_DistancesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_ClosestTriangleIdArrayPathKey,
                                                                                                                                    k_ClosestTriangleIdArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
