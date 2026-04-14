#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{
namespace detail
{
inline constexpr StringLiteral k_LessThan = "< [Less Than]";
inline constexpr StringLiteral k_GreaterThan = "> [Greater Than]";
inline const ChoicesParameter::Choices k_OperationChoices = {k_LessThan, k_GreaterThan};
} // namespace detail

/**
 * @struct ReplaceElementAttributesWithNeighborValuesInputValues
 * @brief Holds all user-supplied parameters for the ReplaceElementAttributesWithNeighborValues algorithm.
 *
 * Populated by the filter's preflight/execute methods and passed into the
 * algorithm to decouple it from the parameter system.
 */
struct SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValuesInputValues
{
  float32 MinConfidence;                          ///< Threshold value for the comparison (e.g., confidence index cutoff)
  ChoicesParameter::ValueType SelectedComparison; ///< 0 = Less Than (replace voxels below threshold), 1 = Greater Than (replace voxels above threshold)
  bool Loop;                                      ///< If true, repeat until no voxels remain that fail the threshold test
  DataPath InputArrayPath;                        ///< Path to the scalar cell array used for the threshold comparison
  DataPath SelectedImageGeometryPath;             ///< Path to the ImageGeom that defines the voxel grid
};

/**
 * @class ReplaceElementAttributesWithNeighborValues
 * @brief Iteratively replaces voxel data that fails a threshold comparison with
 *        the best-scoring face-neighbor value, optimized for out-of-core (OOC)
 *        data stores.
 *
 * ## Algorithm Overview
 *
 * 1. For each voxel whose value fails the threshold test (less-than or
 *    greater-than the user's cutoff), examine its 6 face neighbors.
 * 2. Among neighbors that pass the threshold, find the one with the
 *    best (highest or lowest, depending on comparison direction) value.
 * 3. Mark that voxel to be replaced by the best neighbor's data.
 * 4. After scanning a Z-slice, commit the marks by copying tuple data from
 *    source to destination for ALL arrays in the Attribute Matrix.
 * 5. If Loop is true, repeat until no failing voxels remain.
 *
 * Unlike the ErodeDilateBadData algorithm (which only marks voxels within
 * +/- 1 Z-slice via face neighbors), this algorithm's marks are strictly
 * within the current Z-slice: a failing voxel is replaced by one of its
 * own face neighbors, so the source is always within +/- 1 Z of the
 * destination. This allows each Z-slice to be committed immediately after
 * processing rather than being deferred.
 *
 * ## OOC Optimization Strategy
 *
 * 1. **3-slice rolling window for the input array**: Three Z-slices of the
 *    comparison array (prev, current, next) are held in typed buffers loaded
 *    via copyIntoBuffer(). All face-neighbor comparisons index into these
 *    buffers rather than the OOC store.
 *
 * 2. **Per-slice best-neighbor marks**: A single O(sliceSize) mark array
 *    tracks the best replacement source for each voxel in the current
 *    Z-slice. Marks are committed immediately via SliceBufferedTransferOneZ
 *    and then cleared for the next slice.
 *
 * 3. **Type-dispatched via ExecuteDataFunction**: The inner algorithm is
 *    templated on the input array's element type to avoid virtual dispatch
 *    overhead during the tight comparison loop. The template also handles
 *    the std::vector<bool> bit-packing issue by using unique_ptr<T[]> for
 *    all buffer types.
 *
 * 4. **Per-pass re-read**: Each pass re-loads the rolling window from the
 *    store because the previous pass's SliceBufferedTransferOneZ calls
 *    may have changed the comparison array's values.
 */
class SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValues
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ReplaceElementAttributesWithNeighborValuesInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ReplaceElementAttributesWithNeighborValues() noexcept;

  ReplaceElementAttributesWithNeighborValues(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;

  /**
   * @brief Executes the threshold-based neighbor replacement algorithm.
   *
   * Dispatches to a type-specific inner loop via ExecuteDataFunction. Each
   * pass processes all Z-slices sequentially using a 3-slice rolling window,
   * committing per-slice marks immediately via SliceBufferedTransferOneZ.
   * Repeats until no failing voxels remain (if Loop is true) or for one
   * pass (if Loop is false).
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
  DataStructure& m_DataStructure;                                                       ///< Reference to the DataStructure holding all arrays
  const ReplaceElementAttributesWithNeighborValuesInputValues* m_InputValues = nullptr; ///< User-supplied algorithm parameters
  const std::atomic_bool& m_ShouldCancel;                                               ///< Cancellation flag checked between passes
  const IFilter::MessageHandler& m_MessageHandler;                                      ///< Handler for progress/status messages
};

} // namespace nx::core
