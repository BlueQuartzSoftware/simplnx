#include "simplnx/Utilities/ImageProcessing/DiscreteGaussianEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

template <class T>
std::vector<T> RunDG(const std::vector<T>& field, usize dx, usize dy, usize dz, std::vector<float64> variance, uint32 maxKernelWidth, std::vector<float64> maxError, bool useSpacing, FloatVec3 spacing,
                     const DiscreteGaussianPassOptions& options = {})
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  DataStore<T> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ApplyDiscreteGaussian<T>(inStore, outStore, SizeVec3{dx, dy, dz}, variance, maxKernelWidth, maxError, useSpacing, spacing, shouldCancel, messageHandler, options);
  REQUIRE(r.valid());
  std::vector<T> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: kernel coefficients are normalized + symmetric", "[ImageProcessing][DiscreteGaussianEngine]")
{
  const std::vector<float64> k = nx::core::ImageProcessing::detail::GaussianKernelCoefficients(4.0, 0.01, 32);
  double sum = std::accumulate(k.begin(), k.end(), 0.0);
  REQUIRE(sum == Approx(1.0).margin(1e-9)); // normalized
  REQUIRE(k.size() % 2 == 1);               // odd length (symmetric about center)
  for(usize i = 0; i < k.size() / 2; ++i)
  {
    REQUIRE(k[i] == Approx(k[k.size() - 1 - i]).margin(1e-12)); // symmetric
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: storage plan selects automatic fusion and OOC scratch format", "[ImageProcessing][DiscreteGaussianEngine]")
{
  constexpr const char* kInputResidentFormat = "DataStore-Input";
  constexpr const char* kOutputResidentFormat = "DataStore-Output";
  constexpr const char* kInputOocFormat = "HDF5-OOC-Input";
  constexpr const char* kOutputOocFormat = "HDF5-OOC-Output";

  SECTION("resident input and resident output keep the direct path and input format")
  {
    const auto plan =
        nx::core::ImageProcessing::detail::SelectDiscreteGaussianStoragePlan(IDataStore::StoreType::InMemory, kInputResidentFormat, IDataStore::StoreType::InMemory, kOutputResidentFormat);
    REQUIRE_FALSE(plan.useFusedPlanePass);
    REQUIRE_FALSE(plan.useTemporaryRecordStore);
    REQUIRE(plan.workingDataFormat == kInputResidentFormat);
  }

  SECTION("resident input and OOC output automatically fuse and select the output format")
  {
    const auto plan = nx::core::ImageProcessing::detail::SelectDiscreteGaussianStoragePlan(IDataStore::StoreType::InMemory, kInputResidentFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(plan.useFusedPlanePass);
    REQUIRE(plan.useTemporaryRecordStore);
    REQUIRE(plan.workingDataFormat == kOutputOocFormat);
  }

  SECTION("OOC input and resident output automatically fuse and select the input format")
  {
    const auto plan = nx::core::ImageProcessing::detail::SelectDiscreteGaussianStoragePlan(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::InMemory, kOutputResidentFormat);
    REQUIRE(plan.useFusedPlanePass);
    REQUIRE(plan.useTemporaryRecordStore);
    REQUIRE(plan.workingDataFormat == kInputOocFormat);
  }

  SECTION("OOC input takes scratch-format precedence when both endpoints are OOC")
  {
    const auto plan = nx::core::ImageProcessing::detail::SelectDiscreteGaussianStoragePlan(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(plan.useFusedPlanePass);
    REQUIRE(plan.useTemporaryRecordStore);
    REQUIRE(plan.workingDataFormat == kInputOocFormat);
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: true 2D planner bounds full-width blocks and overwide tiles", "[ImageProcessing][DiscreteGaussianEngine]")
{
  constexpr usize kResidentLimit = 64ULL * 1024ULL * 1024ULL;
  const auto benchmarkPlan = ImageProcessing::detail::BuildDiscreteGaussian2DBufferPlan(5824, 5824, 4, 4, sizeof(float32), kResidentLimit);
  REQUIRE(benchmarkPlan.valid);
  REQUIRE_FALSE(benchmarkPlan.overflow);
  REQUIRE(benchmarkPlan.coreCols == 5824);
  REQUIRE(benchmarkPlan.coreRows > 0);
  REQUIRE(benchmarkPlan.coreRows < 5824);
  REQUIRE(benchmarkPlan.residentBytes <= kResidentLimit);

  const auto tilePlan = ImageProcessing::detail::BuildDiscreteGaussian2DBufferPlan(16385, 17, 4, 4, sizeof(float64), 4096);
  REQUIRE(tilePlan.valid);
  REQUIRE_FALSE(tilePlan.overflow);
  REQUIRE(tilePlan.coreRows == 1);
  REQUIRE(tilePlan.coreCols > 0);
  REQUIRE(tilePlan.coreCols < 16385);
  REQUIRE(tilePlan.residentBytes <= 4096);

  const auto oneColumnPlan = ImageProcessing::detail::BuildDiscreteGaussian2DBufferPlan(16385, 17, 4, 4, sizeof(float64), 728);
  REQUIRE(oneColumnPlan.valid);
  REQUIRE(oneColumnPlan.coreCols == 1);
  REQUIRE(oneColumnPlan.residentBytes <= 728);

  REQUIRE_FALSE(ImageProcessing::detail::BuildDiscreteGaussian2DBufferPlan(16385, 17, 4, 4, sizeof(float64), 727).valid);
  const auto overflowPlan = ImageProcessing::detail::BuildDiscreteGaussian2DBufferPlan(std::numeric_limits<usize>::max(), 17, 4, 4, sizeof(float64), kResidentLimit);
  REQUIRE_FALSE(overflowPlan.valid);
  REQUIRE(overflowPlan.overflow);
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: bounded true 2D blocks and tiles match the resident cascade", "[ImageProcessing][DiscreteGaussianEngine]")
{
  SECTION("forced multi-block")
  {
    constexpr usize kDimX = 37;
    constexpr usize kDimY = 29;
    std::vector<float32> field(kDimX * kDimY);
    for(usize index = 0; index < field.size(); ++index)
    {
      field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.17) * 13.0 + static_cast<double>((index * 11) % 23));
    }
    const std::vector<float32> expected = RunDG<float32>(field, kDimX, kDimY, 1, {2.0, 2.0, 1.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
    DiscreteGaussianPassOptions options;
    options.forceBounded2D = true;
    options.residentLimit2D = 2048;
    const std::vector<float32> actual = RunDG<float32>(field, kDimX, kDimY, 1, {2.0, 2.0, 1.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, options);
    REQUIRE(actual == expected);
  }

  SECTION("forced overwide tiles preserve integer narrowing")
  {
    constexpr usize kDimX = 1000;
    constexpr usize kDimY = 8;
    std::vector<int16> field(kDimX * kDimY);
    for(usize index = 0; index < field.size(); ++index)
    {
      field[index] = static_cast<int16>((index * 17 + index / 13) % 257 - 128);
    }
    const std::vector<int16> expected = RunDG<int16>(field, kDimX, kDimY, 1, {2.0, 2.0, 1.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
    DiscreteGaussianPassOptions options;
    options.forceBounded2D = true;
    options.residentLimit2D = 1024;
    const std::vector<int16> actual = RunDG<int16>(field, kDimX, kDimY, 1, {2.0, 2.0, 1.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, options);
    REQUIRE(actual == expected);
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: constant image preserved", "[ImageProcessing][DiscreteGaussianEngine]")
{
  const usize d = 12;
  const std::vector<float64> field(d * d * d, 20.0);
  const std::vector<float64> out = RunDG<float64>(field, d, d, d, {4.0, 4.0, 4.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  for(float64 v : out)
  {
    REQUIRE(v == Approx(20.0).margin(1e-6)); // normalized kernel preserves a constant, incl. at edge-clamped boundary
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: impulse response sums to ~1 and is symmetric along X", "[ImageProcessing][DiscreteGaussianEngine]")
{
  const usize dx = 21, dy = 5, dz = 5;
  std::vector<float64> field(dx * dy * dz, 0.0);
  const usize cx = dx / 2, cy = dy / 2, cz = dz / 2;
  field[FlatIndex(cx, cy, cz, dx, dy)] = 1.0;
  // Smooth only along X (variance 0 on Y,Z => identity there) to isolate the 1-D response on the center row.
  const std::vector<float64> out = RunDG<float64>(field, dx, dy, dz, {4.0, 0.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  double sum = 0.0;
  for(usize x = 0; x < dx; ++x)
  {
    sum += out[FlatIndex(x, cy, cz, dx, dy)];
  }
  REQUIRE(sum == Approx(1.0).margin(1e-3));
  REQUIRE(out[FlatIndex(cx, cy, cz, dx, dy)] > out[FlatIndex(cx + 2, cy, cz, dx, dy)]);
  REQUIRE(out[FlatIndex(cx + 2, cy, cz, dx, dy)] == Approx(out[FlatIndex(cx - 2, cy, cz, dx, dy)]).margin(1e-9));
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: deterministic + tall-Z streaming", "[ImageProcessing][DiscreteGaussianEngine]")
{
  const usize dx = 8, dy = 8, dz = 64;
  std::vector<int16> field(dx * dy * dz);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<int16>((i * 29) % 251);
  }
  const std::vector<int16> a = RunDG<int16>(field, dx, dy, dz, {2.0, 2.0, 2.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  const std::vector<int16> b = RunDG<int16>(field, dx, dy, dz, {2.0, 2.0, 2.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  REQUIRE(a == b);
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: batched Z streaming matches a single batch", "[ImageProcessing][DiscreteGaussianEngine]")
{
  constexpr usize kDimX = 7;
  constexpr usize kDimY = 11;
  constexpr usize kDimZ = 13;
  std::vector<int16> field(kDimX * kDimY * kDimZ);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<int16>((i * 29 + i / 3) % 251);
  }

  const std::vector<int16> singleBatch = RunDG<int16>(field, kDimX, kDimY, kDimZ, {2.0, 2.0, 2.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  DiscreteGaussianPassOptions batchedOptions;
  batchedOptions.stagingByteBudget = 1600; // Four Y rows per batch, leaving a three-row final batch.
  const std::vector<int16> batched = RunDG<int16>(field, kDimX, kDimY, kDimZ, {2.0, 2.0, 2.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, batchedOptions);
  REQUIRE(batched == singleBatch);
}

TEMPLATE_TEST_CASE("ImageProcessing::DiscreteGaussianEngine: forced fused plane pass exactly matches direct axis passes", "[ImageProcessing][DiscreteGaussianEngine]", int16, float32)
{
  constexpr usize kDimX = 7;
  constexpr usize kDimY = 11;
  constexpr usize kDimZ = 13;
  std::vector<TestType> field(kDimX * kDimY * kDimZ);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<TestType>((i * 37 + i / 5) % 251);
  }

  DiscreteGaussianPassOptions fusedOptions;
  fusedOptions.forceFusedPlanePass = true;

  SECTION("2D")
  {
    const usize values2D = kDimX * kDimY;
    field.resize(values2D);
    const std::vector<TestType> direct = RunDG<TestType>(field, kDimX, kDimY, 1, {2.0, 3.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
    const std::vector<TestType> fused = RunDG<TestType>(field, kDimX, kDimY, 1, {2.0, 3.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, fusedOptions);
    REQUIRE(fused == direct);
  }

  SECTION("3D with a short final Z-staging batch")
  {
    DiscreteGaussianPassOptions directOptions;
    directOptions.stagingByteBudget = 1600;
    fusedOptions.stagingByteBudget = 1600; // int16: 4+4+3 Y rows; float32: 2+2+2+2+2+1 Y rows.
    const std::vector<TestType> direct = RunDG<TestType>(field, kDimX, kDimY, kDimZ, {2.0, 3.0, 4.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, directOptions);
    const std::vector<TestType> fused = RunDG<TestType>(field, kDimX, kDimY, kDimZ, {2.0, 3.0, 4.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, fusedOptions);
    REQUIRE(fused == direct);
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: fused plane pass preserves the integer intermediate cast", "[ImageProcessing][DiscreteGaussianEngine]")
{
  constexpr usize kDimX = 9;
  constexpr usize kDimY = 7;
  std::vector<int16> integerField(kDimX * kDimY);
  std::vector<float64> floatingField(integerField.size());
  for(usize i = 0; i < integerField.size(); ++i)
  {
    integerField[i] = static_cast<int16>((i * 61 + (i % kDimX) * 17) % 257);
    floatingField[i] = static_cast<float64>(integerField[i]);
  }

  const std::vector<int16> direct = RunDG<int16>(integerField, kDimX, kDimY, 1, {2.0, 3.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  DiscreteGaussianPassOptions fusedOptions;
  fusedOptions.forceFusedPlanePass = true;
  const std::vector<int16> fused = RunDG<int16>(integerField, kDimX, kDimY, 1, {2.0, 3.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, fusedOptions);
  REQUIRE(fused == direct);

  const std::vector<float64> floating = RunDG<float64>(floatingField, kDimX, kDimY, 1, {2.0, 3.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  bool differsFromFloatingIntermediate = false;
  for(usize i = 0; i < fused.size(); ++i)
  {
    differsFromFloatingIntermediate = differsFromFloatingIntermediate || fused[i] != static_cast<int16>(floating[i]);
  }
  REQUIRE(differsFromFloatingIntermediate);
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: pre-cancellation leaves fused output unchanged", "[ImageProcessing][DiscreteGaussianEngine]")
{
  constexpr usize kDimX = 7;
  constexpr usize kDimY = 5;
  constexpr usize kDimZ = 3;
  constexpr int16 kPoison = 777;
  DataStore<int16> inputStore(ShapeType{kDimZ, kDimY, kDimX}, ShapeType{1}, int16{42});
  DataStore<int16> outputStore(ShapeType{kDimZ, kDimY, kDimX}, ShapeType{1}, kPoison);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};
  DiscreteGaussianPassOptions options;
  options.forceFusedPlanePass = true;

  const Result<> result =
      ApplyDiscreteGaussian<int16>(inputStore, outputStore, SizeVec3{kDimX, kDimY, kDimZ}, {2.0, 2.0, 2.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler, options);
  REQUIRE(result.valid());
  for(usize i = 0; i < outputStore.getSize(); ++i)
  {
    REQUIRE(outputStore.getValue(i) == kPoison);
  }
}

TEST_CASE("ImageProcessing::DiscreteGaussianEngine: ZeroFluxNeumann boundary keeps a ramp monotonic + linear interior", "[ImageProcessing][DiscreteGaussianEngine]")
{
  const usize dx = 41, dy = 3, dz = 3;
  std::vector<float64> field(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float64>(x); // ramp along X, constant in Y/Z
  // Smooth along X only (variance 0 on Y/Z => identity there) so the boundary behavior on the X line is isolated.
  const std::vector<float64> out = RunDG<float64>(field, dx, dy, dz, {2.0, 0.0, 0.0}, 32, {0.01, 0.01, 0.01}, false, {1.0f, 1.0f, 1.0f});
  const usize yc = dy / 2, zc = dz / 2;
  auto at = [&](usize x) { return out[FlatIndex(x, yc, zc, dx, dy)]; };
  // A symmetric, sum-normalized kernel preserves a linear ramp in the interior (sum(k)=1, sum(k*offset)=0), well away
  // from the clamped ends.
  for(usize x = 10; x <= dx - 11; ++x)
  {
    REQUIRE(at(x) == Approx(static_cast<float64>(x)).margin(1e-6));
  }
  // ZeroFluxNeumann (edge-clamp) keeps the blurred ramp monotonically non-decreasing across the WHOLE line, including
  // the boundaries, and keeps every value within the data range [0, dx-1]. A periodic boundary would wrap the large
  // right-edge values into the left edge (a non-monotonic bump / out-of-range value); this catches that class of bug.
  // Exact clamp-vs-mirror discrimination is delegated to the filter-level live-ITK full-array parity grid.
  for(usize x = 1; x < dx; ++x)
  {
    REQUIRE(at(x) >= at(x - 1) - 1e-9);
  }
  REQUIRE(at(0) >= 0.0);
  REQUIRE(at(dx - 1) <= static_cast<float64>(dx - 1) + 1e-6);
}
