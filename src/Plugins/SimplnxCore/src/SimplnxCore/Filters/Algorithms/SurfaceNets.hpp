#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct SurfaceNetsInputValues
 * @brief Stores smoothing, winding, transfer, geometry, and output selections.
 */
struct SIMPLNXCORE_EXPORT SurfaceNetsInputValues
{
  bool ApplySmoothing;        ///< When true, run iterative relaxation smoothing on mesh vertices
  bool RepairTriangleWinding; ///< When true, run winding repair on the output triangle mesh
  ChoicesParameter::ValueType BoundingBoxSkinMode;
  int32 SmoothingIterations;    ///< Number of smoothing iterations to perform
  float32 MaxDistanceFromVoxel; ///< Maximum distance a vertex can move from its voxel center during smoothing
  float32 RelaxationFactor;     ///< Blending factor for neighbor-averaging during smoothing (0..1)

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
 * @class SurfaceNets
 * @brief Creates a triangle mesh from labeled ImageGeom feature boundaries.
 *
 * The implementation follows the multimaterial Surface Nets method from
 * Frisken (2022). Padded exterior cells close the volume boundary. Optional
 * relaxation moves vertices toward face-neighbor averages and clamps each local
 * coordinate around its cell center. Face labels use ascending feature order;
 * exterior label zero becomes -1.
 *
 * Every participating input, transfer output, mesh store, NodeTypes, and
 * FaceLabels store drives dispatch. Direct uses a complete padded MMCellMap.
 * Scanline uses two Feature ID slices and padded-cell temporary records behind
 * a bounded page cache. Storage overrides can force either path.
 *
 * The triangle-area helper passes its cross-product output by value, so both
 * candidate areas remain zero. Quad call sites also provide zero positions.
 * Each oriented quad therefore uses its default diagonal. Both paths subtract
 * half of Y spacing from world Z. Anisotropic Y and Z spacing therefore shifts Z.
 */
class SIMPLNXCORE_EXPORT SurfaceNets
{
public:
  /**
   * @brief Initializes the Surface Nets dispatcher.
   * @param dataStructure Contains input and output objects.
   * @param mesgHandler Receives phase and winding messages.
   * @param shouldCancel Signals cancellation at implementation checkpoints.
   * @param inputValues Selects smoothing, winding, transfers, and paths.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues);
  /**
   * @brief Destroys the Surface Nets dispatcher.
   */
  ~SurfaceNets() noexcept;

  SurfaceNets(const SurfaceNets&) = delete;
  SurfaceNets(SurfaceNets&&) noexcept = delete;
  SurfaceNets& operator=(const SurfaceNets&) = delete;
  SurfaceNets& operator=(SurfaceNets&&) noexcept = delete;

  /**
   * @brief Builds the mesh with the storage-appropriate implementation.
   * @return Allocation, storage, transfer, geometry, or winding result.
   * @pre FeatureIdsArrayPath is scalar Int32 and matches the ImageGeom cells.
   * @pre CreatedDataArrayPaths lists cell outputs, then feature outputs, in selection order.
   *
   * Cancellation returns success in most phases and does not roll back resized
   * or written mesh arrays. Different output arrays can contain different
   * completed ranges after cancellation or error.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
