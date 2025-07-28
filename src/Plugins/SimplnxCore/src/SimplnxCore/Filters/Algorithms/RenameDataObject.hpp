#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RenameDataObjectInputValues inputValues;
  inputValues.AllowOverwrite = filterArgs.value<BoolParameter::ValueType>(allow_overwrite);
  inputValues.NewName = filterArgs.value<StringParameter::ValueType>(new_name);
  inputValues.SourceDataObjectPath = filterArgs.value<DataPathSelectionParameter::ValueType>(source_data_object_path);
  return RenameDataObject(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RenameDataObjectInputValues
{
  BoolParameter::ValueType AllowOverwrite;
  StringParameter::ValueType NewName;
  DataPathSelectionParameter::ValueType SourceDataObjectPath;
};

/**
 * @class RenameDataObject
 * @brief This algorithm implements support code for the RenameDataObjectFilter
 */

class SIMPLNXCORE_EXPORT RenameDataObject
{
public:
  RenameDataObject(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameDataObjectInputValues* inputValues);
  ~RenameDataObject() noexcept;

  RenameDataObject(const RenameDataObject&) = delete;
  RenameDataObject(RenameDataObject&&) noexcept = delete;
  RenameDataObject& operator=(const RenameDataObject&) = delete;
  RenameDataObject& operator=(RenameDataObject&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RenameDataObjectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
