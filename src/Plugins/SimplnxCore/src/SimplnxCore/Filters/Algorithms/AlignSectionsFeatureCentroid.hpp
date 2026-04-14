#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

namespace nx::core
{

/**
 * @struct AlignSectionsFeatureCentroidInputValues
 * @brief Holds all user-configured parameters for the AlignSectionsFeatureCentroid algorithm.
 */
struct SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroidInputValues
{
  DataPath ImageGeometryPath; ///< Path to the ImageGeom whose Z-slices will be aligned.
  DataPath MaskArrayPath;     ///< Path to the boolean/uint8 mask array identifying "good" cells.

  bool UseReferenceSlice; ///< If true, align all slices to a single reference slice rather than progressively.
  int32 ReferenceSlice;   ///< Z-index of the reference slice (only used when UseReferenceSlice is true).

  bool StoreAlignmentShifts;          ///< If true, write per-slice shift diagnostics to output arrays.
  DataPath AlignmentAMPath;           ///< Path to the Attribute Matrix that will hold the diagnostic arrays.
  DataPath SlicesArrayPath;           ///< Output: slice indices (uint32, 2-component).
  DataPath RelativeShiftsArrayPath;   ///< Output: per-slice relative X/Y shifts (int64, 2-component).
  DataPath CumulativeShiftsArrayPath; ///< Output: per-slice cumulative X/Y shifts (int64, 2-component).
  DataPath CentroidsArrayPath;        ///< Output: per-slice XY centroids (float32, 2-component).
};

/**
 * @class AlignSectionsFeatureCentroid
 * @brief Aligns Z-slices of an ImageGeom by computing the centroid of masked
 * (good) cells in each slice and shifting slices so their centroids align.
 *
 * The algorithm iterates over Z-slices from top to bottom, computes the weighted
 * centroid of all "good" cells in each slice (as defined by the mask array), then
 * derives per-slice X/Y shifts that bring consecutive (or reference-relative)
 * centroids into alignment. Shifts are rounded to integer voxel increments.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * When the mask array resides in an out-of-core (OOC) DataStore, per-element
 * MaskCompare::isTrue() calls would trigger a chunk load/evict cycle for every
 * voxel, making the centroid loop extremely slow. The OOC path (findShiftsOoc)
 * instead bulk-reads one full Z-slice of mask data at a time via copyIntoBuffer(),
 * then iterates over the in-memory buffer. This converts O(N) random chunk
 * accesses into O(Z) sequential bulk reads, where Z is the number of slices.
 */
class SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroid : public AlignSections
{
public:
  AlignSectionsFeatureCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsFeatureCentroidInputValues* inputValues);
  ~AlignSectionsFeatureCentroid() noexcept override;

  AlignSectionsFeatureCentroid(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid(AlignSectionsFeatureCentroid&&) noexcept = delete;
  AlignSectionsFeatureCentroid& operator=(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid& operator=(AlignSectionsFeatureCentroid&&) noexcept = delete;

  /**
   * @brief Executes the alignment algorithm: computes centroids, derives shifts,
   * and applies them via the base-class AlignSections::execute() method.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes per-slice X/Y shifts by comparing centroids of masked cells.
   *
   * Dispatches to findShiftsOoc() when the mask array is out-of-core; otherwise
   * uses the in-memory MaskCompare path for per-element access.
   * @param xShifts Output vector of X shifts per Z-slice (in voxel units).
   * @param yShifts Output vector of Y shifts per Z-slice (in voxel units).
   * @return Result<> indicating success or error.
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief OOC-optimized shift computation that bulk-reads the mask array one
   * Z-slice at a time via copyIntoBuffer(), avoiding per-element chunk thrashing.
   * @param xShifts Output vector of X shifts per Z-slice (in voxel units).
   * @param yShifts Output vector of Y shifts per Z-slice (in voxel units).
   * @return Result<> indicating success or error.
   */
  Result<> findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts);

  DataStructure& m_DataStructure;                                         ///< Reference to the DataStructure.
  const AlignSectionsFeatureCentroidInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                                 ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                        ///< Message handler for progress.
};

} // namespace nx::core
