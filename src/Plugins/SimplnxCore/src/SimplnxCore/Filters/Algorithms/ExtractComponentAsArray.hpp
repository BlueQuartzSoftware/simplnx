#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct ExtractComponentAsArrayInputValues
 * @brief Collects component options and array paths.
 */
struct SIMPLNXCORE_EXPORT ExtractComponentAsArrayInputValues
{
  bool MoveComponentsToNewArray;
  bool RemoveComponentsFromArray;
  int32 CompNumber;
  DataPath TempArrayPath;
  DataPath BaseArrayPath;
  DataPath NewArrayPath;
};

/**
 * @class ExtractComponentAsArray
 * @brief Extracts one component into a scalar array and/or removes it from its source array.
 *
 * A temporary source snapshot prevents aliasing when the base array is reduced.
 * If extraction is disabled, the implementation removes the component even
 * when RemoveComponentsFromArray is false.
 *
 * Contiguous stores use direct access only when every participating store is
 * contiguous. Other stores use bulk transfers. Scratch holds approximately
 * 65,536 values, or one complete tuple when it has more components.
 */
class SIMPLNXCORE_EXPORT ExtractComponentAsArray
{
public:
  /**
   * @brief Initializes component extraction.
   * @param dataStructure Contains source and destination arrays.
   * @param mesgHandler Supplies the common interface. This algorithm emits no messages.
   * @param shouldCancel Signals cancellation between chunks.
   * @param inputValues Selects the component, options, and paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractComponentAsArrayInputValues* inputValues);
  ~ExtractComponentAsArray() noexcept;

  ExtractComponentAsArray(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray(ExtractComponentAsArray&&) noexcept = delete;
  ExtractComponentAsArray& operator=(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray& operator=(ExtractComponentAsArray&&) noexcept = delete;

  /**
   * @brief Extracts and/or removes the selected component.
   * @return Success, or a bulk-transfer error.
   * @pre The absolute component number is in range and is not INT32_MIN.
   * @pre Source and destination arrays have compatible types and shapes.
   *
   * Cancellation returns success. Output chunks written before cancellation
   * remain. A transfer error can occur after the extracted chunk is written but
   * before the matching reduced chunk is written.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
