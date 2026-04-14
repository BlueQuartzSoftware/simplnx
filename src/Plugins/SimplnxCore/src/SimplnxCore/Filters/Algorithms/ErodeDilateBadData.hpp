#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
namespace detail
{
static inline constexpr StringLiteral k_DilateString = "Dilate";
static inline constexpr StringLiteral k_ErodeString = "Erode";
static inline const ChoicesParameter::Choices k_OperationChoices = {k_DilateString, k_ErodeString};

static inline constexpr ChoicesParameter::ValueType k_DilateIndex = 0ULL;
static inline constexpr ChoicesParameter::ValueType k_ErodeIndex = 1ULL;
} // namespace detail

/**
 * @struct ErodeDilateBadDataInputValues
 * @brief Holds all user-supplied parameters for the ErodeDilateBadData algorithm.
 *
 * This struct is populated by the filter's preflight/execute methods and passed
 * into the algorithm so that the algorithm itself has no dependency on the
 * parameter system.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateBadDataInputValues
{
  ChoicesParameter::ValueType Operation;                         ///< Morphological operation: detail::k_DilateIndex (0) or detail::k_ErodeIndex (1)
  int32 NumIterations;                                           ///< Number of erosion/dilation passes to perform
  bool XDirOn;                                                   ///< Whether to consider neighbors along the X axis
  bool YDirOn;                                                   ///< Whether to consider neighbors along the Y axis
  bool ZDirOn;                                                   ///< Whether to consider neighbors along the Z axis
  DataPath FeatureIdsArrayPath;                                  ///< Path to the Int32 FeatureIds cell array (0 = bad data)
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths; ///< Arrays excluded from the data transfer phase
  DataPath InputImageGeometry;                                   ///< Path to the ImageGeom that defines the voxel grid
};

/**
 * @class ErodeDilateBadData
 * @brief Iterative morphological erosion or dilation of "bad" voxels (FeatureId == 0)
 *        on a structured ImageGeom grid, optimized for out-of-core (OOC) data stores.
 *
 * ## Algorithm Overview
 *
 * **Bad data** is any cell whose FeatureId is 0, meaning it failed some prior
 * classification step. This algorithm either grows those bad regions (dilation)
 * or shrinks them (erosion) by one voxel per iteration, repeating for a
 * user-specified number of iterations.
 *
 * - **Dilation**: For every bad voxel that has a non-zero (good) face neighbor,
 *   the good neighbor is marked to become bad. This expands the bad region
 *   outward by one cell.
 * - **Erosion**: For every bad voxel, the surrounding good neighbors are tallied
 *   and the most common FeatureId is chosen. The bad voxel is replaced with
 *   that FeatureId, shrinking the bad region inward by one cell.
 *
 * After marks are determined, all sibling data arrays in the same Attribute
 * Matrix (except those in the ignored list) are updated in bulk using
 * SliceBufferedTransferOneZ, so that every array stays consistent with the
 * modified FeatureIds.
 *
 * ## OOC Optimization Strategy
 *
 * When data resides in an out-of-core (chunked, disk-backed) store, random
 * voxel access triggers expensive chunk load/evict cycles ("chunk thrashing").
 * This implementation avoids that by:
 *
 * 1. **3-slice rolling window for FeatureIds**: Three Z-slices (prev, current,
 *    next) are held in contiguous std::vector buffers, loaded via
 *    copyIntoBuffer(). Face-neighbor lookups index into these buffers instead
 *    of hitting the OOC store. As the Z loop advances, the window shifts
 *    forward with O(1) pointer swaps.
 *
 * 2. **Per-slice mark arrays instead of a full-volume neighbor array**: Classic
 *    implementations allocate an O(totalPoints) neighbors array. This version
 *    uses three O(sliceSize) mark arrays (one per Z-slice in the window),
 *    reducing peak memory from O(volume) to O(3 * sliceSize).
 *
 * 3. **Deferred, sequential Z-slice writes**: Marks are accumulated as the
 *    scan proceeds. When slice z finishes, the marks for slice z-1 are
 *    guaranteed complete (no future voxel can modify them), so z-1 is
 *    transferred via SliceBufferedTransferOneZ. This keeps writes sequential
 *    and aligned with OOC chunk boundaries.
 *
 * 4. **Per-iteration re-read**: Each iteration re-initializes the rolling
 *    window from the store because the previous iteration's transfers may
 *    have changed FeatureId values.
 */
class SIMPLNXCORE_EXPORT ErodeDilateBadData
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ErodeDilateBadData() noexcept;

  ErodeDilateBadData(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData(ErodeDilateBadData&&) noexcept = delete;
  ErodeDilateBadData& operator=(const ErodeDilateBadData&) = delete;
  ErodeDilateBadData& operator=(ErodeDilateBadData&&) noexcept = delete;

  /**
   * @brief Executes the erode/dilate bad data algorithm.
   *
   * Runs NumIterations passes of the selected morphological operation
   * (erosion or dilation) over the entire ImageGeom volume. Each pass
   * processes all Z-slices sequentially using a 3-slice rolling window
   * for FeatureId lookups and deferred SliceBufferedTransferOneZ calls
   * for the data-copy phase.
   *
   * @return Result<> indicating success or any errors encountered during execution
   */
  Result<> operator()();

  /**
   * @brief Returns a reference to the cancellation flag.
   * @return const reference to the atomic cancellation flag
   */
  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;                               ///< Reference to the DataStructure holding all arrays
  const ErodeDilateBadDataInputValues* m_InputValues = nullptr; ///< User-supplied algorithm parameters
  const std::atomic_bool& m_ShouldCancel;                       ///< Cancellation flag checked between iterations
  const IFilter::MessageHandler& m_MessageHandler;              ///< Handler for progress/status messages
};

} // namespace nx::core
