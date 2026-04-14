#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesScanline
 * @brief Out-of-core (OOC) optimized algorithm for identifying surface features using
 * Z-slice sequential bulk I/O with a 3-slice rolling window.
 *
 * **The problem this solves**: When the FeatureIds array is stored out-of-core in
 * chunked format, the Direct variant's operator[] access to check face neighbors
 * triggers chunk thrashing -- especially the +/-Z neighbor lookups that are
 * dimX*dimY elements apart in flat index space. This makes the algorithm orders
 * of magnitude slower on OOC data.
 *
 * **How the rolling window solves it**: This variant reads the FeatureIds array one
 * native Z-slice at a time using copyIntoBuffer(), maintaining three in-memory
 * buffers (prevSlice, curSlice, nextSlice). All neighbor lookups are performed on
 * these in-memory buffers:
 *   - X and Y neighbors: simple index arithmetic within curSlice.
 *   - Z neighbors: same position in prevSlice (-Z) or nextSlice (+Z).
 *
 * **2D geometry support**: For geometries with one degenerate dimension (size == 1),
 * the algorithm still iterates the native Z-Y-X grid but remaps coordinates to
 * the 2D plane for boundary and neighbor checks. This unified approach avoids
 * separate 2D/3D code paths while maintaining sequential I/O.
 *
 * **Output caching**: The SurfaceFeatures output is a small feature-level array
 * (one element per feature, not per voxel). To avoid per-voxel OOC writes, the
 * results are accumulated in a local std::vector and bulk-written once at the end
 * via copyFromBuffer().
 *
 * **Memory overhead**: 3 input buffers of size (dimX * dimY * 4 bytes) for the
 * rolling window, plus a local vector of size (numFeatures * 1 byte) for the
 * cached output. For typical datasets this is a few MB.
 *
 * @see ComputeSurfaceFeaturesDirect for the in-core variant.
 * @see ComputeSurfaceFeatures for the dispatcher.
 * @see DispatchAlgorithm for the selection mechanism.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesScanline
{
public:
  /**
   * @brief Constructs the OOC-optimized surface feature identifier.
   * @param dataStructure The DataStructure containing FeatureIds and SurfaceFeatures arrays.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, flags).
   */
  ComputeSurfaceFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeSurfaceFeaturesInputValues* inputValues);
  ~ComputeSurfaceFeaturesScanline() noexcept;

  ComputeSurfaceFeaturesScanline(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline(ComputeSurfaceFeaturesScanline&&) noexcept = delete;
  ComputeSurfaceFeaturesScanline& operator=(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline& operator=(ComputeSurfaceFeaturesScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized surface feature identification using a
   * 3-slice rolling window with copyIntoBuffer bulk I/O.
   *
   * Handles both 3D and 2D geometries within a single Z-iteration loop,
   * using coordinate remapping for 2D cases.
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
