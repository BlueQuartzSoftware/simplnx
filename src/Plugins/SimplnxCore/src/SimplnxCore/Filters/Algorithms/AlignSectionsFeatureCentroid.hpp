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
 * @brief Defines mask-centroid alignment and optional diagnostic outputs.
 */
struct SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroidInputValues
{
  DataPath ImageGeometryPath;
  DataPath MaskArrayPath;
  bool UseReferenceSlice;
  int32 ReferenceSlice;
  bool StoreAlignmentShifts;
  DataPath AlignmentAMPath;
  DataPath SlicesArrayPath;
  DataPath RelativeShiftsArrayPath;
  DataPath CumulativeShiftsArrayPath;
  DataPath CentroidsArrayPath;
};

/**
 * @class AlignSectionsFeatureCentroid
 * @brief Aligns Z-slices of an ImageGeom by computing the centroid of masked
 * (good) cells in each slice and shifting slices so their centroids align.
 *
 * The algorithm visits Z slices from highest to lowest. It calculates the mean
 * X and Y coordinates of selected cells. It then derives consecutive or
 * reference-relative shifts and truncates them to integer voxel offsets.
 *
 * An out-of-core mask selects a sequential path that reads one full mask slice.
 * Memory scales with the X-by-Y slice size. The current path does not inspect
 * mask bulk-read Result values. Cancellation returns success and can leave cell
 * arrays partially shifted by AlignSections.
 *
 * When diagnostics are enabled, the direct path stores intermediate relative
 * shifts in usize. Negative values can wrap. The scanline path uses int64 and
 * does not have that diagnostic-storage limitation.
 */
class SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroid : public AlignSections
{
public:
  /**
   * @brief Initializes mask-centroid section alignment.
   * @param dataStructure Provides the geometry, mask, cell arrays, and outputs.
   * @param mesgHandler Receives progress and shift warnings.
   * @param shouldCancel Signals cancellation between slices.
   * @param inputValues Defines reference and diagnostic settings.
   * @pre All arguments outlive this executor.
   */
  AlignSectionsFeatureCentroid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsFeatureCentroidInputValues* inputValues);
  ~AlignSectionsFeatureCentroid() noexcept override;

  AlignSectionsFeatureCentroid(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid(AlignSectionsFeatureCentroid&&) noexcept = delete;
  AlignSectionsFeatureCentroid& operator=(const AlignSectionsFeatureCentroid&) = delete;
  AlignSectionsFeatureCentroid& operator=(AlignSectionsFeatureCentroid&&) noexcept = delete;

  /**
   * @brief Computes centroid shifts and applies them through AlignSections.
   * @return Mask-type or base alignment errors.
   * @pre Every slice contains at least one selected mask cell.
   * @pre Image spacing is positive and ReferenceSlice is in range when enabled.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes X and Y voxel shifts from per-slice mask centroids.
   * @param xShifts Receives cumulative X shifts by traversal index.
   * @param yShifts Receives cumulative Y shifts by traversal index.
   * @return Mask-type errors. Cancellation returns success after completed centroids.
   *
   * Out-of-core or forced-OOC masks delegate to findShiftsOoc().
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief Computes shifts from one bulk-read mask slice at a time.
   * @param xShifts Receives cumulative X shifts by traversal index.
   * @param yShifts Receives cumulative Y shifts by traversal index.
   * @return Unsupported-mask errors. Bulk-read failures are not returned.
   */
  Result<> findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts);

  DataStructure& m_DataStructure;
  const AlignSectionsFeatureCentroidInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
