#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ChunkIndex.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ChunkShapePolicy.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkLoop.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <hdf5.h>
#include <zlib.h>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/global_control.h>
#endif

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::HDF5;

namespace
{
const std::string k_DatasetName = "data";

/**
 * @brief Multiplies all dimensions in a shape.
 * @param shape Dimensions to multiply.
 * @return Product of all dimensions, or 1 for an empty shape.
 */
usize product(const std::vector<uint64>& shape)
{
  return std::accumulate(shape.begin(), shape.end(), usize{1}, std::multiplies<usize>());
}

/**
 * @brief Creates a chunked, deflated dataset through the HDF5 C API.
 * @param filePath Output HDF5 file path.
 * @param tupleShape Dataset tuple dimensions.
 * @param componentShape Dataset component dimensions.
 * @param chunkShape Tuple-space chunk dimensions.
 * @param h5Type HDF5 element type.
 * @param bytes Raw element bytes to write.
 * @param deflateLevel Deflate level from 0 through 9.
 *
 * Components form the trailing chunk dimensions. Direct C API calls keep this
 * test independent of higher-level compression wiring.
 */
void createChunkedDeflateDataset(const fs::path& filePath, const std::vector<uint64>& tupleShape, const std::vector<uint64>& componentShape, const std::vector<uint64>& chunkShape, hid_t h5Type,
                                 const std::vector<std::byte>& bytes, int deflateLevel = 5)
{
  const usize tupleRank = tupleShape.size();
  const usize fullRank = tupleRank + componentShape.size();

  std::vector<hsize_t> dims(fullRank);
  std::vector<hsize_t> chunkDims(fullRank);
  for(usize d = 0; d < tupleRank; ++d)
  {
    dims[d] = static_cast<hsize_t>(tupleShape[d]);
    chunkDims[d] = static_cast<hsize_t>(chunkShape[d]);
  }
  for(usize d = 0; d < componentShape.size(); ++d)
  {
    dims[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
    chunkDims[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
  }

  const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(fileId >= 0);
  const hid_t space = H5Screate_simple(static_cast<int>(fullRank), dims.data(), nullptr);
  REQUIRE(space >= 0);
  const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
  REQUIRE(dcpl >= 0);
  REQUIRE(H5Pset_chunk(dcpl, static_cast<int>(fullRank), chunkDims.data()) >= 0);
  REQUIRE(H5Pset_deflate(dcpl, static_cast<unsigned int>(deflateLevel)) >= 0);
  const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), h5Type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  REQUIRE(H5Dwrite(dataset, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes.data()) >= 0);
  H5Dclose(dataset);
  H5Pclose(dcpl);
  H5Sclose(space);
  H5Fclose(fileId);
}

/**
 * @brief Reads the entire dataset with one serial H5Dread call.
 * @param filePath HDF5 file path.
 * @param totalBytes Number of output bytes.
 * @param h5Type HDF5 element type.
 * @return Dataset bytes in file order.
 */
std::vector<std::byte> serialReadWhole(const fs::path& filePath, usize totalBytes, hid_t h5Type)
{
  std::vector<std::byte> out(totalBytes);
  const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  REQUIRE(fileId >= 0);
  const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  REQUIRE(H5Dread(dataset, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data()) >= 0);
  H5Dclose(dataset);
  H5Fclose(fileId);
  return out;
}

/**
 * @brief Creates an empty chunked, deflated dataset through the HDF5 C API.
 * @param filePath Output HDF5 file path.
 * @param tupleShape Dataset tuple dimensions.
 * @param componentShape Dataset component dimensions.
 * @param chunkShape Tuple-space chunk dimensions.
 * @param h5Type HDF5 element type.
 * @param fileIdOut Receives an open HDF5 file identifier.
 * @param datasetIdOut Receives an open HDF5 dataset identifier.
 * @param deflateLevel Deflate level from 0 through 9.
 *
 * The caller owns and closes the returned identifiers.
 */
void createEmptyChunkedDeflateDataset(const fs::path& filePath, const std::vector<uint64>& tupleShape, const std::vector<uint64>& componentShape, const std::vector<uint64>& chunkShape, hid_t h5Type,
                                      hid_t& fileIdOut, hid_t& datasetIdOut, int deflateLevel = 5)
{
  const usize tupleRank = tupleShape.size();
  const usize fullRank = tupleRank + componentShape.size();

  std::vector<hsize_t> dims(fullRank);
  std::vector<hsize_t> chunkDims(fullRank);
  for(usize d = 0; d < tupleRank; ++d)
  {
    dims[d] = static_cast<hsize_t>(tupleShape[d]);
    chunkDims[d] = static_cast<hsize_t>(chunkShape[d]);
  }
  for(usize d = 0; d < componentShape.size(); ++d)
  {
    dims[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
    chunkDims[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
  }

  fileIdOut = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(fileIdOut >= 0);
  const hid_t space = H5Screate_simple(static_cast<int>(fullRank), dims.data(), nullptr);
  REQUIRE(space >= 0);
  const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
  REQUIRE(dcpl >= 0);
  REQUIRE(H5Pset_chunk(dcpl, static_cast<int>(fullRank), chunkDims.data()) >= 0);
  REQUIRE(H5Pset_deflate(dcpl, static_cast<unsigned int>(deflateLevel)) >= 0);
  datasetIdOut = H5Dcreate(fileIdOut, k_DatasetName.c_str(), h5Type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
  REQUIRE(datasetIdOut >= 0);
  H5Pclose(dcpl);
  H5Sclose(space);
}

/**
 * @brief Reads one clamped chunk region with a serial hyperslab H5Dread.
 * @param dataset Open HDF5 dataset identifier.
 * @param bounds Tuple-space region to read.
 * @param componentShape Dataset component dimensions.
 * @param elementSize Size of one element in bytes.
 * @param numComponents Number of components per tuple.
 * @param h5Type HDF5 element type.
 * @return Bytes in the selected region.
 */
std::vector<std::byte> serialReadChunkRegion(hid_t dataset, const Extent& bounds, const std::vector<uint64>& componentShape, usize elementSize, usize numComponents, hid_t h5Type)
{
  const usize tupleRank = bounds.min.size();
  const usize fullRank = tupleRank + componentShape.size();

  std::vector<uint64> clampedTupleDims(tupleRank);
  std::vector<hsize_t> start(fullRank, 0);
  std::vector<hsize_t> count(fullRank, 0);
  std::vector<hsize_t> memDims(fullRank, 0);
  for(usize d = 0; d < tupleRank; ++d)
  {
    clampedTupleDims[d] = bounds.max[d] - bounds.min[d] + 1;
    start[d] = static_cast<hsize_t>(bounds.min[d]);
    count[d] = static_cast<hsize_t>(clampedTupleDims[d]);
    memDims[d] = static_cast<hsize_t>(clampedTupleDims[d]);
  }
  for(usize d = 0; d < componentShape.size(); ++d)
  {
    count[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
    memDims[tupleRank + d] = static_cast<hsize_t>(componentShape[d]);
  }

  const usize clampedTuples = product(clampedTupleDims);
  std::vector<std::byte> out(clampedTuples * numComponents * elementSize);
  const hid_t fileSpace = H5Dget_space(dataset);
  const hid_t memSpace = H5Screate_simple(static_cast<int>(fullRank), memDims.data(), nullptr);
  REQUIRE(H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start.data(), nullptr, count.data(), nullptr) >= 0);
  REQUIRE(H5Dread(dataset, h5Type, memSpace, fileSpace, H5P_DEFAULT, out.data()) >= 0);
  H5Sclose(memSpace);
  H5Sclose(fileSpace);
  return out;
}

/**
 * @brief Resolves a test file name in the binary output directory.
 * @param name File name.
 * @return Output file path.
 */
fs::path testFilePath(const std::string& name)
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view()) / name;
}

/**
 * @brief Creates an unchunked, unfiltered dataset through the HDF5 C API.
 * @param filePath Output HDF5 file path.
 * @param dims Dataset dimensions.
 * @param h5Type HDF5 element type.
 * @param bytes Raw element bytes to write.
 *
 * DatasetIO must use its serial fallback for this dataset.
 */
void createContiguousDataset(const fs::path& filePath, const std::vector<uint64>& dims, hid_t h5Type, const std::vector<std::byte>& bytes)
{
  std::vector<hsize_t> hDims(dims.begin(), dims.end());
  const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(fileId >= 0);
  const hid_t space = H5Screate_simple(static_cast<int>(hDims.size()), hDims.data(), nullptr);
  REQUIRE(space >= 0);
  const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), h5Type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  REQUIRE(H5Dwrite(dataset, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes.data()) >= 0);
  H5Dclose(dataset);
  H5Sclose(space);
  H5Fclose(fileId);
}

/**
 * @brief Creates a chunked shuffle and deflate dataset through the HDF5 C API.
 * @param filePath Output HDF5 file path.
 * @param dims Dataset dimensions.
 * @param chunkShape Chunk dimensions.
 * @param h5Type HDF5 element type.
 * @param bytes Raw element bytes to write.
 *
 * The multi-filter pipeline is ineligible for the parallel codec.
 */
void createShuffleDeflateDataset(const fs::path& filePath, const std::vector<uint64>& dims, const std::vector<uint64>& chunkShape, hid_t h5Type, const std::vector<std::byte>& bytes)
{
  std::vector<hsize_t> hDims(dims.begin(), dims.end());
  std::vector<hsize_t> hChunk(chunkShape.begin(), chunkShape.end());
  const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  REQUIRE(fileId >= 0);
  const hid_t space = H5Screate_simple(static_cast<int>(hDims.size()), hDims.data(), nullptr);
  REQUIRE(space >= 0);
  const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
  REQUIRE(dcpl >= 0);
  REQUIRE(H5Pset_chunk(dcpl, static_cast<int>(hChunk.size()), hChunk.data()) >= 0);
  REQUIRE(H5Pset_shuffle(dcpl) >= 0);
  REQUIRE(H5Pset_deflate(dcpl, 5) >= 0);
  const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), h5Type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  REQUIRE(H5Dwrite(dataset, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes.data()) >= 0);
  H5Dclose(dataset);
  H5Pclose(dcpl);
  H5Sclose(space);
  H5Fclose(fileId);
}
} // namespace

