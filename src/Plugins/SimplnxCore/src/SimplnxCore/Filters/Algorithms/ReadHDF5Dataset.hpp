#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"

namespace nx::core
{

/**
 * @struct ReadHDF5DatasetInputValues
 * @brief Stores the HDF5 file, parent path, and selected datasets.
 */
struct SIMPLNXCORE_EXPORT ReadHDF5DatasetInputValues
{
  ReadHDF5DatasetParameter::ValueType ImportHdf5Object;
};

/**
 * @class ReadHDF5Dataset
 * @brief Reads one or more datasets from an HDF5 file into the DataStructure.
 *
 * FillDataArray performs each dataset transfer through the HDF5 support layer.
 * The algorithm checks cancellation only between selected datasets.
 */
class SIMPLNXCORE_EXPORT ReadHDF5Dataset
{
public:
  /**
   * @brief Creates an HDF5 dataset reader.
   * @param dataStructure Receives imported DataArrays.
   * @param mesgHandler Is retained but not used.
   * @param shouldCancel Stops before later datasets when true.
   * @param inputValues Specifies the file and datasets. The caller must keep
   * this object alive for the reader lifetime.
   */
  ReadHDF5Dataset(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadHDF5DatasetInputValues* inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadHDF5Dataset() noexcept;

  ReadHDF5Dataset(const ReadHDF5Dataset&) = delete;
  ReadHDF5Dataset(ReadHDF5Dataset&&) noexcept = delete;
  ReadHDF5Dataset& operator=(const ReadHDF5Dataset&) = delete;
  ReadHDF5Dataset& operator=(ReadHDF5Dataset&&) noexcept = delete;

  /**
   * @brief Imports selected numeric datasets in selection order.
   * @return File, type, or dataset-transfer error, or success after cancellation.
   *
   * Cancellation or an error can retain datasets imported before the current dataset.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadHDF5DatasetInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
