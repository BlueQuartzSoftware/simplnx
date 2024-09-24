#pragma once

#include "DataStructureWriter.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/IO/HDF5/EmptyDataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"

#include <vector>

namespace nx::core::HDF5
{
/**
 * @brief The DataArrayIO class serves as the basis for reading and writing DataArrays from HDF5
 */
template <typename T>
class DataArrayIO : public IDataIO
{
public:
  using data_type = DataArray<T>;
  using store_type = AbstractDataStore<T>;

  DataArrayIO() = default;
  ~DataArrayIO() noexcept override = default;

  /**
   * @brief Creates and imports a DataArray based on the provided DatasetIO
   * @param dataStructure
   * @param datasetReader
   * @param dataArrayName
   * @param importId
   * @param err
   * @param parentId
   * @param preflight
   */
  template <typename K>
  static void importDataArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& datasetReader, const std::string dataArrayName, DataObject::IdType importId,
                              nx::core::HDF5::ErrorType& err, const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    std::unique_ptr<AbstractDataStore<K>> dataStore =
        preflight ? std::unique_ptr<AbstractDataStore<K>>(EmptyDataStoreIO::ReadDataStore<K>(datasetReader)) : std::unique_ptr<AbstractDataStore<K>>(DataStoreIO::ReadDataStore<K>(datasetReader));
    DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
    err = (data == nullptr) ? -400 : 0;
  }

  /**
   * @brief Attempts to read the DataArray from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param dataArrayName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& dataArrayName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override
  {
    auto datasetReaderResult = parentGroup.openDataset(dataArrayName);
    if(datasetReaderResult.invalid())
    {
      std::string ss = fmt::format("Could not open data set '{}' which is a child of '{}'", dataArrayName, parentGroup.getName());
      return MakeErrorResult(-900, ss);
    }
    auto datasetReader = std::move(datasetReaderResult.value());

    auto type = datasetReader.getType();
    const std::string typeStr = type.string();

    std::string dataTypeStr;
    datasetReader.readAttribute(Constants::k_ObjectTypeTag, dataTypeStr);
    const bool isBoolArray = (dataTypeStr.compare("DataArray<bool>") == 0);

    // Check ability to import the data
    int32 importable = 0;
    datasetReader.readAttribute(Constants::k_ImportableTag, importable);
    if(importable == 0)
    {
      return {};
    }

    int32 err = 0;

    {
      if(typeStr.compare("Float32") == 0)
      {
        importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(typeStr.compare("Float64") == 0)
      {
        importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<int8>") == 0)
      {
        importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<int16>") == 0)
      {
        importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<int32>") == 0)
      {
        importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<int64>") == 0)
      {
        importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<bool>") == 0)
      {
        importDataArray<bool>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<uint8>") == 0)
      {
        importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<uint16>") == 0)
      {
        importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<uint32>") == 0)
      {
        importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else if(dataTypeStr.compare("DataArray<uint64>") == 0)
      {
        importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else
      {
        err = -777;
      }
    }

    if(err < 0)
    {
      return MakeErrorResult(err, fmt::format("Error importing dataset from HDF5 file. DataArray name '{}' that is a child of '{}'", dataArrayName, parentGroup.getName()));
    }

    return {};
  }

  /**
   * @brief Attempts to write a DataArray to HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureWriter
   * @param dataArray
   * @param parentGroup
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const nx::core::DataArray<T>& dataArray, group_writer_type& parentGroup, bool importable) const
  {
    auto datasetWriterResult = parentGroup.createDataset(dataArray.getName());
    if(datasetWriterResult.invalid())
    {
      return ConvertResult(std::move(datasetWriterResult));
    }
    auto datasetWriter = std::move(datasetWriterResult.value());

    Result<> result = DataStoreIO::WriteDataStore<T>(datasetWriter, dataArray.getDataStoreRef());
    if(result.invalid())
    {
      return result;
    }

    return WriteObjectAttributes(dataStructureWriter, dataArray, datasetWriter, importable);
  }

  /**
   * @brief Returns the target DataObject::Type for this IO class.
   * @return DataObject::Type
   */
  DataObject::Type getDataType() const override
  {
    return DataObject::Type::DataArray;
  }

  /**
   * @brief Returns the target DataObject type name for this IO class.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return data_type::GetTypeName();
  }

  /**
   * @brief Attempts to write the DataArray to HDF5.
   * Returns an error if the provided DataObject could not be cast to the corresponding DataArray type.
   * Otherwise, this method returns writeData(...)
   * @param dataStructructureWriter
   * @param dataObject
   * @param parentWriter
   * @return Result<>
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override
  {
    return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
  }

  DataArrayIO(const DataArrayIO& other) = delete;
  DataArrayIO(DataArrayIO&& other) = delete;
  DataArrayIO& operator=(const DataArrayIO& rhs) = delete;
  DataArrayIO& operator=(DataArrayIO&& rhs) = delete;
};

using Int8ArrayIO = DataArrayIO<int8>;
using Int16ArrayIO = DataArrayIO<int16>;
using Int32ArrayIO = DataArrayIO<int32>;
using Int64ArrayIO = DataArrayIO<int64>;

using UInt8ArrayIO = DataArrayIO<uint8>;
using UInt16ArrayIO = DataArrayIO<uint16>;
using UInt32ArrayIO = DataArrayIO<uint32>;
using UInt64ArrayIO = DataArrayIO<uint64>;

using BoolArrayIO = DataArrayIO<bool>;
using Float32ArrayIO = DataArrayIO<float32>;
using Float64ArrayIO = DataArrayIO<float64>;
} // namespace nx::core::HDF5
