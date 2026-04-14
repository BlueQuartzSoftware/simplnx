#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the ReadHDF5Dataset algorithm.
 */
struct SIMPLNXCORE_EXPORT ReadHDF5DatasetInputValues
{
  ReadHDF5DatasetParameter::ValueType ImportHdf5Object; ///< HDF5 import configuration (file path, parent, dataset list).
};

/**
 * @class ReadHDF5Dataset
 * @brief Reads one or more datasets from an HDF5 file into the DataStructure.
 *
 * Iterates over the user-selected datasets, reads each via the HDF5 support library,
 * and populates the corresponding DataArray. Progress messages report the current
 * dataset being imported.
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
