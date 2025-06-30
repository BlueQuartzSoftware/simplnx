#include "SampleSurfaceMesh.hpp"

#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <chrono>

using namespace nx::core;

namespace
{
class SampleSurfaceMeshImpl
{
public:
  SampleSurfaceMeshImpl(const TriangleGeom& faces, const std::vector<std::vector<int32>>& faceIds, const std::vector<BoundingBox3Df>& faceBBs, const std::vector<Point3Df>& points,
                        Int32AbstractDataStore& polyIds, const std::atomic_bool& shouldCancel)
  : m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_PolyIds(polyIds)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~SampleSurfaceMeshImpl() = default;

  SampleSurfaceMeshImpl(const SampleSurfaceMeshImpl&) = default;           // Copy Constructor Default Implemented
  SampleSurfaceMeshImpl(SampleSurfaceMeshImpl&&) noexcept = default;       // Move Constructor Default Implemented
  SampleSurfaceMeshImpl& operator=(const SampleSurfaceMeshImpl&) = delete; // Copy Assignment Not Implemented
  SampleSurfaceMeshImpl& operator=(SampleSurfaceMeshImpl&&) = delete;      // Move Assignment Not Implemented

  void checkPoints(usize start, usize end) const
  {
    for(usize iter = start; iter < end; iter++)
    {
      usize numPoints = m_Points.size();

      // find bounding box for current feature
      BoundingBox3Df boundingBox(GeometryMath::FindBoundingBoxOfFaces(m_Faces, m_FaceIds[iter]));
      float32 radius = GeometryMath::FindDistanceBetweenPoints(boundingBox.getMinPoint(), boundingBox.getMaxPoint()) / 2;

      // check points in vertex array to see if they are in the bounding box of the feature
      for(usize i = 0; i < numPoints; i++)
      {
        // Check for the filter being cancelled.
        if(m_ShouldCancel)
        {
          return;
        }

        Point3Df point = m_Points[i];
        if(m_PolyIds[i] == 0)
        {
          char code = GeometryMath::IsPointInPolyhedron(m_Faces, m_FaceIds[iter], m_FaceBBs, point, boundingBox, radius);
          if(code == 'i' || code == 'V' || code == 'E' || code == 'F')
          {
            m_PolyIds[i] = iter;
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
  const std::vector<std::vector<int32>>& m_FaceIds;
  const std::vector<BoundingBox3Df>& m_FaceBBs;
  const std::vector<Point3Df>& m_Points;
  Int32AbstractDataStore& m_PolyIds;
  const std::atomic_bool& m_ShouldCancel;
};

// -----------------------------------------------------------------------------
class SampleSurfaceMeshImplByPoints
{
public:
  SampleSurfaceMeshImplByPoints(SampleSurfaceMesh* filter, const TriangleGeom& faces, const std::vector<int32>& faceIds, const std::vector<BoundingBox3Df>& faceBBs,
                                const std::vector<Point3Df>& points, const usize featureId, Int32AbstractDataStore& polyIds, const std::atomic_bool& shouldCancel,
                                ProgressMessageHelper& progressMessageHelper)
  : m_Filter(filter)
  , m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_PolyIds(polyIds)
  , m_FeatureId(featureId)
  , m_ShouldCancel(shouldCancel)
  , m_ProgressMessageHelper(progressMessageHelper)
  {
  }
  virtual ~SampleSurfaceMeshImplByPoints() = default;

  void checkPoints(usize start, usize end) const
  {
    ProgressMessenger progressMessenger = m_ProgressMessageHelper.createProgressMessenger();

    usize iter = m_FeatureId;

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
        progressMessenger.sendProgressMessage(
            1000, [&](usize currentProgress, usize maxProgress) { return fmt::format("Feature {} | Points Completed: {} of {}", m_FeatureId, currentProgress, maxProgress); });
      }
      // Check for the filter being cancelled.
      if(m_ShouldCancel)
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
  const std::vector<int32>& m_FaceIds;
  const std::vector<BoundingBox3Df>& m_FaceBBs;
  const std::vector<Point3Df>& m_Points;
  Int32AbstractDataStore& m_PolyIds;
  const usize m_FeatureId = 0;
  const std::atomic_bool& m_ShouldCancel;
  ProgressMessageHelper& m_ProgressMessageHelper;
};
} // namespace

// -----------------------------------------------------------------------------
SampleSurfaceMesh::SampleSurfaceMesh(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_MessageHelper(m_MessageHandler)
{
}

// -----------------------------------------------------------------------------
SampleSurfaceMesh::~SampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
Result<> SampleSurfaceMesh::execute(SampleSurfaceMeshInputValues& inputValues)
{
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(inputValues.TriangleGeometryPath);
  auto& faceLabelsSM = m_DataStructure.getDataAs<Int32Array>(inputValues.SurfaceMeshFaceLabelsArrayPath)->getDataStoreRef();

  // pull down faces
  usize numFaces = faceLabelsSM.getNumberOfTuples();

  m_MessageHelper.sendMessage("Counting number of Features...");

  // walk through faces to see how many features there are
  int32 g1 = 0, g2 = 0;
  int32 maxFeatureId = 0;
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
  if(m_ShouldCancel)
  {
    return {};
  }

  // add one to account for feature 0
  usize numFeatures = maxFeatureId + 1;

  std::vector<std::vector<int32>> faceLists(numFeatures);
  m_MessageHelper.sendMessage("Counting number of triangle faces per feature ...");

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
  if(m_ShouldCancel)
  {
    return {};
  }

  m_MessageHelper.sendMessage("Allocating triangle faces per feature ...");

  // fill out lists with number of references to cells
  std::vector<int32> linkLoc(numFaces, 0);

  std::vector<BoundingBox3Df> faceBBs;
  {
    // !!! DO NOT USE GeometryStoreCache ELSEWHERE, SPECIAL CASE !!!
    GeometryMath::detail::GeometryStoreCache cache(triangleGeom.getVertices()->getDataStoreRef(), triangleGeom.getFaces()->getDataStoreRef(), triangleGeom.getNumberOfVerticesPerFace());

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
  if(m_ShouldCancel)
  {
    return {};
  }

  m_MessageHelper.sendMessage("Vertex Geometry generating sampling points");

  // generate the list of sampling points from subclass
  std::vector<Point3Df> points = {};
  generatePoints(points);

  // create array to hold which polyhedron (feature) each point falls in
  auto& polyIds = m_DataStructure.getDataAs<Int32Array>(inputValues.FeatureIdsArrayPath)->getDataStoreRef();

  m_MessageHelper.sendMessage("Sampling triangle geometry ...");

  ProgressMessageHelper progressMessageHelper = m_MessageHelper.createProgressMessageHelper();
  progressMessageHelper.setMaxProgresss(points.size());

  // C++11 RIGHT HERE....
  auto nthreads = static_cast<int32>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  // If the number of features is larger than the number of cores to do the work then parallelize over the number of features
  // otherwise parallelize over the number of triangle points.
  if(numFeatures > nthreads)
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numFeatures);
    dataAlg.execute(SampleSurfaceMeshImpl(triangleGeom, faceLists, faceBBs, points, polyIds, m_ShouldCancel));
  }
  else
  {
    for(int32 featureId = 0; featureId < numFeatures; featureId++)
    {
      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, points.size());
      dataAlg.execute(SampleSurfaceMeshImplByPoints(this, triangleGeom, faceLists[featureId], faceBBs, points, featureId, polyIds, m_ShouldCancel, progressMessageHelper));
    }
  }

  m_MessageHelper.sendMessage("Complete");

  return {};
}
