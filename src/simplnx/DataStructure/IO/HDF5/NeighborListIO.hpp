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
   * @brief Reads NeighborList<T> data from an HDF5 dataset.
   *
   * When useEmptyDataStore is true, only the TupleDimensions attribute from
   * the linked NumNeighbors dataset is read, and an EmptyListStore placeholder
   * is returned. The actual data is loaded later by finishImportingData().
   *
   * When useEmptyDataStore is false, the full flat data array is read from
   * HDF5, split into per-tuple vectors using the NumNeighbors companion
   * array, and packed into an in-memory ListStore.
   *
   * If the NumNeighbors companion array is a placeholder (element count
   * mismatch), warnings are accumulated and nullptr is returned. The caller
   * should treat this as a skip, not an error.
   *
   * @param parentGroup The HDF5 group containing the dataset and its companion
   * @param dataReader The HDF5 dataset containing the flat packed neighbor data
   * @param useEmptyDataStore If true, return an EmptyListStore placeholder
   * @param warnings Output vector to accumulate any warnings encountered
   * @return std::shared_ptr<store_type> The created list store, or nullptr on error/placeholder
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
        // NumNeighbors is a placeholder — cannot populate NeighborList
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
      // In-core branch of the import pipeline: allocate a plain in-memory
      // ListStore. The OOC branch is intercepted upstream by the data store
      // import handler.
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
   * @brief Attempts to read the NeighborList<T> from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param objectName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
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
      // Placeholder detected — skip this NeighborList, propagate warnings
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
   * @brief Replaces the placeholder AbstractListStore with real data from the
   * HDF5 dataset. This is the "backfill" step called after preflight when the
   * DataStructure was initially loaded with empty stores.
   *
   * Reads the flat data array from HDF5 and scatters it into per-tuple vectors
   * in an in-memory ListStore. OOC format decisions for imported data are
   * handled at a higher level by the backfill strategy.
   *
   * @param dataStructure The DataStructure containing the NeighborList to populate
   * @param dataPath Path to the NeighborList in the DataStructure
   * @param parentGroup The HDF5 group containing the dataset
   * @return Result<>
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

    // Read the "NumNeighbors" companion array, which stores the per-tuple
    // neighbor count used to interpret the flat packed data array.
    auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);
    auto numNeighborsResult = DataStoreIO::ReadDataStoreIntoMemory<int32>(numNeighborsReader);

    Result<> result;
    for(auto&& warning : numNeighborsResult.warnings())
    {
      result.m_Warnings.push_back(std::move(warning));
    }
    if(numNeighborsResult.value() == nullptr)
    {
      // NumNeighbors is a placeholder — cannot populate NeighborList, propagate warnings
      return result;
    }
    auto& numNeighborsStore = *numNeighborsResult.value();

    const auto numTuples = numNeighborsStore.getNumberOfTuples();
    const auto tupleShape = numNeighborsStore.getTupleShape();

    // Format resolution for imported data is handled by the backfill strategy
    // at a higher level (CreateNeighborListAction / ImportH5ObjectPathsAction).
    // During the eager HDF5 read path, we always load in-core.
    //
    // Read the entire flat data array from HDF5 and scatter it into
    // per-tuple vectors in an in-memory ListStore.
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
    // In-core branch of the import pipeline: allocate a plain in-memory ListStore.
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
   * @brief Attempts to write the NeighborList<T> to HDF5.
   * @param dataStructureWriter
   * @param neighborList
   * @param parentGroupWriter
   * @param importable
   * @return Result<>
   */
  Result<> writeData(DataStructureWriter& dataStructureWriter, const NeighborList<T>& neighborList, group_writer_type& parentGroupWriter, bool importable) const
  {
    DataStructure tmp;

    // Create NumNeighbors DataStore
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

    // Write NumNeighbors data
    DataArrayIO<int32> dataArrayIO;
    Result<> result = dataArrayIO.writeData(dataStructureWriter, *numNeighborsArray, parentGroupWriter, false);
    if(result.invalid())
    {
      return result;
    }

    // Create flattened neighbor DataStore
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

    // Write flattened array to HDF5 as a separate array. NeighborLists can be very large
    // (millions of ints across all tuples), so apply the configured compression level here too.
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
   * @brief Attempts to write the DataObject to HDF5.
   * Returns an error if the DataObject cannot be cast to a NeighborList<T>.
   * Otherwise, this method returns writeData(...)
   * @param dataStructureWriter
   * @param dataObject
   * @param parentWriter
   * @return Result<>
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
