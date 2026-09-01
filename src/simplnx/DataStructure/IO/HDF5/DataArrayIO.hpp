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
 * @class DataArrayIO
 * @brief Reads and writes one numeric DataArray type.
 * @tparam T DataArray value type registered with this I/O factory.
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
   * @brief Imports one typed data array from an HDF5 dataset.
   * @tparam K Dataset value type to import.
   * @param dataStructure Destination data structure.
   * @param datasetReader Source HDF5 dataset.
   * @param dataArrayName Imported array name.
   * @param importId Imported object identifier.
   * @param err Receives zero or a negative import error code.
   * @param parentId Optional parent object identifier.
   * @param preflight True to create an EmptyDataStore placeholder.
   * @param warnings Receives placeholder and read warnings.
   *
   * Preflight retains shapes without materializing values. The eager path skips
   * a recovery placeholder after preserving its warnings.
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
      // A recovery placeholder has no inline values. Preserve warnings and skip it.
      err = 0;
      return;
    }
    DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(storeResult.value()), parentId);
    err = (data == nullptr) ? -400 : 0;
  }

  /**
   * @brief Replaces an imported array's placeholder data store.
   * @tparam K Dispatch type retained for this factory interface.
   * @param dataArray Imported DataArray to update.
   * @param dataPath Unused imported array path.
   * @param datasetReader Source HDF5 dataset.
   * @return Read warnings or errors.
   * @pre dataArray is non-null.
   *
   * The current implementation reads the factory's T store type. K does not
   * select the read type.
   */
  template <typename K>
  static Result<> importDataStore(data_type* dataArray, const DataPath& dataPath, const nx::core::HDF5::DatasetIO& datasetReader)
  {
    auto storeResult = DataStoreIO::ReadDataStoreIntoMemory<T>(datasetReader);
    Result<> result;
    result.m_Warnings = std::move(storeResult.warnings());
    if(storeResult.value() == nullptr)
    {
      // A recovery placeholder has no inline values. Preserve warnings and skip it.
      return result;
    }
    dataArray->setDataStore(std::move(storeResult.value()));
    return result;
  }

  /**
   * @brief Materializes a deferred imported DataArray.
   * @param dataStructure Destination data structure.
   * @param dataPath Imported array path.
   * @param parentGroupReader HDF5 group that owns the dataset.
   * @return Read warnings or errors.
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
   * @brief Imports a DataArray from its HDF5 dataset.
   * @param dataStructureReader Destination reader context.
   * @param parentGroup HDF5 group that owns the dataset.
   * @param dataArrayName Dataset and array name.
   * @param importId Imported object identifier.
   * @param parentId Optional parent object identifier.
   * @param useEmptyDataStore True for metadata-only import.
   * @return Import warnings or errors.
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

    // The importable attribute excludes objects that the writer marked unavailable.
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
   * @brief Writes a DataArray and its HDF5 attributes.
   * @param dataStructureWriter Writer that supplies options.
   * @param dataArray Source array.
   * @param parentGroup Destination HDF5 group.
   * @param importable Stored importable state.
   * @return Write warnings or errors.
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

  DataObject::Type getDataType() const override
  {
    return DataObject::Type::DataArray;
  }

  std::string getTypeName() const override
  {
    return data_type::GetTypeName();
  }

  /**
   * @brief Writes a DataObject after verifying the handled array type.
   * @param dataStructureWriter Writer that supplies options.
   * @param dataObject Object to write.
   * @param parentWriter Destination HDF5 group.
   * @return Type-validation or write errors.
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
