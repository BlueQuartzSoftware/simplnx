#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesDirect
 * @brief In-core (direct memory access) algorithm for identifying surface features.
 *
 * This is the traditional algorithm that uses operator[] to read FeatureIds and write
 * SurfaceFeatures directly through the DataStore abstraction. It supports both 3D and
 * 2D image geometries, branching into separate helper functions based on the geometry's
 * dimensionality.
 *
 * **When this variant is selected**: DispatchAlgorithm selects this class when all
 * input arrays are backed by contiguous in-memory DataStore.
 *
 * **3D algorithm**: Iterates all voxels in Z-Y-X order. For each voxel, checks:
 * - Whether the voxel is on the outer boundary (x/y/z == 0 or max).
 * - Whether any of its 6 face neighbors has FeatureId == 0 (if MarkFeature0Neighbors
 *   is enabled).
 * If either condition is met, the feature owning that voxel is marked as a surface feature.
 *
 * **2D algorithm**: Determines which dimension is degenerate (size == 1) and performs
 * the equivalent 4-neighbor boundary check on the non-degenerate plane.
 *
 * **Why a separate OOC variant exists**: The Direct variant accesses the FeatureIds
 * array via operator[], and for the 3D case, neighbor lookups span +/-1, +/-dimX,
 * and +/-(dimX*dimY) in flat index space. When FeatureIds is stored in chunked OOC
 * format, these scattered accesses cause chunk thrashing. The Scanline variant reads
 * entire Z-slices sequentially to avoid this.
 *
 * @see ComputeSurfaceFeaturesScanline for the OOC-optimized variant.
 * @see ComputeSurfaceFeatures for the dispatcher.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesDirect
{
public:
  /**
   * @brief Constructs the in-core surface feature identifier.
   * @param dataStructure The DataStructure containing FeatureIds and SurfaceFeatures arrays.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, flags).
   */
  ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeaturesDirect() noexcept;

  ComputeSurfaceFeaturesDirect(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect(ComputeSurfaceFeaturesDirect&&) noexcept = delete;
  ComputeSurfaceFeaturesDirect& operator=(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect& operator=(ComputeSurfaceFeaturesDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core surface feature identification algorithm.
   *
   * Validates the feature-to-attribute-matrix mapping, determines whether the
   * geometry is 2D or 3D, and delegates to the appropriate helper function.
   *
   * @return Result<> indicating success, errors, or unsupported dimensionality.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                   ///< Reference to the DataStructure containing all data.
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr; ///< Algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                           ///< Cooperative cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                  ///< Progress message handler.
};

} // namespace nx::core
