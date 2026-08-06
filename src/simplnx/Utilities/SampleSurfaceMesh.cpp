#include "SampleSurfaceMesh.hpp"

#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
template <typename OutputT, typename FaceLabelsT>
class SampleSurfaceMeshImpl
{
public:
  SampleSurfaceMeshImpl(const TriangleGeom& faces, const std::vector<std::vector<FaceLabelsT>>& faceIds, const std::vector<BoundingBox3Df>& faceBBs, const std::vector<Point3Df>& points,
                        IDataArray& iPolyIds, const std::atomic_bool& shouldCancel, std::atomic_bool& overflowHit)
  : m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_PolyIds(iPolyIds.getIDataStoreRefAs<AbstractDataStore<OutputT>>())
  , m_ShouldCancel(shouldCancel)
  , m_OverflowHit(overflowHit)
  {
  }

  ~SampleSurfaceMeshImpl() = default;

  SampleSurfaceMeshImpl(const SampleSurfaceMeshImpl&) = default;           // Copy Constructor Default Implemented
  SampleSurfaceMeshImpl(SampleSurfaceMeshImpl&&) noexcept = default;       // Move Constructor Default Implemented
  SampleSurfaceMeshImpl& operator=(const SampleSurfaceMeshImpl&) = delete; // Copy Assignment Not Implemented
  SampleSurfaceMeshImpl& operator=(SampleSurfaceMeshImpl&&) = delete;      // Move Assignment Not Implemented

  void checkPoints(const usize start, const usize end) const
  {
    if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
    {
      if(std::numeric_limits<OutputT>::max() < end - 1)
      {
        m_OverflowHit = true;
      }
    }
    for(usize iter = start; iter < end; iter++)
    {
      const usize numPoints = m_Points.size();

      // find bounding box for current feature
      BoundingBox3Df boundingBox(GeometryMath::FindBoundingBoxOfFaces(m_Faces, m_FaceIds[iter]));
      float32 radius = GeometryMath::FindDistanceBetweenPoints(boundingBox.getMinPoint(), boundingBox.getMaxPoint()) / 2;

      // check points in vertex array to see if they are in the bounding box of the feature
      for(usize i = 0; i < numPoints; i++)
      {
        // Check for the filter being canceled.
        if(m_ShouldCancel || m_OverflowHit)
        {
          return;
        }

        Point3Df point = m_Points[i];
        if(m_PolyIds[i] == 0)
        {
          char code = GeometryMath::IsPointInPolyhedron(m_Faces, m_FaceIds[iter], m_FaceBBs, point, boundingBox, radius);
          if(code == 'i' || code == 'V' || code == 'E' || code == 'F')
          {
            m_PolyIds[i] = static_cast<OutputT>(iter);
          }
        }
      }
    }
  }

  void operator()(const Range& range) const
  {
    checkPoints(range.min(), range.max());
  }

private:
  const TriangleGeom& m_Faces;
  const std::vector<std::vector<FaceLabelsT>>& m_FaceIds;
  const std::vector<BoundingBox3Df>& m_FaceBBs;
  const std::vector<Point3Df>& m_Points;
  AbstractDataStore<OutputT>& m_PolyIds;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_bool& m_OverflowHit;
};

// -----------------------------------------------------------------------------
template <typename OutputT, typename FaceLabelsT>
class SampleSurfaceMeshImplByPoints
{
public:
  SampleSurfaceMeshImplByPoints(SampleSurfaceMesh* filter, const TriangleGeom& faces, const std::vector<FaceLabelsT>& faceIds, const std::vector<BoundingBox3Df>& faceBBs, IDataArray& iPolyIds,
                                const std::vector<Point3Df>& points, const usize featureId, const std::atomic_bool& shouldCancel, std::atomic_bool& overflowHit)
  : m_Filter(filter)
  , m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_PolyIds(iPolyIds.getIDataStoreRefAs<AbstractDataStore<OutputT>>())
  , m_FeatureId(featureId)
  , m_ShouldCancel(shouldCancel)
  , m_OverflowHit(overflowHit)
  {
  }
  virtual ~SampleSurfaceMeshImplByPoints() = default;

