#pragma once

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

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
inline Result<> WriteDataStoreChunk(nx::core::HDF5::DatasetIO& datasetWriter, const HDF5::ChunkedDataInfo& chunkInfo, const AbstractDataStore<T>& store,
                                    const nx::core::HDF5::DatasetIO::DimsType& h5dims,
                                    const nx::core::HDF5::DatasetIO::DimsType& chunkDims, const IDataStore::ShapeType& index)
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
  auto result = datasetWriter.writeChunk(chunkInfo, h5dims, nonstd::span<const T>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

template <>
inline Result<> WriteDataStoreChunk<bool>(nx::core::HDF5::DatasetIO& datasetWriter, const HDF5::ChunkedDataInfo& chunkInfo, const AbstractDataStore<bool>& store, const nx::core::HDF5::DatasetIO::DimsType& h5dims,
                                          const nx::core::HDF5::DatasetIO::DimsType& chunkDims, const IDataStore::ShapeType& index)
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
  auto result = datasetWriter.writeChunk(chunkInfo, h5dims, nonstd::span<const uint8>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(result.invalid())
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}

template <typename T>
inline Result<> RecursivelyWriteChunks(nx::core::HDF5::DatasetIO& datasetWriter, const AbstractDataStore<T>& store, const nx::core::HDF5::DatasetIO::DimsType& h5dims,
                                       const nx::core::HDF5::DatasetIO::DimsType& chunkDims, IDataStore::ShapeType index, const nx::core::HDF5::DatasetIO::DimsType& chunkLayout, uint64 i = 0)
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

  // Successfully wrote all chunkShape
  return {};
}

template <typename T>
inline Result<> WriteDataStoreChunks(nx::core::HDF5::DatasetIO& datasetWriter, const AbstractDataStore<T>& store, const nx::core::HDF5::DatasetIO::DimsType& h5dims)
{
  auto shapeDims = store.getTupleShape();
  const auto componentDims = store.getComponentShape();
  shapeDims.insert(shapeDims.end(), componentDims.begin(), componentDims.end());

  const auto storeChunkShape = store.getChunkShape().value();
  nx::core::HDF5::DatasetIO::DimsType chunkDims(storeChunkShape.begin(), storeChunkShape.end());

  usize rank = shapeDims.size();

  nx::core::HDF5::DatasetIO::DimsType chunkLayout(rank);
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
inline Result<> WriteDataStore(nx::core::HDF5::DatasetIO& datasetWriter, const AbstractDataStore<T>& dataStore)
{
  //if(!datasetWriter.isValid())
  //{
  //  std::string ss = "Failed to write DataArray. DatasetIO not valid";
  //  return MakeErrorResult(-1, ss);
  //}

  auto writeResult = dataStore.writeHdf5(datasetWriter);
  if (writeResult.invalid())
  {
    return writeResult;
  }

  // Write shape attributes to the dataset
  const auto tupleShape = dataStore.getTupleShape();
  const auto componentShape = dataStore.getComponentShape();
  datasetWriter.writeVectorAttribute(IOConstants::k_TupleShapeTag, tupleShape);
  datasetWriter.writeVectorAttribute(IOConstants::k_ComponentShapeTag, componentShape);

  return {};
}

/**
 * @brief Attempts to read a DataStore<T> from the dataset reader
 * @param datasetReader
 * @return std::unique_ptr<DataStore<T>>
 */
template <typename T>
inline std::unique_ptr<DataStore<T>> ReadDataStore(const nx::core::HDF5::DatasetIO& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = std::make_unique<DataStore<T>>(tupleShape, componentShape, static_cast<T>(0));
  dataStore->readHdf5(datasetReader);
  /*auto dataSpan = dataStore->createSpan();
  Result<> result = datasetReader.readIntoSpan(dataSpan);
  if(result.invalid())
  {
    throw std::runtime_error(fmt::format("Error reading data array from DataStore from HDF5 at {} called {}", datasetReader.getFilePath().string(),
                                         datasetReader.getName()));
  }*/

  return dataStore;
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace nx::core
