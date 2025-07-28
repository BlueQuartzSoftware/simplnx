#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadHDF5DatasetInputValues inputValues;
  inputValues.ImportHdf5Object = filterArgs.value<ReadHDF5DatasetParameter::ValueType>(import_hdf5_object);
  return ReadHDF5Dataset(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadHDF5DatasetInputValues
{
  ReadHDF5DatasetParameter::ValueType ImportHdf5Object;
};

/**
 * @class ReadHDF5Dataset
 * @brief This algorithm implements support code for the ReadHDF5DatasetFilter
 */

class SIMPLNXCORE_EXPORT ReadHDF5Dataset
{
public:
  ReadHDF5Dataset(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadHDF5DatasetInputValues* inputValues);
  ~ReadHDF5Dataset() noexcept;

  ReadHDF5Dataset(const ReadHDF5Dataset&) = delete;
  ReadHDF5Dataset(ReadHDF5Dataset&&) noexcept = delete;
  ReadHDF5Dataset& operator=(const ReadHDF5Dataset&) = delete;
  ReadHDF5Dataset& operator=(ReadHDF5Dataset&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadHDF5DatasetInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
