#include "SampleSurfaceMesh.hpp"

#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <chrono>

using namespace complex;

namespace
{
class SampleSurfaceMeshImplByPoints
{
  SampleSurfaceMesh* m_Filter = nullptr;
  TriangleGeom::Pointer m_Faces;
  Int32Int32DynamicListArray::Pointer m_FaceIds;
  VertexGeom::Pointer m_FaceBBs;
  VertexGeom::Pointer m_Points;
  size_t m_FeatureId = 0;
  int32_t* m_PolyIds = nullptr;

public:
  SampleSurfaceMeshImplByPoints(SampleSurfaceMesh* filter, TriangleGeom::Pointer faces, Int32Int32DynamicListArray::Pointer faceIds, VertexGeom::Pointer faceBBs, VertexGeom::Pointer points,
                                size_t featureId, int32_t* polyIds)
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

  void checkPoints(size_t start, size_t end) const
  {
    float radius = 0.0f;
    float distToBoundary = 0.0f;
    int64_t numPoints = m_Points->getNumberOfVertices();
    std::array<float, 3> lowerLeft = {0.0F, 0.0F, 0.0F};
    std::array<float, 3> upperRight = {0.0F, 0.0F, 0.0F};
    float* point = nullptr;
    char code = ' ';

    size_t iter = m_FeatureId;

    // find bounding box for current feature
    GeometryMath::FindBoundingBoxOfFaces(m_Faces.get(), m_FaceIds->getElementList(iter), lowerLeft.data(), upperRight.data());
    GeometryMath::FindDistanceBetweenPoints(lowerLeft.data(), upperRight.data(), radius);
    int64_t pointsVisited = 0;
    // check points in vertex array to see if they are in the bounding box of the feature
    for(int64_t i = static_cast<int64_t>(start); i < static_cast<int64_t>(end); i++)
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
      if(m_Filter->getCancel())
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
  float radius = 0.0f;
  float distToBoundary = 0.0f;
  int64_t numPoints = m_Points->getNumberOfVertices();
  std::array<float, 3> lowerLeft = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> upperRight = {0.0F, 0.0F, 0.0F};
  float* point = nullptr;
  char code = ' ';
};

// -----------------------------------------------------------------------------
class SampleSurfaceMeshImpl
{
public:
  SampleSurfaceMeshImpl() = delete;
  SampleSurfaceMeshImpl(const SampleSurfaceMeshImpl&) = default;     // Copy Constructor Default Implemented
  SampleSurfaceMeshImpl(SampleSurfaceMeshImpl&&) noexcept = default; // Move Constructor Default Implemented

  SampleSurfaceMeshImpl(SampleSurfaceMesh* filter, TriangleGeom::Pointer faces, Int32Int32DynamicListArray::Pointer faceIds, VertexGeom::Pointer faceBBs, VertexGeom::Pointer points, int32_t* polyIds)
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
    float radius = 0.0f;
    float distToBoundary = 0.0f;
    int64_t numPoints = m_Points->getNumberOfVertices();
    std::array<float, 3> lowerLeft = {0.0F, 0.0F, 0.0F};
    std::array<float, 3> upperRight = {0.0F, 0.0F, 0.0F};
    float* point = nullptr;
    char code = ' ';

