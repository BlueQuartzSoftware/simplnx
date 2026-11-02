#include "simplnx/Utilities/ImageProcessing/FastChamferDistanceEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
constexpr float32 kW0 = 0.92644f; // axis weight
constexpr float32 kW1 = 1.34065f; // face-diagonal weight
constexpr float32 kW2 = 1.65849f; // cube-diagonal weight

usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

std::vector<float32> RunChamfer(std::vector<float32> field, usize dx, usize dy, usize dz, float32 maxDist, bool negateOutput = false)
{
  DataStore<float32> store(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  for(usize i = 0; i < field.size(); ++i)
  {
    store.setValue(i, field[i]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ApplyFastChamferDistance(store, SizeVec3{dx, dy, dz}, maxDist, shouldCancel, messageHandler, negateOutput);
  REQUIRE(r.valid());
  std::vector<float32> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = store.getValue(i);
  }
  return out;
}

struct ReferenceChamferNeighbor
{
  int32 offsetX = 0;
  int32 offsetY = 0;
  int32 offsetZ = 0;
  float32 weight = 0.0f;
};

std::vector<ReferenceChamferNeighbor> MakeReferenceForwardNeighbors()
{
  const std::array<float32, 3> weights = {kW0, kW1, kW2};
  std::vector<ReferenceChamferNeighbor> neighbors;
  neighbors.reserve(13);
  for(int32 offsetZ = -1; offsetZ <= 1; ++offsetZ)
  {
    for(int32 offsetY = -1; offsetY <= 1; ++offsetY)
    {
      for(int32 offsetX = -1; offsetX <= 1; ++offsetX)
      {
        if(9 * offsetZ + 3 * offsetY + offsetX <= 0)
        {
          continue;
        }
        const usize weightIndex = static_cast<usize>(std::abs(offsetX) + std::abs(offsetY) + std::abs(offsetZ) - 1);
        neighbors.push_back({offsetX, offsetY, offsetZ, weights[weightIndex]});
      }
    }
  }
  return neighbors;
}

std::vector<float32> ReferenceChamfer(std::vector<float32> field, usize dx, usize dy, usize dz, float32 maxDist, bool negate)
{
  const std::vector<ReferenceChamferNeighbor> forwardNeighbors = MakeReferenceForwardNeighbors();
  std::vector<ReferenceChamferNeighbor> backwardNeighbors;
  backwardNeighbors.reserve(forwardNeighbors.size());
  for(const ReferenceChamferNeighbor& neighbor : forwardNeighbors)
  {
    backwardNeighbors.push_back({-neighbor.offsetX, -neighbor.offsetY, -neighbor.offsetZ, neighbor.weight});
  }

  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        const float32 current = field[FlatIndex(x, y, z, dx, dy)];
        if(current >= maxDist || current <= -maxDist)
        {
          continue;
        }
        const bool updatePositive = current > -kW0;
        const bool updateNegative = current < kW0;
        for(const ReferenceChamferNeighbor& neighbor : forwardNeighbors)
        {
          const int64 targetX = static_cast<int64>(x) + neighbor.offsetX;
          const int64 targetY = static_cast<int64>(y) + neighbor.offsetY;
          const int64 targetZ = static_cast<int64>(z) + neighbor.offsetZ;
          if(targetX < 0 || targetX >= static_cast<int64>(dx) || targetY < 0 || targetY >= static_cast<int64>(dy) || targetZ < 0 || targetZ >= static_cast<int64>(dz))
          {
            continue;
          }
          float32& target = field[FlatIndex(static_cast<usize>(targetX), static_cast<usize>(targetY), static_cast<usize>(targetZ), dx, dy)];
          const float32 positiveCandidate = current + neighbor.weight;
          if(updatePositive && positiveCandidate < target)
          {
            target = positiveCandidate;
          }
          const float32 negativeCandidate = current - neighbor.weight;
          if(updateNegative && negativeCandidate > target)
          {
            target = negativeCandidate;
          }
        }
      }
    }
  }

  for(usize z = dz; z-- > 0;)
  {
    for(usize y = dy; y-- > 0;)
    {
      for(usize x = dx; x-- > 0;)
      {
        const float32 current = field[FlatIndex(x, y, z, dx, dy)];
        if(current >= maxDist || current <= -maxDist)
        {
          continue;
        }
        const bool updatePositive = current > -kW0;
        const bool updateNegative = current < kW0;
        for(const ReferenceChamferNeighbor& neighbor : backwardNeighbors)
        {
          const int64 targetX = static_cast<int64>(x) + neighbor.offsetX;
          const int64 targetY = static_cast<int64>(y) + neighbor.offsetY;
          const int64 targetZ = static_cast<int64>(z) + neighbor.offsetZ;
          if(targetX < 0 || targetX >= static_cast<int64>(dx) || targetY < 0 || targetY >= static_cast<int64>(dy) || targetZ < 0 || targetZ >= static_cast<int64>(dz))
          {
            continue;
          }
          float32& target = field[FlatIndex(static_cast<usize>(targetX), static_cast<usize>(targetY), static_cast<usize>(targetZ), dx, dy)];
          const float32 positiveCandidate = current + neighbor.weight;
          if(updatePositive && positiveCandidate < target)
          {
            target = positiveCandidate;
          }
          const float32 negativeCandidate = current - neighbor.weight;
          if(updateNegative && negativeCandidate > target)
          {
            target = negativeCandidate;
          }
        }
      }
    }
  }

  if(negate)
  {
    for(float32& value : field)
    {
      value = -value;
    }
  }
  return field;
}

