#pragma once

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
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
  static std::vector<shared_vector_type> ReadHdf5Data(const nx::core::HDF5::GroupReader& parentGroup, const nx::core::HDF5::DatasetReader& dataReader)
  {
    auto numNeighborsAttributeName = dataReader.getAttribute("Linked NumNeighbors Dataset");
    auto numNeighborsName = numNeighborsAttributeName.readAsString();

    auto numNeighborsReader = parentGroup.openDataset(numNeighborsName);

    auto numNeighborsPtr = DataStoreIO::ReadDataStore<int32>(numNeighborsReader);
    auto& numNeighborsStore = *numNeighborsPtr.get();

    std::vector<T> flatDataStore = dataReader.template readAsVector<T>();
    if(flatDataStore.empty())
    {
      throw std::runtime_error(fmt::format("Error reading neighbor list from DataStore from HDF5 at {}/{}", nx::core::HDF5::Support::GetObjectPath(dataReader.getParentId()), dataReader.getName()));
    }

    std::vector<shared_vector_type> dataVector;
    usize offset = 0;
    const auto numTuples = numNeighborsStore.getNumberOfTuples();
    for(usize i = 0; i < numTuples; i++)
    {
      const auto numNeighbors = numNeighborsStore[i];
      auto sharedVector = std::make_shared<std::vector<T>>(numNeighbors);
      std::vector<T>& vector = *sharedVector.get();

      size_t neighborListStart = offset;
      size_t neighborListEnd = offset + numNeighbors;
      sharedVector->assign(flatDataStore.begin() + neighborListStart, flatDataStore.begin() + neighborListEnd);
      offset += numNeighbors;
      dataVector.push_back(sharedVector);
    }

    return dataVector;
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
    auto dataVector = ReadHdf5Data(parentGroup, datasetReader);
    auto* dataObject = data_type::Import(dataStructureReader.getDataStructure(), objectName, importId, dataVector, parentId);
    if(dataObject == nullptr)
    {
      std::string ss = "Failed to import NeighborList from HDF5";
      return MakeErrorResult(-505, ss);
    }
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
    auto datasetWriter = parentGroupWriter.createDatasetWriter(neighborList.getName());
    Result<> flattenedResult = DataStoreIO::WriteDataStore<T>(datasetWriter, flattenedData);
    if(flattenedResult.invalid())
    {
      return flattenedResult;
    }
    auto linkedDatasetAttribute = datasetWriter.createAttribute("Linked NumNeighbors Dataset");
    result = linkedDatasetAttribute.writeString(neighborList.getNumNeighborsArrayName());
    if(result.invalid())
    {
      std::string ss = "Failed to write NeighborList dataset data name";
      return MakeErrorResult(result.errors()[0].code, ss);
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
