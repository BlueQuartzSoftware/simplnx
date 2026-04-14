#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeSurfaceFeaturesInputValues
 * @brief Holds all user-configurable parameters for the ComputeSurfaceFeatures algorithm.
 *
 * These values are extracted from the filter's parameter map and passed through
 * the dispatcher to whichever algorithm variant (Direct or Scanline) is selected.
 */
struct SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesInputValues
{
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath; ///< Path to the Feature-level AttributeMatrix that sizes the output array.
  ArraySelectionParameter::ValueType FeatureIdsPath;                       ///< Path to the cell-level Int32 FeatureIds array.
  GeometrySelectionParameter::ValueType InputImageGeometryPath;            ///< Path to the ImageGeom that defines grid dimensions and dimensionality.
  BoolParameter::ValueType MarkFeature0Neighbors;                          ///< When true, features adjacent to FeatureId==0 voxels are marked as surface features.
  DataObjectNameParameter::ValueType SurfaceFeaturesArrayName;             ///< Name for the output UInt8 surface-features array (created under the FeatureAttributeMatrix).
};

/**
 * @class ComputeSurfaceFeatures
 * @brief Dispatcher that selects between the in-core (Direct) and out-of-core (Scanline)
 * surface-feature identification algorithms at runtime.
 *
 * This class contains no algorithm logic. Its operator()() inspects the storage backing
 * of the FeatureIds array and calls
 * `DispatchAlgorithm<ComputeSurfaceFeaturesDirect, ComputeSurfaceFeaturesScanline>(...)`.
 *
 * **Algorithm overview**: A feature is considered a "surface feature" if any voxel
 * belonging to that feature satisfies one of these conditions:
 * 1. The voxel sits on the outer boundary of the image geometry (x/y/z min or max).
 * 2. The voxel has a face neighbor with FeatureId == 0 (when MarkFeature0Neighbors
 *    is enabled).
 *
 * The algorithm supports both 3D geometries (full 6-neighbor check) and 2D geometries
 * (one dimension is degenerate, reducing to a 4-neighbor check on the non-degenerate
 * plane).
 *
 * The output is a feature-level UInt8 array where 0 = interior, 1 = surface.
 *
 * **Dispatch rules** (see AlgorithmDispatch.hpp):
 * - If all input arrays are in-memory, the Direct variant is selected.
 * - If any input array uses OOC storage, the Scanline variant is selected.
 * - Test-override flags can force either path.
 *
 * @see ComputeSurfaceFeaturesDirect, ComputeSurfaceFeaturesScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeatures
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all arrays and geometries.
   * @param mesgHandler Handler for sending progress/info messages back to the UI.
   * @param shouldCancel Atomic flag checked periodically to support user cancellation.
   * @param inputValues User-configured parameters for the algorithm.
   */
  ComputeSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeatures() noexcept;

  ComputeSurfaceFeatures(const ComputeSurfaceFeatures&) = delete;
  ComputeSurfaceFeatures(ComputeSurfaceFeatures&&) noexcept = delete;
  ComputeSurfaceFeatures& operator=(const ComputeSurfaceFeatures&) = delete;
  ComputeSurfaceFeatures& operator=(ComputeSurfaceFeatures&&) noexcept = delete;

  /**
   * @brief Dispatches to the appropriate algorithm variant (Direct or Scanline)
   * based on whether the FeatureIds array uses out-of-core storage.
   * @return Result<> indicating success or any errors encountered.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                   ///< Reference to the DataStructure containing all data.
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr; ///< User-configured algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                           ///< Atomic flag for cooperative cancellation.
  const IFilter::MessageHandler& m_MessageHandler;                  ///< Handler for progress and informational messages.
};

} // namespace nx::core
