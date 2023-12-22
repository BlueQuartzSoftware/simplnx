#pragma once

#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"

namespace nx::core::HDF5
{
template <typename T>
class ScalarDataIO : public IDataIO
{
public:
  using data_type = ScalarData<T>;

  ScalarDataIO() = default;
  virtual ~ScalarDataIO() noexcept = default;

  /**
   * @brief Attempts to read the ScalarData from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param scalarName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& scalarName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override
  {
    auto datasetReader = parentGroup.openDataset(scalarName);
    std::array<T, 1> buffer{};
    bool result = datasetReader.readIntoSpan<T>(nonstd::span<T>{buffer});
    if(!result)
    {
      std::string ss = "Failed to read ScalarData. Incorrect number of values";
      return MakeErrorResult(-458, ss);
    }

    ScalarData<T>* scalar = ScalarData<T>::Import(dataStructureReader.getDataStructure(), scalarName, importId, buffer[0], parentId);
    if(scalar == nullptr)
    {
      std::string ss = "Failed to read ScalarData. No data imported";
      return MakeErrorResult(-459, ss);
    }

    return {};
  }

  /**
   * @brief Attempts to write an EdgeGeom to HDF5.
   * @param dataStructureWriter
   * @param scalarData
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const ScalarData<T>& scalarData, group_writer_type& parentGroup, bool importable) const
  {
    auto datasetWriter = parentGroup.createDatasetWriter(scalarData.getName());

    nx::core::HDF5::DatasetWriter::DimsType dims = {1};
    std::array<T, 1> dataVector = {scalarData.getValue()};
    Result<> h5Result = datasetWriter.writeSpan(dims, nonstd::span<const T>{dataVector});
    if(h5Result.invalid())
    {
      return MakeErrorResult(-460, "Failed to write ScalarData");
    }
    return WriteObjectAttributes(dataStructureWriter, scalarData, datasetWriter, importable);
  }

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a ScalarData.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override
  {
    return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
  }

  DataObject::Type getDataType() const override
  {
    return DataObject::Type::ScalarData;
  }

  std::string getTypeName() const override
  {
    return data_type::GetTypeName();
  }

  ScalarDataIO(const ScalarDataIO& other) = delete;
  ScalarDataIO(ScalarDataIO&& other) = delete;
  ScalarDataIO& operator=(const ScalarDataIO& rhs) = delete;
  ScalarDataIO& operator=(ScalarDataIO&& rhs) = delete;
};
} // namespace nx::core::HDF5
