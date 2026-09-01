#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeDifferencesMapInputValues
 * @brief Stores paths for a difference-map execution.
 */
struct SIMPLNXCORE_EXPORT ComputeDifferencesMapInputValues
{
  ArrayCreationParameter::ValueType DifferenceMapArrayPath;
  ArraySelectionParameter::ValueType FirstInputArrayPath;
  ArraySelectionParameter::ValueType SecondInputArrayPath;
};

/**
 * @class ComputeDifferencesMap
 * @brief Computes the component-wise absolute difference between two arrays.
 *
 * Input and output values are streamed through bounded, component-aligned buffers so
 * out-of-core stores use bulk I/O without allocating memory proportional to the array.
 */
class SIMPLNXCORE_EXPORT ComputeDifferencesMap
{
public:
  /**
   * @brief Initializes the difference-map algorithm.
   * @param dataStructure Contains the source and output arrays.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between chunks.
   * @param inputValues Identifies the source and output arrays.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeDifferencesMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeDifferencesMapInputValues* inputValues);
  /**
   * @brief Destroys the difference-map algorithm.
   */
  ~ComputeDifferencesMap() noexcept;

  ComputeDifferencesMap(const ComputeDifferencesMap&) = delete;
  ComputeDifferencesMap(ComputeDifferencesMap&&) noexcept = delete;
  ComputeDifferencesMap& operator=(const ComputeDifferencesMap&) = delete;
  ComputeDifferencesMap& operator=(ComputeDifferencesMap&&) noexcept = delete;

  /**
   * @brief Computes component-wise absolute differences.
   * @return Success, or a source or output bulk-I/O error.
   * @pre Each same-type absolute difference is representable. Signed integer subtraction can otherwise overflow.
   *
   * When a chunk checkpoint observes cancellation, the method returns success. Data written before
   * that checkpoint remains in the output array. Later tuples are not written.
   * A bulk-I/O error can leave completed output chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeDifferencesMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
