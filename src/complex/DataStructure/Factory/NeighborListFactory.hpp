#pragma once

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/NeighborList.hpp"
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
/**
 * @class NeighborListFactory
 * @brief
 */
template <typename T>
class COMPLEX_EXPORT NeighborListFactory : public IDataFactory
{
public:
  NeighborListFactory()
  : IDataFactory()
  {
  }

  ~NeighborListFactory() override = default;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return NeighborList<T>::GetTypeName();
  }

   /**
   * @brief Creates and imports a NeighborList based on the provided DatasetReader
   * @param dataStructure
   * @param parentReader
   * @param datasetReader
   * @param dataArrayName
   * @param parentId
   * @param preflight
   */
  template <typename K>
  inline void importNeighborList(DataStructure& dataStructure, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader, const std::string& dataArrayName,
                                 DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool preflight)
  {
    using NeighborListType = NeighborList<K>;
    auto dataVector = NeighborListType::ReadHdf5Data(parentReader, datasetReader);
    NeighborListType::Import(dataStructure, dataArrayName, importId, dataVector, parentId);
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
                                                   const std::optional<DataObject::IdType>& parentId, bool preflight)
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
    case H5::Type::float32:
      importNeighborList<float32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::float64:
      importNeighborList<float64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::int8:
      importNeighborList<int8>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::int16:
      importNeighborList<int16>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::int32:
      importNeighborList<int32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::int64:
      importNeighborList<int64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::uint8:
      importNeighborList<uint8>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::uint16:
      importNeighborList<uint16>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::uint32:
      importNeighborList<uint32>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    case H5::Type::uint64:
      importNeighborList<uint64>(dataStructureReader.getDataStructure(), parentReader, datasetReader, dataArrayName, importId, parentId, preflight);
      break;
    default:
      err = -777;
      break;
    }

    return err;
  }
};

// Declare aliases
using UInt8NeighborFactory = NeighborListFactory<uint8>;
using UInt16NeighborFactory = NeighborListFactory<uint16>;
using UInt32NeighborFactory = NeighborListFactory<uint32>;
using UInt64NeighborFactory = NeighborListFactory<uint64>;

using Int8NeighborFactory = NeighborListFactory<int8>;
using Int16NeighborFactory = NeighborListFactory<int16>;
using Int32NeighborFactory = NeighborListFactory<int32>;
using Int64NeighborFactory = NeighborListFactory<int64>;

using USizeNeighborFactory = NeighborListFactory<usize>;

using Float32NeighborFactory = NeighborListFactory<float32>;
using Float64NeighborFactory = NeighborListFactory<float64>;

using BoolNeighborFactory = NeighborListFactory<bool>;

} // namespace H5
} // namespace complex
