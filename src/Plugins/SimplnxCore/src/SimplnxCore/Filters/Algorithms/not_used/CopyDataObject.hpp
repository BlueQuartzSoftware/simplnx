#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyDataObjectInputValues inputValues;
  inputValues.ExistingDataPath = filterArgs.value<MultiPathSelectionParameter::ValueType>(existing_data_path);
  inputValues.NewDataPath = filterArgs.value<DataGroupSelectionParameter::ValueType>(new_data_path);
  inputValues.NewPathSuffix = filterArgs.value<StringParameter::ValueType>(new_path_suffix);
  inputValues.UseNewParent = filterArgs.value<BoolParameter::ValueType>(use_new_parent);
  return CopyDataObject(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CopyDataObjectInputValues
{
  MultiPathSelectionParameter::ValueType ExistingDataPath;
  DataGroupSelectionParameter::ValueType NewDataPath;
  StringParameter::ValueType NewPathSuffix;
  BoolParameter::ValueType UseNewParent;
};

/**
 * @class CopyDataObject
 * @brief This algorithm implements support code for the CopyDataObjectFilter
 */

class SIMPLNXCORE_EXPORT CopyDataObject
{
public:
  CopyDataObject(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyDataObjectInputValues* inputValues);
  ~CopyDataObject() noexcept;

  CopyDataObject(const CopyDataObject&) = delete;
  CopyDataObject(CopyDataObject&&) noexcept = delete;
  CopyDataObject& operator=(const CopyDataObject&) = delete;
  CopyDataObject& operator=(CopyDataObject&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyDataObjectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
