#include "IterativeClosestPointFilter.hpp"

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include "ComplexCore/utils/nanoflann.hpp"

namespace complex
{
namespace
{
constexpr int64 k_MissingMovingVertex = -4500;
constexpr int64 k_MissingTargetVertex = -4501;
constexpr int64 k_BadNumIterations = -4502;
constexpr int64 k_MissingVertices = -4503;

template <typename Derived>
struct VertexGeomAdaptor
{
  const Derived& obj;

  VertexGeomAdaptor(const Derived& obj_)
  : obj(obj_)
  {
  }

  inline const Derived& derived() const
  {
    return obj;
  }

  inline usize kdtree_get_point_count() const
  {
    return derived()->getNumberOfVertices();
  }

  inline float kdtree_get_pt(const usize idx, const usize dim) const
  {
    auto derivedVertices = derived()->getVertices();
    auto numComponents = derivedVertices->getNumberOfComponents();
    auto offset = idx * numComponents;

    if(dim == 0)
    {
      return (*derived()->getVertices())[offset + 0];
    }
    if(dim == 1)
    {
      return (*derived()->getVertices())[offset + 1];
    }

    return (*derived()->getVertices())[offset + 2];
  }

  template <class BBOX>
  bool kdtree_get_bbox(BBOX& /*bb*/) const
  {
    return false;
  }
};
} // namespace

std::string IterativeClosestPointFilter::name() const
{
  return FilterTraits<IterativeClosestPointFilter>::name;
}

std::string IterativeClosestPointFilter::className() const
{
  return FilterTraits<IterativeClosestPointFilter>::className;
}

Uuid IterativeClosestPointFilter::uuid() const
{
  return FilterTraits<IterativeClosestPointFilter>::uuid;
}

std::string IterativeClosestPointFilter::humanName() const
{
  return "Iterative Closest Point";
}

Parameters IterativeClosestPointFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_MovingVertexPath_Key, "Moving Vertex Geometry", "Numeric Type of data to create", DataPath()));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_TargetVertexPath_Key, "Target Vertex Geometry", "Number of components", DataPath()));
  params.insert(std::make_unique<UInt64Parameter>(k_NumIterations_Key, "Number of Iterations", "Number of components", 1));
  params.insert(std::make_unique<BoolParameter>(k_ApplyTransformation_Key, "Apply Transformation to Moving Geometry", "Number of components", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformArrayPath_Key, "Transform Array", "Number of tuples", DataPath()));
  return params;
}

IFilter::UniquePointer IterativeClosestPointFilter::clone() const
{
  return std::make_unique<IterativeClosestPointFilter>();
}

