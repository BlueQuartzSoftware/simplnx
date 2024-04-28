#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CombineAttributeArraysInputValues
{
  bool NormalizeData = {};
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  DataPath StackedDataArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT CombineAttributeArrays
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

} // namespace nx::core
