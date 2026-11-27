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

#include <iterator>
#include <vector>

namespace nx::core::hdf5
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

  using StoreType = AbstractDataStore<T>;

  DataArrayIO() = default;

  ~DataArrayIO() noexcept override = default;

  /**
   * @brief Imports one typed data array from an HDF5 dataset.
   * @tparam K Dataset value type to import.
   * @param dataStructure Destination data structure.
   * @param datasetReader Source HDF5 dataset.
   * @param dataArrayName Imported array name.
   * @param importId Imported object identifier.
   * @param parentId Optional parent object identifier.
   * @param preflight True to create an EmptyDataStore placeholder.
   * @return Planning, insertion, or value-read errors and warnings.
   *
   * Preflight retains shapes without materializing values. The eager path skips
   * a recovery placeholder after preserving its warnings.
   */
  template <typename K>
  static Result<> ImportDataArray(DataStructure& dataStructure, const nx::core::hdf5::DatasetIO& datasetReader, const std::string& dataArrayName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    if(preflight)
    {
      auto shapesResult = empty_data_store_io::ReadShapes(datasetReader);
      if(shapesResult.invalid())
      {
        for(auto& error : shapesResult.errors())
        {
          error.message = fmt::format("Cannot read numeric metadata for dataset '{}': {}", datasetReader.getObjectPath(), error.message);
        }
        return ConvertResult(std::move(shapesResult));
      }
      auto [tupleShape, componentShape] = std::move(shapesResult.value());
      auto warnings = std::move(shapesResult.warnings());
      auto plannedResult = ConvertResult(DataArray<K>::ImportPlanned(dataStructure, dataArrayName, importId, tupleShape, componentShape, "", parentId));
      plannedResult.warnings().insert(plannedResult.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      return plannedResult;
    }

    auto storeResult = data_store_io::ReadDataStoreIntoMemory<K>(datasetReader);
    if(storeResult.invalid())
    {
      return ConvertResult(std::move(storeResult));
    }
    Result<> result;
    result.warnings() = std::move(storeResult.warnings());
    if(storeResult.value() == nullptr)
    {
      // A recovery placeholder has no inline values. Preserve warnings and skip it.
      return result;
    }
    DataArray<K>* dataPtr = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(storeResult.value()), parentId);
    if(dataPtr == nullptr)
    {
      auto errorResult = MakeErrorResult(-400, fmt::format("Cannot import numeric dataset '{}' into the DataStructure.", datasetReader.getObjectPath()));
      errorResult.warnings() = std::move(result.warnings());
      return errorResult;
    }
    return result;
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
  static Result<> ImportDataStore(data_type* dataArray, const DataPath& dataPath, const nx::core::hdf5::DatasetIO& datasetReader)
  {
    auto storeResult = data_store_io::ReadDataStoreIntoMemory<T>(datasetReader);
    if(storeResult.invalid())
    {
      return ConvertResult(std::move(storeResult));
    }
    Result<> result;
    result.m_warnings = std::move(storeResult.warnings());
    if(storeResult.value() == nullptr)
    {
      // A recovery placeholder has no inline values. Preserve warnings and skip it.
      return result;
    }
    auto replaceResult = dataArray->setDataStore(std::move(storeResult.value()));
    replaceResult.warnings().insert(replaceResult.warnings().begin(), std::make_move_iterator(result.warnings().begin()), std::make_move_iterator(result.warnings().end()));
    if(replaceResult.invalid())
    {
      for(auto& error : replaceResult.errors())
      {
        error.message = fmt::format("Cannot install the imported numeric store at path '{}': {}", dataPath.toString(), error.message);
      }
    }
    return replaceResult;
  }

  /**
   * @brief Materializes a deferred imported DataArray.
   * @param dataStructure Destination data structure.
   * @param dataPath Imported array path.
   * @param parentGroupReader HDF5 group that owns the dataset.
   * @return Read warnings or errors.
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const GroupReaderType& parentGroupReader) const override
  {
    if(!dataStructure.containsData(dataPath))
    {
      return MakeErrorResult(-150200, fmt::format("Imported DataStructure Object at path '{}' does not exist.", dataPath.toString()));
    }

    auto* dataArrayPtr = dataStructure.getDataAs<data_type>(dataPath);
    if(dataArrayPtr == nullptr)
    {
      return MakeErrorResult(-150201, fmt::format("Imported DataStructure Object at path '{}' is not of the expected type.", dataPath.toString()));
    }

    auto datasetReader = parentGroupReader.openDataset(dataPath.getTargetName());
    auto dataTypeStrResult = datasetReader.readStringAttribute(constants::k_ObjectTypeTag);
    if(dataTypeStrResult.invalid())
    {
      return ConvertResult(std::move(dataTypeStrResult));
    }
    std::string dataTypeStr = std::move(dataTypeStrResult.value());
    const bool isBoolArray = dataTypeStr == "DataArray<bool>";

    auto typeResult = datasetReader.getDataType();
    if(typeResult.invalid())
    {
      return ConvertResult(std::move(typeResult));
    }
    const auto type = typeResult.value();
    switch(type)
    {
    case DataType::float32:
      return ImportDataStore<float32>(dataArrayPtr, dataPath, datasetReader);
    case DataType::float64:
      return ImportDataStore<float64>(dataArrayPtr, dataPath, datasetReader);
    case DataType::int8:
      return ImportDataStore<int8>(dataArrayPtr, dataPath, datasetReader);
    case DataType::int16:
      return ImportDataStore<int16>(dataArrayPtr, dataPath, datasetReader);
    case DataType::int32:
      return ImportDataStore<int32>(dataArrayPtr, dataPath, datasetReader);
    case DataType::int64:
      return ImportDataStore<int64>(dataArrayPtr, dataPath, datasetReader);
    case DataType::uint8: {
      if(isBoolArray)
      {
        return ImportDataStore<bool>(dataArrayPtr, dataPath, datasetReader);
      }

      return ImportDataStore<uint8>(dataArrayPtr, dataPath, datasetReader);
    }
    break;
    case DataType::uint16:
      return ImportDataStore<uint16>(dataArrayPtr, dataPath, datasetReader);
    case DataType::uint32:
      return ImportDataStore<uint32>(dataArrayPtr, dataPath, datasetReader);
    case DataType::uint64:
      return ImportDataStore<uint64>(dataArrayPtr, dataPath, datasetReader);
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
  Result<> readData(DataStructureReader& dataStructureReader, const GroupReaderType& parentGroup, const std::string& dataArrayName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override
  {
    auto datasetReader = parentGroup.openDataset(dataArrayName);

    auto typeResult = datasetReader.getDataType();
    if(typeResult.invalid())
    {
      return ConvertResult(std::move(typeResult));
    }
    const auto type = typeResult.value();

    std::string dataTypeStr;
    auto dataTypeStrResult = datasetReader.readStringAttribute(constants::k_ObjectTypeTag);
    if(dataTypeStrResult.invalid())
    {
      return ConvertResult(std::move(dataTypeStrResult));
    }
    dataTypeStr = std::move(dataTypeStrResult.value());
    const bool isBoolArray = (dataTypeStr == "DataArray<bool>");

    // The importable attribute excludes objects that the writer marked unavailable.
    auto importableResult = datasetReader.readScalarAttribute<int32>(constants::k_ImportableTag);
    if(importableResult.invalid())
    {
      return ConvertResult(std::move(importableResult));
    }
    const int32 importable = importableResult.value();
    auto importableWarnings = std::move(importableResult.warnings());
    if(importable == 0)
    {
      Result<> result;
      result.warnings() = std::move(importableWarnings);
      return result;
    }

    Result<> result;
    switch(type)
    {
    case DataType::float32:
      result = ImportDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::float64:
      result = ImportDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int8:
      result = ImportDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int16:
      result = ImportDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int32:
      result = ImportDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int64:
      result = ImportDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint8: {
      if(isBoolArray)
      {
        result = ImportDataArray<bool>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
        break;
      }
      result = ImportDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    }
    case DataType::uint16:
      result = ImportDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint32:
      result = ImportDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint64:
      result = ImportDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    default:
      result = MakeErrorResult(-777, fmt::format("Cannot import numeric dataset '{}' because its type is unsupported.", datasetReader.getObjectPath()));
      break;
    }
    result.warnings().insert(result.warnings().begin(), std::make_move_iterator(importableWarnings.begin()), std::make_move_iterator(importableWarnings.end()));
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
  Result<> writeData(DataStructureWriter& dataStructureWriter, const nx::core::DataArray<T>& dataArray, GroupWriterType& parentGroup, bool importable) const
  {
    auto datasetWriter = parentGroup.createDataset(dataArray.getName());
    datasetWriter.setCompressionLevel(dataStructureWriter.getWriteOptions().compression_level);
    Result<> storeResult = data_store_io::WriteDataStore<T>(datasetWriter, dataArray.getDataStoreRef());
    if(storeResult.invalid())
    {
      storeResult.errors().push_back({-43255, fmt::format("Error writing data array '{}' to hdf5 file.", dataArray.getName())});
      return storeResult;
    }

    auto storeWarnings = std::move(storeResult.warnings());
    auto attributeResult = WriteObjectAttributes(dataStructureWriter, dataArray, datasetWriter, importable);
    attributeResult.warnings().insert(attributeResult.warnings().begin(), std::make_move_iterator(storeWarnings.begin()), std::make_move_iterator(storeWarnings.end()));
    return attributeResult;
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
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const override
  {
    return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
  }

  DataArrayIO(const DataArrayIO& other) = delete;
  DataArrayIO(DataArrayIO&& other) = delete;
  DataArrayIO& operator=(const DataArrayIO& rhs) = delete;
  DataArrayIO& operator=(DataArrayIO&& rhs) = delete;
};

using Int8ArrayIoType = DataArrayIO<int8>;
using Int16ArrayIoType = DataArrayIO<int16>;
using Int32ArrayIoType = DataArrayIO<int32>;
using Int64ArrayIoType = DataArrayIO<int64>;

using UInt8ArrayIoType = DataArrayIO<uint8>;
using UInt16ArrayIoType = DataArrayIO<uint16>;
using UInt32ArrayIoType = DataArrayIO<uint32>;
using UInt64ArrayIoType = DataArrayIO<uint64>;

using BoolArrayIoType = DataArrayIO<bool>;
using Float32ArrayIoType = DataArrayIO<float32>;
using Float64ArrayIoType = DataArrayIO<float64>;
} // namespace nx::core::hdf5