std::vector<float32> RunChamferEngine(const std::vector<float32>& field, usize dx, usize dy, usize dz, float32 maxDist, bool negate, usize rowGroupRows)
{
  DataStore<float32> store(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  for(usize index = 0; index < field.size(); ++index)
  {
    store.setValue(index, field[index]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  FastChamferDistance engine(store, SizeVec3{dx, dy, dz}, maxDist, shouldCancel, messageHandler, ImageProcessing::detail::k_Chamfer2DResidentLimit, negate, rowGroupRows);
  const Result<> result = engine();
  REQUIRE(result.valid());
  std::vector<float32> output(field.size());
  for(usize index = 0; index < output.size(); ++index)
  {
    output[index] = store.getValue(index);
  }
  return output;
}

void RequireBitIdentical(const std::vector<float32>& actual, const std::vector<float32>& expected)
{
  REQUIRE(actual.size() == expected.size());
  for(usize index = 0; index < actual.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(std::bit_cast<uint32>(actual[index]) == std::bit_cast<uint32>(expected[index]));
  }
}

std::vector<float32> MakeBandField(usize dx, usize dy, usize dz, float32 maxDist, uint32 seed)
{
  std::mt19937 generator(seed);
  std::uniform_real_distribution<float32> distribution(-3.0f, 3.0f);
  std::vector<float32> field(dx * dy * dz);
  const usize boundaryPeriod = std::max<usize>(1, dx / 2);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      const usize boundaryX = dx / 3 + ((2 * y + z) % boundaryPeriod);
      for(usize x = 0; x < dx; ++x)
      {
        float32 value = distribution(generator);
        if(x < boundaryX && boundaryX - x > 1)
        {
          value = -(maxDist + 1.0f);
        }
        else if(x > boundaryX && x - boundaryX > 1)
        {
          value = maxDist + 1.0f;
        }
        field[FlatIndex(x, y, z, dx, dy)] = value;
      }
    }
  }
  return field;
}

std::vector<float32> MakeRandomField(usize dx, usize dy, usize dz, float32 maxDist, uint32 seed)
{
  std::mt19937 generator(seed);
  std::uniform_real_distribution<float32> distribution(-maxDist - 2.0f, maxDist + 2.0f);
  std::vector<float32> field(dx * dy * dz);
  for(float32& value : field)
  {
    value = distribution(generator);
  }

  const std::array<float32, 7> replacements = {+0.0f, -0.0f, kW0, -kW0, maxDist, -maxDist, 0.5f * kW0};
  usize replacementIndex = 0;
  for(usize index = 0; index < field.size(); index += 7)
  {
    field[index] = replacements[replacementIndex % replacements.size()];
    ++replacementIndex;
  }
  return field;
}

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadValues += buffer.size();
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WrittenValues += buffer.size();
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  [[nodiscard]] usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

private:
  mutable usize m_ReadValues = 0;
  usize m_WrittenValues = 0;
};

