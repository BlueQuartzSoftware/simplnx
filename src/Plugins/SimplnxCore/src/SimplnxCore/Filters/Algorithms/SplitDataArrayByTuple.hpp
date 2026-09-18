#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

namespace nx::core
{
/**
 * @struct SplitDataArrayByTupleInputValues
 * @brief Stores the source, ordered outputs, and split tuple dimension.
 */
struct SIMPLNXCORE_EXPORT SplitDataArrayByTupleInputValues
{
  DataPath InputArrayPath;
  std::vector<DataPath> OutputArrayPaths;
  usize SplitDimension;
};

/**
 * @class SplitDataArrayByTuple
 * @brief Splits an array into ordered N-dimensional tuple blocks.
 *
 * Each output consumes the next extent along SplitDimension. Numeric arrays use
 * parallel raw-pointer copies when every store is concrete and resident. Other
 * numeric stores use checked row-major bulk transfers with a one-MiB target. A
 * tuple wider than that target requires a larger one-tuple buffer.
 *
 * StringArray and NeighborList outputs run as independent tasks through
 * CopyDataND. Those tasks inspect cancellation only before each complete output
 * copy and discard CopyDataND results. A failure is not returned by this class.
 * Numeric bulk-I/O errors are returned without rolling back prior output blocks.
 */
class SIMPLNXCORE_EXPORT SplitDataArrayByTuple
{
public:
  /**
   * @enum OutputContainer
   * @brief Selects where filter preflight creates split outputs.
   */
  enum class OutputContainer : uint8
  {
    NewDataGroup = 0,      ///< Creates a new data group.
    ExistingDataGroup = 1, ///< Uses a selected data group.
    NewAttrMatrix = 2,     ///< Creates a new AttributeMatrix.
    ExistingAttrMatrix = 3 ///< Uses a selected AttributeMatrix.
  };

  /**
   * @enum ErrorCodes
   * @brief Defines filter preflight and execution error codes.
   */
  enum class ErrorCodes : int32
  {
    NoInputArray = -65400,                           ///< Input path does not identify an array.
    SplitDimLessThanZero = -65401,                   ///< Split dimension is negative.
    SplitDimOutOfRange = -65402,                     ///< Split dimension exceeds the tuple rank.
    SplitCountLessThanZero = -65403,                 ///< An output extent is negative.
    SplitCountSumNotEqual = -65404,                  ///< Output extents do not cover the source dimension.
    AttrMatrixTupleShapeNegative = -65405,           ///< AttributeMatrix shape contains a negative extent.
    AttrMatrixTupleShapeNoCommonMultiplier = -65406, ///< AttributeMatrix shape cannot contain each split block.
    AnyArrayType = -65407,                           ///< Runtime array reports the internal Any type.
    UnsupportedArrayType = -65408,                   ///< Runtime array type has no split implementation.
    MultiDimensionalSplitCount = -65409              ///< Split-count table has more than one dimension.
  };

  /**
   * @brief Initializes the tuple-split algorithm.
   * @param dataStructure Contains source and output arrays.
   * @param mesgHandler Receives one message per scheduled output.
   * @param shouldCancel Signals cancellation between outputs and copy blocks.
   * @param inputValues Selects source, outputs, and dimension.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  SplitDataArrayByTuple(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitDataArrayByTupleInputValues* inputValues);
  /**
   * @brief Destroys the tuple-split algorithm.
   */
  ~SplitDataArrayByTuple() noexcept;

  SplitDataArrayByTuple(const SplitDataArrayByTuple&) = delete;
  SplitDataArrayByTuple(SplitDataArrayByTuple&&) noexcept = delete;
  SplitDataArrayByTuple& operator=(const SplitDataArrayByTuple&) = delete;
  SplitDataArrayByTuple& operator=(SplitDataArrayByTuple&&) noexcept = delete;

  /**
   * @brief Copies ordered tuple blocks to preflight-created outputs.
   * @return Numeric bulk-I/O, component-count, or unsupported-array result.
   * @pre SplitDimension indexes the source and every output tuple shape.
   * @pre Ordered output extents sum to the source extent on SplitDimension.
   * @pre Tuple-shape and component products fit usize.
   *
   * Cancellation returns success. Outputs can contain different completed
   * blocks. StringArray and NeighborList copy failures are not returned.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitDataArrayByTupleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
