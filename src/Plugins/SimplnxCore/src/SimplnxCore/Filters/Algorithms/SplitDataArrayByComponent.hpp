#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace nx::core
{
/**
 * @struct SplitDataArrayByComponentInputValues
 * @brief Stores the source, output suffix, and ordered component indexes.
 */
struct SIMPLNXCORE_EXPORT SplitDataArrayByComponentInputValues
{
  DataPath InputArrayPath;
  std::string SplitArraysSuffix;
  std::vector<usize> ExtractComponents;
};

/**
 * @class SplitDataArrayByComponent
 * @brief Splits selected components from a multi-component array into scalar arrays.
 *
 * Each output name appends the suffix and source component index. Concrete
 * in-memory stores use parallel raw pointers. Other stores read one interleaved
 * input chunk and then write each selected scalar output. The transfer targets
 * 65,536 input values but always retains one complete tuple. A tuple with more
 * components can therefore exceed that target.
 *
 * Cancellation can stop between direct blocks, input chunks, or component
 * writes. Outputs can contain different completed ranges. Bulk-I/O errors are
 * checked, but prior output writes are not restored.
 */
class SIMPLNXCORE_EXPORT SplitDataArrayByComponent
{
public:
  /**
   * @brief Initializes the component-split algorithm.
   * @param dataStructure Contains the input and output arrays.
   * @param messageHandler Preserves the common constructor signature.
   * @param shouldCancel Signals cancellation between work blocks.
   * @param inputValues Selects input, suffix, and components.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  SplitDataArrayByComponent(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, SplitDataArrayByComponentInputValues* inputValues);

  /**
   * @brief Destroys the component-split algorithm.
   */
  ~SplitDataArrayByComponent() noexcept;

  SplitDataArrayByComponent(const SplitDataArrayByComponent&) = delete;
  SplitDataArrayByComponent(SplitDataArrayByComponent&&) noexcept = delete;
  SplitDataArrayByComponent& operator=(const SplitDataArrayByComponent&) = delete;
  SplitDataArrayByComponent& operator=(SplitDataArrayByComponent&&) noexcept = delete;

  /**
   * @brief Copies selected components to scalar output arrays.
   * @return Input or output bulk-I/O result.
   * @pre Every selected component indexes the source component shape.
   * @pre Each preflight-created output has the source tuple shape and value type.
   *
   * Cancellation returns success and does not roll back completed output ranges.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SplitDataArrayByComponentInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
