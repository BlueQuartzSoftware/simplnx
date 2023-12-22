#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Writers/DatasetWriter.hpp"

#include "fmt/format.h"

namespace nx::core
{
namespace HDF5
{
namespace DataStoreIO
{
namespace Chunks
{
constexpr int32 k_DimensionMismatchError = -2654;

template <typename T>
inline Result<> WriteDataStoreChunk(nx::core::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims,
                                    const nx::core::HDF5::DatasetWriter::DimsType& chunkDims, const IDataStore::ShapeType& index)
{
  auto rank = chunkDims.size();

  // Find and write chunk values to HDF5
  std::vector<hsize_t> offset(rank);
  for(uint64 i = 0; i < rank && i < index.size(); i++)
  {
    offset[i] = index[i] * chunkDims[i];
  }
  const std::vector<T> chunkVector = store.getChunkValues(index);
  const usize cCount = chunkVector.size();
  T* chunkPtr = new T[cCount];
  for(usize i = 0; i < cCount; i++)
  {
    chunkPtr[i] = chunkVector[i];
  }
  if(chunkDims.size() != h5dims.size())
  {
    std::string ss = fmt::format("Dimension mismatch when writing DataStore chunk. Num Shape Dimensions: {} Num Chunk Dimensions: {}", h5dims.size(), chunkDims.size());
    return MakeErrorResult(k_DimensionMismatchError, ss);
  }
  auto result = datasetWriter.writeChunk(h5dims, nonstd::span<const T>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

template <>
inline Result<> WriteDataStoreChunk<bool>(nx::core::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<bool>& store, const std::vector<hsize_t>& h5dims,
                                          const nx::core::HDF5::DatasetWriter::DimsType& chunkDims, const IDataStore::ShapeType& index)
{
  auto rank = chunkDims.size();

  // Find and write chunk values to HDF5
  std::vector<hsize_t> offset(rank);
  for(uint64 i = 0; i < rank; i++)
  {
    offset[i] = index[i] * chunkDims[i];
  }
  const std::vector<bool> chunkVector = store.getChunkValues(index);
  const usize cCount = chunkVector.size();
  uint8* chunkPtr = new uint8[cCount];
  for(usize i = 0; i < cCount; i++)
  {
    chunkPtr[i] = chunkVector[i];
  }
  auto result = datasetWriter.writeChunk(h5dims, nonstd::span<const uint8>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

template <typename T>
inline Result<> RecursivelyWriteChunks(nx::core::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims,
                                       const nx::core::HDF5::DatasetWriter::DimsType& chunkDims, IDataStore::ShapeType index, const std::vector<hsize_t>& chunkLayout, uint64 i = 0)
{
  // uint64 rank = chunkLayout.size();
  const uint64 rank = chunkDims.size();
  if(i >= rank)
  {
    return {};
  }

  const auto maxValue = chunkLayout[i];
  // const auto maxValue = rank;
  const uint64 nextIndexPos = i + 1;
  for(hsize_t value = 0; value < maxValue; value++)
  {
    index[i] = value;
    Result<> result = WriteDataStoreChunk<T>(datasetWriter, store, h5dims, chunkDims, index);
    if(result.invalid())
    {
      return result;
    }

    result = RecursivelyWriteChunks<T>(datasetWriter, store, h5dims, chunkDims, index, chunkLayout, nextIndexPos);
    if(result.invalid())
    {
      return result;
    }
  }

  // Successfully wrote all chunks
  return {};
}

template <typename T>
inline Result<> WriteDataStoreChunks(nx::core::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims)
{
  auto shapeDims = store.getTupleShape();
  const auto componentDims = store.getComponentShape();
  shapeDims.insert(shapeDims.end(), componentDims.begin(), componentDims.end());

  const auto storeChunkShape = store.getChunkShape().value();
  nx::core::HDF5::DatasetWriter::DimsType chunkDims(storeChunkShape.begin(), storeChunkShape.end());

  usize rank = shapeDims.size();

  std::vector<hsize_t> chunkLayout(rank);
  for(usize i = 0; i < rank; i++)
  {
    const bool hasRemainder = (shapeDims[i] % chunkDims[i] != 0);
    chunkLayout[i] = shapeDims[i] / chunkDims[i];
    if(hasRemainder)
    {
      chunkLayout[i]++;
    }
  }

  IDataStore::ShapeType index(chunkDims.size());
  std::fill(index.begin(), index.end(), 0);

  return RecursivelyWriteChunks<T>(datasetWriter, store, h5dims, chunkDims, index, chunkLayout);
}
} // namespace Chunks

/**
 * @brief Writes the data store to HDF5. Returns the HDF5 error code should
 * one be encountered. Otherwise, returns 0.
 * @param datasetWriter
 * @return H5::ErrorType
 */
template <typename T>
inline Result<> WriteDataStore(nx::core::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& dataStore)
{
  if(!datasetWriter.isValid())
  {
    std::string ss = "Failed to write DataArray. DatasetWriter not valid";
    return MakeErrorResult(-1, ss);
  }

  // Consolidate the Tuple and Component Dims into a single array which is used
  // to write the entire data array to HDF5
  std::vector<hsize_t> h5dims;
  for(const auto& value : dataStore.getTupleShape())
  {
    h5dims.push_back(static_cast<hsize_t>(value));
  }
  for(const auto& value : dataStore.getComponentShape())
  {
    h5dims.push_back(static_cast<hsize_t>(value));
  }

  if(dataStore.getChunkShape().has_value() == false)
  {
    usize count = dataStore.getSize();
    auto dataPtr = std::make_unique<T[]>(count);
    for(usize i = 0; i < count; ++i)
    {
      dataPtr[i] = dataStore[i];
    }

    Result<> result = datasetWriter.writeSpan(h5dims, nonstd::span<const T>{dataPtr.get(), count});
    if(result.invalid())
    {
      std::string ss = "Failed to write DataStore span to Dataset";
      return MakeErrorResult(result.errors()[0].code, ss);
    }
  }
  else
  {
    Result<> writeResult = Chunks::WriteDataStoreChunks<T>(datasetWriter, dataStore, h5dims);
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  // Write shape attributes to the dataset
  auto tupleAttribute = datasetWriter.createAttribute(IOConstants::k_TupleShapeTag);
  Result<> result = tupleAttribute.writeVector({dataStore.getTupleShape().size()}, dataStore.getTupleShape());
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore tuple shape property";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  auto componentAttribute = datasetWriter.createAttribute(IOConstants::k_ComponentShapeTag);
  result = componentAttribute.writeVector({dataStore.getComponentShape().size()}, dataStore.getComponentShape());
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore component shape property";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

/**
 * @brief Attempts to read a DataStore<T> from the dataset reader
 * @param datasetReader
 * @return std::unique_ptr<DataStore<T>>
 */
template <typename T>
inline std::unique_ptr<DataStore<T>> ReadDataStore(const nx::core::HDF5::DatasetReader& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = std::make_unique<DataStore<T>>(tupleShape, componentShape, static_cast<T>(0));
  if(!datasetReader.readIntoSpan(dataStore->createSpan()))
  {
    throw std::runtime_error(fmt::format("Error reading data array from DataStore from HDF5 at {}/{}", nx::core::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }

  return dataStore;
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
