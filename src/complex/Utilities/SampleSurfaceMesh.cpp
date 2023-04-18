#include "SampleSurfaceMesh.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"
#include "complex/Utilities/Math/GeometryMath.hpp"
#include "complex/Common/BoundingBox.hpp"
#include "complex/DataStructure/NeighborList.hpp"

#include <chrono>
#include <random>

using namespace complex;

namespace
{
class SampleSurfaceMeshImplByPoints
{
public:
  SampleSurfaceMeshImplByPoints(SampleSurfaceMesh* filter, const TriangleGeom& faces, const Int32NeighborList& faceIds, VertexGeom::Pointer faceBBs, VertexGeom::Pointer points, usize featureId,
                                int32_t* polyIds)
  : m_Filter(filter)
  , m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_FeatureId(featureId)
  , m_PolyIds(polyIds)
  {
  }
  virtual ~SampleSurfaceMeshImplByPoints() = default;

  void checkPoints(usize start, usize end) const
  {
    float distToBoundary = 0.0f;
    float* point = nullptr;
    usize iter = m_FeatureId;


    // find bounding box for current feature
    BoundingBox<float> tempBoundingBox = GeometryMath::FindBoundingBoxOfFace(m_Faces, m_FaceIds->getElementList(iter));

    float radius = GeometryMath::FindDistanceBetweenPoints(Point3D<float>(boundingBox.getMinPoint()), Point3D<float>(boundingBox.getMaxPoint()));
    usize pointsVisited = 0;
    // check points in vertex array to see if they are in the bounding box of the feature
    for(usize i = static_cast<usize>(start); i < static_cast<usize>(end); i++)
    {
      point = m_Points->getVertexPointer(i);
      if(m_PolyIds[i] == 0 && GeometryMath::PointInBox(point, lowerLeft.data(), upperRight.data()))
      {
        code = GeometryMath::PointInPolyhedron(m_Faces.get(), m_FaceIds->getElementList(iter), m_FaceBBs.get(), point, lowerLeft.data(), upperRight.data(), radius, distToBoundary);
        if(code == 'i' || code == 'V' || code == 'E' || code == 'F')
        {
          m_PolyIds[i] = iter;
        }
      }
      pointsVisited++;

      // Send some feedback
      if(pointsVisited % 1000 == 0)
      {
        m_Filter->sendThreadSafeProgressMessage(m_FeatureId, 1000, numPoints);
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
  const Int32NeighborList& m_FaceIds;
  VertexGeom::Pointer m_FaceBBs;
  VertexGeom::Pointer m_Points;
  Int32Array& m_PolyIds = nullptr;
  usize m_FeatureId = 0;
  const std::atomic_bool& m_ShouldCancel;
};

// -----------------------------------------------------------------------------
class SampleSurfaceMeshImpl
{
public:
  SampleSurfaceMeshImpl() = delete;
  SampleSurfaceMeshImpl(const SampleSurfaceMeshImpl&) = default;     // Copy Constructor Default Implemented
  SampleSurfaceMeshImpl(SampleSurfaceMeshImpl&&) noexcept = default; // Move Constructor Default Implemented

  SampleSurfaceMeshImpl(SampleSurfaceMesh* filter, TriangleGeom& faces, Int32Int32DynamicListArray::Pointer faceIds, VertexGeom::Pointer faceBBs, VertexGeom::Pointer points, Int32Array& polyIds)
  : m_Filter(filter)
  , m_Faces(faces)
  , m_FaceIds(faceIds)
  , m_FaceBBs(faceBBs)
  , m_Points(points)
  , m_PolyIds(polyIds)
  {
  }

  ~SampleSurfaceMeshImpl() = default;

  SampleSurfaceMeshImpl& operator=(const SampleSurfaceMeshImpl&) = delete; // Copy Assignment Not Implemented
  SampleSurfaceMeshImpl& operator=(SampleSurfaceMeshImpl&&) = delete;      // Move Assignment Not Implemented

  void checkPoints(usize start, usize end) const
  {
    for(size_t iter = start; iter < end; iter++)
    {
      float radius = 0.0f;
      float distToBoundary = 0.0f;
      int64_t numPoints = m_Points->getNumberOfVertices();\
      std::pair<Point3D<float>, Point3D<float>> boundingBoxPoints; //structure Pair<lower left, upper right>
      std::array<float, 3> lowerLeft = {0.0F, 0.0F, 0.0F};
      std::array<float, 3> upperRight = {0.0F, 0.0F, 0.0F};
      float* point = nullptr;

      // find bounding box for current feature
      GeometryMath::FindBoundingBoxOfFaces(m_Faces.get(), m_FaceIds->getElementList(iter), lowerLeft.data(), upperRight.data());
      radius = GeometryMath::FindDistanceBetweenPoints(boundingBoxPoints.first, boundingBoxPoints.second);

      // check points in vertex array to see if they are in the bounding box of the feature
      for(int64_t i = 0; i < numPoints; i++)
      {
        // Check for the filter being cancelled.
        if(m_ShouldCancel)
        {
          return;
        }

        point = m_Points->getVertexPointer(i);
        if(m_PolyIds[i] == 0 && GeometryMath::PointInBox(point, lowerLeft.data(), upperRight.data()))
        {
          code = GeometryMath::PointInPolyhedron(m_Faces.get(), m_FaceIds->getElementList(iter), m_FaceBBs.get(), point, lowerLeft.data(), upperRight.data(), radius, distToBoundary);
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
  SampleSurfaceMesh* m_Filter = nullptr;
  TriangleGeom& m_Faces;
  Int32Int32DynamicListArray::Pointer m_FaceIds;
  VertexGeom::Pointer m_FaceBBs;
  VertexGeom::Pointer m_Points;
  Int32Array& m_PolyIds = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
SampleSurfaceMesh::SampleSurfaceMesh(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(data)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SampleSurfaceMesh::~SampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> SampleSurfaceMesh::execute(SampleSurfaceMeshInputValues* inputValues)
{
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(inputValues->TriangleGeometryPath);
  auto& faceLabelsSM = m_DataStructure.getDataRefAs<Int32Array>(inputValues->SurfaceMeshFaceLabelsArrayPath);

  // pull down faces
  usize numFaces = faceLabelsSM.getNumberOfTuples();

  updateProgress("Counting number of Features...");

  // walk through faces to see how many features there are
  int32_t g1 = 0, g2 = 0;
  int32_t maxFeatureId = 0;
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

  // create a dynamic list array to hold face lists
  Int32Int32DynamicListArray::Pointer faceLists = Int32Int32DynamicListArray::New();
  std::vector<int32> linkCount(numFeatures, 0);

  // fill out lists with number of references to cells
  Int32Array linkLoc = Int32Array::CreateArray(numFaces, std::string("_INTERNAL_USE_ONLY_cell refs"), true)->initializeWithZeros();

  updateProgress("Counting number of triangle faces per feature ...");

  // traverse data to determine number of faces belonging to each feature
  for(int64_t i = 0; i < numFaces; i++)
  {
    g1 = faceLabelsSM[2 * i];
    g2 = faceLabelsSM[2 * i + 1];
    if(g1 > 0)
    {
      linkCount[g1]++;
    }
    if(g2 > 0)
    {
      linkCount[g2]++;
    }
  }

  // Check for user canceled flag.
  if(m_ShouldCancel)
  {
    return;
  }

  // now allocate storage for the faces
  faceLists->allocateLists(linkCount);

  updateProgress("Allocating triangle faces per feature ...");

  // create array to hold bounding vertices for each face
  Float32Array llPtr = Float32Array::CreateArray(3, std::string("_INTERNAL_USE_ONLY_Lower_Left"), true);
  Float32Array urPtr = Float32Array::CreateArray(3, std::string("_INTERNAL_USE_ONLY_Upper_Right"), true);
  float* ll = llPtr->getPointer(0);
  float* ur = urPtr->getPointer(0);
  VertexGeom::Pointer faceBBs = VertexGeom::CreateGeometry(2 * numFaces, "_INTERNAL_USE_ONLY_faceBBs");

  // traverse data again to get the faces belonging to each feature
  for(int64_t i = 0; i < numFaces; i++)
  {
    g1 = faceLabelsSM[2 * i];
    g2 = faceLabelsSM[2 * i + 1];
    if(g1 > 0)
    {
      faceLists->insertCellReference(g1, (linkLoc[g1])++, i);
    }
    if(g2 > 0)
    {
      faceLists->insertCellReference(g2, (linkLoc[g2])++, i);
    }
    // find bounding box for each face
    GeometryMath::FindBoundingBoxOfFace(triangleGeom.get(), i, ll, ur);
    faceBBs->setCoords(2 * i, ll);
    faceBBs->setCoords(2 * i + 1, ur);
  }

  // Check for user canceled flag.
  if(m_ShouldCancel)
  {
    return {};
  }

  updateProgress("Vertex Geometry generating sampling points");

  // generate the list of sampling points from subclass
  VertexGeom points = generate_points();
  m_TotalElements = points.getNumberOfVertices();

  // create array to hold which polyhedron (feature) each point falls in
  Int32Array& polyIds = Int32Array::CreateArray(m_TotalElements, std::string("_INTERNAL_USE_ONLY_polyhedronIds"), true)->initializeWithZeros();

  updateProgress("Sampling triangle geometry ...");

  // C++11 RIGHT HERE....
  auto nthreads = static_cast<int32>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  // If the number of features is larger than the number of cores to do the work then parallelize over the number of features
  // otherwise parallelize over the number of triangle points.
  if(numFeatures > nthreads)
  {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numFeatures), SampleSurfaceMeshImpl(this, triangleGeom, faceLists, faceBBs, points, polyIds), tbb::auto_partitioner());
    SampleSurfaceMeshImpl serial(this, triangleGeom, faceLists, faceBBs, points, polyIds);
    serial.checkPoints(0, numFeatures);
  }
  else
  {
    for(int featureId = 0; featureId < numFeatures; featureId++)
    {
      tbb::parallel_for(tbb::blocked_range<size_t>(0, m_TotalElements), SampleSurfaceMeshImplByPoints(this, triangleGeom, faceLists, faceBBs, points, featureId, polyIds), tbb::auto_partitioner());
      SampleSurfaceMeshImplByPoints serial(this, triangleGeom, faceLists, faceBBs, points, featureId, polyIds);
      serial.checkPoints(0, m_TotalElements);
    }
  }

  updateProgress("Complete");

  return {};
}

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::sendThreadSafeProgressMessage(usize featureId, size_t numCompleted, size_t totalFeatures)
{
  std::lock_guard<std::mutex> lock(m_ProgressMessage_Mutex);

  m_ProgressCounter += numCompleted;
  auto now = std::chrono::steady_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(m_InitialTime - now).count();
  if(diff > 1000)
  {
    std::string progMessage = fmt::format("Feature {} | Points Completed: {} of {}", featureId, m_ProgressCounter, m_TotalElements);
    float inverseRate = static_cast<float>(diff) / static_cast<float>(m_ProgressCounter - m_LastProgressInt);
    auto remainMillis = std::chrono::milliseconds(static_cast<int64>(inverseRate * (m_TotalElements - m_ProgressCounter)));
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(remainMillis);
    remainMillis -= std::chrono::duration_cast<std::chrono::milliseconds>(secs);
    auto mins = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= std::chrono::duration_cast<std::chrono::seconds>(mins);
    auto hour = std::chrono::duration_cast<std::chrono::hours>(mins);
    mins -= std::chrono::duration_cast<std::chrono::minutes>(hour);
    progMessage += fmt::format(" || Est. Time Remain: {} hours {} minutes {} seconds", hour.count(), mins.count(), secs.count());
    m_MessageHandler({IFilter::Message::Type::Info, progMessage});
    m_InitialTime = std::chrono::steady_clock::now();
    m_LastProgressInt = m_ProgressCounter;
  }
}
