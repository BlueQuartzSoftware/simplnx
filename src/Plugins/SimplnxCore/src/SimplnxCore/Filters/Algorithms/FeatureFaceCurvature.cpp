#include "FeatureFaceCurvature.hpp"

#include "SimplnxCore/Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp"
#include "SimplnxCore/Filters/Algorithms/FindNRingNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>

using namespace nx::core;

using FaceIds_t = std::vector<int64_t>;
using SharedFeatureFaces_t = std::map<int32_t, FaceIds_t>;

namespace
{
// Finalize explicitly after the producer or after all workers join. Removal can throw.
void CleanupCreatedIncidence(DataStructure& dataStructure, TriangleGeom& geometry, const DataPath& geometryPath, const std::optional<DataObject::IdType>& entryId, bool childExisted)
{
  if(!childExisted)
  {
    // A failed producer can leave this new child after clearing its association.
    const auto* child = dataStructure.getData(geometryPath.createChildPath(TriangleGeom::k_EltsContainingVert));
    if(child != nullptr)
    {
      const auto createdId = child->getId();
      if(!dataStructure.removeData(createdId))
      {
        throw std::runtime_error(fmt::format("Cannot remove temporary support object {} from geometry '{}'.", createdId, geometryPath.toString()));
      }
    }
  }
  geometry.setElementContainingVertId(entryId);
}
} // namespace

// -----------------------------------------------------------------------------
FeatureFaceCurvature::FeatureFaceCurvature(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureFaceCurvatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
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
void FeatureFaceCurvature::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementCount(counter);
}

// -----------------------------------------------------------------------------
void FeatureFaceCurvature::sendThreadSafeStatusMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_MessageHandler.sendInfoMessage(message);
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

  // FindNRingNeighbors requires vertex incidence before its workers start.
  const auto entryIncidenceId = triangleGeomPtr->getElementContainingVertId();
  const bool incidenceChildExisted = m_DataStructure.getData(m_InputValues->triangleGeomPath.createChildPath(TriangleGeom::k_EltsContainingVert)) != nullptr;
  const auto incidenceResult = triangleGeomPtr->findElementsContainingVert(false);
  if(incidenceResult.invalid())
  {
    CleanupCreatedIncidence(m_DataStructure, *triangleGeomPtr, m_InputValues->triangleGeomPath, entryIncidenceId, incidenceChildExisted);
    return incidenceResult;
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

  const auto nonEmptyCount = static_cast<usize>(std::count_if(sharedFeatureFaces.begin(), sharedFeatureFaces.end(), [](const auto& featureFace) { return !featureFace.second.empty(); }));

  ParallelTaskAlgorithm taskAlgorithm;
  taskAlgorithm.requireArraysInMemory({surfaceMeshPrincipalCurvature1sArrayPtr, surfaceMeshPrincipalCurvature2sArrayPtr, surfaceMeshPrincipalDirection1sArrayPtr,
                                       surfaceMeshPrincipalDirection2sArrayPtr, surfaceMeshMeanCurvaturesArrayPtr, surfaceMeshGaussianCurvaturesArrayPtr, surfaceMeshWeingartenMatrixArrayPtr,
                                       surfaceMeshFaceLabelsArrayPtr, surfaceMeshFaceNormalsArrayPtr, surfaceMeshTriangleCentroidsArrayPtr, surfaceMeshFeatureFaceIdsArrayPtr,
                                       triangleGeomPtr->getVertices(), triangleGeomPtr->getFaces()});

#ifdef SIMPLNX_ENABLE_MULTICORE
  // Resident arrays use a raw TBB task_group instead of ParallelTaskAlgorithm. All feature faces are queued at once,
  // and TBB starts the next task as soon as a thread is free. ParallelTaskAlgorithm runs at most one batch of tasks per
  // core at a time, which was about 3x slower for this many small tasks. Shared stores that are not resident must be
  // accessed by one worker at a time, so then the workers run inline and no additional task_group is created.
  std::unique_ptr<tbb::task_group> g;
  if(taskAlgorithm.getParallelizationEnabled())
  {
    g = std::make_unique<tbb::task_group>();
    sendThreadSafeStatusMessage(fmt::format("Adding {} Feature Faces to the work queue....", nonEmptyCount));
  }
#endif

  m_Throttle.reset(nonEmptyCount, "Calculating Curvatures");

  for(auto& sharedFeatureFace : sharedFeatureFaces)
  {
    FaceIds_t& triangleIds = sharedFeatureFace.second;
    if(triangleIds.empty())
    {
      continue;
    }

    CalculateTriangleGroupCurvatures func(this, m_InputValues->NRingCount, triangleIds, m_InputValues->useNormalsForCurveFitting, surfaceMeshPrincipalCurvature1sArrayPtr,
                                          surfaceMeshPrincipalCurvature2sArrayPtr, surfaceMeshPrincipalDirection1sArrayPtr, surfaceMeshPrincipalDirection2sArrayPtr,
                                          surfaceMeshGaussianCurvaturesArrayPtr, surfaceMeshMeanCurvaturesArrayPtr, surfaceMeshWeingartenMatrixArrayPtr, triangleGeomPtr, surfaceMeshFaceLabelsArrayPtr,
                                          surfaceMeshFaceNormalsArrayPtr, surfaceMeshTriangleCentroidsArrayPtr, m_MessageHandler, m_ShouldCancel);

#ifdef SIMPLNX_ENABLE_MULTICORE
    if(g != nullptr)
    {
      g->run(func);
    }
    else
#endif
    {
      func();
    }
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  if(g != nullptr)
  {
    sendThreadSafeStatusMessage("Waiting on computations to complete.");
    g->wait();
  }
#endif

  // All workers have joined, so the throttle can send its final value before the cleanup step.
  m_Throttle.flush();

  CleanupCreatedIncidence(m_DataStructure, *triangleGeomPtr, m_InputValues->triangleGeomPath, entryIncidenceId, incidenceChildExisted);
  return {};
}
