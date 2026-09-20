#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

/**
 * @struct ChangeAngleRepresentationInputValues
 * @brief Identifies the angle array and conversion direction.
 */
struct SIMPLNXCORE_EXPORT ChangeAngleRepresentationInputValues
{
  ArraySelectionParameter::ValueType AnglesArrayPath;
  ChoicesParameter::ValueType ConversionTypeIndex;
};

/**
 * @class ChangeAngleRepresentation
 * @brief Converts float32 angle values in place using storage-aware execution.
 *
 * Contiguous in-memory stores use direct parallel multiplication. Out-of-core
 * stores use sequential pages of at most 65,536 values. Both paths modify the
 * source array in place. Cancellation returns success and preserves converted
 * values. A bulk-I/O error can leave earlier pages converted.
 */

class SIMPLNXCORE_EXPORT ChangeAngleRepresentation
{
public:
  /**
   * @brief Initializes in-place angle conversion.
   * @param dataStructure Provides the angle array.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the array and conversion direction.
   * @pre All arguments outlive this executor.
   */
  ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ChangeAngleRepresentationInputValues* inputValues);
  ~ChangeAngleRepresentation() noexcept;

  ChangeAngleRepresentation(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation(ChangeAngleRepresentation&&) noexcept = delete;
  ChangeAngleRepresentation& operator=(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation& operator=(ChangeAngleRepresentation&&) noexcept = delete;

  /**
   * @brief Converts every Float32 value between degrees and radians.
   * @return Bulk-I/O errors from the page path.
   * @pre ConversionTypeIndex is zero for degrees-to-radians or one for the reverse.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ChangeAngleRepresentationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
