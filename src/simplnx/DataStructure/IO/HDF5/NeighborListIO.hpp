#pragma once

#include "DataStructureReader.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include <vector>

namespace nx::core
{
namespace HDF5
{

/**
 * @class NeighborListIO
 * @brief Reads and writes one NeighborList value type.
 * @tparam T Neighbor value type registered with this I/O factory.
 */
template <typename T>
class NeighborListIO : public IDataIO
{
public:
  using data_type = NeighborList<T>;

  using store_type = typename data_type::store_type;

  using shared_vector_type = typename data_type::SharedVectorType;

  NeighborListIO() = default;

  ~NeighborListIO() noexcept override = default;

  /**
   * @brief Reads packed neighbor values and their companion counts.
   * @param parentGroup HDF5 group that owns both datasets.
   * @param dataReader Packed neighbor-value dataset.
   * @param useEmptyDataStore True for metadata-only import.
   * @param warnings Receives recovery-placeholder warnings.
   * @return List store, or nullptr for a failure or recovery placeholder.
   *
   * The NumNeighbors companion array maps flat values to ragged tuple vectors.
   * Metadata import returns EmptyListStore until finishImportingData().
   */
  static std::shared_ptr<store_type> ReadHdf5Data(const nx::core::HDF5::GroupIO& parentGroup, const nx::core::HDF5::DatasetIO& dataReader, bool useEmptyDataStore, std::vector<Warning>& warnings)
  {
    try
    {
      std::string numNeighborsName;
      auto numNeighborsNameResult = dataReader.readStringAttribute("Linked NumNeighbors Dataset");
      if(numNeighborsNameResult.invalid())
      {
        return {};
      }
      numNeighborsName = std::move(numNeighborsNameResult.value());
      auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);

      if(useEmptyDataStore)
      {
        auto tupleDimsResult = numNeighborsReader.readVectorAttribute<usize>("TupleDimensions");
        if(tupleDimsResult.invalid())
        {
          return nullptr;
        }

        return std::make_shared<EmptyListStore<T>>(tupleDimsResult.value());
      }

      auto numNeighborsResult = DataStoreIO::ReadDataStoreIntoMemory<int32>(numNeighborsReader);
      for(auto&& warning : numNeighborsResult.warnings())
      {
        warnings.push_back(std::move(warning));
      }
      if(numNeighborsResult.value() == nullptr)
      {
        // A placeholder count array cannot define ragged-list boundaries.
        return nullptr;
      }
      auto& numNeighborsStore = *numNeighborsResult.value();

      auto flatDataStorePtr = dataReader.template readAsDataStore<T>();
      if(flatDataStorePtr == nullptr)
      {
        throw std::runtime_error(fmt::format("Error reading neighbor list from DataStore from HDF5 at '{}' called '{}'", dataReader.getFilePath().string(), dataReader.getName()));
      }

      const AbstractDataStore<T>& flatDataStore = *flatDataStorePtr.get();
      if(flatDataStore.empty())
      {
        throw std::runtime_error(fmt::format("Error reading neighbor list from DataStore from HDF5 at '{}' called '{}'", dataReader.getFilePath().string(), dataReader.getName()));
      }

      usize offset = 0;
      const auto numTuples = numNeighborsStore.getNumberOfTuples();
      // The higher import layer selects out-of-core stores. This branch
      // materializes an in-memory ListStore.
      auto listStorePtr = std::make_shared<ListStore<T>>(numNeighborsStore.getTupleShape());
      AbstractListStore<T>& listStore = *listStorePtr.get();
      for(usize i = 0; i < numTuples; i++)
      {
        const auto numNeighbors = numNeighborsStore[i];
        std::vector<T> vector(numNeighbors);

        size_t neighborListStart = offset;
        size_t neighborListEnd = offset + numNeighbors;
        vector.assign(flatDataStore.begin() + neighborListStart, flatDataStore.begin() + neighborListEnd);
        offset += numNeighbors;
        listStore.setList(i, vector);
      }

      return listStorePtr;
    } catch(const std::exception& e)
    {
      std::cout << "Cannot Read Neighborlist Dataset at path '" << dataReader.getObjectPath() << "' with error '" << e.what() << "'" << std::endl;
      return nullptr;
    }
  }

