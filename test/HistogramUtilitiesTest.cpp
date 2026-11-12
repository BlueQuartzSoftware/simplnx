#include "simplnx/Utilities/HistogramUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>

using namespace nx::core;

TEST_CASE("simplnx::HistogramUtilities: bin range indices", "[simplnx][HistogramUtilities]")
{
  const usize bin = GENERATE(static_cast<usize>(std::numeric_limits<int32>::max()) - 1, static_cast<usize>(std::numeric_limits<int32>::max()));
  CAPTURE(bin);
  const usize lowerIndex = HistogramUtilities::serial::BinRangeIndex(bin);
  if(bin == static_cast<usize>(std::numeric_limits<int32>::max()))
  {
    CHECK(lowerIndex == uint64{4294967294});
    CHECK(lowerIndex + 1 == uint64{4294967295});
  }
  else
  {
    CHECK(lowerIndex == uint64{4294967292});
    CHECK(lowerIndex + 1 == uint64{4294967293});
  }
}

TEST_CASE("simplnx::HistogramUtilities: paired bin ranges control", "[simplnx][HistogramUtilities]")
{
  std::array<float32, 10> ranges{};
  HistogramUtilities::serial::FillBinRanges(ranges, std::pair<float32, float32>{0, 10}, 5);
  const std::array<float32, 10> expected = {0, 2, 2, 4, 4, 6, 6, 8, 8, 10};
  CHECK(ranges == expected);

  std::array<float32, 2> oneRange{};
  HistogramUtilities::serial::FillBinRanges(oneRange, std::pair<float32, float32>{0, 10}, 1);
  CHECK(oneRange == std::array<float32, 2>{0, 10});
}
