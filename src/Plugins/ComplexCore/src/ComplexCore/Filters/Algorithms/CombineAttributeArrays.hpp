#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace complex
{

struct COMPLEXCORE_EXPORT CombineAttributeArraysInputValues
{
  bool NormalizeData = {};
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  DataPath StackedDataArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT CombineAttributeArrays
{
public:
  CombineAttributeArrays(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineAttributeArraysInputValues* inputValues);
  ~CombineAttributeArrays() noexcept;

  CombineAttributeArrays(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays(CombineAttributeArrays&&) noexcept = delete;
  CombineAttributeArrays& operator=(const CombineAttributeArrays&) = delete;
  CombineAttributeArrays& operator=(CombineAttributeArrays&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineAttributeArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
