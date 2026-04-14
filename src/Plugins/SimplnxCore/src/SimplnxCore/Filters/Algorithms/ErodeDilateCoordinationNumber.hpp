#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct ErodeDilateCoordinationNumberInputValues
 * @brief Holds all user-supplied parameters for the ErodeDilateCoordinationNumber algorithm.
 *
 * Populated by the filter's preflight/execute methods and passed into the
 * algorithm to decouple it from the parameter system.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumberInputValues
{
  int32 CoordinationNumber;                                      ///< Maximum tolerated coordination number. Voxels at a good/bad boundary
                                                                 ///< whose coordination number meets or exceeds this value will be modified.
  bool Loop;                                                     ///< If true, repeat the operation until no more voxels exceed the threshold
  DataPath FeatureIdsArrayPath;                                  ///< Path to the Int32 FeatureIds cell array (0 = bad data)
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths; ///< Arrays excluded from the data transfer phase
  DataPath InputImageGeometry;                                   ///< Path to the ImageGeom that defines the voxel grid
};

/**
 * @class ErodeDilateCoordinationNumber
 * @brief Smooths voxel boundaries by eroding or dilating based on coordination
 *        number thresholds, optimized for out-of-core (OOC) data stores.
 *
 * ## Algorithm Overview
 *
 * The "coordination number" of a voxel on a good/bad boundary is the count of
 * its 6 face neighbors that belong to the opposite class (good vs. bad, where
 * bad means FeatureId == 0). A high coordination number indicates that a voxel
 * is surrounded mostly by the opposite type and is therefore likely noise or a
 * rough boundary artifact.
 *
 * For each voxel on the boundary:
 * 1. Count the face neighbors of the opposite type (coordination number).
 * 2. Among those opposite-type neighbors, find the most common FeatureId.
 * 3. If the coordination number meets or exceeds the user's threshold, mark
 *    the voxel to be replaced by the most common neighbor's data.
 *
 * This is repeated until no voxels exceed the threshold (if Loop is true) or
 * for a single pass (if Loop is false).
 *
 * ## OOC Optimization Strategy
 *
 * The same 3-slice rolling window and per-slice mark/coordination array pattern
 * used in ErodeDilateBadData applies here:
 *
 * 1. **3-slice rolling window for FeatureIds**: Three Z-slices (prev, current,
 *    next) are held in std::vector buffers loaded via copyIntoBuffer(). All
 *    face-neighbor FeatureId lookups read from these buffers.
 *
 * 2. **Per-slice mark and coordination arrays**: Instead of full-volume arrays,
 *    three O(sliceSize) neighbor-mark arrays and three O(sliceSize) coordination
 *    arrays track the results for the rolling window. This reduces peak memory
 *    from O(volume) to O(sliceSize).
 *
 * 3. **Deferred sequential writes**: Marks for slice z-1 are committed after
 *    processing slice z, because only voxels at z-2 through z can affect z-1.
 *    The transfer is conditional: only voxels whose coordination number meets
 *    the threshold are actually committed, using SliceBufferedTransferOneZ.
 *
 * 4. **Per-pass re-read**: Each pass re-reads FeatureIds from the store because
 *    the previous pass's transfers may have changed boundary conditions.
 */
class SIMPLNXCORE_EXPORT ErodeDilateCoordinationNumber
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateCoordinationNumberInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ErodeDilateCoordinationNumber() noexcept;

  ErodeDilateCoordinationNumber(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber(ErodeDilateCoordinationNumber&&) noexcept = delete;
  ErodeDilateCoordinationNumber& operator=(const ErodeDilateCoordinationNumber&) = delete;
  ErodeDilateCoordinationNumber& operator=(ErodeDilateCoordinationNumber&&) noexcept = delete;

  /**
   * @brief Executes the coordination-number-based erosion/dilation algorithm.
   *
   * Runs one or more passes (depending on the Loop flag) over the entire
   * ImageGeom volume. Each pass processes all Z-slices sequentially using a
   * 3-slice rolling window, accumulating per-voxel coordination numbers and
   * best-neighbor marks. After each Z-slice completes, the previous slice's
   * qualifying marks are committed via SliceBufferedTransferOneZ.
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
  DataStructure& m_DataStructure;                                          ///< Reference to the DataStructure holding all arrays
  const ErodeDilateCoordinationNumberInputValues* m_InputValues = nullptr; ///< User-supplied algorithm parameters
  const std::atomic_bool& m_ShouldCancel;                                  ///< Cancellation flag checked between passes
  const IFilter::MessageHandler& m_MessageHandler;                         ///< Handler for progress/status messages
};

} // namespace nx::core
