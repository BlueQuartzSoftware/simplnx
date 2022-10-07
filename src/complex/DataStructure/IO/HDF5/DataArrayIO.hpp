#pragma once

#include "DataStructureWriter.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/IO/HDF5/DataStoreIO.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/DataStructure/IO/HDF5/EmptyDataStoreIO.hpp"
#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

#include <vector>

namespace complex
{
namespace HDF5
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
  virtual ~DataArrayIO() noexcept = default;

  /**
   * @brief Creates and imports a DataArray based on the provided DatasetReader
   * @param dataStructure
   * @param datasetReader
   * @param dataArrayName
   * @param importId
   * @param err
   * @param parentId
   * @param preflight
   */
  template <typename K>
  static void importDataArray(DataStructure& dataStructure, const H5::DatasetReader& datasetReader, const std::string dataArrayName, DataObject::IdType importId, H5::ErrorType& err,
                              const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    std::unique_ptr<AbstractDataStore<K>> dataStore =
        preflight ? std::unique_ptr<AbstractDataStore<K>>(EmptyDataStoreIO::ReadDataStore<K>(datasetReader)) : std::unique_ptr<AbstractDataStore<K>>(DataStoreIO::ReadDataStore<K>(datasetReader));
    DataArray<K>* data = DataArray<K>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
    err = (data == nullptr) ? -400 : 0;
  }

  static Result<> WriteDataStore(H5::DatasetWriter& datasetWriter, const store_type& store)
  {
    if(!datasetWriter.isValid())
    {
      std::string ss = "Failed to write DataArray. DatasetWriter not valid";
      return MakeErrorResult(-1, ss);
    }

    // Consolidate the Tuple and Component Dims into a single array which is used
    // to write the entire data array to HDF5
    std::vector<hsize_t> h5dims;
    for(const auto& value : store.getTupleShape())
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }
    for(const auto& value : store.getComponentShape())
    {
      h5dims.push_back(static_cast<hsize_t>(value));
    }

    usize count = store.getSize();
    T* dataPtr = new T[count];
    for(usize i = 0; i < count; ++i)
    {
      dataPtr[i] = store[i];
    }
    herr_t err = datasetWriter.writeSpan(h5dims, nonstd::span<const T>{const_cast<T*>(dataPtr), count});
    if(err < 0)
    {
      std::string ss = "Failed to write DataStore span to Dataset";
      return MakeErrorResult(err, ss);
    }

    // Write shape attributes to the dataset
    auto tupleAttribute = datasetWriter.createAttribute(complex::H5::k_TupleShapeTag);
    err = tupleAttribute.writeVector({store.getTupleShape().size()}, store.getTupleShape());
    if(err < 0)
    {
      std::string ss = "Failed to write DataStore tuple shape property";
      return MakeErrorResult(err, ss);
    }

    auto componentAttribute = datasetWriter.createAttribute(complex::H5::k_ComponentShapeTag);
    err = componentAttribute.writeVector({store.getComponentShape().size()}, store.getComponentShape());
    if(err < 0)
    {
      std::string ss = "Failed to write DataStore component shape property";
      return MakeErrorResult(err, ss);
    }

    return {};
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
    if(!datasetReader.isValid())
    {
      std::string ss = "Could not open Dataset";
      return MakeErrorResult(-900, ss);
    }

    H5::Type type = datasetReader.getType();
    if(type == H5::Type::unknown)
    {
      std::string ss = "Invalid Dataset data type";
      return MakeErrorResult(-901, ss);
    }

    auto dataTypeAttribute = datasetReader.getAttribute(complex::Constants::k_ObjectTypeTag);
    const bool isBoolArray = (dataTypeAttribute.isValid() && dataTypeAttribute.readAsString().compare("DataArray<bool>") == 0);

    // Check importablility
    auto importableAttribute = datasetReader.getAttribute(complex::Constants::k_ImportableTag);
    if(importableAttribute.isValid() && importableAttribute.readAsValue<int32>() == 0)
    {
      return {};
    }

    H5::ErrorType err = 0;

    switch(type)
    {
    case H5::Type::float32:
      importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::float64:
      importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::int8:
      importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::int16:
      importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::int32:
      importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::int64:
      importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::uint8:
      if(isBoolArray)
      {
        importDataArray<bool>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      else
      {
        importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      }
      break;
    case H5::Type::uint16:
      importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::uint32:
      importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    case H5::Type::uint64:
      importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, useEmptyDataStore);
      break;
    default:
      err = -777;
      break;
    }

    if(err < 0)
    {
      std::string ss = "Error importing DataArray";
      return MakeErrorResult(err, ss);
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
  Result<> writeData(DataStructureWriter& dataStructureWriter, const DataArray<T>& dataArray, group_writer_type& parentGroup, bool importable) const
  {
    auto datasetWriter = parentGroup.createDatasetWriter(dataArray.getName());
    Result<> result = WriteDataStore(datasetWriter, dataArray.getDataStoreRef());
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
    auto* dataArray = dynamic_cast<const data_type*>(dataObject);
    if(dataArray == nullptr)
    {
      std::string ss = "Provided DataObject could not be cast to the target type";
      return MakeErrorResult(-800, ss);
    }

    return writeData(dataStructureWriter, *dataArray, parentWriter, true);
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
} // namespace HDF5
} // namespace complex