TEST_CASE("ParallelChunkCodec inflate matches serial H5Dread", "[ParallelChunkCodec]")
{
  SECTION("uint16, multiple chunks with an edge chunk")
  {
    // 10x7 (tuple), chunk 4x4 -> 3x2 chunks; last row/col are edge chunks.
    const std::vector<uint64> tupleShape = {10, 7};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<uint16> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<uint16>((i * 7 + 3) & 0xFFFF);
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), values.data(), bytes.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_uint16.h5");
    createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, bytes);

    const std::vector<std::byte> bufferA = serialReadWhole(filePath, bytes.size(), H5T_NATIVE_UINT16);

    const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::vector<std::byte> bufferB(bytes.size());
    codec.inflateChunksIntoSpan(bufferB, allChunks);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(bufferA == bufferB);
  }

  SECTION("uint8 single-byte element (no byte-swap path)")
  {
    const std::vector<uint64> tupleShape = {9, 5};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint8);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<std::byte> bytes(numElements * elementSize);
    for(usize i = 0; i < numElements; ++i)
    {
      bytes[i] = static_cast<std::byte>((i * 13 + 1) & 0xFF);
    }

    const fs::path filePath = testFilePath("ParallelChunkCodec_uint8.h5");
    createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT8, bytes);

    const std::vector<std::byte> bufferA = serialReadWhole(filePath, bytes.size(), H5T_NATIVE_UINT8);

    const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::vector<std::byte> bufferB(bytes.size());
    codec.inflateChunksIntoSpan(bufferB, allChunks);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(bufferA == bufferB);
  }

  SECTION("multi-component (componentShape {3}) scatter")
  {
    const std::vector<uint64> tupleShape = {11, 6};
    const std::vector<uint64> componentShape = {3};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(float32);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<float32> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<float32>(i) * 1.5f - 7.0f;
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), values.data(), bytes.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_vec3.h5");
    createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_FLOAT, bytes);

    const std::vector<std::byte> bufferA = serialReadWhole(filePath, bytes.size(), H5T_NATIVE_FLOAT);

    const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::vector<std::byte> bufferB(bytes.size());
    codec.inflateChunksIntoSpan(bufferB, allChunks);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(bufferA == bufferB);
  }

  SECTION("inflateChunk per-chunk equality on an edge chunk")
  {
    const std::vector<uint64> tupleShape = {10, 7};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numComponents = product(componentShape);
    const usize numElements = product(tupleShape) * numComponents;

    std::vector<uint16> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<uint16>((i * 5 + 11) & 0xFFFF);
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), values.data(), bytes.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_edge.h5");
    createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, bytes);

    const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);

    // Last chunk is the bottom-right edge chunk: rows 8-9 (clamped from 8-11), cols 4-6 (clamped from 4-7).
    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    const uint64 edgeChunk = numChunks - 1;
    const Extent bounds = getChunkBounds(edgeChunk, tupleShape, chunkShape);

    const std::vector<std::byte> expected = serialReadChunkRegion(dataset, bounds, componentShape, elementSize, numComponents, H5T_NATIVE_UINT16);
    const std::vector<std::byte> actual = codec.inflateChunk(edgeChunk);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(actual == expected);
  }

  SECTION("isEligible reflects the dataset filter pipeline")
  {
    const std::vector<uint64> tupleShape = {8, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numElements = product(tupleShape) * product(componentShape);
    std::vector<std::byte> bytes(numElements * elementSize, std::byte{0});

    // Eligible: single-filter deflate.
    {
      const fs::path filePath = testFilePath("ParallelChunkCodec_eligible.h5");
      createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, bytes);
      const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
      ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
      REQUIRE(codec.isEligible());
      H5Dclose(dataset);
      H5Fclose(fileId);
    }

    // Ineligible: shuffle + deflate (multi-filter pipeline).
    {
      const fs::path filePath = testFilePath("ParallelChunkCodec_shuffle.h5");
      std::vector<hsize_t> dims = {8, 8};
      std::vector<hsize_t> chunkDims = {4, 4};
      const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      const hid_t space = H5Screate_simple(2, dims.data(), nullptr);
      const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
      REQUIRE(H5Pset_chunk(dcpl, 2, chunkDims.data()) >= 0);
      REQUIRE(H5Pset_shuffle(dcpl) >= 0);
      REQUIRE(H5Pset_deflate(dcpl, 5) >= 0);
      const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), H5T_NATIVE_UINT16, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
      REQUIRE(H5Dwrite(dataset, H5T_NATIVE_UINT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, bytes.data()) >= 0);
      H5Dclose(dataset);
      H5Pclose(dcpl);
      H5Sclose(space);
      H5Fclose(fileId);

      const hid_t rFileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      const hid_t rDataset = H5Dopen(rFileId, k_DatasetName.c_str(), H5P_DEFAULT);
      ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, rDataset);
      REQUIRE_FALSE(codec.isEligible());
      H5Dclose(rDataset);
      H5Fclose(rFileId);
    }
  }

  SECTION("unallocated chunk falls back to fill-value serial read")
  {
    // The 8 by 8 dataset has four 4 by 4 chunks.
    // Incremental allocation leaves untouched chunks unallocated.
    // Writing only chunks 0 and 3 forces the serial fill-value fallback for chunks 1 and 2.
    // Late allocation would materialize every deflated chunk and would not test this path.
    const std::vector<uint64> tupleShape = {8, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numComponents = product(componentShape);
    const usize chunkTuples = product(chunkShape);
    const usize chunkElements = chunkTuples * numComponents;
    const usize chunkBytes = chunkElements * elementSize;
    const uint16 fillValue = 0xBEEFu;

    std::vector<hsize_t> dims = {8, 8};
    std::vector<hsize_t> chunkDims = {4, 4};

    const fs::path filePath = testFilePath("ParallelChunkCodec_fill.h5");
    const hid_t fileId = H5Fcreate(filePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(fileId >= 0);
    const hid_t space = H5Screate_simple(2, dims.data(), nullptr);
    REQUIRE(space >= 0);
    const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    REQUIRE(dcpl >= 0);
    REQUIRE(H5Pset_chunk(dcpl, 2, chunkDims.data()) >= 0);
    REQUIRE(H5Pset_deflate(dcpl, 5) >= 0);
    // Incremental allocation: a chunk's space is allocated only when that chunk is written.
    REQUIRE(H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_INCR) >= 0);
    REQUIRE(H5Pset_fill_value(dcpl, H5T_NATIVE_UINT16, &fillValue) >= 0);
    const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), H5T_NATIVE_UINT16, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    REQUIRE(dataset >= 0);

    // Write only chunks 0 and 3 with deflate-compressed bytes.
    // The dataset filter therefore receives already-filtered chunk data.
    const std::vector<uint64> writtenChunks = {0, 3};
    for(const uint64 flatChunk : writtenChunks)
    {
      const Extent bounds = getChunkBounds(flatChunk, tupleShape, chunkShape);

      // Build the chunk's nominal (4x4) element bytes from a deterministic pattern.
      std::vector<uint16> chunkValues(chunkElements);
      for(usize i = 0; i < chunkElements; ++i)
      {
        chunkValues[i] = static_cast<uint16>((flatChunk * 1000 + i * 7 + 1) & 0xFFFF);
      }
      std::vector<std::byte> nominalBytes(chunkBytes);
      std::memcpy(nominalBytes.data(), chunkValues.data(), chunkBytes);

      // Deflate the nominal bytes with zlib (the same codec HDF5's deflate filter uses).
      uLongf compressedLen = compressBound(static_cast<uLong>(chunkBytes));
      std::vector<std::byte> compressed(static_cast<usize>(compressedLen));
      const int zret = compress(reinterpret_cast<Bytef*>(compressed.data()), &compressedLen, reinterpret_cast<const Bytef*>(nominalBytes.data()), static_cast<uLong>(chunkBytes));
      REQUIRE(zret == Z_OK);
      compressed.resize(static_cast<usize>(compressedLen));

      std::vector<hsize_t> chunkOffset = {static_cast<hsize_t>(bounds.min[0]), static_cast<hsize_t>(bounds.min[1])};
      REQUIRE(H5Dwrite_chunk(dataset, H5P_DEFAULT, 0u, chunkOffset.data(), static_cast<size_t>(compressed.size()), compressed.data()) >= 0);
    }
    H5Dclose(dataset);
    H5Pclose(dcpl);
    H5Sclose(space);
    H5Fclose(fileId);

    // Independent reference: a serial whole-dataset H5Dread (HDF5 supplies the fill value
    // for the unallocated chunks itself).
    const usize totalBytes = product(tupleShape) * numComponents * elementSize;
    const std::vector<std::byte> bufferA = serialReadWhole(filePath, totalBytes, H5T_NATIVE_UINT16);

    const hid_t rFileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    const hid_t rDataset = H5Dopen(rFileId, k_DatasetName.c_str(), H5P_DEFAULT);

    // Confirm that only the written subset is allocated.
    // The untouched chunks must use the serial fill-value fallback.
    {
      const hid_t allocSpace = H5Dget_space(rDataset);
      hsize_t allocatedChunks = 0;
      REQUIRE(H5Dget_num_chunks(rDataset, allocSpace, &allocatedChunks) >= 0);
      H5Sclose(allocSpace);
      REQUIRE(allocatedChunks == writtenChunks.size());
      REQUIRE(allocatedChunks < getNumberOfChunks(tupleShape, chunkShape));
    }

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, rDataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::vector<std::byte> bufferB(totalBytes);
    codec.inflateChunksIntoSpan(bufferB, allChunks);
    H5Dclose(rDataset);
    H5Fclose(rFileId);

    // The fill-value regions (chunks 1 and 2) come from the codec's serialFillRead fallback;
    // byte-equality with HDF5's own fill behavior confirms that path ran and is correct.
    REQUIRE(bufferA == bufferB);

    // Sample chunk 1 to prove that an unallocated region contains the configured fill value.
    uint16 sampledFill = 0;
    std::memcpy(&sampledFill, bufferA.data() + (0 * tupleShape[1] + 4) * elementSize, elementSize);
    REQUIRE(sampledFill == fillValue);
  }
}