    for(size_t iter = start; iter < end; iter++)
    {
      // find bounding box for current feature
      GeometryMath::FindBoundingBoxOfFaces(m_Faces.get(), m_FaceIds->getElementList(iter), lowerLeft.data(), upperRight.data());
      GeometryMath::FindDistanceBetweenPoints(lowerLeft.data(), upperRight.data(), radius);

      // check points in vertex array to see if they are in the bounding box of the feature
      for(int64_t i = 0; i < numPoints; i++)
      {
        // Check for the filter being cancelled.
        if(m_Filter->getCancel())
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
  TriangleGeom::Pointer m_Faces;
  Int32Int32DynamicListArray::Pointer m_FaceIds;
  VertexGeom::Pointer m_FaceBBs;
  VertexGeom::Pointer m_Points;
  int32_t* m_PolyIds = nullptr;
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
const std::atomic_bool& SampleSurfaceMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> SampleSurfaceMesh::execute(const SizeVec3& udims)
{
  DataContainer::Pointer sm = getDataContainerArray()->getDataContainer(m_SurfaceMeshFaceLabelsArrayPath.getDataContainerName());
  SIMPL_RANDOMNG_NEW()

  TriangleGeom::Pointer triangleGeom = sm->getGeometryAs<TriangleGeom>();

  // pull down faces
  int64_t numFaces = m_SurfaceMeshFaceLabelsPtr.lock()->getNumberOfTuples();

  // create array to hold bounding vertices for each face
  FloatArrayType::Pointer llPtr = FloatArrayType::CreateArray(3, std::string("_INTERNAL_USE_ONLY_Lower_Left"), true);
  FloatArrayType::Pointer urPtr = FloatArrayType::CreateArray(3, std::string("_INTERNAL_USE_ONLY_Upper_Right"), true);
  float* ll = llPtr->getPointer(0);
  float* ur = urPtr->getPointer(0);
  VertexGeom::Pointer faceBBs = VertexGeom::CreateGeometry(2 * numFaces, "_INTERNAL_USE_ONLY_faceBBs");

  notifyStatusMessage("Counting number of Features...");

  // walk through faces to see how many features there are
  int32_t g1 = 0, g2 = 0;
  int32_t maxFeatureId = 0;
  for(int64_t i = 0; i < numFaces; i++)
  {
    g1 = m_SurfaceMeshFaceLabels[2 * i];
    g2 = m_SurfaceMeshFaceLabels[2 * i + 1];
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
  if(getCancel())
  {
    return;
  }

  // add one to account for feature 0
  int32_t numFeatures = maxFeatureId + 1;

  // create a dynamic list array to hold face lists
  Int32Int32DynamicListArray::Pointer faceLists = Int32Int32DynamicListArray::New();
  std::vector<int32_t> linkCount(numFeatures, 0);

  // fill out lists with number of references to cells
  Int32ArrayType::Pointer linkLocPtr = Int32ArrayType::CreateArray(numFaces, std::string("_INTERNAL_USE_ONLY_cell refs"), true);
  linkLocPtr->initializeWithZeros();
  int32_t* linkLoc = linkLocPtr->getPointer(0);

  notifyStatusMessage("Counting number of triangle faces per feature ...");

  // traverse data to determine number of faces belonging to each feature
  for(int64_t i = 0; i < numFaces; i++)
  {
    g1 = m_SurfaceMeshFaceLabels[2 * i];
    g2 = m_SurfaceMeshFaceLabels[2 * i + 1];
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
  if(getCancel())
  {
    return;
  }

  // now allocate storage for the faces
  faceLists->allocateLists(linkCount);

  notifyStatusMessage("Allocating triangle faces per feature ...");

  // traverse data again to get the faces belonging to each feature
  for(int64_t i = 0; i < numFaces; i++)
  {
    g1 = m_SurfaceMeshFaceLabels[2 * i];
    g2 = m_SurfaceMeshFaceLabels[2 * i + 1];
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
  if(getCancel())
  {
    return;
  }

  notifyStatusMessage("Vertex Geometry generating sampling points");

  // generate the list of sampling points from subclass
  VertexGeom::Pointer points = generate_points();
  if(getErrorCode() < 0 || nullptr == points.get())
  {
    return;
  }
  int64_t numPoints = points->getNumberOfVertices();

  // create array to hold which polyhedron (feature) each point falls in
  Int32ArrayType::Pointer iArray = Int32ArrayType::NullPointer();
  iArray = Int32ArrayType::CreateArray(numPoints, std::string("_INTERNAL_USE_ONLY_polyhedronIds"), true);
  iArray->initializeWithZeros();
  int32_t* polyIds = iArray->getPointer(0);

  notifyStatusMessage("Sampling triangle geometry ...");

  // C++11 RIGHT HERE....
  int32_t nthreads = static_cast<int32_t>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  // If the number of features is larger than the number of cores to do the work then parallelize over the number of features
  // otherwise parallelize over the number of triangle points.
  if(numFeatures > nthreads)
  {
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numFeatures), SampleSurfaceMeshImpl(this, triangleGeom, faceLists, faceBBs, points, polyIds), tbb::auto_partitioner());
#else
    SampleSurfaceMeshImpl serial(this, triangleGeom, faceLists, faceBBs, points, polyIds);
    serial.checkPoints(0, numFeatures);
#endif
  }
  else
  {
    for(int featureId = 0; featureId < numFeatures; featureId++)
    {
      m_NumCompleted = 0;
      m_StartMillis = QDateTime::currentMSecsSinceEpoch();
      m_Millis = m_StartMillis;
      size_t numPoints = points->getNumberOfVertices();

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
      tbb::parallel_for(tbb::blocked_range<size_t>(0, numPoints), SampleSurfaceMeshImplByPoints(this, triangleGeom, faceLists, faceBBs, points, featureId, polyIds), tbb::auto_partitioner());

#else
      SampleSurfaceMeshImplByPoints serial(this, triangleGeom, faceLists, faceBBs, points, featureId, polyIds);
      serial.checkPoints(0, numPoints);
#endif
    }
  }
  assign_points(iArray);

  notifyStatusMessage("Complete");

  return {};
}

// -----------------------------------------------------------------------------
void SampleSurfaceMesh::sendThreadSafeProgressMessage(int featureId, size_t numCompleted, size_t totalFeatures)
{
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  qint64 currentMillis = QDateTime::currentMSecsSinceEpoch();
  m_NumCompleted = m_NumCompleted + numCompleted;
  if(currentMillis - m_Millis > 1000)
  {
    float inverseRate = static_cast<float>(currentMillis - m_Millis) / static_cast<float>(m_NumCompleted - m_LastCompletedPoints);
    qint64 remainMillis = inverseRate * (totalFeatures - m_NumCompleted);
    QString ss = QObject::tr("Feature %3 | Points Completed: %1 of %2").arg(m_NumCompleted).arg(totalFeatures).arg(featureId);
    ss = ss + QObject::tr(" || Est. Time Remain: %1").arg(DREAM3D::convertMillisToHrsMinSecs(remainMillis));
    notifyStatusMessage(ss);
    m_Millis = QDateTime::currentMSecsSinceEpoch();
    m_LastCompletedPoints = m_NumCompleted;
  }
}
