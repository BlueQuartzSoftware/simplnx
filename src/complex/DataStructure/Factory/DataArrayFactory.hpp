#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

#include <memory>
#include <numeric>
#include <optional>

namespace complex
{
namespace H5
{
template <typename T>
class DataArrayFactory : public IDataFactory
{
public:
  DataArrayFactory()
  : IDataFactory()
  {
  }

  ~DataArrayFactory() override = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return DataArray<T>::GetTypeName();
  }

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
  template <typename T>
  void importDataArray(DataStructure& dataStructure, const H5::DatasetReader& datasetReader, const std::string dataArrayName, DataObject::IdType importId, H5::ErrorType& err,
                       const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    DataArray<T>* data = nullptr;
    if(preflight)
    {
      auto dataStore = EmptyDataStore<T>::readHdf5(datasetReader);
      data = DataArray<T>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
    }
    else
    {
      auto dataStore = DataStore<T>::readHdf5(datasetReader);
      data = DataArray<T>::Import(dataStructure, dataArrayName, importId, std::move(dataStore), parentId);
    }
    err = (data == nullptr) ? -400 : 0;
  }

  /**
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Active DataStructureReader
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                            const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false) override
  {
    return -1;
  }

  /**
   * @brief Reads an HDF5 Dataset that makes up a DataStructure node.
   * @param dataStructureReader Active DataStructureReader
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param datasetReader Wrapper around the HDF5 Dataset.
   * @param parentId The HDF5 ID of the parent object.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                              const std::optional<DataObject::IdType>& parentId = {}, bool preflight = false) override
  {
    H5::ErrorType err = 0;
    H5::Type type = datasetReader.getType();
    if(type == H5::Type::unknown)
    {
      return -1;
    }

    std::string dataArrayName = datasetReader.getName();
    DataObject::IdType importId = ReadObjectId(datasetReader);

    // Check importablility
    auto importableAttribute = datasetReader.getAttribute(complex::Constants::k_ImportableTag);
    if(importableAttribute.isValid() && importableAttribute.readAsValue<int32>() == 0)
    {
      return 0;
    }

    switch(type)
    {
    case H5::Type::float32:
      importDataArray<float32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::float64:
      importDataArray<float64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::int8:
      importDataArray<int8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::int16:
      importDataArray<int16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::int32:
      importDataArray<int32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::int64:
      importDataArray<int64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::uint8:
      importDataArray<uint8>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::uint16:
      importDataArray<uint16>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::uint32:
      importDataArray<uint32>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    case H5::Type::uint64:
      importDataArray<uint64>(dataStructureReader.getDataStructure(), datasetReader, dataArrayName, importId, err, parentId, preflight);
      break;
    default:
      err = -777;
      break;
    }

    return err;
  }
};

// Declare aliases
using UInt8ArrayFactory = DataArrayFactory<uint8>;
using UInt16ArrayFactory = DataArrayFactory<uint16>;
using UInt32ArrayFactory = DataArrayFactory<uint32>;
using UInt64ArrayFactory = DataArrayFactory<uint64>;

using Int8ArrayFactory = DataArrayFactory<int8>;
using Int16ArrayFactory = DataArrayFactory<int16>;
using Int32ArrayFactory = DataArrayFactory<int32>;
using Int64ArrayFactory = DataArrayFactory<int64>;

using USizeArrayFactory = DataArrayFactory<usize>;

using Float32ArrayFactory = DataArrayFactory<float32>;
using Float64ArrayFactory = DataArrayFactory<float64>;

using BoolArrayFactory = DataArrayFactory<bool>;

} // namespace H5
} // namespace complex