TEST_CASE("ParallelChunkCodec batch-locates chunks for parallel inflate", "[ParallelChunkCodec]")
{
  const std::vector<uint64> tupleShape = {4, 64};
  const std::vector<uint64> componentShape = {1};
  const std::vector<uint64> chunkShape = {1, 64};
  std::vector<std::byte> source(product(tupleShape));
  for(usize index = 0; index < source.size(); ++index)
  {
    source[index] = static_cast<std::byte>((index * 17 + index / 13) & 0xFF);
  }

  const fs::path filePath = testFilePath("ParallelChunkCodec_metadata_batch.h5");
  createChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT8, source, /*deflateLevel=*/1);

  const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  REQUIRE(fileId >= 0);
  const hid_t dataset = H5Dopen(fileId, k_DatasetName.c_str(), H5P_DEFAULT);
  REQUIRE(dataset >= 0);
  ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, sizeof(uint8), dataset);
  REQUIRE(codec.isEligible());

  const std::vector<uint64> chunkIndices = {0, 1, 2, 3};
  const std::vector<ParallelChunkCodec::ChunkInfo> chunkInfos = codec.getChunkInfos(chunkIndices);
  REQUIRE(chunkInfos.size() == chunkIndices.size());
  for(usize localIndex = 0; localIndex < chunkIndices.size(); ++localIndex)
  {
    INFO("Chunk " << chunkIndices[localIndex]);
    REQUIRE(chunkInfos[localIndex].allocated);
    REQUIRE(chunkInfos[localIndex].storedSize > 0);
    const std::vector<std::byte> inflated = codec.inflateChunk(chunkIndices[localIndex], chunkInfos[localIndex]);
    REQUIRE(std::equal(inflated.begin(), inflated.end(), source.begin() + static_cast<std::ptrdiff_t>(localIndex * chunkShape[1])));
  }

  H5Dclose(dataset);
  H5Fclose(fileId);
}

