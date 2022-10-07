#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"
#include "complex/DataStructure/ScalarData.hpp"

namespace complex
{
namespace HDF5
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
    auto dataVector = datasetReader.readAsVector<T>();
    if(dataVector.size() != 1)
    {
      std::string ss = "Failed to read ScalarData. Incorrect number of values";
      return MakeErrorResult(-458, ss);
    }

    T buffer = dataVector.front();
    ScalarData<T>* scalar = ScalarData<T>::Import(dataStructureReader.getDataStructure(), scalarName, importId, buffer, parentId);
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

    H5::DatasetWriter::DimsType dims = {1};
    std::array<T, 1> dataVector = {scalarData.getValue()};
    auto error = datasetWriter.writeSpan(dims, nonstd::span<const T>{dataVector});
    if(error == 0)
    {
      error = WriteObjectAttributes(dataStructureWriter, datasetWriter, importable);
    }
    return error;
  }

  /**
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a ScalarData.
   * Otherwise, this method returns writeData(...)
   * Return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override
  {
    auto* targetData = dynamic_cast<const data_type*>(dataObject);
    if(targetData == nullptr)
    {
      std::string ss = "Provided DataObject could not be cast to the target type";
      return MakeErrorResult(-800, ss);
    }

    return writeData(dataStructureWriter, *targetData, parentWriter, true);
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
} // namespace HDF5
} // namespace complex