template <class T>
class OocReportingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  [[nodiscard]] usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

private:
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
};

template <class StoreT>
std::vector<float32> ReadStoreValues(const StoreT& store)
{
  std::vector<float32> values(store.getSize());
  for(usize index = 0; index < values.size(); ++index)
  {
    values[index] = store.getValue(index);
  }
  return values;
}

// ScopedBudget sets the shared working-memory budget for one test.
// The destructor restores the previous budget after normal completion or a failed assertion.
class ScopedBudget
{
public:
  explicit ScopedBudget(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }
  ~ScopedBudget()
  {
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }
  ScopedBudget(const ScopedBudget&) = delete;
  ScopedBudget(ScopedBudget&&) = delete;
  ScopedBudget& operator=(const ScopedBudget&) = delete;
  ScopedBudget& operator=(ScopedBudget&&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget;
};
} // namespace

// A single 0-seed in a large (frozen) positive field: the two-pass chamfer computes the EXACT optimized-chamfer
// distance to the seed. Immediate neighbors are the raw weights; multi-step voxels are the cheapest weighted path.
TEST_CASE("ImageProcessing::FastChamferDistanceEngine: single seed == chamfer metric (3D)", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize D = 9;
  constexpr float32 kMax = 100.0f;
  std::vector<float32> field(D * D * D, kMax + 1.0f);
  field[FlatIndex(4, 4, 4, D, D)] = 0.0f;
  const std::vector<float32> out = RunChamfer(field, D, D, D, kMax);

  REQUIRE(out[FlatIndex(4, 4, 4, D, D)] == 0.0f);                             // seed
  REQUIRE(out[FlatIndex(5, 4, 4, D, D)] == Approx(kW0).epsilon(1e-5));        // +x axis
  REQUIRE(out[FlatIndex(3, 4, 4, D, D)] == Approx(kW0).epsilon(1e-5));        // -x axis
  REQUIRE(out[FlatIndex(4, 5, 4, D, D)] == Approx(kW0).epsilon(1e-5));        // +y axis
  REQUIRE(out[FlatIndex(4, 4, 5, D, D)] == Approx(kW0).epsilon(1e-5));        // +z axis
  REQUIRE(out[FlatIndex(5, 5, 4, D, D)] == Approx(kW1).epsilon(1e-5));        // xy face diagonal
  REQUIRE(out[FlatIndex(3, 5, 4, D, D)] == Approx(kW1).epsilon(1e-5));        // mixed-direction face diagonal
  REQUIRE(out[FlatIndex(5, 5, 5, D, D)] == Approx(kW2).epsilon(1e-5));        // xyz cube diagonal
  REQUIRE(out[FlatIndex(6, 4, 4, D, D)] == Approx(2.0f * kW0).epsilon(1e-5)); // two +x axis steps
  REQUIRE(out[FlatIndex(2, 4, 4, D, D)] == Approx(2.0f * kW0).epsilon(1e-5)); // two -x axis steps
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: single seed == chamfer metric (2D)", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize D = 9;
  constexpr float32 kMax = 100.0f;
  std::vector<float32> field(D * D, kMax + 1.0f);
  field[FlatIndex(4, 4, 0, D, D)] = 0.0f;
  const std::vector<float32> out = RunChamfer(field, D, D, 1, kMax);

  REQUIRE(out[FlatIndex(4, 4, 0, D, D)] == 0.0f);
  REQUIRE(out[FlatIndex(5, 4, 0, D, D)] == Approx(kW0).epsilon(1e-5));
  REQUIRE(out[FlatIndex(4, 3, 0, D, D)] == Approx(kW0).epsilon(1e-5));
  REQUIRE(out[FlatIndex(5, 5, 0, D, D)] == Approx(kW1).epsilon(1e-5));
  REQUIRE(out[FlatIndex(3, 3, 0, D, D)] == Approx(kW1).epsilon(1e-5));
  REQUIRE(out[FlatIndex(6, 4, 0, D, D)] == Approx(2.0f * kW0).epsilon(1e-5));
}

