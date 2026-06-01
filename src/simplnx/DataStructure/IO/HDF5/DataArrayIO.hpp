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
   * @param warnings Output vector to accumulate any warnings encountered (e.g. placeholder datasets)
   */
  template <typename K>
  static void importDataArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& datasetReader, const std::string dataArrayName, DataObject::IdType importId,
                              nx::core::HDF5::ErrorType& err, const std::optional<DataObject::IdType>& parentId, bool preflight, std::vector<Warning>& warnings)
  {
    if(preflight)
    {
      std::shared_ptr<AbstractDataStore<K>> dataStore(EmptyDataStoreIO::ReadDataStore<K>(datasetReader));
      DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
      err = (data == nullptr) ? -400 : 0;
      return;
    }

    auto storeResult = DataStoreIO::ReadDataStoreIntoMemory<K>(datasetReader);
    for(auto&& warning : storeResult.warnings())
    {
      warnings.push_back(std::move(warning));
    }
    if(storeResult.value() == nullptr)
    {
      // Placeholder detected — skip this array without error
      err = 0;
      return;
    }
    DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(storeResult.value()), parentId);
    err = (data == nullptr) ? -400 : 0;
  }

  /**
   * @brief Imports and replaces a DataArray's data store from an HDF5 dataset.
   * @tparam K The data type to import
   * @param dataArray The DataArray to update
   * @param dataPath The path to the DataArray
   * @param datasetReader The HDF5 dataset reader to read from
   * @return Result<> Result with any errors or warnings
   */
  template <typename K>
  static Result<> importDataStore(data_type* dataArray, const DataPath& dataPath, const nx::core::HDF5::DatasetIO& datasetReader)
  {
    auto storeResult = DataStoreIO::ReadDataStoreIntoMemory<T>(datasetReader);
    Result<> result;
    result.m_Warnings = std::move(storeResult.warnings());
    if(storeResult.value() == nullptr)
    {
      // Placeholder detected — propagate warnings, skip without error
      return result;
    }
    dataArray->setDataStore(std::move(storeResult.value()));
    return result;
  }

  /**
   * @brief Replaces the AbstractDataStore using data from the HDF5 dataset.
   * @param dataStructure
   * @param dataPath
   * @param dataStructureReader
   * @return Result<>
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroupReader) const override
  {
    if(!dataStructure.containsData(dataPath))
    {
      return MakeErrorResult(-150200, fmt::format("Imported DataStructure Object at path '{}' does not exist.", dataPath.toString()));
    }

    auto* dataArray = dataStructure.getDataAs<data_type>(dataPath);
    if(dataArray == nullptr)
    {
      return MakeErrorResult(-150201, fmt::format("Imported DataStructure Object at path '{}' is not of the expected type.", dataPath.toString()));
    }

    auto datasetReader = parentGroupReader.openDataset(dataPath.getTargetName());
    auto dataTypeStrResult = datasetReader.readStringAttribute(Constants::k_ObjectTypeTag);
    std::string dataTypeStr = std::move(dataTypeStrResult.value());
    const bool isBoolArray = dataTypeStr == "DataArray<bool>";

    auto typeResult = datasetReader.getDataType();
    const auto type = std::move(typeResult.value());
    switch(type)
    {
    case DataType::float32:
      return importDataStore<float32>(dataArray, dataPath, datasetReader);
    case DataType::float64:
      return importDataStore<float64>(dataArray, dataPath, datasetReader);
    case DataType::int8:
      return importDataStore<int8>(dataArray, dataPath, datasetReader);
    case DataType::int16:
      return importDataStore<int16>(dataArray, dataPath, datasetReader);
    case DataType::int32:
      return importDataStore<int32>(dataArray, dataPath, datasetReader);
    case DataType::int64:
      return importDataStore<int64>(dataArray, dataPath, datasetReader);
    case DataType::uint8: {
      if(isBoolArray)
      {
        return importDataStore<bool>(dataArray, dataPath, datasetReader);
      }
      else
      {
        return importDataStore<uint8>(dataArray, dataPath, datasetReader);
      }
    }
    break;
    case DataType::uint16:
      return importDataStore<uint16>(dataArray, dataPath, datasetReader);
    case DataType::uint32:
      return importDataStore<uint32>(dataArray, dataPath, datasetReader);
    case DataType::uint64:
      return importDataStore<uint64>(dataArray, dataPath, datasetReader);
    default:
      return MakeErrorResult(-150209, fmt::format("Undetermined DataArray type: '{}'", dataTypeStr));
    }
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
    auto datasetReader = parentGroup.openDataset(dataArrayName);

    auto typeResult = datasetReader.getDataType();
    const auto type = typeResult.value();

    std::string dataTypeStr;
    auto dataTypeStrResult = datasetReader.readStringAttribute(Constants::k_ObjectTypeTag);
    dataTypeStr = std::move(dataTypeStrResult.value());
    const bool isBoolArray = (dataTypeStr == "DataArray<bool>");

    // Check ability to import the data
    int32 importable = 0;
    auto importableResult = datasetReader.readScalarAttribute<int32>(Constants::k_ImportableTag);
    if(importableResult.valid())
    {
      importable = importableResult.value();
    }
    if(importable == 0)
    {
      return {};
    }

    int32 err = 0;
    std::vector<Warning> warnings;
    switch(type)
    {
    case DataType::float32:
      importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::float64:
      importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::int8:
      importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::int16:
      importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::int32:
      importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::int64:
      importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::uint8: {
      if(isBoolArray)
      {
        importDataArray<bool>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      }
      else
      {
        importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      }
    }
    break;
    case DataType::uint16:
      importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::uint32:
      importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    case DataType::uint64:
      importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore, warnings);
      break;
    default: {
      err = -777;
      break;
    }
    }

    if(err < 0)
    {
      auto result = MakeErrorResult(err, fmt::format("Error importing dataset from HDF5 file. DataArray name '{}' that is a child of '{}'", dataArrayName, parentGroup.getName()));
      result.m_Warnings = std::move(warnings);
      return result;
    }

    Result<> result;
    result.m_Warnings = std::move(warnings);
    return result;
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
    auto datasetWriter = parentGroup.createDataset(dataArray.getName());
    datasetWriter.setCompressionLevel(dataStructureWriter.getWriteOptions().compressionLevel);
    Result<> result = DataStoreIO::WriteDataStore<T>(datasetWriter, dataArray.getDataStoreRef());
    if(result.invalid())
    {
      result.errors().push_back({-43255, fmt::format("Error writing data array '{}' to hdf5 file.", dataArray.getName())});
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
