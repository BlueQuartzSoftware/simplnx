#include "SampleSurfaceMesh.hpp"

#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

#include <chrono>
#include <memory>

using namespace nx::core;

namespace
{
/**
 * @struct FeatureBoundingVolume
 * @brief Stores one feature bounding box and its ray-test radius.
 *
 * Both values depend only on triangle geometry. Precomputation lets all sample
 * points reuse them.
 */
struct FeatureBoundingVolume
{
  BoundingBox3Df Box;
  float32 Radius = 0.0f;
};

/**
 * @class SliceSampleSurfaceMeshImpl
 * @brief Assigns one slice of sample points to the first enclosing feature.
 * @tparam OutputT Specifies the output feature-ID type.
 * @tparam FaceLabelsT Specifies the face-label type.
 *
 * Parallel ranges read immutable face lists and bounding data. They write
 * different slice-output positions. Triangle geometry reads use the concrete
 * store contract described by SampleSurfaceMesh.
 */
template <typename OutputT, typename FaceLabelsT>
class SliceSampleSurfaceMeshImpl
{
public:
  SliceSampleSurfaceMeshImpl(const TriangleGeom& faces, const std::vector<std::vector<FaceLabelsT>>& faceLists, const std::vector<BoundingBox3Df>& faceBBs,
                             const std::vector<FeatureBoundingVolume>& featureBounds, const std::vector<Point3Df>& slicePoints, nonstd::span<OutputT> sliceOutput, const std::atomic_bool& shouldCancel,
                             std::atomic_bool& overflowHit)
  : m_Faces(faces)
  , m_FaceLists(faceLists)
  , m_FaceBBs(faceBBs)
  , m_FeatureBounds(featureBounds)
  , m_SlicePoints(slicePoints)
  , m_SliceOutput(sliceOutput)
  , m_ShouldCancel(shouldCancel)
  , m_OverflowHit(overflowHit)
  {
  }
  ~SliceSampleSurfaceMeshImpl() = default;

  SliceSampleSurfaceMeshImpl(const SliceSampleSurfaceMeshImpl&) = default;
  SliceSampleSurfaceMeshImpl(SliceSampleSurfaceMeshImpl&&) noexcept = default;
  SliceSampleSurfaceMeshImpl& operator=(const SliceSampleSurfaceMeshImpl&) = delete;
  SliceSampleSurfaceMeshImpl& operator=(SliceSampleSurfaceMeshImpl&&) = delete;

