#pragma once

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
class COMPLEX_EXPORT DataArrayFactory : public IDataFactory
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
   * @brief Creates and adds an HexahedralGeom to the provided DataStructure from
   * the target HDF5 ID.
   * @param dataStructureReader Active DataStructureReader
   * @param parentReader Wrapper around the parent HDF5 group.
   * @param groupReader Wrapper around an HDF5 group.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @return H5::ErrorType
   */
  H5::ErrorType readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId = {}) override
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
  H5::ErrorType readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId = {}) override
  {
    H5::ErrorType err = 0;
    H5::Type type = datasetReader.getType();
    if(type == H5::Type::unknown)
    {
      return -1;
    }

    std::string dataArrayName = datasetReader.getName();
    DataObject::IdType importId = ReadObjectId(datasetReader);

    switch(type)
    {
    case H5::Type::float32: {
      auto dataStore = DataStore<float>::readHdf5(datasetReader);
      DataArray<float>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::float64: {
      auto dataStore = DataStore<double>::readHdf5(datasetReader);
      DataArray<double>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::int8: {
      auto dataStore = DataStore<int8_t>::readHdf5(datasetReader);
      DataArray<int8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::int16: {
      auto dataStore = DataStore<int16_t>::readHdf5(datasetReader);
      DataArray<int16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::int32: {
      auto dataStore = DataStore<int32_t>::readHdf5(datasetReader);
      DataArray<int32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::int64: {
      auto dataStore = DataStore<int64_t>::readHdf5(datasetReader);
      DataArray<int64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::uint8: {
      auto dataStore = DataStore<uint8_t>::readHdf5(datasetReader);
      DataArray<uint8_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::uint16: {
      auto dataStore = DataStore<uint16_t>::readHdf5(datasetReader);
      DataArray<uint16_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::uint32: {
      auto dataStore = DataStore<uint32_t>::readHdf5(datasetReader);
      DataArray<uint32_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
    case H5::Type::uint64: {
      auto dataStore = DataStore<uint64_t>::readHdf5(datasetReader);
      DataArray<uint64_t>::Import(dataStructureReader.getDataStructure(), dataArrayName, importId, std::move(dataStore), parentId);
      break;
    }
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
