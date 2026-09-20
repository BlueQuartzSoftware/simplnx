#include <catch2/catch.hpp>

#include "simplnx/Utilities/Parsing/HDF5/ChunkIndex.hpp"

using namespace nx::core;
using namespace nx::core::HDF5;

TEST_CASE("flatToNd and ndToFlat round-trip", "[ChunkIndex]")
{
  SECTION("1D shape")
  {
    std::vector<uint64> shape = {10};
    for(uint64 i = 0; i < 10; ++i)
    {
      auto nd = flatToNd(i, shape);
      REQUIRE(nd.size() == 1);
      REQUIRE(nd[0] == i);
      REQUIRE(ndToFlat(nd, shape) == i);
    }
  }

  SECTION("2D shape")
  {
    std::vector<uint64> shape = {3, 4};
    for(uint64 i = 0; i < 12; ++i)
    {
      auto nd = flatToNd(i, shape);
      REQUIRE(nd.size() == 2);
      REQUIRE(ndToFlat(nd, shape) == i);
    }
    // Verify specific values (row-major: last dim fastest)
    auto pos = flatToNd(5, shape); // 5 = 1*4 + 1
    REQUIRE(pos[0] == 1);
    REQUIRE(pos[1] == 1);
  }

  SECTION("3D shape")
  {
    std::vector<uint64> shape = {2, 3, 4};
    for(uint64 i = 0; i < 24; ++i)
    {
      auto nd = flatToNd(i, shape);
      REQUIRE(nd.size() == 3);
      REQUIRE(ndToFlat(nd, shape) == i);
    }
    // Verify specific: flat index 13 = 1*12 + 0*4 + 1
    auto pos = flatToNd(13, shape);
    REQUIRE(pos[0] == 1);
    REQUIRE(pos[1] == 0);
    REQUIRE(pos[2] == 1);
  }
}

TEST_CASE("positionToChunkNd", "[ChunkIndex]")
{
  std::vector<uint64> chunkShape = {4, 4};

  SECTION("Position at chunk boundary start")
  {
    auto chunkNd = positionToChunkNd({0, 0}, chunkShape);
    REQUIRE(chunkNd[0] == 0);
    REQUIRE(chunkNd[1] == 0);
  }

  SECTION("Position in middle of chunk")
  {
    auto chunkNd = positionToChunkNd({2, 3}, chunkShape);
    REQUIRE(chunkNd[0] == 0);
    REQUIRE(chunkNd[1] == 0);
  }

  SECTION("Position at next chunk boundary")
  {
    auto chunkNd = positionToChunkNd({4, 0}, chunkShape);
    REQUIRE(chunkNd[0] == 1);
    REQUIRE(chunkNd[1] == 0);
  }

  SECTION("Last element of a 10x10 array")
  {
    auto chunkNd = positionToChunkNd({9, 9}, chunkShape);
    REQUIRE(chunkNd[0] == 2);
    REQUIRE(chunkNd[1] == 2);
  }
}

TEST_CASE("getChunkBounds", "[ChunkIndex]")
{
  // 10x10 array with 4x4 chunks: 3x3 = 9 chunks
  // Chunk layout per dim: [0..3], [4..7], [8..9]
  std::vector<uint64> tupleShape = {10, 10};
  std::vector<uint64> chunkShape = {4, 4};

  SECTION("First chunk (0,0)")
  {
    auto bounds = getChunkBounds(0, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 0);
    REQUIRE(bounds.min[1] == 0);
    REQUIRE(bounds.max[0] == 3);
    REQUIRE(bounds.max[1] == 3);
  }

  SECTION("Middle chunk (1,1) - flat index 4")
  {
    // chunksPerDim = {3, 3}. Chunk (1,1) = flat 1*3 + 1 = 4
    auto bounds = getChunkBounds(4, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 4);
    REQUIRE(bounds.min[1] == 4);
    REQUIRE(bounds.max[0] == 7);
    REQUIRE(bounds.max[1] == 7);
  }

  SECTION("Last edge chunk (2,2) - flat index 8, clamped")
  {
    // Chunk (2,2) = flat 2*3 + 2 = 8
    auto bounds = getChunkBounds(8, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 8);
    REQUIRE(bounds.min[1] == 8);
    REQUIRE(bounds.max[0] == 9); // Clamped from 11 to 9
    REQUIRE(bounds.max[1] == 9); // Clamped from 11 to 9
  }

  SECTION("Edge chunk along only one dimension")
  {
    // Chunk (0,2) = flat 0*3 + 2 = 2
    auto bounds = getChunkBounds(2, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 0);
    REQUIRE(bounds.min[1] == 8);
    REQUIRE(bounds.max[0] == 3); // Full size
    REQUIRE(bounds.max[1] == 9); // Clamped
  }
}

TEST_CASE("getNumberOfChunks", "[ChunkIndex]")
{
  SECTION("Evenly divisible")
  {
    REQUIRE(getNumberOfChunks({8, 6}, {4, 3}) == 4); // 2 * 2
  }

  SECTION("Non-evenly divisible")
  {
    REQUIRE(getNumberOfChunks({10, 10}, {4, 4}) == 9); // 3 * 3
  }

  SECTION("1D")
  {
    REQUIRE(getNumberOfChunks({100}, {32}) == 4); // ceil(100/32) = 4
  }
}

TEST_CASE("getChunksPerDimension", "[ChunkIndex]")
{
  SECTION("Evenly divisible")
  {
    auto cpd = getChunksPerDimension({12, 8}, {4, 4});
    REQUIRE(cpd[0] == 3);
    REQUIRE(cpd[1] == 2);
  }

  SECTION("Non-evenly divisible")
  {
    auto cpd = getChunksPerDimension({10, 7}, {4, 3});
    REQUIRE(cpd[0] == 3); // ceil(10/4) = 3
    REQUIRE(cpd[1] == 3); // ceil(7/3) = 3
  }
}

TEST_CASE("ChunkIndex edge cases", "[ChunkIndex]")
{
  SECTION("1D single element array")
  {
    std::vector<uint64> tupleShape = {1};
    std::vector<uint64> chunkShape = {4};
    REQUIRE(getNumberOfChunks(tupleShape, chunkShape) == 1);
    auto bounds = getChunkBounds(0, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 0);
    REQUIRE(bounds.max[0] == 0);
  }

  SECTION("Chunk larger than array")
  {
    std::vector<uint64> tupleShape = {3, 5};
    std::vector<uint64> chunkShape = {10, 10};
    REQUIRE(getNumberOfChunks(tupleShape, chunkShape) == 1);
    auto bounds = getChunkBounds(0, tupleShape, chunkShape);
    REQUIRE(bounds.min[0] == 0);
    REQUIRE(bounds.min[1] == 0);
    REQUIRE(bounds.max[0] == 2); // Clamped to 3-1
    REQUIRE(bounds.max[1] == 4); // Clamped to 5-1
  }

  SECTION("1D array flat/nd round-trip")
  {
    std::vector<uint64> shape = {1};
    auto nd = flatToNd(0, shape);
    REQUIRE(nd[0] == 0);
    REQUIRE(ndToFlat(nd, shape) == 0);
  }
}