  /**
   * @brief Processes one disjoint sample-point range.
   * @param range Specifies the half-open local point range.
   */
  void operator()(const Range& range) const
  {
    const usize numFeatures = m_FeatureBounds.size();
    for(usize i = range.min(); i < range.max(); i++)
    {
      // One point can require a test against every feature. Check cancellation
      // per point to bound response latency during this expensive inner work.
      if(m_ShouldCancel || m_OverflowHit)
      {
        return;
      }

      const Point3Df point = m_SlicePoints[i];
      OutputT assignedFeature = 0;
      for(usize featureId = 0; featureId < numFeatures; featureId++)
      {
        const FeatureBoundingVolume& featureBounds = m_FeatureBounds[featureId];
        char code = GeometryMath::IsPointInPolyhedron(m_Faces, m_FaceLists[featureId], m_FaceBBs, point, featureBounds.Box, featureBounds.Radius);
        if(code == 'i' || code == 'V' || code == 'E' || code == 'F')
        {
          assignedFeature = static_cast<OutputT>(featureId);
          break;
        }
      }
      m_SliceOutput[i] = assignedFeature;
    }
  }

private:
  const TriangleGeom& m_Faces;
  const std::vector<std::vector<FaceLabelsT>>& m_FaceLists;
  const std::vector<BoundingBox3Df>& m_FaceBBs;
  const std::vector<FeatureBoundingVolume>& m_FeatureBounds;
  const std::vector<Point3Df>& m_SlicePoints;
  nonstd::span<OutputT> m_SliceOutput;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_bool& m_OverflowHit;
};

/**
 * @struct SampleSlicesFunctor
 * @brief Dispatches the bounded Z-slice loop for an output feature-ID type.
 */
struct SampleSlicesFunctor
{
  /**
   * @brief Generates, tests, and writes all sampling-grid slices.
   * @tparam OutputT Specifies the output feature-ID type.
   * @tparam FaceLabelsT Specifies the face-label type.
   * @param algorithm Supplies grid dimensions and serial slice-point generation.
   * @param triangleGeom Supplies the surface geometry.
   * @param faceLists Maps each feature to triangle indices.
   * @param faceBBs Supplies one bounding box per triangle.
   * @param featureBounds Supplies one precomputed bounding volume per feature.
   * @param polyIds Receives output feature IDs.
   * @param shouldCancel Supplies the cancellation flag.
   * @param messageHelper Reports progress.
   * @return Valid result, bulk-write error, or feature-ID overflow error.
   */
  template <typename OutputT, typename FaceLabelsT>
  Result<> operator()(SampleSurfaceMesh* algorithm, const TriangleGeom& triangleGeom, const std::vector<std::vector<FaceLabelsT>>& faceLists, const std::vector<BoundingBox3Df>& faceBBs,
                      const std::vector<FeatureBoundingVolume>& featureBounds, IDataArray& polyIds, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
  {
    const usize numFeatures = faceLists.size();

    // Reject a feature range that the output integer type cannot represent.
    std::atomic_bool overflowHit(false);
    if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
    {
      if(std::numeric_limits<OutputT>::max() < numFeatures - 1)
      {
        overflowHit = true;
      }
    }

    auto& outputStore = polyIds.getIDataStoreRefAs<AbstractDataStore<OutputT>>();

    const SizeVec3 gridDims = algorithm->getGridDimensions();
    const usize cellsPerSlice = gridDims.getX() * gridDims.getY();
    const usize numSlices = gridDims.getZ();

    messageHelper.sendMessage("Sampling triangle geometry ...");
    ProgressMessageHelper progressMessageHelper = messageHelper.createProgressMessageHelper();
    progressMessageHelper.setMaxProgresss(numSlices);
    progressMessageHelper.setProgressMessageTemplate("Sampling triangle geometry: {:.1f}%");
    auto progressMessenger = progressMessageHelper.createProgressMessenger(std::chrono::milliseconds(1000));

    // Reuse point and output buffers whose size is proportional to one XY slice.
    std::vector<Point3Df> slicePoints(cellsPerSlice);
    auto sliceOutput = std::make_unique<OutputT[]>(cellsPerSlice);

    for(usize zSlice = 0; zSlice < numSlices; zSlice++)
    {
      if(shouldCancel)
      {
        break;
      }

      // Serial increasing-Z generation preserves a stateful random draw sequence.
      algorithm->generateSlicePoints(zSlice, slicePoints);

      SliceSampleSurfaceMeshImpl<OutputT, FaceLabelsT> impl(triangleGeom, faceLists, faceBBs, featureBounds, slicePoints, nonstd::span<OutputT>(sliceOutput.get(), cellsPerSlice), shouldCancel,
                                                            overflowHit);
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, cellsPerSlice);
      dataAlg.execute(impl);

      if(overflowHit || shouldCancel)
      {
        break;
      }

      Result<> copyResult = outputStore.copyFromBuffer(zSlice * cellsPerSlice, nonstd::span<const OutputT>(sliceOutput.get(), cellsPerSlice));
      if(copyResult.invalid())
      {
        return copyResult;
      }

      progressMessenger.sendProgressMessage(1);
    }

    if(overflowHit)
    {
      return MakeErrorResult(
          -158630, fmt::format("Overflow occurred when downcasting a Face Label value of type {} to a feature Id value of type {}. Feature count of {} is greater than max value ({})",
                               DataTypeToHumanString(GetDataType<FaceLabelsT>()), DataTypeToHumanString(polyIds.getDataType()), numFeatures - 1, DataTypeToHumanString(polyIds.getDataType())));
    }

    messageHelper.sendMessage("Complete");

    return {};
  }
};

/**
 * @struct SampleSurfaceMeshFunctor
 * @brief Dispatches face-label processing for one runtime integer type.
 */
struct SampleSurfaceMeshFunctor
{
  /**
   * @brief Builds mesh lookup data and starts bounded slice sampling.
   * @tparam T Specifies the face-label integer type.
   * @param algorithm Supplies slice generation.
   * @param triangleGeom Supplies the surface geometry.
   * @param iFaceLabels Supplies two feature labels per triangle.
   * @param polyIds Receives cell feature IDs.
   * @param shouldCancel Supplies the cancellation flag.
   * @param messageHelper Reports progress.
   * @return Valid result, bulk-write error, or feature-ID overflow error.
   */
  template <typename T>
  Result<> operator()(SampleSurfaceMesh* algorithm, const TriangleGeom& triangleGeom, const IDataArray& iFaceLabels, IDataArray& polyIds, const std::atomic_bool& shouldCancel,
                      MessageHelper& messageHelper)
  {
    const AbstractDataStore<T>& faceLabelsSM = dynamic_cast<const DataArray<T>&>(iFaceLabels).getDataStoreRef();
    const usize numFaces = faceLabelsSM.getNumberOfTuples();

    messageHelper.sendMessage("Counting number of Features...");

    // The largest positive face label determines the feature-list count.
    T g1 = 0, g2 = 0;
    T maxFeatureId = 0;
    for(usize i = 0; i < numFaces; i++)
    {
      g1 = faceLabelsSM[2 * i];
      g2 = faceLabelsSM[2 * i + 1];
      if(g1 > maxFeatureId)
      {
        maxFeatureId = g1;
      }
      if(g2 > maxFeatureId)
      {
        maxFeatureId = g2;
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    // Include feature zero for background-compatible indexing.
    usize numFeatures = maxFeatureId + 1;

    std::vector<std::vector<T>> faceLists(numFeatures);
    messageHelper.sendMessage("Counting number of triangle faces per feature ...");

    // Size each feature list from its positive label occurrences.
    for(usize i = 0; i < numFaces; i++)
    {
      g1 = faceLabelsSM[2 * i];
      g2 = faceLabelsSM[2 * i + 1];
      if(g1 > 0)
      {
        faceLists[g1].push_back(0);
      }
      if(g2 > 0)
      {
        faceLists[g2].push_back(0);
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    messageHelper.sendMessage("Allocating triangle faces per feature ...");

    // Track the next insertion position for each pre-sized face list.
    std::vector<int32> linkLoc(numFaces, 0);

    std::vector<BoundingBox3Df> faceBBs;
    {
      // Keep this GeometryStoreCache in the serial face traversal. It performs
      // per-element generic-store reads and does not make concurrent access safe.
      const GeometryMath::detail::GeometryStoreCache cache(triangleGeom.getVertices()->getDataStoreRef(), triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getNumberOfVerticesPerFace());

      // Reuse the vertex-index buffer for each triangle.
      std::vector<usize> verts(cache.NumVertsPerFace);

      // Fill feature face lists and calculate one bounding box per triangle.
      for(int32 i = 0; i < numFaces; i++)
      {
        g1 = faceLabelsSM[2 * i];
        g2 = faceLabelsSM[2 * i + 1];
        if(g1 > 0)
        {
          faceLists[g1][(linkLoc[g1])++] = i;
        }
        if(g2 > 0)
        {
          faceLists[g2][(linkLoc[g2])++] = i;
        }
        faceBBs.emplace_back(GeometryMath::FindBoundingBoxOfFace(cache, triangleGeom, i, verts));
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    // Feature bounding volumes depend only on the mesh and serve all sample points.
    std::vector<FeatureBoundingVolume> featureBounds;
    featureBounds.reserve(numFeatures);
    for(usize featureId = 0; featureId < numFeatures; featureId++)
    {
      BoundingBox3Df boundingBox(GeometryMath::FindBoundingBoxOfFaces(triangleGeom, faceLists[featureId]));
      float32 radius = GeometryMath::FindDistanceBetweenPoints(boundingBox.getMinPoint(), boundingBox.getMaxPoint()) / 2;
      featureBounds.emplace_back(FeatureBoundingVolume{boundingBox, radius});
    }

    if(shouldCancel)
    {
      return {};
    }

    // Dispatch output type after mesh-scale lookup data is complete.
    return ExecuteDataFunctionIntType(SampleSlicesFunctor{}, polyIds.getDataType(), algorithm, triangleGeom, faceLists, faceBBs, featureBounds, polyIds, shouldCancel, messageHelper);
  }
};
} // namespace

SampleSurfaceMesh::SampleSurfaceMesh(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_MessageHelper(m_MessageHandler)
{
}

SampleSurfaceMesh::~SampleSurfaceMesh() noexcept = default;

Result<> SampleSurfaceMesh::execute(SampleSurfaceMeshInputValues& inputValues)
{
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(inputValues.TriangleGeometryPath);
  const auto& iFaceLabels = m_DataStructure.getDataRefAs<IDataArray>(inputValues.SurfaceMeshFaceLabelsArrayPath);

  // Resolve the existing output that receives one feature ID per sample point.
  auto& polyIds = m_DataStructure.getDataRefAs<IDataArray>(inputValues.FeatureIdsArrayPath);

  // Parameter validation restricts face labels to integer types.
  return ExecuteDataFunctionIntType(SampleSurfaceMeshFunctor{}, iFaceLabels.getDataType(), this, triangleGeom, iFaceLabels, polyIds, m_ShouldCancel, m_MessageHelper);
}
