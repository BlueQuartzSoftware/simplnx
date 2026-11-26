#include "IterativeClosestPoint.hpp"

#include <cmath>

#include "nanoflann/nanoflann.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <Eigen/Geometry>

using namespace nx::core;

namespace
{
constexpr int32 k_MissingVertices = -4503;
constexpr int32 k_EmptyVertices = -4505;

template <typename Derived>
struct VertexGeomAdaptor
{
  const Derived& obj;
  AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>* verts;
  size_t num_components = 0;
  size_t num_tuples = 0;

  explicit VertexGeomAdaptor(const Derived& obj_)
  : obj(obj_)
  {
    // These values never change for the lifetime of this object so cache them now.
    verts = derived()->getVertices()->getDataStore();
    num_components = verts->getNumberOfComponents();
    num_tuples = verts->getNumberOfTuples();
  }

  [[nodiscard]] const Derived& derived() const
  {
    return obj;
  }

  [[nodiscard]] usize kdtree_get_point_count() const
  {
    return num_tuples;
  }

  [[nodiscard]] float kdtree_get_pt(const usize idx, const usize dim) const
  {
    auto offset = idx * num_components;
    return verts->getValue(offset + dim);
  }

  template <class BBOX>
  bool kdtree_get_bbox(BBOX& /*bb*/) const
  {
    return false;
  }
};
} // namespace

// -----------------------------------------------------------------------------
IterativeClosestPoint::IterativeClosestPoint(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             IterativeClosestPointInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& IterativeClosestPoint::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IterativeClosestPoint::operator()()
{
  auto* movingVertexGeomPtr = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->MovingVertexPath);
  auto* targetVertexGeomPtr = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->TargetVertexPath);

  if(movingVertexGeomPtr == nullptr)
  {
    return MakeErrorResult(k_MissingVertices, fmt::format("Moving Vertex Geometry not found at path '{}'", m_InputValues->MovingVertexPath.toString()));
  }
  if(targetVertexGeomPtr == nullptr)
  {
    return MakeErrorResult(k_MissingVertices, fmt::format("Target Vertex Geometry not found at path '{}'", m_InputValues->TargetVertexPath.toString()));
  }

  if(movingVertexGeomPtr->getVertices() == nullptr)
  {
    return MakeErrorResult(k_MissingVertices, fmt::format("Moving Vertex Geometry does not contain a vertex array"));
  }
  if(targetVertexGeomPtr->getVertices() == nullptr)
  {
    return MakeErrorResult(k_MissingVertices, fmt::format("Target Vertex Geometry does not contain a vertex array"));
  }

  Float32AbstractDataStore& movingStore = movingVertexGeomPtr->getVertices()->getDataStoreRef();
  if(movingStore.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(k_EmptyVertices, fmt::format("Moving Vertex Geometry does not contain any vertices"));
  }
  Float32AbstractDataStore& targetStore = targetVertexGeomPtr->getVertices()->getDataStoreRef();
  if(targetStore.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(k_EmptyVertices, fmt::format("Target Vertex Geometry does not contain any vertices"));
  }

  std::vector<float32> movingVector(movingStore.begin(), movingStore.end());
  float32* movingCopyPtr = movingVector.data();
  DataStructure tmp;

  usize numMovingVerts = movingVertexGeomPtr->getNumberOfVertices();
  std::vector<float32> dynTarget(numMovingVerts * 3, 0.0F);
  float* dynTargetPtr = dynTarget.data();

  using AdaptorType = VertexGeomAdaptor<VertexGeom*>;
  const AdaptorType adaptor(targetVertexGeomPtr);

  m_MessageHandler.sendInfoMessage("Building kd-tree index...");

  using KDtreeType = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Adaptor<float32, AdaptorType>, AdaptorType, 3>;
  KDtreeType index(3, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(30));
  index.buildIndex();

  const usize nn = 1;

  using PointCloudType = Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::ColMajor>;
  using UmeyamaTransformType = Eigen::Matrix<float, 4, 4, Eigen::ColMajor>;

  UmeyamaTransformType globalTransform;
  globalTransform << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;

  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  progressThrottle.reset(m_InputValues->NumIterations, "Performing Registration Iterations");
  for(usize i = 0; i < m_InputValues->NumIterations; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    for(usize j = 0; j < numMovingVerts; j++)
    {
      usize identifier = 0;
      float dist = NAN;
      nanoflann::KNNResultSet<float> results(nn);
      results.init(&identifier, &dist);
      index.findNeighbors(results, movingCopyPtr + (3 * j), nanoflann::SearchParams());
      dynTargetPtr[3 * j + 0] = targetStore[3 * identifier + 0];
      dynTargetPtr[3 * j + 1] = targetStore[3 * identifier + 1];
      dynTargetPtr[3 * j + 2] = targetStore[3 * identifier + 2];
    }

    Eigen::Map<PointCloudType> moving(movingCopyPtr, 3, numMovingVerts);
    Eigen::Map<PointCloudType> target(dynTargetPtr, 3, numMovingVerts);

    UmeyamaTransformType transform = Eigen::umeyama(moving, target, false);

    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position(movingCopyPtr[3 * j + 0], movingCopyPtr[3 * j + 1], movingCopyPtr[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = transform * position;
      std::memcpy(movingCopyPtr + (3 * j), transformedPosition.data(), sizeof(float) * 3);
    }
    // Update the global transform
    globalTransform = transform * globalTransform;

    progressThrottle.updateCount(i + 1);
  }

  auto& transformStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->TransformArrayPath)->getDataStoreRef();

  if(m_InputValues->ApplyTransformation)
  {
    for(usize j = 0; j < numMovingVerts; j++)
    {
      Eigen::Vector4f position(movingStore[3 * j + 0], movingStore[3 * j + 1], movingStore[3 * j + 2], 1);
      Eigen::Vector4f transformedPosition = globalTransform * position;
      for(usize k = 0; k < 3; k++)
      {
        movingStore[3 * j + k] = transformedPosition.data()[k];
      }
    }
  }

  globalTransform.transposeInPlace();
  for(usize j = 0; j < 16; j++)
  {
    transformStore[j] = globalTransform.data()[j];
  }

  return {};
}
