#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct M3CSurfaceMeshingInputValues
 * @brief Stores meshing paths and winding-repair options.
 */
struct SIMPLNXCORE_EXPORT M3CSurfaceMeshingInputValues
{
  bool RepairTriangleWinding;
  ChoicesParameter::ValueType BoundingBoxSkinMode;
  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths;
  DataPath TriangleGeometryPath;
  DataPath VertexGroupDataPath;
  DataPath NodeTypesDataPath;
  DataPath FaceGroupDataPath;
  DataPath FaceLabelsDataPath;
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;
};

/**
 * @class M3CSurfaceMeshing
 * @brief Multi-Material Marching Cubes surface meshing with resident and
 * bounded external-scratch implementations.
 *
 * This port retains the legacy DREAM3D M3CEntireVolume algorithm, contributed
 * by Dr. Sukbin Lee at CMU and based on Wu and Sullivan 2003. Resident inputs
 * use a sliding window. Disk-backed inputs use two passes with temporary record
 * stores for volume and mesh state.
 *
 * A local ghost layer and Feature Id 0 renumbering preserve legacy interfaces
 * without changing the input array.
 */
class SIMPLNXCORE_EXPORT M3CSurfaceMeshing
{
public:
  /**
   * @brief Defines indexes used by TriangleGeom mesh arrays.
   */
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Creates an M3C surface-meshing algorithm.
   * @param dataStructure Provides selected geometries and arrays.
   * @param inputValues Specifies validated meshing paths and options. The caller
   * must keep this object alive for the algorithm lifetime.
   * @param shouldCancel Stops later meshing phases when true.
   * @param mesgHandler Receives progress messages.
   */
  M3CSurfaceMeshing(DataStructure& dataStructure, M3CSurfaceMeshingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  /**
   * @brief Destroys the non-owning M3C algorithm.
   */
  ~M3CSurfaceMeshing() noexcept;

  M3CSurfaceMeshing(const M3CSurfaceMeshing&) = delete;
  M3CSurfaceMeshing(M3CSurfaceMeshing&&) noexcept = delete;
  M3CSurfaceMeshing& operator=(const M3CSurfaceMeshing&) = delete;
  M3CSurfaceMeshing& operator=(M3CSurfaceMeshing&&) noexcept = delete;

  /**
   * @brief Selects resident or external-scratch meshing.
   * @return Error from selected meshing, or success after cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const M3CSurfaceMeshingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Runs the serial whole-volume reference implementation.
   * @return Error during meshing, or success after cancellation.
   *
   * This validation path allocates per-site scratch for the complete volume.
   */
  Result<> runEntireVolume();

  /**
   * @brief Sweeps resident input with bounded Z-window square scratch.
   * @param parallel Selects serial legacy or parallel cube processing.
   * @return Error during meshing, or success after cancellation.
   *
   * Node types still span the complete volume.
   */
  Result<> runWindowed(bool parallel);

  /**
   * @brief Runs bounded external-scratch meshing for disk-backed targets.
   *
   * Temporary record stores hold volume and mesh state. Fixed pages limit RAM.
   * Genuine OOC execution fails when external scratch storage is unavailable.
   * @param dispatchTargets Provides residency target arrays.
   * @param usesOutOfCoreStore Indicates actual disk-backed dispatch.
   * @return Storage, topology, or output error, or success after cancellation.
   */
  Result<> runOutOfCore(const std::vector<const IArray*>& dispatchTargets, bool usesOutOfCoreStore);
};
} // namespace nx::core
