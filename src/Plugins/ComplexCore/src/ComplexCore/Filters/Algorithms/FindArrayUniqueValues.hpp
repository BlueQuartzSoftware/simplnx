#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindArrayUniqueValuesInputValues
{
  DataPath SelectedArrayPath;
  DataPath DestinationAttributeMatrix;
  bool UseMask;
  DataPath MaskArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindArrayUniqueValues
{
public:
  FindArrayUniqueValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindArrayUniqueValuesInputValues* inputValues);
  ~FindArrayUniqueValues() noexcept;

  FindArrayUniqueValues(const FindArrayUniqueValues&) = delete;
  FindArrayUniqueValues(FindArrayUniqueValues&&) noexcept = delete;
  FindArrayUniqueValues& operator=(const FindArrayUniqueValues&) = delete;
  FindArrayUniqueValues& operator=(FindArrayUniqueValues&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindArrayUniqueValuesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