  /**
   * @brief Imports a NeighborList from HDF5.
   * @param dataStructureReader Destination reader context.
   * @param parentGroup HDF5 group that owns the datasets.
   * @param objectName NeighborList name.
   * @param importId Imported object identifier.
   * @param parentId Optional parent object identifier.
   * @param useEmptyDataStore True for metadata-only import.
   * @return Import warnings or errors.
   */
  Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override
  {
    auto datasetReader = parentGroup.openDataset(objectName);
    std::vector<Warning> warnings;
    auto listStorePtr = ReadHdf5Data(parentGroup, datasetReader, useEmptyDataStore, warnings);

    Result<> result;
    result.m_Warnings = std::move(warnings);

    if(listStorePtr == nullptr && !result.m_Warnings.empty())
    {
      // A recovery placeholder cannot materialize a NeighborList.
      return result;
    }

    auto* dataObject = data_type::Import(dataStructureReader.getDataStructure(), objectName, importId, listStorePtr, parentId);
    if(dataObject == nullptr)
    {
      std::string ss = "Failed to import NeighborList from HDF5";
      return MakeErrorResult(-505, ss);
    }
    return result;
  }

  /**
   * @brief Materializes a deferred NeighborList import.
   * @param dataStructure Destination data structure.
   * @param dataPath Imported NeighborList path.
   * @param parentGroup HDF5 group that owns the datasets.
   * @return Read warnings or errors.
   *
   * The method reconstructs tuple vectors from packed values and companion
   * counts. A higher import layer selects an out-of-core store when applicable.
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroup) const override
  {
    if(!dataStructure.containsData(dataPath))
    {
      return MakeErrorResult(-150200, fmt::format("Imported DataStructure Object at path '{}' does not exist.", dataPath.toString()));
    }

    NeighborList<T>& neighborList = dataStructure.getDataRefAs<NeighborList<T>>(dataPath);
    auto dataReader = parentGroup.openDataset(dataPath.getTargetName());

    std::string numNeighborsName;
    auto numNeighborsNameResult = dataReader.readStringAttribute("Linked NumNeighbors Dataset");
    if(numNeighborsNameResult.invalid())
    {
      return {};
    }
    numNeighborsName = std::move(numNeighborsNameResult.value());

    // The companion count array maps flat values to per-tuple list boundaries.
    auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);
    auto numNeighborsResult = DataStoreIO::ReadDataStoreIntoMemory<int32>(numNeighborsReader);

    Result<> result;
    for(auto&& warning : numNeighborsResult.warnings())
    {
      result.m_Warnings.push_back(std::move(warning));
    }
    if(numNeighborsResult.value() == nullptr)
    {
      // A placeholder count array cannot define ragged-list boundaries.
      return result;
    }
    auto& numNeighborsStore = *numNeighborsResult.value();

    const auto numTuples = numNeighborsStore.getNumberOfTuples();
    const auto tupleShape = numNeighborsStore.getTupleShape();

    // The higher import layer selects out-of-core stores. This branch reads the
    // packed values and reconstructs an in-memory ListStore.
    auto flatDataStorePtr = dataReader.template readAsDataStore<T>();
    if(flatDataStorePtr == nullptr)
    {
      return MakeErrorResult(-150201, fmt::format("Imported DataStructure Object at path '{}' is not of the expected type.", dataPath.toString()));
    }
    AbstractDataStore<T>& flatDataStore = *(flatDataStorePtr.get());
    if(flatDataStore.empty())
    {
      throw std::runtime_error(fmt::format("Error reading neighbor list from DataStore from HDF5 at '{}' called '{}'", dataReader.getFilePath().string(), dataReader.getName()));
    }

    usize offset = 0;
    // This eager HDF5 path materializes an in-memory ListStore.
    auto listStorePtr = std::make_shared<ListStore<T>>(tupleShape);
    AbstractListStore<T>& listStore = *listStorePtr.get();
    for(usize i = 0; i < numTuples; i++)
    {
      const auto numNeighbors = numNeighborsStore[i];
      std::vector<T> vector(numNeighbors);

      size_t neighborListStart = offset;
      size_t neighborListEnd = offset + numNeighbors;
      vector.assign(flatDataStore.begin() + neighborListStart, flatDataStore.begin() + neighborListEnd);
      offset += numNeighbors;
      listStore.setList(i, vector);
    }

    neighborList.setStore(listStorePtr);
    return result;
  }

  /**
   * @brief Writes packed neighbor values and their companion counts.
   * @param dataStructureWriter Writer that supplies options.
   * @param neighborList Source NeighborList.
   * @param parentGroupWriter Destination HDF5 group.
   * @param importable Stored importable state.
   * @return Write warnings or errors.
   * @pre Each list size fits int32 and the packed value count fits usize.
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const NeighborList<T>& neighborList, group_writer_type& parentGroupWriter, bool importable) const
  {
    DataStructure tmp;

    // Store list lengths separately so the reader can rebuild ragged vectors.
    const auto neighborData = neighborList.getVectors();
    const usize arraySize = neighborData.size();
    auto* numNeighborsArray = Int32Array::CreateWithStore<Int32DataStore>(tmp, neighborList.getNumNeighborsArrayName(), std::vector<usize>{arraySize}, std::vector<usize>{1});
    auto& numNeighborsStore = numNeighborsArray->getDataStoreRef();
    usize totalItems = 0;
    for(usize i = 0; i < arraySize; i++)
    {
      const auto numNeighbors = neighborData[i].size();
      numNeighborsStore[i] = static_cast<int32>(numNeighbors);
      totalItems += numNeighbors;
    }

    DataArrayIO<int32> dataArrayIO;
    Result<> result = dataArrayIO.writeData(dataStructureWriter, *numNeighborsArray, parentGroupWriter, false);
    if(result.invalid())
    {
      return result;
    }

    DataStore<T> flattenedData(totalItems, static_cast<T>(0));
    usize offset = 0;
    for(const auto& segment : neighborData)
    {
      usize numElements = segment.size();
      if(numElements == 0)
      {
        continue;
      }
      const T* start = segment.data();
      for(usize i = 0; i < numElements; i++)
      {
        flattenedData[offset + i] = start[i];
      }
      offset += numElements;
    }

    // Neighbor values can be large. Apply the configured compression level to
    // the packed array as well as the companion counts.
    auto datasetWriter = parentGroupWriter.createDataset(neighborList.getName());
    datasetWriter.setCompressionLevel(dataStructureWriter.getWriteOptions().compressionLevel);
    result = DataStoreIO::WriteDataStore<T>(datasetWriter, flattenedData);
    if(result.invalid())
    {
      return result;
    }
    result = datasetWriter.writeStringAttribute("Linked NumNeighbors Dataset", neighborList.getNumNeighborsArrayName());
    if(result.invalid())
    {
      return result;
    }
    return WriteObjectAttributes(dataStructureWriter, neighborList, datasetWriter, importable);
  }

  /**
   * @brief Writes a DataObject after verifying the handled NeighborList type.
   * @param dataStructureWriter Writer that supplies options.
   * @param dataObject Object to write.
   * @param parentWriter Destination HDF5 group.
   * @return Type-validation or write errors.
   */
  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override
  {
    return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
  }

  DataObject::Type getDataType() const override
  {
    return DataObject::Type::NeighborList;
  }

  std::string getTypeName() const override
  {
    return data_type::GetTypeName();
  }

  NeighborListIO(const NeighborListIO& other) = delete;
  NeighborListIO(NeighborListIO&& other) = delete;
  NeighborListIO& operator=(const NeighborListIO& rhs) = delete;
  NeighborListIO& operator=(NeighborListIO&& rhs) = delete;
};

using Int8NeighborIO = NeighborListIO<int8>;
using Int16NeighborIO = NeighborListIO<int16>;
using Int32NeighborIO = NeighborListIO<int32>;
using Int64NeighborIO = NeighborListIO<int64>;

using UInt8NeighborIO = NeighborListIO<uint8>;
using UInt16NeighborIO = NeighborListIO<uint16>;
using UInt32NeighborIO = NeighborListIO<uint32>;
using UInt64NeighborIO = NeighborListIO<uint64>;

// using BoolNeighborIO = NeighborListIO<bool>;
using Float32NeighborIO = NeighborListIO<float32>;
using Float64NeighborIO = NeighborListIO<float64>;
} // namespace HDF5
} // namespace nx::core
