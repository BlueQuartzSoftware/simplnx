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
  auto* surfaceMeshPrincipalCurvature1sArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalCurvature1sPath);
  auto* surfaceMeshPrincipalCurvature2sArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalCurvature2sPath);
  auto* surfaceMeshPrincipalDirection1sArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalDirection1sPath);
  auto* surfaceMeshPrincipalDirection2sArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshPrincipalDirection2sPath);
  auto* surfaceMeshMeanCurvaturesArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshMeanCurvaturesPath);
  auto* surfaceMeshGaussianCurvaturesArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshGaussianCurvaturesPath);
  auto* surfaceMeshFaceLabelsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->surfaceMeshFaceLabelsPath);
  auto* surfaceMeshFaceNormalsArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshFaceNormalsPath);
  auto* surfaceMeshTriangleCentroidsArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshTriangleCentroidsPath);
  auto* surfaceMeshWeingartenMatrixArrayPtr = m_DataStructure.getDataAs<Float64Array>(m_InputValues->surfaceMeshWeingartenMatrixPath);

  auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->triangleGeomPath);
  auto* surfaceMeshFeatureFaceIdsArrayPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->surfaceMeshFeatureFaceIdsPath);
  auto& surfaceMeshFeatureFaceIds = surfaceMeshFeatureFaceIdsArrayPtr->getDataStoreRef();

  // Just to double-check we have everything.
  auto numTriangles = static_cast<int64>(triangleGeomPtr->getNumberOfFaces());

  // Make sure the Face Connectivity is created because the FindNRing algorithm needs this and will
  // assert if the data is NOT in the SurfaceMesh Data Container
  const IGeometry::ElementDynamicList* vertLinks = triangleGeomPtr->getElementsContainingVert();
  if(nullptr == vertLinks)
  {
    triangleGeomPtr->findElementsContainingVert();
  }

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

  SharedFeatureFaces_t sharedFeatureFaces;
  // Allocate all the vectors that we need
  for(size_t iter = 0; iter < faceSizes.size(); ++iter)
  {
    FaceIds_t v;
    v.reserve(faceSizes[iter]);
    sharedFeatureFaces[static_cast<int32>(iter)] = v;
  }

  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    sharedFeatureFaces[surfaceMeshFeatureFaceIds[t]].push_back(t);
  }

/*********************************
 * We are going to specifically invoke TBB directly instead of using ParallelTaskAlgorithm since we can just queue up all
 * the tasks while the first tasks start up. TBB will then grab a new task from it's own queue to work on it up to the
 * number of hardware limit of the machine. If we use ParallelTaskAlgorithm then at most we can only work on the number
 * of tasks that equal the number of cores on the machine. We spend a lot of overhead spinning up new threads to do each
 * task. This takes much longer (3x longer) in some cases.
 */
#ifdef SIMPLNX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  m_MessageHandler(fmt::format("Adding {} Feature Faces to the work queue....", maxFaceId));
#else

#endif
  m_TotalElements = sharedFeatureFaces.size();

  for(auto& sharedFeatureFace : sharedFeatureFaces)
  {
    FaceIds_t& triangleIds = sharedFeatureFace.second;

    CalculateTriangleGroupCurvatures func(this, m_InputValues->NRingCount, triangleIds, m_InputValues->useNormalsForCurveFitting, surfaceMeshPrincipalCurvature1sArrayPtr,
                                          surfaceMeshPrincipalCurvature2sArrayPtr, surfaceMeshPrincipalDirection1sArrayPtr, surfaceMeshPrincipalDirection2sArrayPtr,
                                          surfaceMeshGaussianCurvaturesArrayPtr, surfaceMeshMeanCurvaturesArrayPtr, surfaceMeshWeingartenMatrixArrayPtr, triangleGeomPtr, surfaceMeshFaceLabelsArrayPtr,
                                          surfaceMeshFaceNormalsArrayPtr, surfaceMeshTriangleCentroidsArrayPtr, m_MessageHandler, m_ShouldCancel);

#ifdef SIMPLNX_ENABLE_MULTICORE
    {
      g->run(func);
    }
#else
    m_MessageHandler(fmt::format("Working on Face Id {}/{}", std::to_string((sharedFeatureFace).first), std::to_string(maxFaceId)));
    {
      func();
    }
#endif
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  m_MessageHandler(fmt::format("Waiting on computations to complete."));
  g->wait(); // Wait for all the threads to complete before moving on.
#endif

  return {};
}

// -----------------------------------------------------------------------------
void FeatureFaceCurvature::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;
  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialPoint).count() < 1000)
  {
    return;
  }

  auto progressInt = static_cast<usize>((static_cast<float32>(m_ProgressCounter) / static_cast<float32>(m_TotalElements)) * 100.0f);
  std::string ss = fmt::format("{}/{}", m_ProgressCounter, m_TotalElements);
  m_MessageHandler(IFilter::Message::Type::Info, ss);

  m_LastProgressInt = progressInt;
  m_InitialPoint = std::chrono::steady_clock::now();
}