TEST_CASE("ParallelChunkCodec deflate round-trips byte-identically", "[ParallelChunkCodec]")
{
  SECTION("uint16 full-array round-trip with an edge chunk")
  {
    // 10x7 (tuple), chunk 4x4 -> 3x2 chunks; last row/col are edge chunks. The deflate path
    // gathers + compresses + H5Dwrite_chunk's every chunk; a serial H5Dread is the oracle.
    const std::vector<uint64> tupleShape = {10, 7};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<uint16> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<uint16>((i * 7 + 3) & 0xFFFF);
    }
    std::vector<std::byte> source(numElements * elementSize);
    std::memcpy(source.data(), values.data(), source.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_deflate_uint16.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::string errorOut;
    REQUIRE(codec.deflateSpanIntoChunks(source, allChunks, 0, &errorOut));
    REQUIRE(errorOut.empty());
    H5Dclose(dataset);
    H5Fclose(fileId);

    const std::vector<std::byte> readBack = serialReadWhole(filePath, source.size(), H5T_NATIVE_UINT16);
    REQUIRE(readBack == source);
  }

  SECTION("band write (sourceStartTuple != 0) of a dim-0 chunk-row range")
  {
    // 12x8 (tuple), chunk 4x8 -> 3 dim-0 chunk-rows that each span the FULL width, so a band of
    // whole chunk-rows is a contiguous tuple range. Write only chunk-rows 1 and 2 (tuple rows
    // 4..11) from a band-sized source; the oracle reads that region back via a serial hyperslab.
    const std::vector<uint64> tupleShape = {12, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 8};
    const usize elementSize = sizeof(uint16);
    const usize numComponents = product(componentShape);

    // Band = chunk-rows 1 and 2 -> tuple rows [4, 12) -> sourceStartTuple = 4 * 8 = 32.
    const std::vector<uint64> bandChunks = {1, 2};
    const uint64 sourceStartTuple = 4ULL * tupleShape[1];
    const uint64 bandTuples = 8ULL * tupleShape[1]; // 8 tuple-rows * 8 cols
    std::vector<uint16> bandValues(bandTuples * numComponents);
    for(usize i = 0; i < bandValues.size(); ++i)
    {
      bandValues[i] = static_cast<uint16>((i * 13 + 5) & 0xFFFF);
    }
    std::vector<std::byte> bandSource(bandValues.size() * elementSize);
    std::memcpy(bandSource.data(), bandValues.data(), bandSource.size());

    // Repeat the two-chunk batch to detect unsafe worker-thread commit routing.
    for(usize iteration = 0; iteration < 64; ++iteration)
    {
      INFO("band-write iteration " << iteration);
      const fs::path filePath = testFilePath("ParallelChunkCodec_deflate_band.h5");
      hid_t fileId = H5I_INVALID_HID;
      hid_t dataset = H5I_INVALID_HID;
      createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

      ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
      REQUIRE(codec.isEligible());

      std::string errorOut;
      REQUIRE(codec.deflateSpanIntoChunks(bandSource, bandChunks, sourceStartTuple, &errorOut));
      REQUIRE(errorOut.empty());

      // Oracle: serial hyperslab read of the band region [rows 4..11, all cols].
      const Extent bandBounds{{4, 0}, {11, 7}};
      const std::vector<std::byte> readBack = serialReadChunkRegion(dataset, bandBounds, componentShape, elementSize, numComponents, H5T_NATIVE_UINT16);
      H5Dclose(dataset);
      H5Fclose(fileId);

      REQUIRE(readBack == bandSource);
    }
  }

  SECTION("multi-component (componentShape {3}) full-array round-trip")
  {
    const std::vector<uint64> tupleShape = {11, 6};
    const std::vector<uint64> componentShape = {3};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(float32);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<float32> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<float32>(i) * 1.5f - 7.0f;
    }
    std::vector<std::byte> source(numElements * elementSize);
    std::memcpy(source.data(), values.data(), source.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_deflate_vec3.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_FLOAT, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    std::string errorOut;
    REQUIRE(codec.deflateSpanIntoChunks(source, allChunks, 0, &errorOut));
    REQUIRE(errorOut.empty());
    H5Dclose(dataset);
    H5Fclose(fileId);

    const std::vector<std::byte> readBack = serialReadWhole(filePath, source.size(), H5T_NATIVE_FLOAT);
    REQUIRE(readBack == source);
  }

  SECTION("cross-codec: deflate then inflateChunksIntoSpan equals the source")
  {
    // Write with the deflate path and read with the codec inflate path.
    // The round trip must be byte-identical.
    const std::vector<uint64> tupleShape = {13, 9};
    const std::vector<uint64> componentShape = {2};
    const std::vector<uint64> chunkShape = {5, 5};
    const usize elementSize = sizeof(uint16);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<uint16> values(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      values[i] = static_cast<uint16>((i * 17 + 9) & 0xFFFF);
    }
    std::vector<std::byte> source(numElements * elementSize);
    std::memcpy(source.data(), values.data(), source.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_deflate_cross.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    REQUIRE(codec.deflateSpanIntoChunks(source, allChunks, 0));

    std::vector<std::byte> readBack(source.size());
    codec.inflateChunksIntoSpan(readBack, allChunks);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(readBack == source);
  }

  SECTION("failure signaling: mis-sized source and out-of-range index return false")
  {
    const std::vector<uint64> tupleShape = {8, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numElements = product(tupleShape) * product(componentShape);

    std::vector<std::byte> source(numElements * elementSize, std::byte{0});

    const fs::path filePath = testFilePath("ParallelChunkCodec_deflate_fail.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    const uint64 numChunks = getNumberOfChunks(tupleShape, chunkShape);
    std::vector<uint64> allChunks(numChunks);
    std::iota(allChunks.begin(), allChunks.end(), uint64{0});

    // Mis-sized source (one byte short of a whole tuple).
    {
      std::vector<std::byte> shortSource(source.size() - 1, std::byte{0});
      std::string errorOut;
      REQUIRE_FALSE(codec.deflateSpanIntoChunks(shortSource, allChunks, 0, &errorOut));
      REQUIRE_FALSE(errorOut.empty());
    }

    // Out-of-range chunk index.
    {
      std::vector<uint64> badIndices = allChunks;
      badIndices.push_back(numChunks); // one past the last valid index
      std::string errorOut;
      REQUIRE_FALSE(codec.deflateSpanIntoChunks(source, badIndices, 0, &errorOut));
      REQUIRE_FALSE(errorOut.empty());
    }

    H5Dclose(dataset);
    H5Fclose(fileId);
  }
}

TEST_CASE("ParallelChunkCodec skips deflate only for incompressible chunks", "[ParallelChunkCodec]")
{
  constexpr usize k_ChunkBytes = 256 * 1024;
  const std::vector<uint64> tupleShape = {2, k_ChunkBytes};
  const std::vector<uint64> componentShape = {1};
  const std::vector<uint64> chunkShape = {1, k_ChunkBytes};

  std::vector<std::byte> source(2 * k_ChunkBytes, std::byte{0});
  std::mt19937_64 rng(0xD3D0C0DEC0FFEEULL);
  std::uniform_int_distribution<uint32> distribution(0, 255);
  for(usize index = 0; index < k_ChunkBytes; ++index)
  {
    source[index] = static_cast<std::byte>(distribution(rng));
  }

  const fs::path filePath = testFilePath("ParallelChunkCodec_adaptive_deflate.h5");
  hid_t fileId = H5I_INVALID_HID;
  hid_t dataset = H5I_INVALID_HID;
  createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT8, fileId, dataset, /*deflateLevel=*/1);

  ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, sizeof(uint8), dataset);
  REQUIRE(codec.isEligible());

  const std::vector<uint64> allChunks = {0, 1};
  REQUIRE(codec.deflateSpanIntoChunks(source, allChunks));
  REQUIRE(H5Fflush(fileId, H5F_SCOPE_LOCAL) >= 0);

  unsigned int randomFilterMask = 0;
  haddr_t randomAddress = HADDR_UNDEF;
  hsize_t randomStoredSize = 0;
  const std::vector<hsize_t> randomOffset = {0, 0, 0};
  REQUIRE(H5Dget_chunk_info_by_coord(dataset, randomOffset.data(), &randomFilterMask, &randomAddress, &randomStoredSize) >= 0);

  unsigned int compressibleFilterMask = 0;
  haddr_t compressibleAddress = HADDR_UNDEF;
  hsize_t compressibleStoredSize = 0;
  const std::vector<hsize_t> compressibleOffset = {1, 0, 0};
  REQUIRE(H5Dget_chunk_info_by_coord(dataset, compressibleOffset.data(), &compressibleFilterMask, &compressibleAddress, &compressibleStoredSize) >= 0);

  // A raw chunk is legal in a deflate dataset when filter-mask bit zero records that the
  // filter was skipped. The compressible neighbor must still run deflate and remain smaller.
  REQUIRE((randomFilterMask & 0x1U) != 0U);
  REQUIRE(randomStoredSize == k_ChunkBytes);
  REQUIRE((compressibleFilterMask & 0x1U) == 0U);
  REQUIRE(compressibleStoredSize < k_ChunkBytes);

  std::vector<std::byte> readBack(source.size());
  REQUIRE(H5Dread(dataset, H5T_NATIVE_UINT8, H5S_ALL, H5S_ALL, H5P_DEFAULT, readBack.data()) >= 0);
  REQUIRE(readBack == source);

  H5Dclose(dataset);
  H5Fclose(fileId);
}

TEST_CASE("ParallelChunkCodec preserves rewritten skipped-deflate metadata for live writable datasets", "[ParallelChunkCodec]")
{
  constexpr usize k_ChunkBytes = 256 * 1024;
  const std::vector<uint64> tupleShape = {1, k_ChunkBytes};
  const std::vector<uint64> componentShape = {1};
  const std::vector<uint64> chunkShape = tupleShape;

  std::vector<std::byte> source(k_ChunkBytes);
  std::mt19937_64 rng(0xA11CE5C0FFEEULL);
  std::uniform_int_distribution<uint32> distribution(0, 255);
  for(std::byte& value : source)
  {
    value = static_cast<std::byte>(distribution(rng));
  }

  const fs::path filePath = testFilePath("ParallelChunkCodec_live_skipped_deflate.h5");
  hid_t fileId = H5I_INVALID_HID;
  hid_t dataset = H5I_INVALID_HID;
  createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT8, fileId, dataset, /*deflateLevel=*/1);
  auto fileGuard = MakeScopeGuard([&fileId]() noexcept { H5Fclose(fileId); });
  auto datasetGuard = MakeScopeGuard([&dataset]() noexcept { H5Dclose(dataset); });

  {
    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, sizeof(uint8), dataset);
    REQUIRE(codec.isEligible());
    const std::vector<uint64> chunkIndices = {0};
    const std::vector<std::byte> initial(k_ChunkBytes, std::byte{0});
    REQUIRE(codec.deflateSpanIntoChunks(initial, chunkIndices));

    unsigned int initialFilterMask = 0;
    haddr_t initialStoredAddress = HADDR_UNDEF;
    hsize_t initialStoredSize = 0;
    const std::vector<hsize_t> offset = {0, 0, 0};
    REQUIRE(H5Dget_chunk_info_by_coord(dataset, offset.data(), &initialFilterMask, &initialStoredAddress, &initialStoredSize) >= 0);
    REQUIRE(initialStoredAddress != HADDR_UNDEF);
    REQUIRE((initialFilterMask & 0x1U) == 0U);
    REQUIRE(initialStoredSize < k_ChunkBytes);

    REQUIRE(codec.deflateSpanIntoChunks(source, chunkIndices));
    unsigned int filterMask = 0;
    haddr_t storedAddress = HADDR_UNDEF;
    hsize_t storedSize = 0;
    REQUIRE(H5Dget_chunk_info_by_coord(dataset, offset.data(), &filterMask, &storedAddress, &storedSize) >= 0);
    REQUIRE(storedAddress != HADDR_UNDEF);
    REQUIRE((filterMask & 0x1U) != 0U);
    REQUIRE(storedSize == k_ChunkBytes);

    // The writable dataset remains open and unflushed after replacing a compressed
    // chunk with skipped-deflate bytes. The metadata filter mask is authoritative even
    // if the raw-read API reports a stale mask for this live chunk.
    REQUIRE(codec.inflateChunk(0) == source);
  }
}

