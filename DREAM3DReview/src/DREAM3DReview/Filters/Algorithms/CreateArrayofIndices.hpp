#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateArrayofIndicesInputValues inputValues;

  inputValues.IndexArrayPath = filterArgs.value<DataPath>(k_IndexArrayPath_Key);

  return CreateArrayofIndices(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT CreateArrayofIndicesInputValues
{
  DataPath IndexArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT CreateArrayofIndices
{
public:
  CreateArrayofIndices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateArrayofIndicesInputValues* inputValues);
  ~CreateArrayofIndices() noexcept;

  CreateArrayofIndices(const CreateArrayofIndices&) = delete;
  CreateArrayofIndices(CreateArrayofIndices&&) noexcept = delete;
  CreateArrayofIndices& operator=(const CreateArrayofIndices&) = delete;
  CreateArrayofIndices& operator=(CreateArrayofIndices&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateArrayofIndicesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
