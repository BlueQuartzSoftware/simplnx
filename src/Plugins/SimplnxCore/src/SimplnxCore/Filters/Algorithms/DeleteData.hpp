#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiPathSelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DeleteDataInputValues inputValues;
  inputValues.RemovedDataPath = filterArgs.value<MultiPathSelectionParameter::ValueType>(removed_data_path);
  return DeleteData(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT DeleteDataInputValues
{
  MultiPathSelectionParameter::ValueType RemovedDataPath;
};

/**
 * @class DeleteData
 * @brief This algorithm implements support code for the DeleteDataFilter
 */

class SIMPLNXCORE_EXPORT DeleteData
{
public:
  DeleteData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DeleteDataInputValues* inputValues);
  ~DeleteData() noexcept;

  DeleteData(const DeleteData&) = delete;
  DeleteData(DeleteData&&) noexcept = delete;
  DeleteData& operator=(const DeleteData&) = delete;
  DeleteData& operator=(DeleteData&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DeleteDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