TEST_CASE("ParallelChunkCodec per-chunk compress + write primitives", "[ParallelChunkCodec]")
{
  SECTION("compressNominalChunk then writeCompressedChunk round-trips one interior chunk")
  {
    // Compress one interior 4 by 4 chunk with the public primitive.
    // Store it with the write primitive and compare a serial read with the nominal bytes.
    const std::vector<uint64> tupleShape = {8, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize numComponents = product(componentShape);
    const usize chunkTuples = product(chunkShape);
    const usize nominalElements = chunkTuples * numComponents;

    std::vector<uint16> nominalValues(nominalElements);
    for(usize i = 0; i < nominalElements; ++i)
    {
      nominalValues[i] = static_cast<uint16>((i * 23 + 7) & 0xFFFF);
    }
    std::vector<std::byte> nominalBytes(nominalElements * elementSize);
    std::memcpy(nominalBytes.data(), nominalValues.data(), nominalBytes.size());

    const fs::path filePath = testFilePath("ParallelChunkCodec_primitives.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    // Chunk 1 is interior, so its nominal and clamped layouts are identical.
    const uint64 targetChunk = 1;
    std::string errorOut;
    const std::vector<std::byte> compressed = codec.compressNominalChunk(targetChunk, nominalBytes, &errorOut);
    REQUIRE_FALSE(compressed.empty());
    REQUIRE(errorOut.empty());

    REQUIRE(codec.writeCompressedChunk(targetChunk, compressed, &errorOut));
    REQUIRE(errorOut.empty());

    const Extent bounds = getChunkBounds(targetChunk, tupleShape, chunkShape);
    const std::vector<std::byte> readBack = serialReadChunkRegion(dataset, bounds, componentShape, elementSize, numComponents, H5T_NATIVE_UINT16);
    H5Dclose(dataset);
    H5Fclose(fileId);

    REQUIRE(readBack == nominalBytes);
  }

  SECTION("writeCompressedChunk on a closed dataset handle reports the failure")
  {
    // A closed dataset handle must make H5Dwrite_chunk fail and populate errorOut.
    const std::vector<uint64> tupleShape = {8, 8};
    const std::vector<uint64> componentShape = {1};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize elementSize = sizeof(uint16);
    const usize nominalElements = product(chunkShape) * product(componentShape);

    std::vector<std::byte> nominalBytes(nominalElements * elementSize, std::byte{0});

    const fs::path filePath = testFilePath("ParallelChunkCodec_primitives_fail.h5");
    hid_t fileId = H5I_INVALID_HID;
    hid_t dataset = H5I_INVALID_HID;
    createEmptyChunkedDeflateDataset(filePath, tupleShape, componentShape, chunkShape, H5T_NATIVE_UINT16, fileId, dataset);

    ParallelChunkCodec codec(filePath, k_DatasetName, tupleShape, chunkShape, componentShape, elementSize, dataset);
    REQUIRE(codec.isEligible());

    std::string compressError;
    const std::vector<std::byte> compressed = codec.compressNominalChunk(0, nominalBytes, &compressError);
    REQUIRE_FALSE(compressed.empty());
    REQUIRE(compressError.empty());

    // Invalidate the handle the codec captured, so the leaf H5Dwrite_chunk fails.
    H5Dclose(dataset);
    H5Fclose(fileId);

    std::string writeError;
    REQUIRE_FALSE(codec.writeCompressedChunk(0, compressed, &writeError));
    REQUIRE_FALSE(writeError.empty());
  }
}

TEST_CASE("DatasetIO::readIntoSpan routes compressed chunked reads through the parallel codec", "[DatasetIO][ParallelChunkCodec]")
{
  using namespace nx::core::HDF5;

  SECTION("eligible: DatasetIO write path (compression level > 0) round-trips through the codec branch")
  {
    // This dataset exceeds the small-array threshold and uses the compressed chunk path.
    // Its dimensions produce multiple chunks and an edge chunk under the writer policy.
    const std::vector<usize> dims = {64, 999};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    std::vector<uint32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint32>(i * 2654435761u + 1013904223u);
    }

    const fs::path filePath = testFilePath("DatasetIO_codec_uint32.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<uint32>(dims, nonstd::span<const uint32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    // Confirm the write produced a real chunked dataset (otherwise the read would not exercise the
    // codec branch at all and this test would prove nothing).
    REQUIRE_FALSE(dataset.getChunkDimensions().empty());

    std::vector<uint32> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint32>(nonstd::span<uint32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("eligible: bool dataset round-trips through the codec branch via the H5_BOOL_TYPE delegation")
  {
    const std::vector<usize> dims = {64, 999};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    // std::vector<bool> is bit-packed and has no contiguous bool buffer for a nonstd::span<bool>,
    // so the known source and read-back buffers are real bool[] arrays.
    auto source = std::make_unique<bool[]>(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = ((i * 7 + 1) % 3 == 0);
    }

    const fs::path filePath = testFilePath("DatasetIO_codec_bool.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<bool>(dims, nonstd::span<const bool>(source.get(), numElements));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    REQUIRE_FALSE(dataset.getChunkDimensions().empty());

    auto readBack = std::make_unique<bool[]>(numElements);
    const Result<> readResult = dataset.readIntoSpan<bool>(nonstd::span<bool>(readBack.get(), numElements));
    REQUIRE(readResult.valid());
    bool allMatch = true;
    for(usize i = 0; i < numElements; ++i)
    {
      allMatch = allMatch && (readBack[i] == source[i]);
    }
    REQUIRE(allMatch);
  }

  SECTION("eligible edge-chunk (raw-API chunked + deflate) reads byte-identically via the codec branch")
  {
    // A controlled multi-chunk geometry with an edge chunk: 10 x 7 uint16, chunk 4 x 4 -> 3 x 2
    // chunks, last row/col clamped. Built with the raw HDF5 API so the chunk shape is exact.
    const std::vector<uint64> dims = {10, 7};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize numElements = product(dims);
    const usize elementSize = sizeof(uint16);

    std::vector<uint16> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint16>((i * 11 + 5) & 0xFFFF);
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), source.data(), bytes.size());

    const fs::path filePath = testFilePath("DatasetIO_codec_edge_uint16.h5");
    createChunkedDeflateDataset(filePath, dims, {1}, chunkShape, H5T_NATIVE_UINT16, bytes);

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    REQUIRE_FALSE(dataset.getChunkDimensions().empty());

    std::vector<uint16> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint16>(nonstd::span<uint16>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("ineligible: shuffle + deflate (multi-filter) reads correctly via the serial fallback")
  {
    const std::vector<uint64> dims = {10, 7};
    const std::vector<uint64> chunkShape = {4, 4};
    const usize numElements = product(dims);
    const usize elementSize = sizeof(uint16);

    std::vector<uint16> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint16>((i * 13 + 7) & 0xFFFF);
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), source.data(), bytes.size());

    const fs::path filePath = testFilePath("DatasetIO_serial_shuffle.h5");
    createShuffleDeflateDataset(filePath, dims, chunkShape, H5T_NATIVE_UINT16, bytes);

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    // The dataset IS chunked, but the multi-filter pipeline makes the codec ineligible, so the read
    // must take the serial fallback and still be correct.
    REQUIRE_FALSE(dataset.getChunkDimensions().empty());

    std::vector<uint16> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint16>(nonstd::span<uint16>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("ineligible: contiguous (unchunked) dataset reads correctly via the serial fallback")
  {
    const std::vector<uint64> dims = {10, 7};
    const usize numElements = product(dims);
    const usize elementSize = sizeof(float32);

    std::vector<float32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<float32>(i) * 0.25f - 3.0f;
    }
    std::vector<std::byte> bytes(numElements * elementSize);
    std::memcpy(bytes.data(), source.data(), bytes.size());

    const fs::path filePath = testFilePath("DatasetIO_serial_contiguous.h5");
    createContiguousDataset(filePath, dims, H5T_NATIVE_FLOAT, bytes);

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    // No chunk dimensions -> the codec branch is skipped entirely; the serial path must be correct.
    REQUIRE(dataset.getChunkDimensions().empty());

    std::vector<float32> readBack(numElements, 0.0f);
    const Result<> readResult = dataset.readIntoSpan<float32>(nonstd::span<float32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }
}

TEST_CASE("DatasetIO::writeSpan routes compressed chunked writes through the parallel codec", "[DatasetIO][ParallelChunkCodec]")
{
  using namespace nx::core::HDF5;

  // The writer regime the in-core write path uses to pick the on-disk chunk shape.
  const ChunkShapeOptions k_WriterRegime{k_TargetChunkBytes, ChunkShapeRegime::BundleOuterSlabs};

  SECTION("eligible: single-chunk compressed write (level > 0) round-trips and writes the policy chunk shape")
  {
    // This 999 KiB array is above the small-array threshold and below the target chunk size.
    // BundleOuterSlabs therefore stores it in one chunk and exercises single-chunk gather and write.
    const std::vector<usize> dims = {64, 999};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    // Known source oracle (compared against directly, not via the codec).
    std::vector<uint32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint32>(i * 2654435761u + 1013904223u);
    }

    const fs::path filePath = testFilePath("DatasetIO_write_codec_uint32.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<uint32>(dims, nonstd::span<const uint32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);

    // The on-disk chunk shape must match the shared writer policy.
    // This proves that the write used chunked deflate rather than contiguous fallback.
    const std::vector<usize> onDiskChunk = dataset.getChunkDimensions();
    REQUIRE_FALSE(onDiskChunk.empty());
    const ShapeType expectedChunk = computeChunkShape(ShapeType(dims.begin(), dims.end()), /*numComponents=*/1, sizeof(uint32), k_WriterRegime);
    REQUIRE(ShapeType(onDiskChunk.begin(), onDiskChunk.end()) == expectedChunk);

    std::vector<uint32> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint32>(nonstd::span<uint32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("eligible: multi-chunk + edge-chunk compressed write round-trips byte-identically")
  {
    // This 20 MiB dataset produces 20 policy chunks.
    // The final chunk has 12 rows instead of the nominal 52 rows.
    // This case exercises parallel scatter and edge-chunk padding.
    const std::vector<usize> dims = {1000, 5000};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    // Known source oracle: value == flat index (modulo 2^32). Compared against directly.
    std::vector<uint32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint32>(i);
    }

    const fs::path filePath = testFilePath("DatasetIO_write_codec_multichunk_uint32.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<uint32>(dims, nonstd::span<const uint32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);

    const std::vector<usize> onDiskChunk = dataset.getChunkDimensions();
    REQUIRE_FALSE(onDiskChunk.empty());
    const ShapeType expectedChunk = computeChunkShape(ShapeType(dims.begin(), dims.end()), /*numComponents=*/1, sizeof(uint32), k_WriterRegime);
    REQUIRE(ShapeType(onDiskChunk.begin(), onDiskChunk.end()) == expectedChunk);

    // Require multiple chunks and a non-divisible outer extent.
    // These checks keep this test on the parallel edge-chunk path.
    const std::vector<uint64> chunkU64(onDiskChunk.begin(), onDiskChunk.end());
    const std::vector<uint64> dimsU64(dims.begin(), dims.end());
    REQUIRE(getNumberOfChunks(dimsU64, chunkU64) > 1);
    REQUIRE(dimsU64[0] % chunkU64[0] != 0); // edge chunk exists along the banded outer dimension

    std::vector<uint32> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint32>(nonstd::span<uint32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("eligible: multi-component (Nx3) compressed write round-trips byte-identically")
  {
    // The common real path: DataStore::writeHdf5 passes dims = tupleShape ++ componentShape, so
    // every Nx3 / Nx2 array save reaches writeSpan with a trailing component dim. 90000 x 3
    // float32 (~1.03 MiB) bands the outer dim: chunk = [87381, 3] (87381 * 3 * 4 = ~1.0 MiB).
    // 90000 / 87381 = 1 r 2619, so there are 2 chunks and the last is a clamped EDGE chunk. This
    // covers both the multi-chunk write of a component-shaped dataset and the codec's
    // componentShape={} fold (writeSpan models the full dataspace dims as one flat tuple space).
    const std::vector<usize> dims = {90000, 3};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    std::vector<float32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<float32>(i) * 0.125f - 7.0f;
    }

    const fs::path filePath = testFilePath("DatasetIO_write_codec_multicomp_float.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<float32>(dims, nonstd::span<const float32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);

    const std::vector<usize> onDiskChunk = dataset.getChunkDimensions();
    REQUIRE_FALSE(onDiskChunk.empty());
    const ShapeType expectedChunk = computeChunkShape(ShapeType(dims.begin(), dims.end()), /*numComponents=*/1, sizeof(float32), k_WriterRegime);
    REQUIRE(ShapeType(onDiskChunk.begin(), onDiskChunk.end()) == expectedChunk);

    const std::vector<uint64> chunkU64(onDiskChunk.begin(), onDiskChunk.end());
    const std::vector<uint64> dimsU64(dims.begin(), dims.end());
    REQUIRE(getNumberOfChunks(dimsU64, chunkU64) > 1);
    REQUIRE(dimsU64[0] % chunkU64[0] != 0); // edge chunk exists along the banded outer dimension

    std::vector<float32> readBack(numElements, 0.0f);
    const Result<> readResult = dataset.readIntoSpan<float32>(nonstd::span<float32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("eligible: bool write round-trips through the H5_BOOL_TYPE deflate delegation")
  {
    const std::vector<usize> dims = {64, 999};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    // std::vector<bool> is bit-packed and has no contiguous bool buffer for a nonstd::span<bool>,
    // so the known source and read-back buffers are real bool[] arrays.
    auto source = std::make_unique<bool[]>(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = ((i * 7 + 1) % 3 == 0);
    }

    const fs::path filePath = testFilePath("DatasetIO_write_codec_bool.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<bool>(dims, nonstd::span<const bool>(source.get(), numElements));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);

    // bool is stored as H5_BOOL_TYPE on disk; the policy shape uses that element size.
    const std::vector<usize> onDiskChunk = dataset.getChunkDimensions();
    REQUIRE_FALSE(onDiskChunk.empty());
    const ShapeType expectedChunk = computeChunkShape(ShapeType(dims.begin(), dims.end()), /*numComponents=*/1, sizeof(H5_BOOL_TYPE), k_WriterRegime);
    REQUIRE(ShapeType(onDiskChunk.begin(), onDiskChunk.end()) == expectedChunk);

    auto readBack = std::make_unique<bool[]>(numElements);
    const Result<> readResult = dataset.readIntoSpan<bool>(nonstd::span<bool>(readBack.get(), numElements));
    REQUIRE(readResult.valid());
    bool allMatch = true;
    for(usize i = 0; i < numElements; ++i)
    {
      allMatch = allMatch && (readBack[i] == source[i]);
    }
    REQUIRE(allMatch);
  }

  // DatasetIO writeSpan creates either eligible compressed chunks or contiguous data.
  // The sections below verify the contiguous serial fallback.
  SECTION("ineligible: uncompressed write (level 0 -> contiguous) round-trips via the serial path")
  {
    const std::vector<usize> dims = {64, 999};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    std::vector<uint32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<uint32>(i * 40503u + 7u);
    }

    const fs::path filePath = testFilePath("DatasetIO_write_serial_uncompressed.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      // Compression level 0 -> the DCPL falls through to contiguous, so writeSpan takes the
      // serial H5Dwrite backstop (the codec branch is skipped because there are no chunks).
      const Result<> writeResult = dataset.writeSpan<uint32>(dims, nonstd::span<const uint32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    // No compression requested -> contiguous layout -> no chunk dimensions.
    REQUIRE(dataset.getChunkDimensions().empty());

    std::vector<uint32> readBack(numElements, 0u);
    const Result<> readResult = dataset.readIntoSpan<uint32>(nonstd::span<uint32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }

  SECTION("ineligible: small compressed write (below the threshold -> contiguous) round-trips via the serial path")
  {
    // Below k_SmallArrayThresholdBytes (16 KiB): the DCPL bypasses chunking even with a
    // compression level set, so the write takes the serial contiguous path.
    const std::vector<usize> dims = {8, 8};
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    std::vector<float32> source(numElements);
    for(usize i = 0; i < numElements; ++i)
    {
      source[i] = static_cast<float32>(i) * 0.5f - 2.0f;
    }

    const fs::path filePath = testFilePath("DatasetIO_write_serial_small.h5");
    {
      auto fileWriter = FileIO::WriteFile(filePath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(5);
      const Result<> writeResult = dataset.writeSpan<float32>(dims, nonstd::span<const float32>(source.data(), source.size()));
      REQUIRE(writeResult.valid());
    }

    auto fileReader = FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    DatasetIO dataset = fileReader.openDataset(k_DatasetName);
    REQUIRE(dataset.getChunkDimensions().empty());

    std::vector<float32> readBack(numElements, 0.0f);
    const Result<> readResult = dataset.readIntoSpan<float32>(nonstd::span<float32>(readBack.data(), readBack.size()));
    REQUIRE(readResult.valid());
    REQUIRE(readBack == source);
  }
}

// This hidden benchmark compares end-to-end DatasetIO::writeSpan with a serial HDF5 write.
// Both paths use the same chunk shape and deflate level.
// The codec-written file must read back byte-identically through DatasetIO.
// The leading-dot tag excludes the benchmark from normal CTest runs.
TEST_CASE("DatasetIO::writeSpan in-core write benchmark: codec vs serial", "[.][benchmark]")
{
  using namespace nx::core::HDF5;
  using Clock = std::chrono::steady_clock;

  // Match the regime the in-core write path uses to choose the on-disk chunk shape.
  const ChunkShapeOptions k_WriterRegime{k_TargetChunkBytes, ChunkShapeRegime::BundleOuterSlabs};
  const int k_DeflateLevel = 5;

  WARN("ParallelChunkCodec write benchmark — hardware_concurrency = " << std::thread::hardware_concurrency());

  // These single-component float32 cases span approximately 4M, 17M, and 67M elements.
  // Each case produces multiple chunks and a clamped edge chunk.
  struct Case
  {
    std::vector<usize> dims;
    const char* label;
  };
  const std::vector<Case> cases = {
      {{4096, 1024}, "~4.2M float32 (16 MiB)"},
      {{4096, 4096}, "~16.8M float32 (64 MiB)"},
      {{8192, 8192}, "~67.1M float32 (256 MiB)"},
  };

  for(const Case& testCase : cases)
  {
    const std::vector<usize>& dims = testCase.dims;
    const usize numElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<usize>());

    // Repeat a deterministic high-entropy block so the data is compressible without reducing the
    // codec workload to an all-zero fast case. The repeated block also keeps the incompressibility
    // probe from bypassing deflate, so this benchmark measures the parallel compression path.
    std::vector<float32> source(numElements);
    {
      constexpr usize k_PatternElements = 256;
      std::mt19937_64 rng(0x9E3779B97F4A7C15ull);
      std::uniform_int_distribution<uint32> dist(0u, 0xFFFFFFFFu);
      std::vector<uint32> pattern(k_PatternElements);
      for(uint32& bits : pattern)
      {
        bits = dist(rng);
      }
      for(usize i = 0; i < numElements; ++i)
      {
        const uint32 bits = pattern[i % pattern.size()];
        std::memcpy(&source[i], &bits, sizeof(float32)); // Preserve the pattern's exact bits.
      }
    }
    const usize sourceBytes = numElements * sizeof(float32);

    // The on-disk chunk shape both paths use (exactly what writeSpan would pick for these dims).
    const ShapeType chunkShape = computeChunkShape(ShapeType(dims.begin(), dims.end()), /*numComponents=*/1, sizeof(float32), k_WriterRegime);
    const std::vector<uint64> dimsU64(dims.begin(), dims.end());
    const std::vector<uint64> chunkU64(chunkShape.begin(), chunkShape.end());
    const uint64 numChunks = getNumberOfChunks(dimsU64, chunkU64);

    // -------- Serial baseline: one H5Dwrite into a chunked+deflate dataset of that chunk shape ----
    const fs::path serialPath = testFilePath("bench_write_serial.h5");
    double serialMs = 0.0;
    {
      std::vector<hsize_t> hDims(dimsU64.begin(), dimsU64.end());
      std::vector<hsize_t> hChunk(chunkU64.begin(), chunkU64.end());
      const hid_t fileId = H5Fcreate(serialPath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
      REQUIRE(fileId >= 0);
      const hid_t space = H5Screate_simple(static_cast<int>(hDims.size()), hDims.data(), nullptr);
      const hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
      REQUIRE(H5Pset_chunk(dcpl, static_cast<int>(hChunk.size()), hChunk.data()) >= 0);
      REQUIRE(H5Pset_deflate(dcpl, static_cast<unsigned int>(k_DeflateLevel)) >= 0);
      const hid_t dataset = H5Dcreate(fileId, k_DatasetName.c_str(), H5T_NATIVE_FLOAT, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
      REQUIRE(dataset >= 0);

      const auto t0 = Clock::now();
      REQUIRE(H5Dwrite(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, source.data()) >= 0);
      const auto t1 = Clock::now();
      serialMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

      H5Dclose(dataset);
      H5Pclose(dcpl);
      H5Sclose(space);
      H5Fclose(fileId);
    }

    // -------- Codec path: DatasetIO::writeSpan at level 5 (production routes through the codec) ----
    const fs::path codecPath = testFilePath("bench_write_codec.h5");
    double codecMs = 0.0;
    {
      auto fileWriter = FileIO::WriteFile(codecPath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(k_DeflateLevel);

      const auto t0 = Clock::now();
      const Result<> writeResult = dataset.writeSpan<float32>(dims, nonstd::span<const float32>(source.data(), source.size()));
      const auto t1 = Clock::now();
      codecMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
      REQUIRE(writeResult.valid());
    }
    const uintmax_t codecFileBytes = fs::file_size(codecPath);
    REQUIRE(codecFileBytes < sourceBytes);

    // Prove the codec actually wrote the policy chunk shape (i.e. it did NOT silently fall back to
    // the serial backstop, which would make the A/B compare two serial writes).
    double codecReadMs = 0.0;
    {
      auto fileReader = FileIO::ReadFile(codecPath);
      REQUIRE(fileReader.isValid());
      DatasetIO dataset = fileReader.openDataset(k_DatasetName);
      const std::vector<usize> onDiskChunk = dataset.getChunkDimensions();
      REQUIRE_FALSE(onDiskChunk.empty());
      REQUIRE(ShapeType(onDiskChunk.begin(), onDiskChunk.end()) == chunkShape);

      // Byte-identical read-back of the codec-written file through the normal serial read API.
      std::vector<float32> readBack(numElements, 0.0f);
      const auto t0 = Clock::now();
      const Result<> readResult = dataset.readIntoSpan<float32>(nonstd::span<float32>(readBack.data(), readBack.size()));
      const auto t1 = Clock::now();
      codecReadMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
      REQUIRE(readResult.valid());
      REQUIRE(std::memcmp(readBack.data(), source.data(), sourceBytes) == 0);
    }

    // -------- Contiguous path: compression level 0 must bypass the codec entirely -----------------
    const fs::path contiguousPath = testFilePath("bench_write_contiguous.h5");
    double contiguousWriteMs = 0.0;
    {
      auto fileWriter = FileIO::WriteFile(contiguousPath);
      REQUIRE(fileWriter.isValid());
      DatasetIO dataset = fileWriter.createDataset(k_DatasetName);
      dataset.setCompressionLevel(0);

      const auto t0 = Clock::now();
      const Result<> writeResult = dataset.writeSpan<float32>(dims, nonstd::span<const float32>(source.data(), source.size()));
      const auto t1 = Clock::now();
      contiguousWriteMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
      REQUIRE(writeResult.valid());
    }

    double contiguousReadMs = 0.0;
    {
      auto fileReader = FileIO::ReadFile(contiguousPath);
      REQUIRE(fileReader.isValid());
      DatasetIO dataset = fileReader.openDataset(k_DatasetName);
      REQUIRE(dataset.getChunkDimensions().empty());

      std::vector<float32> readBack(numElements, 0.0f);
      const auto t0 = Clock::now();
      const Result<> readResult = dataset.readIntoSpan<float32>(nonstd::span<float32>(readBack.data(), readBack.size()));
      const auto t1 = Clock::now();
      contiguousReadMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
      REQUIRE(readResult.valid());
      REQUIRE(std::memcmp(readBack.data(), source.data(), sourceBytes) == 0);
    }

    const double speedup = (codecMs > 0.0) ? (serialMs / codecMs) : 0.0;
    const double compressionRatio = static_cast<double>(codecFileBytes) / static_cast<double>(sourceBytes);
    WARN("WRITE " << testCase.label << " | chunks=" << numChunks << " chunkShape=[" << chunkU64[0] << "," << chunkU64[1] << "]"
                  << " | serial H5Dwrite=" << serialMs << " ms"
                  << " | codec writeSpan=" << codecMs << " ms"
                  << " | codec readIntoSpan=" << codecReadMs << " ms"
                  << " | contiguous write=" << contiguousWriteMs << " ms"
                  << " | contiguous read=" << contiguousReadMs << " ms"
                  << " | file/source=" << compressionRatio << " | speedup=" << speedup << "x");
  }
}

// ParallelLoadChunks is a pure threads/callables engine: these cases exercise its partition,
// per-chunk skip policy, and error-propagation contract WITHOUT any HDF5 dependency.
TEST_CASE("ParallelChunkCodec: ParallelLoadChunks fills every slot", "[ParallelLoadChunks]")
{
  const std::vector<uint64> chunkIndices = {0, 1, 2, 3, 4};
  std::vector<uint64> results(chunkIndices.size(), 0);

  ParallelLoadChunks<uint64>(
      nonstd::span<const uint64>(chunkIndices.data(), chunkIndices.size()), [](uint64 chunkIndex) -> uint64 { return chunkIndex * 10; },
      [&results](usize localIndex, uint64&& value) { results[localIndex] = std::move(value); });

  REQUIRE(results == std::vector<uint64>{0, 10, 20, 30, 40});
}

TEST_CASE("ParallelChunkCodec: ParallelLoadChunks dispatches concurrent loader work", "[ParallelLoadChunks]")
{
  constexpr usize k_ChunkCount = 64;
  std::vector<uint64> chunkIndices(k_ChunkCount);
  std::iota(chunkIndices.begin(), chunkIndices.end(), uint64{0});
  std::atomic<usize> activeLoaders = 0;
  std::atomic<usize> maximumActiveLoaders = 0;

  ParallelLoadChunks<uint64>(
      nonstd::span<const uint64>(chunkIndices.data(), chunkIndices.size()),
      [&activeLoaders, &maximumActiveLoaders](uint64 chunkIndex) -> uint64 {
        const usize active = activeLoaders.fetch_add(1, std::memory_order_relaxed) + 1;
        usize observedMaximum = maximumActiveLoaders.load(std::memory_order_relaxed);
        while(observedMaximum < active && !maximumActiveLoaders.compare_exchange_weak(observedMaximum, active, std::memory_order_relaxed))
        {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        activeLoaders.fetch_sub(1, std::memory_order_relaxed);
        return chunkIndex;
      },
      [](usize, uint64&&) {});

#ifdef SIMPLNX_ENABLE_MULTICORE
  if(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism) > 1)
  {
    REQUIRE(maximumActiveLoaders.load(std::memory_order_relaxed) > 1);
  }
  else
#endif
  {
    REQUIRE(maximumActiveLoaders.load(std::memory_order_relaxed) == 1);
  }
}

TEST_CASE("ParallelChunkCodec: ParallelLoadChunks skips unallocated chunks", "[ParallelLoadChunks]")
{
  const std::vector<uint64> chunkIndices = {0, 1, 2, 3, 4};
  // Sentinel: a slot left at this value proves the sink was never called for it.
  constexpr uint64 k_Untouched = 999;
  std::vector<uint64> results(chunkIndices.size(), k_Untouched);

  ParallelLoadChunks<uint64>(
      nonstd::span<const uint64>(chunkIndices.data(), chunkIndices.size()),
      [](uint64 chunkIndex) -> uint64 {
        if(chunkIndex == 2)
        {
          throw UnallocatedChunkError("chunk 2 is unallocated");
        }
        return chunkIndex * 10;
      },
      [&results](usize localIndex, uint64&& value) { results[localIndex] = std::move(value); });

  // Slot 2 is skipped (sink never called); every other slot is filled.
  REQUIRE(results == std::vector<uint64>{0, 10, k_Untouched, 30, 40});
}

TEST_CASE("ParallelChunkCodec: ParallelLoadChunks rethrows a non-skip error", "[ParallelLoadChunks]")
{
  const std::vector<uint64> chunkIndices = {0, 1, 2, 3, 4};
  std::vector<uint64> results(chunkIndices.size(), 0);

  // A genuine (non-UnallocatedChunkError) loader failure must be rethrown after all workers join.
  REQUIRE_THROWS_AS(ParallelLoadChunks<uint64>(
                        nonstd::span<const uint64>(chunkIndices.data(), chunkIndices.size()),
                        [](uint64 chunkIndex) -> uint64 {
                          if(chunkIndex == 3)
                          {
                            throw std::runtime_error("genuine inflate failure");
                          }
                          return chunkIndex * 10;
                        },
                        [&results](usize localIndex, uint64&& value) { results[localIndex] = std::move(value); }),
                    std::runtime_error);
}