  void checkPoints(const usize start, const usize end) const
  {

    if constexpr(std::numeric_limits<FaceLabelsT>::max() > std::numeric_limits<OutputT>::max())
    {
      if(std::numeric_limits<OutputT>::max() < m_FeatureId)
      {
        m_OverflowHit = true;
      }
    }

    const OutputT iter = m_FeatureId;

    // find bounding box for current feature
    BoundingBox3Df boundingBox(GeometryMath::FindBoundingBoxOfFaces(m_Faces, m_FaceIds));
    float32 radius = GeometryMath::FindDistanceBetweenPoints(boundingBox.getMinPoint(), boundingBox.getMaxPoint()) / 2;

    usize pointsVisited = 0;
    // check points in vertex array to see if they are in the bounding box of the feature
    for(usize i = start; i < end; i++)
    {
      Point3Df point = m_Points[i];
      if(m_PolyIds[i] == 0)
      {
        char code = GeometryMath::IsPointInPolyhedron(m_Faces, m_FaceIds, m_FaceBBs, point, boundingBox, radius);
        if(code == 'i' || code == 'V' || code == 'E' || code == 'F')
        {
          m_PolyIds[i] = iter;
        }
      }
      pointsVisited++;

      // Send some feedback
      if(pointsVisited % 1000 == 0)
      {
        // The counter is shared across every feature, so the totals were already aggregate; only
        // the per-worker feature id is dropped.
        m_Filter->sendThreadSafeProgressMessage(1000);
      }
      // Check for the filter being canceled.
      if(m_ShouldCancel || m_OverflowHit)
      {
        return;
      }
    }
  }

  void operator()(const Range& range) const
  {
    checkPoints(range.min(), range.max());
  }

private:
  SampleSurfaceMesh* m_Filter = nullptr;
  const TriangleGeom& m_Faces;
  const std::vector<FaceLabelsT>& m_FaceIds;
  const std::vector<BoundingBox3Df>& m_FaceBBs;
  const std::vector<Point3Df>& m_Points;
  AbstractDataStore<OutputT>& m_PolyIds;
  const usize m_FeatureId = 0;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_bool& m_OverflowHit;
};

template <template <typename, typename> class ParallelClassT, typename FaceLabelsT>
struct GenerateParallelClassFunctor
{
  template <typename OutputT, typename... ArgsT>
  auto operator()(ArgsT&&... args)
  {
    return ParallelClassT<OutputT, FaceLabelsT>(std::forward<ArgsT>(args)...);
  }
};

struct SampleSurfaceMeshFunctor
{
  template <typename T>
  Result<> operator()(SampleSurfaceMesh* algorithm, const TriangleGeom& triangleGeom, const IDataArray& iFaceLabels, IDataArray& polyIds, const std::atomic_bool& shouldCancel,
                      const IFilter::MessageHandler& messageHandler)
  {
    const AbstractDataStore<T>& faceLabelsSM = dynamic_cast<const DataArray<T>&>(iFaceLabels).getDataStoreRef();
    // pull down faces
    const usize numFaces = faceLabelsSM.getNumberOfTuples();

    messageHandler.sendInfoMessage("Counting number of Features...");

    // walk through faces to see how many features there are
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

    // Check for user canceled flag.
    if(shouldCancel)
    {
      return {};
    }

    // add one to account for feature 0
    usize numFeatures = maxFeatureId + 1;

    std::vector<std::vector<T>> faceLists(numFeatures);
    messageHandler.sendInfoMessage("Counting number of triangle faces per feature ...");

    // traverse data to determine number of faces belonging to each feature
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

    // Check for user canceled flag.
    if(shouldCancel)
    {
      return {};
    }

    messageHandler.sendInfoMessage("Allocating triangle faces per feature ...");

    // fill out lists with number of references to cells
    std::vector<int32> linkLoc(numFaces, 0);

    std::vector<BoundingBox3Df> faceBBs;
    {
      // !!! DO NOT USE GeometryStoreCache ELSEWHERE, SPECIAL CASE !!!
      const GeometryMath::detail::GeometryStoreCache cache(triangleGeom.getVertices()->getDataStoreRef(), triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getNumberOfVerticesPerFace());

      // initialize temp storage 'verts' vector to avoid expensive
      // calls during tight loops below
      std::vector<usize> verts(cache.NumVertsPerFace);

      // traverse data again to get the faces belonging to each feature
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
        // find bounding box for each face
        faceBBs.emplace_back(GeometryMath::FindBoundingBoxOfFace(cache, triangleGeom, i, verts));
      }
    }

