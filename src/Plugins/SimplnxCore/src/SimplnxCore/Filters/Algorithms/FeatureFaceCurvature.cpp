#include "FeatureFaceCurvature.hpp"

#include "SimplnxCore/Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp"
#include "SimplnxCore/Filters/Algorithms/FindNRingNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include "fmt/format.h"

using namespace nx::core;

using FaceIds_t = std::vector<int64_t>;
using SharedFeatureFaces_t = std::map<int32_t, FaceIds_t>;

// -----------------------------------------------------------------------------
FeatureFaceCurvature::FeatureFaceCurvature(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureFaceCurvatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FeatureFaceCurvature::~FeatureFaceCurvature() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FeatureFaceCurvature::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FeatureFaceCurvature::operator()()
{

  // Get DataObjects
  auto* surfaceMeshPrincipalCurvature1sArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalCurvature1sPath);
  auto* surfaceMeshPrincipalCurvature2sArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalCurvature2sPath);
  auto* surfaceMeshPrincipalDirection1sArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalDirection1sPath);
  auto* surfaceMeshPrincipalDirection2sArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalDirection2sPath);
  auto* surfaceMeshMeanCurvaturesArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshMeanCurvaturesPath);
  auto* surfaceMeshGaussianCurvaturesArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshGaussianCurvaturesPath);
  auto* surfaceMeshFaceLabelsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->surfaceMeshFaceLabelsPath);
  auto* surfaceMeshFaceNormalsArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshFaceNormalsPath);
  auto* surfaceMeshTriangleCentroidsArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshTriangleCentroidsPath);
  auto* surfaceMeshWeingartenMatrixArray = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshWeingartenMatrixPath);

  TriangleGeom* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->triangleGeomPath);
  Int32Array* surfaceMeshFeatureFaceIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->surfaceMeshFeatureFaceIdsPath);
  auto& surfaceMeshFeatureFaceIds = surfaceMeshFeatureFaceIdsArray->getDataStoreRef();

  // Algorithm
  int32 totalFeatureFaces = -1;
  int32 completedFeatureFaces = -1;

  // Just to double check we have everything.
  int64 numTriangles = triangleGeom->getNumberOfFaces();
  int32 totalTriangles = numTriangles;

  // Make sure the Face Connectivity is created because the FindNRing algorithm needs this and will
  // assert if the data is NOT in the SurfaceMesh Data Container
  const IGeometry::ElementDynamicList* vertLinks = triangleGeom->getElementsContainingVert();
  if(nullptr == vertLinks)
  {
    triangleGeom->findElementsContainingVert();
  }

  // get the QMap from the SharedFeatureFaces filter
  SharedFeatureFaces_t sharedFeatureFaces;

  int32_t maxFaceId = 0;
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    if(surfaceMeshFeatureFaceIds[t] > maxFaceId)
    {
      maxFaceId = surfaceMeshFeatureFaceIds[t];
    }
  }

  std::vector<int32_t> faceSizes(maxFaceId + 1, 0);
  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    faceSizes[surfaceMeshFeatureFaceIds[t]]++;
  }

  // Allocate all the vectors that we need
  for(size_t iter = 0; iter < faceSizes.size(); ++iter)
  {
    FaceIds_t v;
    v.reserve(faceSizes[iter]);
    sharedFeatureFaces[iter] = v;
  }

  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    sharedFeatureFaces[surfaceMeshFeatureFaceIds[t]].push_back(t);
  }

  totalFeatureFaces = sharedFeatureFaces.size();
  completedFeatureFaces = 0;
  totalTriangles = numTriangles;
  std::string ss;

/*********************************
 * We are going to specfically invoke TBB directly instead of using ParallelTaskAlgorithm since we can just queue up all
 * the tasks while the first tasks start up. TBB will then grab a new task from it's own queue to work on it up to the
 * number of hardware limit of the machine. If we use ParallelTaskAlgorithm then at most we can only work on the number
 * of tasks that equal the number of cores on the machine. We spend a lot of overhead spinning up new threads to do each
 * task. This takes much longer (3x longer) in some cases
 */
#ifdef SIMPLNX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  ss = fmt::format("Adding {} Feature Faces to the work queue....", maxFaceId);
  m_MessageHandler(ss);
#else

#endif

  // typedef here for conveneince
  typedef SharedFeatureFaces_t::iterator SharedFeatureFaceIterator_t;
  for(SharedFeatureFaceIterator_t iter = sharedFeatureFaces.begin(); iter != sharedFeatureFaces.end(); ++iter)
  {
    FaceIds_t& triangleIds = (*iter).second;

    CalculateTriangleGroupCurvatures func(m_InputValues->NRingCount, triangleIds, m_InputValues->useNormalsForCurveFitting, surfaceMeshPrincipalCurvature1sArray, surfaceMeshPrincipalCurvature2sArray,
                                          surfaceMeshPrincipalDirection1sArray, surfaceMeshPrincipalDirection2sArray, surfaceMeshGaussianCurvaturesArray, surfaceMeshMeanCurvaturesArray,
                                          surfaceMeshWeingartenMatrixArray, triangleGeom, surfaceMeshFaceLabelsArray, surfaceMeshFaceNormalsArray, surfaceMeshTriangleCentroidsArray, m_MessageHandler,
                                          m_ShouldCancel);

#ifdef SIMPLNX_ENABLE_MULTICORE
    {
      g->run(func);
    }
#else
    ss = fmt::format("Working on Face Id {}/{}", std::to_string((*iter).first), std::to_string(maxFaceId));
    messageHandler(ss);
    {
      func();
    }
#endif
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  ss = fmt::format("Waiting on computations to complete.");
  m_MessageHandler(ss);
  g->wait(); // Wait for all the threads to complete before moving on.
#endif

  return {};
}
