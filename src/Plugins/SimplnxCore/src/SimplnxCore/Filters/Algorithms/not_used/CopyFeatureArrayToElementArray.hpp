#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyFeatureArrayToElementArrayInputValues inputValues;
  inputValues.CreatedArraySuffix = filterArgs.value<StringParameter::ValueType>(created_array_suffix);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(feature_ids_path);
  inputValues.SelectedFeatureArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(selected_feature_array_paths);
  return CopyFeatureArrayToElementArray(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayInputValues
{
  StringParameter::ValueType CreatedArraySuffix;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  MultiArraySelectionParameter::ValueType SelectedFeatureArrayPaths;
};

/**
 * @class CopyFeatureArrayToElementArray
 * @brief This algorithm implements support code for the CopyFeatureArrayToElementArrayFilter
 */

class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArray
{
public:
  CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 CopyFeatureArrayToElementArrayInputValues* inputValues);
  ~CopyFeatureArrayToElementArray() noexcept;

  CopyFeatureArrayToElementArray(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray(CopyFeatureArrayToElementArray&&) noexcept = delete;
  CopyFeatureArrayToElementArray& operator=(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray& operator=(CopyFeatureArrayToElementArray&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyFeatureArrayToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
