#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrIDataFactory.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IArray.hpp"

#include <memory>
#include <numeric>
#include <optional>

namespace complex
{
namespace Zarr
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
  template <typename K>
  void importDataArray(Zarr::DataStructureReader& datasetReader, const FileVec::IArray<K>& fArray, DataObject::IdType importId, error_type& err, const std::optional<DataObject::IdType>& parentId,
                       bool preflight)
  {
    std::unique_ptr<AbstractDataStore<K>> dataStore =
        preflight ? std::unique_ptr<AbstractDataStore<K>>(EmptyDataStore<K>::ReadZarr(fArray)) : std::unique_ptr<AbstractDataStore<K>>(DataStore<K>::ReadZarr(fArray));
    DataArray<K>* data = DataArray<K>::Import(datasetReader.getDataStructure(), fArray.name(), importId, std::move(dataStore), parentId);
    err = (data == nullptr) ? -400 : 0;
  }

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target Zarr object.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent Zarr group.
   * @param baseReader Wrapper around an Zarr object reader.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @param preflight = false
   * @return error_type
   */
  error_type read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                  const std::optional<complex::DataObject::IdType>& parentId = {}, bool preflight = false) override
  {
    const FileVec::IArray<T>& fArray = dynamic_cast<const FileVec::IArray<T>&>(baseReader);
    // const FileVec::DataType type = fArray.header().dataType();

    error_type err = 0;

    DataObject::IdType importId = ReadObjectId(fArray);

    // Check importablility
    const nlohmann::json importableAttribute = fArray.attributes()[complex::Constants::k_ImportableTag];
    if(!importableAttribute.empty() && importableAttribute.get<int32>() == 0)
    {
      return 0;
    }

    importDataArray<T>(dataStructureReader, fArray, importId, err, parentId, preflight);

#if 0
    switch(type)
    {
    case FileVec::DataType::float32:
      importDataArray<float32>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::float64:
      importDataArray<float64>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::int8:
      importDataArray<int8>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::int16:
      importDataArray<int16>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::int32:
      importDataArray<int32>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::int64:
      importDataArray<int64>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::uint8:
      importDataArray<uint8>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::uint16:
      importDataArray<uint16>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::uint32:
      importDataArray<uint32>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::uint64:
      importDataArray<uint64>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    case FileVec::DataType::boolean:
      importDataArray<bool>(dataStructureReader, fArray, importId, err, parentId, preflight);
      break;
    default:
      err = -777;
      break;
    }
#endif

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

} // namespace Zarr
} // namespace complex
