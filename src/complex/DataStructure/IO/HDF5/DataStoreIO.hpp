#pragma once

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/IO/HDF5/IDataStoreIO.hpp"

#include "complex/Utilities/Parsing/HDF5/Writers/DatasetWriter.hpp"

#include "fmt/format.h"

namespace complex
{
namespace HDF5
{
namespace DataStoreIO
{
namespace Chunks
{
constexpr int32 k_DimensionMismatchError = -2654;

template <typename T>
inline Result<> WriteDataStoreChunk(complex::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims,
                                    const complex::HDF5::DatasetWriter::DimsType& chunkDims, const IDataStore::ShapeType& index)
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
  herr_t err = datasetWriter.writeChunk(h5dims, nonstd::span<const T>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(err < 0)
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(err, ss);
  }

  return {};
}

template <>
inline Result<> WriteDataStoreChunk<bool>(complex::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<bool>& store, const std::vector<hsize_t>& h5dims,
                                          const complex::HDF5::DatasetWriter::DimsType& chunkDims, const IDataStore::ShapeType& index)
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
  herr_t err = datasetWriter.writeChunk(h5dims, nonstd::span<const uint8>(chunkPtr, cCount), chunkDims, nonstd::span<const hsize_t>{offset.data(), offset.size()});
  delete[] chunkPtr;
  if(err < 0)
  {
    std::string ss = "Failed to write DataStore chunk to Dataset";
    return MakeErrorResult(err, ss);
  }

  return {};
}

template <typename T>
inline Result<> RecursivelyWriteChunks(complex::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims,
                                       const complex::HDF5::DatasetWriter::DimsType& chunkDims, IDataStore::ShapeType index, const std::vector<hsize_t>& chunkLayout, uint64 i = 0)
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
inline Result<> WriteDataStoreChunks(complex::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& store, const std::vector<hsize_t>& h5dims)
{
  auto shapeDims = store.getTupleShape();
  const auto componentDims = store.getComponentShape();
  shapeDims.insert(shapeDims.end(), componentDims.begin(), componentDims.end());

  const auto storeChunkShape = store.getChunkShape().value();
  complex::HDF5::DatasetWriter::DimsType chunkDims(storeChunkShape.begin(), storeChunkShape.end());

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
inline Result<> WriteDataStore(complex::HDF5::DatasetWriter& datasetWriter, const AbstractDataStore<T>& dataStore)
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
    T* dataPtr = new T[count];
    for(usize i = 0; i < count; ++i)
    {
      dataPtr[i] = dataStore[i];
    }

    herr_t err = datasetWriter.writeSpan(h5dims, nonstd::span<const T>{const_cast<T*>(dataPtr), count});
    if(err < 0)
    {
      std::string ss = "Failed to write DataStore span to Dataset";
      return MakeErrorResult(err, ss);
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
  herr_t err = tupleAttribute.writeVector({dataStore.getTupleShape().size()}, dataStore.getTupleShape());
  if(err < 0)
  {
    std::string ss = "Failed to write DataStore tuple shape property";
    return MakeErrorResult(err, ss);
  }

  auto componentAttribute = datasetWriter.createAttribute(IOConstants::k_ComponentShapeTag);
  err = componentAttribute.writeVector({dataStore.getComponentShape().size()}, dataStore.getComponentShape());
  if(err < 0)
  {
    std::string ss = "Failed to write DataStore component shape property";
    return MakeErrorResult(err, ss);
  }

  return {};
}

/**
 * @brief Attempts to read a DataStore<T> from the dataset reader
 * @param datasetReader
 * @return std::unique_ptr<DataStore<T>>
 */
template <typename T>
inline std::unique_ptr<DataStore<T>> ReadDataStore(const complex::HDF5::DatasetReader& datasetReader)
{
  auto tupleShape = IDataStoreIO::ReadTupleShape(datasetReader);
  auto componentShape = IDataStoreIO::ReadComponentShape(datasetReader);

  // Create DataStore
  auto dataStore = std::make_unique<DataStore<T>>(tupleShape, componentShape, static_cast<T>(0));
  if(!datasetReader.readIntoSpan(dataStore->createSpan()))
  {
    throw std::runtime_error(fmt::format("Error reading data array from DataStore from HDF5 at {}/{}", complex::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }

  return dataStore;
}
} // namespace DataStoreIO
} // namespace HDF5
} // namespace complex
