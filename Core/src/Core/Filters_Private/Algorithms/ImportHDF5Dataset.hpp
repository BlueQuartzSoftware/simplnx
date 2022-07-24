#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportHDF5DatasetInputValues inputValues;

  inputValues.ImportHDF5File = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportHDF5File_Key);
  inputValues.SelectedAttributeMatrix = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);

  return ImportHDF5Dataset(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ImportHDF5DatasetInputValues
{
  <<<NOT_IMPLEMENTED>>> ImportHDF5File;
  /*[x]*/ DataPath SelectedAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ImportHDF5Dataset
{
public:
  ImportHDF5Dataset(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportHDF5DatasetInputValues* inputValues);
  ~ImportHDF5Dataset() noexcept;

  ImportHDF5Dataset(const ImportHDF5Dataset&) = delete;
  ImportHDF5Dataset(ImportHDF5Dataset&&) noexcept = delete;
  ImportHDF5Dataset& operator=(const ImportHDF5Dataset&) = delete;
  ImportHDF5Dataset& operator=(ImportHDF5Dataset&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportHDF5DatasetInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