    // Check for user canceled flag.
    if(shouldCancel)
    {
      return {};
    }

    messageHandler.sendInfoMessage("Vertex Geometry generating sampling points");

    // generate the list of sampling points from subclass
    std::vector<Point3Df> points = {};
    algorithm->generatePoints(points);

    messageHandler.sendInfoMessage("Sampling triangle geometry ...");

    algorithm->resetProgress(points.size(), "Sampling triangle geometry");

    std::atomic_bool overflowHit(false);

    // C++11 RIGHT HERE....
    auto nthreads = static_cast<int32>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
    // If the number of features is larger than the number of cores to do the work then parallelize over the number of features
    // otherwise parallelize over the number of triangle points.
    if(numFeatures > nthreads)
    {
      using PFunctT = GenerateParallelClassFunctor<::SampleSurfaceMeshImpl, T>;
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, numFeatures);
      ExecuteParallelFunctor<PFunctT, ArrayUseIntegerTypes>(PFunctT{}, polyIds.getDataType(), dataAlg, triangleGeom, faceLists, faceBBs, points, polyIds, shouldCancel, overflowHit);
    }
    else
    {
      using PFunctT = GenerateParallelClassFunctor<::SampleSurfaceMeshImplByPoints, T>;
      for(int32 featureId = 0; featureId < numFeatures; featureId++)
      {
        ParallelDataAlgorithm dataAlg;
        dataAlg.setRange(0, points.size());
        ExecuteParallelFunctor<PFunctT, ArrayUseIntegerTypes>(PFunctT{}, polyIds.getDataType(), dataAlg, algorithm, triangleGeom, faceLists[featureId], faceBBs, polyIds, points, featureId,
                                                              shouldCancel, overflowHit);
      }
    }

    if(overflowHit)
    {
      return MakeErrorResult(-158630,
                             fmt::format("Overflow occurred when downcasting a Face Label value of type {} to a feature Id value of type {}. Feature count of {} is greater than max value ({})",
                                         DataTypeToHumanString(GetDataType<T>()), DataTypeToHumanString(polyIds.getDataType()), maxFeatureId, DataTypeToHumanString(polyIds.getDataType())));
    }

    messageHandler.sendInfoMessage("Complete");

    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
SampleSurfaceMesh::SampleSurfaceMesh(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SampleSurfaceMesh::~SampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
const IFilter::MessageHandler& SampleSurfaceMesh::getMessageHandler() const
{
  return m_MessageHandler;
}

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::resetProgress(usize maxProgress, std::string label)
{
  m_Throttle.reset(maxProgress, std::move(label));
}

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementCount(counter);
}

// -----------------------------------------------------------------------------
Result<> SampleSurfaceMesh::execute(SampleSurfaceMeshInputValues& inputValues)
{
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(inputValues.TriangleGeometryPath);
  const auto& iFaceLabels = m_DataStructure.getDataRefAs<IDataArray>(inputValues.SurfaceMeshFaceLabelsArrayPath);

  // create array to hold which polyhedron (feature) each point falls in
  auto& polyIds = m_DataStructure.getDataRefAs<IDataArray>(inputValues.FeatureIdsArrayPath);

  // Face labels are always an integer type (the parameter is restricted to GetIntegerDataTypes()), so dispatch only
  // over the integer types.
  return ExecuteDataFunctionIntType(SampleSurfaceMeshFunctor{}, iFaceLabels.getDataType(), this, triangleGeom, iFaceLabels, polyIds, m_ShouldCancel, m_MessageHandler);
}
