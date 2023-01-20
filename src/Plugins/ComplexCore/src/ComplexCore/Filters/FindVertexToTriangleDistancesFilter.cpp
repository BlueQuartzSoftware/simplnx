#include "FindVertexToTriangleDistancesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindVertexToTriangleDistances.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace
{
inline constexpr StringLiteral k_TriangleBounds("Triangle Bounds");
}

namespace complex
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
  return {"#Sampling", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindVertexToTriangleDistancesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  std::set<BaseGroup::GroupType> allowedTypes = {BaseGroup::GroupType::DataGroup, BaseGroup::GroupType::AttributeMatrix};

  params.insertSeparator(Parameters::Separator{"Required Input Arrays"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexDataContainer_Key, "Source Vertex Geometry", "", DataPath{}, allowedTypes));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainer_Key, "Target Triangle Geometry", "", DataPath{}, allowedTypes));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleNormalsArrayPath_Key, "Triangle Normals", "", DataPath{}, std::set<DataType>{DataType::float64}));

  params.insertSeparator(Parameters::Separator{"Created Output Arrays"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_DistancesArrayPath_Key, "Distances Array Name", "", "Distances"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ClosestTriangleIdArrayPath_Key, "Closest Triangle Ids Array Name", "", "Closest Triangle Ids"));

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
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  auto pTriangleNormalsArrayDataPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesArrayName = filterArgs.value<std::string>(k_DistancesArrayPath_Key);
  auto pClosestTriangleIdArrayName = filterArgs.value<std::string>(k_ClosestTriangleIdArrayPath_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(pVertexGeometryDataPath);
  auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);

  DataPath parentDataPath = pTriangleNormalsArrayDataPath.getParent();
  if(dataStructure.getDataAs<BaseGroup>(parentDataPath) == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-4530, fmt::format("The parent dataObject is not able to hold then new created arrays."))};
  }

  auto distancesDataPath = parentDataPath.createChildPath(pDistancesArrayName);
  auto createDistancesArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, std::vector<usize>{vertexGeomPtr->getNumberOfVertices()}, std::vector<usize>{1}, distancesDataPath);
  resultOutputActions.value().actions.push_back(std::move(createDistancesArrayAction));

  auto closestTriangleIdDataPath = parentDataPath.createChildPath(pClosestTriangleIdArrayName);
  auto createClosestTriangleIdArrayAction =
      std::make_unique<CreateArrayAction>(complex::DataType::int64, std::vector<usize>{vertexGeomPtr->getNumberOfVertices()}, std::vector<usize>{1}, closestTriangleIdDataPath);
  resultOutputActions.value().actions.push_back(std::move(createClosestTriangleIdArrayAction));

  // Create temp array then deferred delete
  auto tempTriBoundsDataPath = DataPath({::k_TriangleBounds});
  auto createTriBoundsArrayAction =
      std::make_unique<CreateArrayAction>(complex::DataType::float32, std::vector<usize>{triangleGeomPtr->getNumberOfFaces()}, std::vector<usize>{6}, tempTriBoundsDataPath);
  resultOutputActions.value().actions.push_back(std::move(createTriBoundsArrayAction));

  auto removeTempArrayAction = std::make_unique<DeleteDataAction>(tempTriBoundsDataPath);
  resultOutputActions.value().deferredActions.push_back(std::move(removeTempArrayAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindVertexToTriangleDistancesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  FindVertexToTriangleDistancesInputValues inputValues;

  inputValues.VertexDataContainer = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  inputValues.TriangleDataContainer = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  inputValues.TriangleNormalsArrayPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);

  inputValues.DistancesArrayPath = inputValues.TriangleNormalsArrayPath.createChildPath(filterArgs.value<std::string>(k_DistancesArrayPath_Key));
  inputValues.ClosestTriangleIdArrayPath = inputValues.TriangleNormalsArrayPath.createChildPath(filterArgs.value<std::string>(k_ClosestTriangleIdArrayPath_Key));

  inputValues.TriBoundsDataPath = DataPath({::k_TriangleBounds});

  return FindVertexToTriangleDistances(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
