#include "ApproximatePointCloudHullFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ApproximatePointCloudHull.hpp"

#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyNeighborRange = -86104;
constexpr int32 k_GridResolutionFinite = -86103;

/**
 * @brief Checks the active spatial parameter domains before output actions.
 * @param resolution Gives physical voxel lengths in XYZ order.
 * @param threshold Gives the required empty-neighbor count.
 * @pre resolution has three components.
 * @return The first active parameter error, otherwise success.
 */
Result<> ValidateHullParameters(nonstd::span<const float32> resolution, uint64 threshold)
{
  auto finite = GeometryUtilities::ValidateFiniteVector(resolution, "Grid Resolution", k_GridResolutionFinite);
  if(finite.invalid())
  {
    return finite;
  }
  auto positive = GeometryUtilities::ValidatePositiveFiniteVector(resolution, "Grid Resolution", -11001);
  if(positive.invalid())
  {
    return positive;
  }
  if(threshold > 26)
  {
    return MakeErrorResult(k_EmptyNeighborRange, fmt::format("Minimum Number of Empty Neighbors value {} must be in the inclusive range 0 through 26.", threshold));
  }
  return {};
}
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ApproximatePointCloudHullFilter::name() const
{
  return FilterTraits<ApproximatePointCloudHullFilter>::name;
}

//------------------------------------------------------------------------------
std::string ApproximatePointCloudHullFilter::className() const
{
  return FilterTraits<ApproximatePointCloudHullFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ApproximatePointCloudHullFilter::uuid() const
{
  return FilterTraits<ApproximatePointCloudHullFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ApproximatePointCloudHullFilter::humanName() const
{
  return "Approximate Point Cloud Hull";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApproximatePointCloudHullFilter::defaultTags() const
{
  return {className(), "Point Cloud", "Grid", "Vertex Geometry", "Geometry", "Hull"};
}

//------------------------------------------------------------------------------
Parameters ApproximatePointCloudHullFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GridResolution_Key, "Grid Resolution", "Geometry resolution", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<UInt64Parameter>(k_MinEmptyNeighbors_Key, "Minimum Number of Empty Neighbors", "Minimum number of empty neighbors", 0));

  params.insertSeparator(Parameters::Separator{"Input Vertex Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeomPath_Key, "Vertex Geometry", "Path to the target Vertex geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));

  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_HullVertexGeomPath_Key, "Hull Vertex Geometry", "Path to create the hull Vertex geometry", DataPath{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ApproximatePointCloudHullFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApproximatePointCloudHullFilter::clone() const
{
  return std::make_unique<ApproximatePointCloudHullFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApproximatePointCloudHullFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto gridResolution = filterArgs.value<std::vector<float32>>(k_GridResolution_Key);
  auto vertexGeomPath = filterArgs.value<DataPath>(k_VertexGeomPath_Key);
  auto hullVertexGeomPath = filterArgs.value<DataPath>(k_HullVertexGeomPath_Key);

  if(auto validation = ValidateHullParameters(gridResolution, filterArgs.value<uint64>(k_MinEmptyNeighbors_Key)); validation.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(validation), {})};
  }

  auto vertexGeom = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);

  usize numVertices = vertexGeom->getNumberOfVertices();
  auto action = std::make_unique<CreateVertexGeometryAction>(hullVertexGeomPath, numVertices, INodeGeometry0D::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName);

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ApproximatePointCloudHullFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ApproximatePointCloudHullInputValues inputValues;
  inputValues.GridResolution = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_VertexGeomPath_Key);
  inputValues.MinEmptyNeighbors = filterArgs.value<UInt64Parameter::ValueType>(k_MinEmptyNeighbors_Key);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_HullVertexGeomPath_Key);

  return ApproximatePointCloudHull(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_GridResolutionKey = "GridResolution";
constexpr StringLiteral k_NumberOfEmptyNeighborsKey = "NumberOfEmptyNeighbors";
constexpr StringLiteral k_VertexDataContainerNameKey = "VertexDataContainerName";
constexpr StringLiteral k_HullDataContainerNameKey = "HullDataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> ApproximatePointCloudHullFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ApproximatePointCloudHullFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_GridResolutionKey, k_GridResolution_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_NumberOfEmptyNeighborsKey, k_MinEmptyNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_VertexDataContainerNameKey, k_VertexGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_HullDataContainerNameKey, k_HullVertexGeomPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
