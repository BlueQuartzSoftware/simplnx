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
   * @param parentId Optional parent object identifier.
   * @param preflight True to create an EmptyDataStore placeholder.
   * @return Planning, insertion, or value-read errors and warnings.
   *
   * Preflight retains shapes without materializing values. The eager path skips
   * a recovery placeholder after preserving its warnings.
   */
  template <typename K>
  static Result<> importDataArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& datasetReader, const std::string& dataArrayName, DataObject::IdType importId,
                                  const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    if(preflight)
    {
      auto shapesResult = EmptyDataStoreIO::ReadShapes(datasetReader);
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

    auto storeResult = DataStoreIO::ReadDataStoreIntoMemory<K>(datasetReader);
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
    DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(storeResult.value()), parentId);
    if(data == nullptr)
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
  static Result<> importDataStore(data_type* dataArray, const DataPath& dataPath, const nx::core::HDF5::DatasetIO& datasetReader)
  {
    auto storeResult = DataStoreIO::ReadDataStoreIntoMemory<T>(datasetReader);
    if(storeResult.invalid())
    {
      return ConvertResult(std::move(storeResult));
    }
    Result<> result;
    result.m_Warnings = std::move(storeResult.warnings());
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
    if(typeResult.invalid())
    {
      return ConvertResult(std::move(typeResult));
    }
    const auto type = typeResult.value();

    std::string dataTypeStr;
    auto dataTypeStrResult = datasetReader.readStringAttribute(Constants::k_ObjectTypeTag);
    if(dataTypeStrResult.invalid())
    {
      return ConvertResult(std::move(dataTypeStrResult));
    }
    dataTypeStr = std::move(dataTypeStrResult.value());
    const bool isBoolArray = (dataTypeStr == "DataArray<bool>");

    // The importable attribute excludes objects that the writer marked unavailable.
    auto importableResult = datasetReader.readScalarAttribute<int32>(Constants::k_ImportableTag);
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
      result = importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::float64:
      result = importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int8:
      result = importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int16:
      result = importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int32:
      result = importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::int64:
      result = importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint8: {
      if(isBoolArray)
      {
        result = importDataArray<bool>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
        break;
      }
      result = importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    }
    case DataType::uint16:
      result = importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint32:
      result = importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
      break;
    case DataType::uint64:
      result = importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, parentId, useEmptyDataStore);
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
  Result<> writeData(DataStructureWriter& dataStructureWriter, const nx::core::DataArray<T>& dataArray, group_writer_type& parentGroup, bool importable) const
  {
    auto datasetWriter = parentGroup.createDataset(dataArray.getName());
    datasetWriter.setCompressionLevel(dataStructureWriter.getWriteOptions().compressionLevel);
    Result<> storeResult = DataStoreIO::WriteDataStore<T>(datasetWriter, dataArray.getDataStoreRef());
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
