#include "simplnx/Utilities/ChunkAlignedTransfer.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nx::core;

namespace Catch
{
/**
 * @brief Prints a piece in assertion messages.
 */
template <>
struct StringMaker<ChunkAlignedPiece>
{
  static std::string convert(const ChunkAlignedPiece& piece)
  {
    return "{" + std::to_string(piece.startTuple) + ", " + std::to_string(piece.tupleCount) + ", " + (piece.isChunkAligned ? "aligned" : "unaligned") + "}";
  }
};
} // namespace Catch

namespace
{
using Pieces = std::vector<ChunkAlignedPiece>;

/**
 * @brief Checks the structural contract of a plan: ordered, gap-free, capped, and flagged by the alignment rule.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension.
 * @param startTuple First flat tuple of the planned range.
 * @param tupleCount Number of tuples in the planned range.
 * @param maxTuples Cap that the plan used.
 * @param pieces Plan to check.
 */
void RequireWellFormedPlan(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuples, const Pieces& pieces)
{
  usize next = startTuple;
  for(const ChunkAlignedPiece& piece : pieces)
  {
    REQUIRE(piece.startTuple == next);
    REQUIRE(piece.tupleCount > 0);
    REQUIRE(piece.tupleCount <= std::max<usize>(1, maxTuples));
    REQUIRE(piece.isChunkAligned == IsChunkAlignedTupleRange(tupleShape, chunkShape, piece.startTuple, piece.tupleCount));
    next += piece.tupleCount;
  }
  REQUIRE(next == startTuple + tupleCount);
}

/**
 * @brief Plans a range and checks the structural contract before returning the plan.
 * @param tupleShape Row-major tuple shape.
 * @param chunkShape Chunk extent for each tuple dimension.
 * @param startTuple First flat tuple of the range.
 * @param tupleCount Number of tuples in the range.
 * @param maxTuples Largest permitted piece.
 * @return Pieces of the plan.
 */
Pieces Plan(const ShapeType& tupleShape, const ShapeType& chunkShape, usize startTuple, usize tupleCount, usize maxTuples)
{
  Pieces pieces = PlanChunkAlignedPieces(tupleShape, chunkShape, startTuple, tupleCount, maxTuples);
  RequireWellFormedPlan(tupleShape, chunkShape, startTuple, tupleCount, maxTuples, pieces);
  return pieces;
}
} // namespace

TEST_CASE("ChunkAlignedTransfer classifies ranges with the parallel chunk writer rule", "[simplnx][ChunkAlignedTransfer]")
{
  const ShapeType rowBand{3, 10, 8};
  const ShapeType rowBandChunk{1, 4, 8};

  SECTION("Row bands start and end on chunk boundaries or at the dataset end")
  {
    CHECK(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 0, 32));
    CHECK(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 64, 16));       // the last band of a slice is shorter
    CHECK(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 0, 240));       // whole slices
    CHECK(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 80, 160));      // slices 1 and 2
    CHECK_FALSE(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 8, 32));  // starts inside a band
    CHECK_FALSE(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 0, 24));  // ends inside a band
    CHECK_FALSE(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 64, 32)); // crosses into the next slice mid-band
    CHECK_FALSE(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 1, 7));   // part of one row
    CHECK_FALSE(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 0, 0));
  }

  SECTION("A slower dimension must hold one-index clamped chunks")
  {
    const ShapeType tupleShape{5, 4, 6};
    const ShapeType chunkShape{2, 2, 6};
    CHECK_FALSE(IsChunkAlignedTupleRange(tupleShape, chunkShape, 0, 12)); // two rows of slice 0; dimension 0 chunk is 2
    CHECK(IsChunkAlignedTupleRange(tupleShape, chunkShape, 0, 48));       // two whole slices
    CHECK(IsChunkAlignedTupleRange(tupleShape, chunkShape, 96, 12));      // slice 4 is a one-slice edge chunk
    CHECK(IsChunkAlignedTupleRange(tupleShape, chunkShape, 96, 24));      // the whole edge slice
    CHECK_FALSE(IsChunkAlignedTupleRange(tupleShape, chunkShape, 96, 6)); // one row inside a two-row chunk
  }

  SECTION("Single tuples are aligned only when every clamped chunk extent is one")
  {
    CHECK(IsChunkAlignedTupleRange({3, 4}, {1, 1}, 5, 1));
    CHECK(IsChunkAlignedTupleRange({3, 5}, {1, 2}, 4, 1)); // x = 4 is a one-tuple edge chunk
    CHECK_FALSE(IsChunkAlignedTupleRange({3, 5}, {1, 2}, 2, 1));
    CHECK_FALSE(IsChunkAlignedTupleRange({3, 5}, {2, 1}, 2, 1));
    CHECK(IsChunkAlignedTupleRange({3, 5}, {2, 1}, 12, 1)); // row 2 is a one-row edge chunk
  }

  SECTION("Chunk extents larger than the dataset are clamped")
  {
    CHECK(IsChunkAlignedTupleRange({10, 20}, {16, 64}, 0, 200));
    CHECK_FALSE(IsChunkAlignedTupleRange({10, 20}, {16, 64}, 0, 100));
    CHECK(IsChunkAlignedTupleRange({1000}, {4096}, 0, 1000));
    CHECK_FALSE(IsChunkAlignedTupleRange({1000}, {4096}, 0, 999));
  }

  SECTION("An invalid grid aligns nothing")
  {
    CHECK_FALSE(IsChunkAlignedTupleRange({100}, {}, 0, 100));
    CHECK_FALSE(IsChunkAlignedTupleRange({100}, {0}, 0, 100));
    CHECK_FALSE(IsChunkAlignedTupleRange({10, 10}, {10}, 0, 100));
  }

  SECTION("A range past the last tuple throws")
  {
    CHECK_THROWS_AS(IsChunkAlignedTupleRange(rowBand, rowBandChunk, 200, 41), std::out_of_range);
    CHECK_THROWS_AS(PlanChunkAlignedPieces(rowBand, rowBandChunk, 241, 0, 10), std::out_of_range);
  }
}