// A signed step with a zero band at x==C: after the chamfer, the value steps by w0 per voxel away from the band, with
// the correct sign (positive on the +x side, negative on the -x side).
TEST_CASE("ImageProcessing::FastChamferDistanceEngine: signed plane band", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize DX = 9, DY = 5, DZ = 1, C = 4;
  constexpr float32 kMax = 100.0f;
  std::vector<float32> field(DX * DY * DZ);
  for(usize y = 0; y < DY; ++y)
  {
    for(usize x = 0; x < DX; ++x)
    {
      field[FlatIndex(x, y, 0, DX, DY)] = (x < C) ? -(kMax + 1.0f) : (x > C) ? (kMax + 1.0f) : 0.0f;
    }
  }
  const std::vector<float32> out = RunChamfer(field, DX, DY, DZ, kMax);
  for(usize y = 0; y < DY; ++y)
  {
    INFO("y=" << y);
    REQUIRE(out[FlatIndex(C, y, 0, DX, DY)] == 0.0f);
    REQUIRE(out[FlatIndex(C + 1, y, 0, DX, DY)] == Approx(kW0).epsilon(1e-5));
    REQUIRE(out[FlatIndex(C + 2, y, 0, DX, DY)] == Approx(2.0f * kW0).epsilon(1e-5));
    REQUIRE(out[FlatIndex(C - 1, y, 0, DX, DY)] == Approx(-kW0).epsilon(1e-5));
    REQUIRE(out[FlatIndex(C - 2, y, 0, DX, DY)] == Approx(-2.0f * kW0).epsilon(1e-5));
  }
}

