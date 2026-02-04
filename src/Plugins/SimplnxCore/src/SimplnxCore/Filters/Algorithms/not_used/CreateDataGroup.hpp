#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateDataGroupInputValues inputValues;
  inputValues.DataObjectPath = filterArgs.value<DataGroupCreationParameter::ValueType>(data_object_path);
  return CreateDataGroup(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateDataGroupInputValues
{
  DataGroupCreationParameter::ValueType DataObjectPath;
};

/**
 * @class CreateDataGroup
 * @brief This algorithm implements support code for the CreateDataGroupFilter
 */

class SIMPLNXCORE_EXPORT CreateDataGroup
{
public:
  CreateDataGroup(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateDataGroupInputValues* inputValues);
  ~CreateDataGroup() noexcept;

  CreateDataGroup(const CreateDataGroup&) = delete;
  CreateDataGroup(CreateDataGroup&&) noexcept = delete;
  CreateDataGroup& operator=(const CreateDataGroup&) = delete;
  CreateDataGroup& operator=(CreateDataGroup&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateDataGroupInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
