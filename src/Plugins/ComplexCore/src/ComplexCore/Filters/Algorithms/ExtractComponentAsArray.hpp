#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ExtractComponentAsArrayInputValues
{
  bool MoveComponentsToNewArray;
  bool RemoveComponentsFromArray;
  int32 CompNumber;
  DataPath TempArrayPath;
  DataPath BaseArrayPath;
  DataPath NewArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ExtractComponentAsArray
{
public:
  ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractComponentAsArrayInputValues* inputValues);
  ~ExtractComponentAsArray() noexcept;

  ExtractComponentAsArray(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray(ExtractComponentAsArray&&) noexcept = delete;
  ExtractComponentAsArray& operator=(const ExtractComponentAsArray&) = delete;
  ExtractComponentAsArray& operator=(ExtractComponentAsArray&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractComponentAsArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