// Small maxDist freezes the far field: a voxel adjacent to the band gets w0 (< maxDist), but a voxel whose chamfer
// distance exceeds maxDist retains a magnitude >= maxDist (the propagation front stops there).
TEST_CASE("ImageProcessing::FastChamferDistanceEngine: maxDist freeze", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize DX = 12, DY = 3, DZ = 1;
  constexpr float32 kMax = 2.0f;
  std::vector<float32> field(DX * DY * DZ, kMax + 1.0f);
  for(usize y = 0; y < DY; ++y)
  {
    field[FlatIndex(0, y, 0, DX, DY)] = 0.0f; // seed band at x==0
  }
  const std::vector<float32> out = RunChamfer(field, DX, DY, DZ, kMax);
  for(usize y = 0; y < DY; ++y)
  {
    REQUIRE(out[FlatIndex(1, y, 0, DX, DY)] == Approx(kW0).epsilon(1e-5)); // within maxDist
    REQUIRE(out[FlatIndex(11, y, 0, DX, DY)] >= kMax);                     // far: frozen at/above maxDist
  }
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: deterministic", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize D = 10;
  std::vector<float32> field(D * D * D, 51.0f);
  field[FlatIndex(5, 5, 5, D, D)] = 0.0f;
  field[FlatIndex(2, 7, 3, D, D)] = -0.0f;
  const std::vector<float32> a = RunChamfer(field, D, D, D, 50.0f);
  const std::vector<float32> b = RunChamfer(field, D, D, D, 50.0f);
  REQUIRE(a.size() == b.size());
  for(usize i = 0; i < a.size(); ++i)
  {
    REQUIRE(a[i] == b[i]);
  }
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: fused output negation is exact", "[ImageProcessing][FastChamferDistanceEngine]")
{
  for(const SizeVec3 dims : {SizeVec3{7, 6, 5}, SizeVec3{9, 8, 1}})
  {
    const usize valueCount = dims[0] * dims[1] * dims[2];
    std::vector<float32> field(valueCount, 51.0f);
    field[FlatIndex(dims[0] / 2, dims[1] / 2, dims[2] / 2, dims[0], dims[1])] = 0.0f;
    const std::vector<float32> ordinary = RunChamfer(field, dims[0], dims[1], dims[2], 50.0f);
    const std::vector<float32> negated = RunChamfer(field, dims[0], dims[1], dims[2], 50.0f, true);
    REQUIRE(negated.size() == ordinary.size());
    for(usize index = 0; index < ordinary.size(); ++index)
    {
      CAPTURE(dims, index);
      REQUIRE(negated[index] == -ordinary[index]);
    }
  }
}

// Tall volume forcing many streaming-window refills: a single-seed z-column still equals the exact axis chamfer
// metric (n * w0), validating the forward/backward streaming bookkeeping.
TEST_CASE("ImageProcessing::FastChamferDistanceEngine: tall-volume streaming", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize DX = 7, DY = 6, DZ = 40;
  constexpr float32 kMax = 200.0f;
  std::vector<float32> field(DX * DY * DZ, kMax + 1.0f);
  field[FlatIndex(3, 3, 20, DX, DY)] = 0.0f;
  const std::vector<float32> out = RunChamfer(field, DX, DY, DZ, kMax);
  for(usize z = 0; z < DZ; ++z)
  {
    const usize dist = (z > 20) ? (z - 20) : (20 - z);
    INFO("z=" << z);
    REQUIRE(out[FlatIndex(3, 3, z, DX, DY)] == Approx(static_cast<float32>(dist) * kW0).epsilon(1e-5));
  }
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: resident state requires a complete dataset-scaled reservation", "[ImageProcessing][FastChamferDistanceEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr usize sliceValues = dimX * dimY;
  constexpr usize valueCount = sliceValues * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = ImageProcessing::detail::CalculateFastChamferResidentWorkingMemoryBytes(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == valueCount * sizeof(float32));
  const auto overflowResult = ImageProcessing::detail::CalculateFastChamferResidentWorkingMemoryBytes(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  REQUIRE(ImageProcessing::detail::ShouldUseFastChamferResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseFastChamferResidentState(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(256 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveFastChamferResidentWorkingMemory(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 64 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(1024 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveFastChamferResidentWorkingMemory(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: 2D planner bounds row blocks and overwide tiles", "[ImageProcessing][FastChamferDistanceEngine]")
{
  const auto benchmark = ImageProcessing::detail::BuildChamfer2DBufferPlan(5888, 5888);
  REQUIRE(benchmark.valid);
  REQUIRE(benchmark.coreCols == 5888);
  REQUIRE(benchmark.coreRows > 1);
  REQUIRE(benchmark.residentBytes <= ImageProcessing::detail::k_Chamfer2DResidentLimit);

  const auto stress = ImageProcessing::detail::BuildChamfer2DBufferPlan(16385, 1025);
  REQUIRE(stress.valid);
  REQUIRE(stress.coreCols == 16385);
  REQUIRE(stress.coreRows > 0);
  REQUIRE(stress.residentBytes <= ImageProcessing::detail::k_Chamfer2DResidentLimit);

  for(const auto dimensions : {std::array<usize, 2>{1, 16385}, std::array<usize, 2>{16385, 1}})
  {
    const auto oneDimensional = ImageProcessing::detail::BuildChamfer2DBufferPlan(dimensions[0], dimensions[1]);
    CAPTURE(dimensions[0], dimensions[1]);
    REQUIRE(oneDimensional.valid);
    REQUIRE(oneDimensional.residentBytes <= ImageProcessing::detail::k_Chamfer2DResidentLimit);
  }

  const auto overwide = ImageProcessing::detail::BuildChamfer2DBufferPlan(100000000, 2);
  REQUIRE(overwide.valid);
  REQUIRE(overwide.coreCols < 100000000);
  REQUIRE(overwide.coreRows == 1);
  REQUIRE(overwide.residentBytes <= ImageProcessing::detail::k_Chamfer2DResidentLimit);

  constexpr usize kSmallLimit = 4500;
  const auto forcedTiled = ImageProcessing::detail::BuildChamfer2DBufferPlan(100, 8, kSmallLimit);
  REQUIRE(forcedTiled.valid);
  REQUIRE(forcedTiled.coreCols < 100);
  REQUIRE(forcedTiled.coreRows == 1);
  REQUIRE(forcedTiled.residentBytes <= kSmallLimit);

  constexpr usize kOneColumnTileLimit = ImageProcessing::detail::k_Chamfer2DFixedStateBytes + ImageProcessing::detail::k_Chamfer2DMaxTileRecords * sizeof(float32);
  const auto oneColumnTiles = ImageProcessing::detail::BuildChamfer2DBufferPlan(9, 7, kOneColumnTileLimit);
  REQUIRE(oneColumnTiles.valid);
  REQUIRE(oneColumnTiles.coreCols == 1);
  REQUIRE(oneColumnTiles.residentBytes == kOneColumnTileLimit);

  const auto oneCell = ImageProcessing::detail::BuildChamfer2DBufferPlan(1, 1);
  REQUIRE(oneCell.valid);
  REQUIRE(oneCell.residentBytes <= ImageProcessing::detail::k_Chamfer2DResidentLimit);
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: 2D planner rejects invalid and overflow dimensions", "[ImageProcessing][FastChamferDistanceEngine]")
{
  const auto overflow = ImageProcessing::detail::BuildChamfer2DBufferPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max());
  const auto zeroX = ImageProcessing::detail::BuildChamfer2DBufferPlan(0, 1);
  const auto zeroY = ImageProcessing::detail::BuildChamfer2DBufferPlan(1, 0);
  REQUIRE_FALSE(overflow.valid);
  REQUIRE(overflow.overflow);
  REQUIRE_FALSE(zeroX.valid);
  REQUIRE(zeroX.overflow);
  REQUIRE_FALSE(zeroY.valid);
  REQUIRE(zeroY.overflow);
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: row-group wavefront is bit-identical to the serial recurrence", "[ImageProcessing][FastChamferDistanceEngine]")
{
  STATIC_REQUIRE(ImageProcessing::detail::k_ChamferRowGroupRows == 8);
  const std::array<SizeVec3, 12> dimensions = {SizeVec3{1, 1, 1}, SizeVec3{1, 1, 5}, SizeVec3{5, 1, 1},    SizeVec3{1, 7, 1},  SizeVec3{2, 2, 2},  SizeVec3{3, 3, 3},
                                               SizeVec3{7, 6, 5}, SizeVec3{9, 8, 1}, SizeVec3{17, 13, 11}, SizeVec3{33, 9, 7}, SizeVec3{5, 40, 3}, SizeVec3{6, 30, 20}};
  for(const SizeVec3& dims : dimensions)
  {
    const usize dx = dims[0];
    const usize dy = dims[1];
    const usize dz = dims[2];
    DYNAMIC_SECTION("dimensions " << dx << " x " << dy << " x " << dz)
    {
      const std::array<usize, 5> rowGroupCounts = {1, 2, 3, 8, dy + 5};
      for(const usize rowGroupRows : rowGroupCounts)
      {
        for(const bool negate : {false, true})
        {
          for(const float32 maxDist : {2.0f, 50.0f})
          {
            for(const bool useBandField : {true, false})
            {
              const char* family = useBandField ? "band" : "random";
              CAPTURE(rowGroupRows, negate, maxDist, family);
              const std::vector<float32> field = useBandField ? MakeBandField(dx, dy, dz, maxDist, 0xBADC0DEu) : MakeRandomField(dx, dy, dz, maxDist, 0xC0FFEEu);
              const std::vector<float32> expected = ReferenceChamfer(field, dx, dy, dz, maxDist, negate);
              const std::vector<float32> actual = RunChamferEngine(field, dx, dy, dz, maxDist, negate, rowGroupRows);
              RequireBitIdentical(actual, expected);
            }
          }
        }
      }
    }
  }
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: in-core resident path performs no bulk transfers", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize dimX = 17;
  constexpr usize dimY = 13;
  constexpr usize dimZ = 11;
  constexpr float32 kMaxDist = 50.0f;
  const std::vector<float32> field = MakeRandomField(dimX, dimY, dimZ, kMaxDist, 0x13579BDFu);
  const std::vector<float32> expected = ReferenceChamfer(field, dimX, dimY, dimZ, kMaxDist, false);
  TransferCountingDataStore<float32> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  for(usize index = 0; index < field.size(); ++index)
  {
    store.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> result = ApplyFastChamferDistance(store, SizeVec3{dimX, dimY, dimZ}, kMaxDist, shouldCancel, messageHandler);
  REQUIRE(result.valid());
  REQUIRE(store.readValues() == 0);
  REQUIRE(store.writtenValues() == 0);
  RequireBitIdentical(ReadStoreValues(store), expected);
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: pre-cancelled run leaves the store untouched", "[ImageProcessing][FastChamferDistanceEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 8;
  constexpr usize dimZ = 7;
  constexpr float32 kPoison = 77.0f;
  TransferCountingDataStore<float32> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, kPoison);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  const Result<> result = ApplyFastChamferDistance(store, SizeVec3{dimX, dimY, dimZ}, 50.0f, shouldCancel, messageHandler);
  REQUIRE(result.valid());
  for(const float32 value : store)
  {
    REQUIRE(value == kPoison);
  }
  REQUIRE(store.readValues() == 0);
  REQUIRE(store.writtenValues() == 0);
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: bounded 3-D route matches the serial recurrence across plane blocks", "[ImageProcessing][FastChamferDistanceEngine][WorkingMemory]")
{
  TemporaryRecordStoreConfig config;
  config.recordSize = 4;
  config.maxRecordsPerBatch = 1;
  config.initialRecordCount = 1;
  {
    auto scratchResult = DataStoreUtilities::CreateTemporaryRecordStore(config);
    if(scratchResult.invalid())
    {
      SUCCEED("no temporary record store provider in this build configuration");
      return;
    }
  }

  constexpr usize dimX = 7;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 40;
  constexpr float32 kMaxDist = 50.0f;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<float32> field = MakeRandomField(dimX, dimY, dimZ, kMaxDist, 0x2468ACE0u);
  const ScopedBudget budget(4096);

  for(const bool negate : {false, true})
  {
    CAPTURE(negate);
    OocReportingDataStore<float32> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    for(usize index = 0; index < field.size(); ++index)
    {
      store.setValue(index, field[index]);
    }
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    FastChamferDistance engine(store, dims, kMaxDist, shouldCancel, messageHandler, ImageProcessing::detail::k_Chamfer2DResidentLimit, negate);
    const Result<> result = engine();
    REQUIRE(result.valid());
    RequireBitIdentical(ReadStoreValues(store), ReferenceChamfer(field, dimX, dimY, dimZ, kMaxDist, negate));
    REQUIRE(store.readCount() == 3);
    REQUIRE(store.writeCount() == 3);
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}

TEST_CASE("ImageProcessing::FastChamferDistanceEngine: complete-grant route copies the volume once and matches", "[ImageProcessing][FastChamferDistanceEngine][WorkingMemory]")
{
  constexpr usize dimX = 7;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 40;
  constexpr float32 kMaxDist = 50.0f;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<float32> field = MakeRandomField(dimX, dimY, dimZ, kMaxDist, 0x2468ACE0u);
  const ScopedBudget budget(1024ULL * 1024ULL * 1024ULL);

  for(const bool negate : {false, true})
  {
    CAPTURE(negate);
    OocReportingDataStore<float32> store(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    for(usize index = 0; index < field.size(); ++index)
    {
      store.setValue(index, field[index]);
    }
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    FastChamferDistance engine(store, dims, kMaxDist, shouldCancel, messageHandler, ImageProcessing::detail::k_Chamfer2DResidentLimit, negate);
    const Result<> result = engine();
    REQUIRE(result.valid());
    RequireBitIdentical(ReadStoreValues(store), ReferenceChamfer(field, dimX, dimY, dimZ, kMaxDist, negate));
    REQUIRE(store.readCount() == 1);
    REQUIRE(store.writeCount() == 1);
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}
