#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

/**
 * @struct CombineAttributeArraysInputValues
 * @brief Selects source arrays, output array, and optional normalization.
 */
struct SIMPLNXCORE_EXPORT CombineAttributeArraysInputValues
{
  bool NormalizeData = {};
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  DataPath StackedDataArrayPath;
};

/**
 * @class CombineAttributeArrays
 * @brief Streams equally typed arrays into a combined array while preserving tuple and component order.
 *
 * All stores use sequential bulk-I/O pages. Page buffers contain at most 65,536
 * output values, so transient memory does not scale with the tuple count. Min
 * and max arrays scale with the combined component count.
 *
 * Normalization performs a complete min/max pass before the output pass. It uses
 * the source value type, so integral arrays use integral division. A constant
 * component becomes zero. Cancellation returns success and can leave output
 * pages partial.
 */
class SIMPLNXCORE_EXPORT CombineAttributeArrays
{
public:
  /**
   * @brief Initializes component-wise array combination.
   * @param dataStructure Provides source and output arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Selects arrays and normalization.
   * @pre All arguments outlive this executor.
   */
  CombineAttributeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineAttributeArraysInputValues* inputValues);
  ~CombineAttributeArrays() noexcept;

  CombineAttributeArrays(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays(CombineAttributeArrays&&) noexcept = delete;
  CombineAttributeArrays& operator=(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays& operator=(CombineAttributeArrays&&) noexcept = delete;

  /**
   * @brief Combines source components into the output array.
   * @return Empty-selection warning or source and destination transfer errors.
   * @pre All source arrays exist, have one common DataType, and have equal tuple counts.
   * @pre Output components equal the sum of source components and output does not alias a source.
   * @pre Component counts and their products fit usize.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineAttributeArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