TEST_CASE("ChunkAlignedTransfer computes the nominal aligned unit", "[simplnx][ChunkAlignedTransfer]")
{
  CHECK(ComputeNominalChunkAlignedUnitTuples({3, 10, 8}, {1, 4, 8}) == 32);
  CHECK(ComputeNominalChunkAlignedUnitTuples({10, 4, 6}, {4, 4, 6}) == 96);
  CHECK(ComputeNominalChunkAlignedUnitTuples({2, 128, 256}, {1, 64, 64}) == 64 * 256);
  CHECK(ComputeNominalChunkAlignedUnitTuples({3, 4}, {1, 1}) == 1);
  CHECK(ComputeNominalChunkAlignedUnitTuples({1, 4, 6}, {8, 2, 6}) == 12); // a one-slice dataset clamps dimension 0 to 1
  CHECK(ComputeNominalChunkAlignedUnitTuples({10, 20}, {16, 64}) == 200);
  CHECK(ComputeNominalChunkAlignedUnitTuples({1000}, {256}) == 256);
  CHECK(ComputeNominalChunkAlignedUnitTuples({100}, {}) == 0);
  CHECK(ComputeNominalChunkAlignedUnitTuples({100}, {0}) == 0);
  CHECK(ComputeNominalChunkAlignedUnitTuples({0, 5}, {1, 5}) == 0);
}

