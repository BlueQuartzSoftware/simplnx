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
static inline constexpr StringLiteral k_DilateString = "Dilate";
static inline constexpr StringLiteral k_ErodeString = "Erode";
static inline const ChoicesParameter::Choices k_OperationChoices = {k_DilateString, k_ErodeString};

static inline constexpr ChoicesParameter::ValueType k_DilateIndex = 0ULL;
static inline constexpr ChoicesParameter::ValueType k_ErodeIndex = 1ULL;
} // namespace detail

/**
 * @struct ErodeDilateMaskInputValues
 * @brief Holds all user-supplied parameters for the ErodeDilateMask algorithm.
 *
 * Populated by the filter's preflight/execute methods and passed into the
 * algorithm to decouple it from the parameter system.
 */
struct SIMPLNXCORE_EXPORT ErodeDilateMaskInputValues
{
  ChoicesParameter::ValueType Operation; ///< Morphological operation: detail::k_DilateIndex (0) or detail::k_ErodeIndex (1)
  int32 NumIterations;                   ///< Number of erosion/dilation passes to perform
  bool XDirOn;                           ///< Whether to consider neighbors along the X axis
  bool YDirOn;                           ///< Whether to consider neighbors along the Y axis
  bool ZDirOn;                           ///< Whether to consider neighbors along the Z axis
  DataPath MaskArrayPath;                ///< Path to the boolean mask cell array to erode/dilate
  DataPath InputImageGeometry;           ///< Path to the ImageGeom that defines the voxel grid
};

/**
 * @class ErodeDilateMask
 * @brief Iterative morphological erosion or dilation of a boolean mask array
 *        using face-neighbor connectivity, optimized for out-of-core (OOC)
 *        data stores.
 *
 * ## Algorithm Overview
 *
 * This algorithm performs morphological erosion or dilation on a boolean mask
 * array (true/false per voxel) rather than on FeatureIds. Unlike the
 * ErodeDilateBadData algorithm, this one operates directly on the mask and
 * does not propagate changes to sibling data arrays.
 *
 * - **Dilation**: For every false (unmasked) voxel, if any face neighbor is
 *   true (masked), the voxel is set to true. This grows the masked region
 *   outward by one cell per iteration.
 * - **Erosion**: For every false voxel, if any face neighbor is true, that
 *   neighbor is set to false. This shrinks the masked region inward by one
 *   cell per iteration.
 *
 * The operation is applied in-place to the mask array for each iteration.
 * A read-then-write pattern (using separate maskSlices and maskCopySlices
 * buffers) ensures that the scan within a single iteration reads the
 * original state while accumulating modifications into the copy.
 *
 * ## OOC Optimization Strategy
 *
 * 1. **3-slice rolling window (dual buffers)**: Two sets of three Z-slice
 *    buffers are maintained: `maskSlices` for reading the original mask state
 *    and `maskCopySlices` for accumulating the modified state. This dual-buffer
 *    approach ensures reads and writes do not interfere within a single
 *    iteration.
 *
 * 2. **uint8 intermediary for bool**: Because std::vector<bool> uses
 *    bit-packing (which prevents taking element addresses), the rolling
 *    window uses uint8 buffers. A separate bool[] buffer handles bulk I/O
 *    with the data store's copyIntoBuffer/copyFromBuffer API.
 *
 * 3. **Deferred sequential writes**: After processing each Z-slice, the
 *    completed z-1 slice is written back to the store using copyFromBuffer.
 *    This keeps writes sequential and aligned with OOC chunk boundaries.
 *
 * 4. **Per-iteration re-read**: Each iteration re-loads the rolling window
 *    from the store because the previous iteration's writes changed the mask.
 */
class SIMPLNXCORE_EXPORT ErodeDilateMask
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ErodeDilateMask() noexcept;

  ErodeDilateMask(const ErodeDilateMask&) = delete;
  ErodeDilateMask(ErodeDilateMask&&) noexcept = delete;
  ErodeDilateMask& operator=(const ErodeDilateMask&) = delete;
  ErodeDilateMask& operator=(ErodeDilateMask&&) noexcept = delete;

  /**
   * @brief Executes the erode/dilate mask algorithm.
   *
   * Runs NumIterations passes of the selected morphological operation over
   * the entire ImageGeom volume. Each pass uses a dual-buffered 3-slice
   * rolling window (read from maskSlices, write into maskCopySlices) and
   * deferred sequential writes back to the data store.
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
  DataStructure& m_DataStructure;                            ///< Reference to the DataStructure holding all arrays
  const ErodeDilateMaskInputValues* m_InputValues = nullptr; ///< User-supplied algorithm parameters
  const std::atomic_bool& m_ShouldCancel;                    ///< Cancellation flag checked between iterations
  const IFilter::MessageHandler& m_MessageHandler;           ///< Handler for progress/status messages
};

} // namespace nx::core
