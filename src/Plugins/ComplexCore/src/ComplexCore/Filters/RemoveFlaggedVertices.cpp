#include "RemoveFlaggedVertices.hpp"

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <string>

using namespace complex;

namespace
{
constexpr int32 k_VertexGeomNotFound = -277;
constexpr int32 k_ArrayNotFound = -278;

struct RemoveFlaggedVerticesFunctor
{
  // copy data to masked geometry
  template <class T>
  void operator()(IDataArray& inputDataPtr, IDataArray& maskedDataPtr, const std::vector<int64>& maskPoints) const
  {
    auto& inputData = dynamic_cast<DataArray<T>&>(inputDataPtr);
    auto& maskedData = dynamic_cast<DataArray<T>&>(maskedDataPtr);

    usize nComps = inputData.getNumberOfComponents();

    for(usize i = 0; i < maskPoints.size(); i++)
    {
      for(usize d = 0; d < nComps; d++)
      {
        usize tmpIndex = nComps * i + d;
        usize ptrIndex = nComps * maskPoints[i] + d;
        maskedData[tmpIndex] = inputData[ptrIndex];
      }
    }
  }
};
} // namespace

namespace complex
{

std::string RemoveFlaggedVertices::name() const
{
  return FilterTraits<RemoveFlaggedVertices>::name;
}

std::string RemoveFlaggedVertices::className() const
{
  return FilterTraits<RemoveFlaggedVertices>::className;
}

Uuid RemoveFlaggedVertices::uuid() const
{
  return FilterTraits<RemoveFlaggedVertices>::uuid;
}

std::string RemoveFlaggedVertices::humanName() const
{
  return "Remove Flagged Vertices";
}

std::vector<std::string> RemoveFlaggedVertices::defaultTags() const
{
  return IFilter::defaultTags();
}

Parameters RemoveFlaggedVertices::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeomPath_Key, "Vertex Geometry", "Path to the target Vertex Geometry", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Vertex}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_ArraySelection_Key, "Target Arrays", "Paths to the target DataArrays", std::vector<DataPath>()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskPath_Key, "Mask Array", "DataPath to the conditional array that will be used to decide which vertices are removed.", DataPath(), false,
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_ReducedVertexPath_Key, "Reduced Vertex Geometry", "Created Vertex Geometry DataPath. This will be created during the filter.", DataPath()));
  return params;
}

IFilter::UniquePointer RemoveFlaggedVertices::clone() const
{
  return std::make_unique<RemoveFlaggedVertices>();
}

IFilter::PreflightResult RemoveFlaggedVertices::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto vertexGeomPath = filterArgs.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskPath_Key);
  auto targetArrayPaths = filterArgs.value<std::vector<DataPath>>(k_ArraySelection_Key);
  auto reducedVertexPath = filterArgs.value<DataPath>(k_ReducedVertexPath_Key);

  OutputActions actions;
  std::vector<DataPath> dataArrayPaths;

  auto* vertex = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  if(vertex == nullptr)
  {
    std::string ss = fmt::format("Vertex Geometry not found at path: '{}'", vertexGeomPath.toString());
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, ss)};
  }
  if(vertex->getVertices() == nullptr)
  {
    std::string ss = fmt::format("Vertex Geometry does not have a vertex list");
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, ss)};
  }

  dataArrayPaths.push_back(vertex->getVertices()->getDataPaths()[0]);

  std::vector<size_t> cDims(1, 1);

  auto* maskArray = dataStructure.getDataAs<DataArray<bool>>(maskArrayPath);
  if(maskArray != nullptr)
  {
    dataArrayPaths.push_back(maskArrayPath);
  }

  // Create vertex geometry
  uint64 numVertices = vertex->getNumberOfVertices();
  auto reduced = std::make_unique<CreateVertexGeometryAction>(reducedVertexPath, numVertices);
  actions.actions.push_back(std::move(reduced));

  dataStructure.validateNumberOfTuples(dataArrayPaths);

  // Create vertex copy
  std::vector<usize> tDims(1, 0);

  for(const auto& targetArrayPath : targetArrayPaths)
  {
    auto* targetArray = dataStructure.getDataAs<IDataArray>(targetArrayPath);
    if(!targetArray)
    {
      std::string ss = fmt::format("Array not found at path: '{}'", targetArrayPath.toString());
      return {MakeErrorResult<OutputActions>(::k_ArrayNotFound, ss)};
    }
    // Create array copy
    auto numericType = static_cast<NumericType>(targetArray->getDataType());
    auto numTuples = targetArray->getNumberOfTuples();
    auto numComponents = targetArray->getNumberOfComponents();
    auto outputPath = reducedVertexPath.createChildPath(targetArray->getName());
    auto action = std::make_unique<CreateArrayAction>(numericType, std::vector<usize>{numTuples}, std::vector<usize>{numComponents}, outputPath);
    actions.actions.push_back(std::move(action));
  }

  return {std::move(actions)};
}

Result<> RemoveFlaggedVertices::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = args.value<DataPath>(k_MaskPath_Key);
  auto targetArrayPaths = args.value<std::vector<DataPath>>(k_ArraySelection_Key);
  auto reducedVertexPath = args.value<DataPath>(k_ReducedVertexPath_Key);

  auto* vertex = data.getDataAs<VertexGeom>(vertexGeomPath);
  int64_t numVerts = vertex->getNumberOfVertices();
  std::vector<int64> maskPoints;
  auto* mask = data.getDataAs<DataArray<bool>>(maskArrayPath);
  maskPoints.reserve(mask->getNumberOfTuples());

  for(int64 i = 0; i < numVerts; i++)
  {
    if(!((*mask)[i]))
    {
      maskPoints.push_back(i);
    }
  }

  maskPoints.shrink_to_fit();

  auto* reducedVertex = data.getDataAs<VertexGeom>(reducedVertexPath);
  reducedVertex->resizeVertexList(maskPoints.size());

  for(std::vector<int64_t>::size_type i = 0; i < maskPoints.size(); i++)
  {
    auto coords = vertex->getCoords(maskPoints[i]);
    reducedVertex->setCoords(i, coords);
  }

  auto* m = data.getDataAs<VertexGeom>(vertexGeomPath);
  std::vector<usize> tDims(1, maskPoints.size());

  for(const auto& targetArrayPath : targetArrayPaths)
  {
    DataPath destinationPath = reducedVertexPath.createChildPath(targetArrayPath.getTargetName());
    auto& src = data.getDataRefAs<IDataArray>(targetArrayPath);
    auto& dest = data.getDataRefAs<IDataArray>(destinationPath);

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, maskPoints);
  }

  return {};
}
} // namespace complex