IFilter::PreflightResult IterativeClosestPointFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto movingVertexPath = args.value<DataPath>(k_MovingVertexPath_Key);
  auto targetVertexPath = args.value<DataPath>(k_TargetVertexPath_Key);
  auto numIterations = args.value<uint64>(k_NumIterations_Key);
  auto applytransformation = args.value<bool>(k_ApplyTransformation_Key);
  auto transformArrayPath = args.value<DataPath>(k_TransformArrayPath_Key);

  if(data.getDataAs<VertexGeom>(movingVertexPath) == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry not found at path: {}", movingVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingMovingVertex, ss}})};
  }
  if(data.getDataAs<VertexGeom>(targetVertexPath) == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry not found at path: {}", targetVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingTargetVertex, ss}})};
  }

  if(numIterations < 1)
  {
    auto ss = fmt::format("Must perform at least 1 iterations");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BadNumIterations, ss}})};
  }

  usize numTuples = 1;
  auto action = std::make_unique<CreateArrayAction>(NumericType::float32, std::vector<usize>{numTuples}, std::vector<usize>{16}, transformArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> IterativeClosestPointFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto movingVertexPath = args.value<DataPath>(k_MovingVertexPath_Key);
  auto targetVertexPath = args.value<DataPath>(k_TargetVertexPath_Key);
  auto numIterations = args.value<uint64>(k_NumIterations_Key);
  auto applyTransformation = args.value<bool>(k_ApplyTransformation_Key);
  auto transformArrayPath = args.value<DataPath>(k_TransformArrayPath_Key);

  auto movingVertexGeom = data.getDataAs<VertexGeom>(movingVertexPath);
  auto targetVertexGeom = data.getDataAs<VertexGeom>(targetVertexPath);

  if(movingVertexGeom == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry not found at path '{}'", movingVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }
  if(targetVertexGeom == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry not found at path '{}'", targetVertexPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }

  auto* movingPtr = movingVertexGeom->getVertices();
  auto* movingCopyPtr = movingVertexGeom->getVertices();
  auto* targetPtr = targetVertexGeom->getVertices();

  if(movingPtr == nullptr)
  {
    auto ss = fmt::format("Moving Vertex Geometry does not contain a vertex array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }
  if(targetPtr == nullptr)
  {
    auto ss = fmt::format("Target Vertex Geometry does not contain a vertex array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingVertices, ss}})};
  }

  auto* movingStore = movingCopyPtr->getDataStore();
  std::vector<float32> movingData(movingStore->begin(), movingStore->end());

  DataStructure tmp;

  usize numMovingVerts = movingVertexGeom->getNumberOfVertices();
  std::vector<usize> cDims(1, 3);
  Float32Array* dynTarget = Float32Array::CreateWithStore<DataStore<float32>>(tmp, "tmp", {numMovingVerts}, cDims, true);
  dynTarget->fill(0.0f);
  auto* dynTargetPtr = dynTarget->getDataStore();
  std::vector<float32> dynTargetData(dynTargetPtr->begin(), dynTargetPtr->end());

  using Adaptor = VertexGeomAdaptor<VertexGeom*>;
  const Adaptor adaptor(targetVertexGeom);

  // notifyStatusMessage("Building kd-tree index...");

  using KDtree = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Adaptor<float32, Adaptor>, Adaptor, 3>;
  KDtree index(3, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(30));
  index.buildIndex();

  usize iters = numIterations;
  const usize nn = 1;

  typedef Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::ColMajor> PointCloud;
  typedef Eigen::Matrix<float, 4, 4, Eigen::ColMajor> UmeyamaTransform;

  UmeyamaTransform globalTransform;
  globalTransform << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;

  int64 progIncrement = iters / 100;
  int64 prog = 1;
  int64 progressInt = 0;
  int64 counter = 0;

  auto targetData = targetPtr->getDataStore();

  for(usize i = 0; i < iters; i++)
  {
    for(usize j = 0; j < numMovingVerts; j++)
    {
      usize id;
      float dist;
      nanoflann::KNNResultSet<float> results(nn);
      results.init(&id, &dist);
      index.findNeighbors(results, movingData.data() + (3 * j), nanoflann::SearchParams());
      const usize dynOffset = 3 * j;
      const usize targetOffset = 3 * id;
      dynTargetPtr->setValue(dynOffset + 0, targetData->getValue(targetOffset + 0));
      (*dynTargetPtr)[dynOffset + 1] = (*targetData)[targetOffset + 1];
      (*dynTargetPtr)[dynOffset + 2] = (*targetData)[targetOffset + 2];
    }

    Eigen::Map<PointCloud> moving_(movingData.data(), 3, numMovingVerts);
    Eigen::Map<PointCloud> target_(dynTargetData.data(), 3, numMovingVerts);

    UmeyamaTransform transform = Eigen::umeyama(moving_, target_, false);

    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position((*movingCopyPtr)[3 * j + 0], (*movingCopyPtr)[3 * j + 1], (*movingCopyPtr)[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = transform * position;
      float* transformPositionData = transformedPosition.data();
      for(usize k = 0; k < 3; k++)
      {
        (*movingCopyPtr)[3 * j + k] = transformPositionData[k];
      }
    }

    globalTransform = transform * globalTransform;

    if(counter > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(counter) / iters) * 100.0f);
      std::string ss = fmt::format("Performing Registration Iterations || {}% Completed", progressInt);
      // notifyStatusMessage(ss);
      prog = prog + progIncrement;
    }
    counter++;
  }

  auto* transformPtr = data.getDataAs<Float32Array>(transformArrayPath)->getDataStore();

  if(applyTransformation)
  {
    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position((*movingPtr)[3 * j + 0], (*movingPtr)[3 * j + 1], (*movingPtr)[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = globalTransform * position;
      auto transformedPositionData = transformedPosition.data();
      for(usize k = 0; k < 3; k++)
      {
        (*movingPtr)[3 * j + k] = transformedPosition.data()[k];
      }
    }
  }

  globalTransform.transposeInPlace();
  for(usize j = 0; j < 16; j++)
  {
    (*transformPtr)[j] = globalTransform.data()[j];
  }

  return {};
}
} // namespace complex
