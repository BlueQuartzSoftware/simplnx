#pragma once

#include "DataStructureReader.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"
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
   * @brief Attempts to read the NeighborList<T> data from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param parentGroup
   * @param dataReader
   * @return Result<>
   */
  static std::shared_ptr<store_type> ReadHdf5Data(const nx::core::HDF5::GroupIO& parentGroup, const nx::core::HDF5::DatasetIO& dataReader, bool useEmptyDataStore = false)
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
        auto tupleDimsResult = numNeighborsReader.readVectorAttribute<uint64>("TupleDimensions");
        if(tupleDimsResult.invalid())
        {
          return nullptr;
        }
        std::vector<uint64> tupleDims = tupleDimsResult.value();
        uint64 numTuples = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<uint64>(1), std::multiplies<>());
        return std::make_shared<EmptyListStore<T>>(numTuples);
      }

      auto numNeighborsPtr = DataStoreIO::ReadDataStore<int32>(numNeighborsReader, useEmptyDataStore ? IDataAction::Mode::Preflight : IDataAction::Mode::Execute);
      auto& numNeighborsStore = *numNeighborsPtr.get();

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
      auto listStorePtr = DataStoreUtilities::CreateListStore<T>(numTuples);
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
    auto listStorePtr = ReadHdf5Data(parentGroup, datasetReader, useEmptyDataStore);
    auto* dataObject = data_type::Import(dataStructureReader.getDataStructure(), objectName, importId, listStorePtr, parentId);
    if(dataObject == nullptr)
    {
      std::string ss = "Failed to import NeighborList from HDF5";
      return MakeErrorResult(-505, ss);
    }
    return {};
  }

  /**
   * @brief Replaces the AbstractListStore using data from the HDF5 dataset.
   * @param dataStructure
   * @param dataPath
   * @param dataStructureReader
   * @return Result<>
   */
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroup, IDataAction::Mode mode) const override
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

    auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);
    auto numNeighborsPtr = DataStoreIO::ReadDataStore<int32>(numNeighborsReader, mode);
    auto& numNeighborsStore = *numNeighborsPtr.get();

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
    const auto numTuples = numNeighborsStore.getNumberOfTuples();
    auto listStorePtr = DataStoreUtilities::CreateListStore<T>(numTuples);
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
    return {};
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

    // Write flattened array to HDF5 as a separate array
    auto datasetWriter = parentGroupWriter.createDataset(neighborList.getName());
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
   * Return Result<>
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
