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
#include "complex/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <string>

using namespace complex;

namespace
{
constexpr int32 k_VertexGeomNotFound = -277;
constexpr int32 k_ArrayNotFound = -278;
constexpr int32 k_TupleShapeNotOneDim = -279;

struct RemoveFlaggedVerticesFunctor
{
  // copy data to masked geometry
  template <class T>
  void operator()(IDataArray& inputDataPtr, IDataArray& maskedDataPtr, const std::vector<int64>& maskPoints) const
  {
    auto& inputData = dynamic_cast<DataArray<T>&>(inputDataPtr);
    auto& maskedData = dynamic_cast<DataArray<T>&>(maskedDataPtr);
    maskedData.getDataStore()->reshapeTuples({maskPoints.size()});
    usize nComps = inputData.getNumberOfComponents();

    for(usize i = 0; i < maskPoints.size(); i++)
    {
      for(usize compIdx = 0; compIdx < nComps; compIdx++)
      {
        usize destinationIndex = nComps * i + compIdx;
        usize sourceIndex = nComps * maskPoints[i] + compIdx;
        maskedData[destinationIndex] = inputData[sourceIndex];
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
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_ArraySelection_Key, "Vertex Data Arrays", "Paths to the target Vertex DataArrays to also reduce.", std::vector<DataPath>(),
                                                               complex::GetAllDataTypes()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskPath_Key, "Mask Array", "DataPath to the conditional array that will be used to decide which vertices are removed.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_ReducedVertexPath_Key, "Reduced Vertex Geometry", "Created Vertex Geometry DataPath. This will be created during the filter.", DataPath()));
  return params;
}

IFilter::UniquePointer RemoveFlaggedVertices::clone() const
{
  return std::make_unique<RemoveFlaggedVertices>();
}

IFilter::PreflightResult RemoveFlaggedVertices::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = filterArgs.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = filterArgs.value<DataPath>(k_MaskPath_Key);
  auto targetArrayPaths = filterArgs.value<std::vector<DataPath>>(k_ArraySelection_Key);
  auto reducedVertexPath = filterArgs.value<DataPath>(k_ReducedVertexPath_Key);

  OutputActions actions;
  std::vector<DataPath> dataArrayPaths;

  const auto* vertex = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  if(vertex == nullptr)
  {
    std::string errorMsg = fmt::format("Vertex Geometry not found at path: '{}'", vertexGeomPath.toString());
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, errorMsg)};
  }
  auto verticesId = vertex->getVertexListId();
  auto* verticesArray = dataStructure.getDataAs<Float32Array>(verticesId);
  if(verticesArray == nullptr)
  {
    std::string errorMsg = fmt::format("Vertex Geometry does not have a vertex list");
    return {MakeErrorResult<OutputActions>(::k_VertexGeomNotFound, errorMsg)};
  }
  // dataArrayPaths.push_back(verticesArray->getDataPaths()[0]);

  std::vector<size_t> cDims(1, 1);

  const auto* maskArray = dataStructure.getDataAs<BoolArray>(maskArrayPath);
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
    const auto* targetArray = dataStructure.getDataAs<IDataArray>(targetArrayPath);
    if(targetArray == nullptr)
    {
      std::string errorMsg = fmt::format("Array not found at path: '{}'", targetArrayPath.toString());
      return {MakeErrorResult<OutputActions>(::k_ArrayNotFound, errorMsg)};
    }
    // Make sure selected arrays are 1D Arrays (number of components does not matter)
    if(targetArray->getIDataStoreRef().getTupleShape().size() != 1)
    {
      std::string errorMsg = fmt::format("Tuple Dimensions at path: '{}' are not 1D: '{}'", targetArrayPath.toString(), fmt::join(targetArray->getIDataStoreRef().getTupleShape(), ", "));
      return {MakeErrorResult<OutputActions>(::k_TupleShapeNotOneDim, errorMsg)};
    }
    // Create array copy
    auto dataType = targetArray->getDataType();
    auto numTuples = targetArray->getNumberOfTuples();
    auto numComponents = targetArray->getNumberOfComponents();
    auto outputPath = reducedVertexPath.createChildPath(targetArray->getName());
    auto action = std::make_unique<CreateArrayAction>(dataType, std::vector<usize>{numTuples}, std::vector<usize>{numComponents}, outputPath);
    actions.actions.push_back(std::move(action));
  }

  return {std::move(actions)};
}

Result<> RemoveFlaggedVertices::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeomPath_Key);
  auto maskArrayPath = args.value<DataPath>(k_MaskPath_Key);
  auto targetArrayPaths = args.value<std::vector<DataPath>>(k_ArraySelection_Key);
  auto reducedVertexPath = args.value<DataPath>(k_ReducedVertexPath_Key);

  VertexGeom& vertex = data.getDataRefAs<VertexGeom>(vertexGeomPath);
  auto& mask = data.getDataRefAs<BoolArray>(maskArrayPath);

  size_t numMaskTuples = mask.getSize();
  size_t trueCount = std::count(mask.begin(), mask.end(), true);
  std::vector<int64> trueIndices(trueCount);
  size_t trueIndex = 0;
  for(int64 i = 0; i < numMaskTuples; i++)
  {
    if(mask[i])
    {
      trueIndices[trueIndex++] = i;
    }
  }

  VertexGeom& reducedVertex = data.getDataRefAs<VertexGeom>(reducedVertexPath);
  reducedVertex.resizeVertexList(trueCount);

  for(size_t i = 0; i < trueCount; i++)
  {
    Point3D<float> coords = std::move(vertex.getCoords(trueIndices[i]));
    reducedVertex.setCoords(i, coords);
  }

  std::vector<usize> tDims = {trueIndices.size()};

  for(const auto& targetArrayPath : targetArrayPaths)
  {
    DataPath destinationPath = reducedVertexPath.createChildPath(targetArrayPath.getTargetName());
    auto& src = data.getDataRefAs<IDataArray>(targetArrayPath);
    auto& dest = data.getDataRefAs<IDataArray>(destinationPath);

    ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, trueIndices);
  }

  return {};
}
} // namespace complex