TEST_CASE("ChunkAlignedTransfer splits ranges into chunk-aligned pieces", "[simplnx][ChunkAlignedTransfer]")
{
  SECTION("Row-band grid {1, r, X}")
  {
    const ShapeType tupleShape{3, 10, 8};
    const ShapeType chunkShape{1, 4, 8};
    CHECK(Plan(tupleShape, chunkShape, 0, 240, 1000) == Pieces{{0, 240, true}});
    // With room for six rows, the last two-row band of each slice joins the band before it and ends at the dataset end.
    CHECK(Plan(tupleShape, chunkShape, 0, 240, 50) == Pieces{{0, 32, true}, {32, 48, true}, {80, 32, true}, {112, 48, true}, {160, 32, true}, {192, 48, true}});
    CHECK(Plan(tupleShape, chunkShape, 0, 240, 40) ==
          Pieces{{0, 32, true}, {32, 32, true}, {64, 16, true}, {80, 32, true}, {112, 32, true}, {144, 16, true}, {160, 32, true}, {192, 32, true}, {224, 16, true}});
  }

  SECTION("Offsets and partial edge chunks")
  {
    const ShapeType tupleShape{3, 10, 8};
    const ShapeType chunkShape{1, 4, 8};
    // The unaligned head ends at the next band boundary; the tail is shorter than a band.
    CHECK(Plan(tupleShape, chunkShape, 5, 100, 1000) == Pieces{{5, 27, false}, {32, 48, true}, {80, 25, false}});
    CHECK(Plan(tupleShape, chunkShape, 5, 100, 40) == Pieces{{5, 27, false}, {32, 32, true}, {64, 16, true}, {80, 25, false}});
  }

  SECTION("Multi-slice grid {k, Y, X}")
  {
    const ShapeType tupleShape{10, 4, 6};
    const ShapeType chunkShape{4, 4, 6};
    CHECK(Plan(tupleShape, chunkShape, 0, 240, 1000) == Pieces{{0, 240, true}});
    // The last chunk holds two slices and ends at the dataset end.
    CHECK(Plan(tupleShape, chunkShape, 0, 240, 200) == Pieces{{0, 192, true}, {192, 48, true}});
    CHECK(Plan(tupleShape, chunkShape, 30, 170, 1000) == Pieces{{30, 66, false}, {96, 96, true}, {192, 8, false}});
  }

  SECTION("Chunks narrower than the rows {1, 64, 64} give aligned 64-row pieces")
  {
    const ShapeType tupleShape{2, 128, 256};
    const ShapeType chunkShape{1, 64, 64};
    CHECK(Plan(tupleShape, chunkShape, 0, 65536, 100 * 256) == Pieces{{0, 16384, true}, {16384, 16384, true}, {32768, 16384, true}, {49152, 16384, true}});
    CHECK(Plan(tupleShape, chunkShape, 0, 65536, 1 << 20) == Pieces{{0, 65536, true}});
  }

  SECTION("Single-tuple pieces")
  {
    CHECK(Plan({3, 4}, {1, 1}, 5, 1, 1) == Pieces{{5, 1, true}});
    CHECK(Plan({3, 4}, {1, 1}, 5, 3, 1) == Pieces{{5, 1, true}, {6, 1, true}, {7, 1, true}});
    CHECK(Plan({3, 5}, {1, 2}, 4, 1, 1) == Pieces{{4, 1, true}});
    CHECK(Plan({3, 5}, {1, 2}, 2, 1, 1) == Pieces{{2, 1, false}});
    // A zero cap is used as one tuple.
    CHECK(Plan({3, 4}, {1, 1}, 0, 2, 0) == Pieces{{0, 1, true}, {1, 1, true}});
  }

  SECTION("1D and 2D shapes")
  {
    CHECK(Plan({1000}, {256}, 0, 1000, 5000) == Pieces{{0, 1000, true}});
    CHECK(Plan({1000}, {256}, 100, 700, 1000) == Pieces{{100, 156, false}, {256, 512, true}, {768, 32, false}});
    CHECK(Plan({10, 20}, {3, 20}, 0, 200, 70) == Pieces{{0, 60, true}, {60, 60, true}, {120, 60, true}, {180, 20, true}});
    CHECK(Plan({4, 20}, {1, 8}, 0, 40, 10) == Pieces{{0, 8, true}, {8, 8, true}, {16, 4, true}, {20, 8, true}, {28, 8, true}, {36, 4, true}});
  }

  SECTION("A count smaller than one aligned unit")
  {
    CHECK(Plan({10, 4, 6}, {4, 4, 6}, 0, 30, 1000) == Pieces{{0, 30, false}});
    CHECK(Plan({1000}, {256}, 256, 100, 1000) == Pieces{{256, 100, false}});
  }

  SECTION("An aligned unit larger than the cap is split into capped unaligned pieces")
  {
    // One aligned unit is 96 tuples; the cap is 50. Unaligned pieces stop at the unit boundaries. The edge chunk fits.
    CHECK(Plan({10, 4, 6}, {4, 4, 6}, 0, 240, 50) == Pieces{{0, 50, false}, {50, 46, false}, {96, 50, false}, {146, 46, false}, {192, 48, true}});
    // A {64, 64, 64} grid whose smallest aligned unit is 64 whole slices.
    const Pieces pieces = Plan({128, 128, 128}, {64, 64, 64}, 0, 128 * 128 * 128, 1 << 18);
    CHECK(pieces.size() == 8);
    for(const ChunkAlignedPiece& piece : pieces)
    {
      CHECK_FALSE(piece.isChunkAligned);
      CHECK(piece.tupleCount == (1 << 18));
    }
  }

  SECTION("An invalid grid gives capped unaligned pieces")
  {
    CHECK(Plan({100}, {}, 0, 100, 30) == Pieces{{0, 30, false}, {30, 30, false}, {60, 30, false}, {90, 10, false}});
    CHECK(Plan({100}, {0}, 10, 20, 30) == Pieces{{10, 20, false}});
  }

  SECTION("An empty range gives no pieces")
  {
    CHECK(Plan({100}, {10}, 100, 0, 30).empty());
    CHECK(ComputeNextChunkAlignedPiece({100}, {10}, 40, 0, 30) == ChunkAlignedPiece{40, 0, false});
  }
}
