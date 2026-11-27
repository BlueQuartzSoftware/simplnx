#include "FeatureFaceCurvature.hpp"

#include "SimplnxCore/Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp"
#include "SimplnxCore/Filters/Algorithms/FindNRingNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <optional>
#include <stdexcept>

using namespace nx::core;

using FaceIdsTType = std::vector<int64_t>;
using SharedFeatureFacesTType = std::map<int32_t, FaceIdsTType>;

namespace
{
// Finalize explicitly after the producer or after all workers join. Removal can throw.
void CleanupCreatedIncidence(DataStructure& dataStructure, TriangleGeom& geometry, const DataPath& geometryPath, const std::optional<DataObject::IdType>& entryId, bool childExisted)
{
  if(!childExisted)
  {
    // A failed producer can leave this new child after clearing its association.
    const auto* childPtr = dataStructure.getData(geometryPath.createChildPath(TriangleGeom::k_EltsContainingVert));
    if(childPtr != nullptr)
    {
      const auto createdId = childPtr->getId();
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
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_Throttle.incrementCount(counter);
}

// -----------------------------------------------------------------------------
void FeatureFaceCurvature::sendThreadSafeStatusMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_MessageHandler.sendInfoMessage(message);
}

// -----------------------------------------------------------------------------
Result<> FeatureFaceCurvature::operator()()
{

  // Get DataObjects
  auto* surfaceMeshPrincipalCurvature1sArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_principal_curvature1s_path);
  auto* surfaceMeshPrincipalCurvature2sArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_principal_curvature2s_path);
  auto* surfaceMeshPrincipalDirection1sArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_principal_direction1s_path);
  auto* surfaceMeshPrincipalDirection2sArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_principal_direction2s_path);
  auto* surfaceMeshMeanCurvaturesArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_mean_curvatures_path);
  auto* surfaceMeshGaussianCurvaturesArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_gaussian_curvatures_path);
  auto* surfaceMeshFaceLabelsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->surface_mesh_face_labels_path);
  auto* surfaceMeshFaceNormalsArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_face_normals_path);
  auto* surfaceMeshTriangleCentroidsArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_triangle_centroids_path);
  auto* surfaceMeshWeingartenMatrixArrayPtr = m_DataStructure.getDataAs<Float64ArrayType>(m_InputValues->surface_mesh_weingarten_matrix_path);

  auto* triangleGeomPtr = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->triangle_geom_path);
  auto* surfaceMeshFeatureFaceIdsArrayPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->surface_mesh_feature_face_ids_path);
  auto& surfaceMeshFeatureFaceIds = surfaceMeshFeatureFaceIdsArrayPtr->getDataStoreRef();

  // Just to double-check we have everything.
  auto numTriangles = static_cast<int64>(triangleGeomPtr->getNumberOfFaces());

  // FindNRingNeighbors requires vertex incidence before its workers start.
  const auto entryIncidenceId = triangleGeomPtr->getElementContainingVertId();
  const bool incidenceChildExisted = m_DataStructure.getData(m_InputValues->triangle_geom_path.createChildPath(TriangleGeom::k_EltsContainingVert)) != nullptr;
  const auto incidenceResult = triangleGeomPtr->findElementsContainingVert(false);
  if(incidenceResult.invalid())
  {
    CleanupCreatedIncidence(m_DataStructure, *triangleGeomPtr, m_InputValues->triangle_geom_path, entryIncidenceId, incidenceChildExisted);
    return incidenceResult;
  }

  int32_t maxFaceId = 0;
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    maxFaceId = std::max<int>(surfaceMeshFeatureFaceIds[t], maxFaceId);
  }

  std::vector<int32_t> faceSizes(maxFaceId + 1, 0);
  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    faceSizes[surfaceMeshFeatureFaceIds[t]]++;
  }

  SharedFeatureFacesTType sharedFeatureFaces;
  // Allocate all the vectors that we need
  for(size_t iter = 0; iter < faceSizes.size(); ++iter)
  {
    FaceIdsTType v;
    v.reserve(faceSizes[iter]);
    sharedFeatureFaces[static_cast<int32>(iter)] = v;
  }

  // Loop through all the Triangles and assign each one to a unique Feature Face Id.
  for(int64_t t = 0; t < numTriangles; ++t)
  {
    sharedFeatureFaces[surfaceMeshFeatureFaceIds[t]].push_back(t);
  }

  const auto nonEmptyCount = static_cast<usize>(std::count_if(sharedFeatureFaces.begin(), sharedFeatureFaces.end(), [](const auto& featureFace) { return !featureFace.second.empty(); }));

/*********************************
 * We are going to specifically invoke TBB directly instead of using ParallelTaskAlgorithm since we can just queue up all
 * the tasks while the first tasks start up. TBB will then grab a new task from it's own queue to work on it up to the
 * number of hardware limit of the machine. If we use ParallelTaskAlgorithm then at most we can only work on the number
 * of tasks that equal the number of cores on the machine. We spend a lot of overhead spinning up new threads to do each
 * task. This takes much longer (3x longer) in some cases.
 */
#ifdef SIMPLNX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  sendThreadSafeStatusMessage(fmt::format("Adding {} Feature Faces to the work queue....", nonEmptyCount));
#endif

  m_Throttle.reset(nonEmptyCount, "Calculating Curvatures");

  for(auto& sharedFeatureFace : sharedFeatureFaces)
  {
    FaceIdsTType& triangleIds = sharedFeatureFace.second;
    if(triangleIds.empty())
    {
      continue;
    }

    CalculateTriangleGroupCurvatures func(this, m_InputValues->n_ring_count, triangleIds, m_InputValues->use_normals_for_curve_fitting, surfaceMeshPrincipalCurvature1sArrayPtr,
                                          surfaceMeshPrincipalCurvature2sArrayPtr, surfaceMeshPrincipalDirection1sArrayPtr, surfaceMeshPrincipalDirection2sArrayPtr,
                                          surfaceMeshGaussianCurvaturesArrayPtr, surfaceMeshMeanCurvaturesArrayPtr, surfaceMeshWeingartenMatrixArrayPtr, triangleGeomPtr, surfaceMeshFaceLabelsArrayPtr,
                                          surfaceMeshFaceNormalsArrayPtr, surfaceMeshTriangleCentroidsArrayPtr, m_MessageHandler, m_ShouldCancel);

#ifdef SIMPLNX_ENABLE_MULTICORE
    {
      g->run(func);
    }
#else
    {
      func();
    }
#endif
  }

#ifdef SIMPLNX_ENABLE_MULTICORE
  sendThreadSafeStatusMessage("Waiting on computations to complete.");
  g->wait(); // Wait for all the threads to complete before moving on.
#endif

  // All workers have joined, so the throttle can send its final value before the cleanup step.
  m_Throttle.flush();

  CleanupCreatedIncidence(m_DataStructure, *triangleGeomPtr, m_InputValues->triangle_geom_path, entryIncidenceId, incidenceChildExisted);
  return {};
}
