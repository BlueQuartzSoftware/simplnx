#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct SurfaceNetsInputValues
 * @brief Aggregates every user-facing parameter and internally-created
 * DataPath needed by the SurfaceNets algorithm family.
 */
struct SIMPLNXCORE_EXPORT SurfaceNetsInputValues
{
  bool ApplySmoothing;          ///< When true, run iterative relaxation smoothing on mesh vertices
  bool RepairTriangleWinding;   ///< When true, run winding repair on the output triangle mesh
  int32 SmoothingIterations;    ///< Number of smoothing iterations to perform
  float32 MaxDistanceFromVoxel; ///< Maximum distance a vertex can move from its voxel center during smoothing
  float32 RelaxationFactor;     ///< Blending factor for neighbor-averaging during smoothing (0..1)

  DataPath GridGeomDataPath;                                             ///< Path to the input ImageGeom
  DataPath FeatureIdsArrayPath;                                          ///< Path to the Int32 FeatureIds cell array
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;    ///< Cell arrays to transfer to the triangle face attribute matrix
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths; ///< Feature arrays to transfer to the triangle face attribute matrix
  DataPath TriangleGeometryPath;                                         ///< Path to the created TriangleGeom output
  DataPath VertexGroupDataPath;                                          ///< Path to the vertex attribute matrix
  DataPath NodeTypesDataPath;                                            ///< Path to the Int8 NodeTypes vertex array
  DataPath FaceGroupDataPath;                                            ///< Path to the face attribute matrix
  DataPath FaceLabelsDataPath;                                           ///< Path to the Int32 FaceLabels (2-component) face array
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;         ///< Paths to the created face arrays (transferred cell/feature data)
};

/**
 * @class SurfaceNets
 * @brief Dispatcher that selects between SurfaceNetsDirect (in-core) and
 * SurfaceNetsScanline (OOC) based on the storage type of input arrays.
 *
 * The Surface Nets algorithm generates a smoothed triangle mesh of feature
 * boundaries using the method from Frisken (2022). Unlike QuickSurfaceMesh
 * which places vertices at dual-grid corners, Surface Nets places vertices
 * at voxel centers where features change, then optionally relaxes positions
 * toward neighbor averages to produce smoother surfaces while preserving
 * sharp boundaries between materials.
 *
 * Dispatch is performed by DispatchAlgorithm: if the FeatureIds array is
 * backed by an in-memory DataStore, SurfaceNetsDirect is used (which
 * delegates to the MMSurfaceNet library). If it uses chunked OOC storage,
 * SurfaceNetsScanline is selected instead.
 *
 * @see SurfaceNetsDirect, SurfaceNetsScanline
 */
class SIMPLNXCORE_EXPORT SurfaceNets
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param mesgHandler Callback for progress and status messages
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   */
  SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues);
  ~SurfaceNets() noexcept;

  SurfaceNets(const SurfaceNets&) = delete;
  SurfaceNets(SurfaceNets&&) noexcept = delete;
  SurfaceNets& operator=(const SurfaceNets&) = delete;
  SurfaceNets& operator=(SurfaceNets&&) noexcept = delete;

  /**
   * @brief Dispatches to the appropriate in-core or OOC algorithm implementation.
   * @return Result<> indicating success or an error code from the selected algorithm
   */
  Result<> operator()();

  /**
   * @brief Returns a reference to the cancellation flag (used by MMSurfaceNet internals).
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                        ///< Reference to the active DataStructure
  const SurfaceNetsInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;       ///< Progress message callback
};

} // namespace nx::core
