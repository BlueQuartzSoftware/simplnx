#include "simplnx/Utilities/ImageProcessing/ZeroCrossingEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;
namespace zero_detail = nx::core::ImageProcessing::detail;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

template <class T>
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    m_ReadCount++;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WriteCount++;
    m_WrittenValues += buffer.size();
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

private:
  mutable usize m_MaxReadValues = 0;
  mutable usize m_ReadCount = 0;
  usize m_MaxWriteValues = 0;
  usize m_WriteCount = 0;
  usize m_WrittenValues = 0;
};

template <class T>
class OutOfCoreTransferCountingDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

template <class T>
std::vector<uint8> ZeroCrossing2DOracle(const std::vector<T>& field, usize dimX, usize dimY, uint8 foreground, uint8 background)
{
  std::vector<uint8> output(field.size(), background);
  const auto clamp = [](int64 value, int64 upper) { return value < 0 ? int64{0} : (value > upper ? upper : value); };
  const auto absolute = [](T value) -> T { return value < T{} ? static_cast<T>(-value) : value; };
  const int64 maxX = static_cast<int64>(dimX) - 1;
  const int64 maxY = static_cast<int64>(dimY) - 1;
  for(usize y = 0; y < dimY; ++y)
  {
    for(usize x = 0; x < dimX; ++x)
    {
      const T center = field[y * dimX + x];
      const auto valueAt = [&](int64 neighborX, int64 neighborY) { return field[static_cast<usize>(clamp(neighborY, maxY)) * dimX + static_cast<usize>(clamp(neighborX, maxX))]; };
      const auto crosses = [&](T neighbor, bool positiveDirection) {
        const bool signChange = ((center < T{}) && (neighbor > T{})) || ((center > T{}) && (neighbor < T{})) || ((center == T{}) && (neighbor != T{})) || ((center != T{}) && (neighbor == T{}));
        if(!signChange)
        {
          return false;
        }
        const T centerMagnitude = absolute(center);
        const T neighborMagnitude = absolute(neighbor);
        return centerMagnitude < neighborMagnitude || (centerMagnitude == neighborMagnitude && positiveDirection);
      };

      const int64 xi = static_cast<int64>(x);
      const int64 yi = static_cast<int64>(y);
      const bool hit = crosses(valueAt(xi - 1, yi), false) || crosses(valueAt(xi, yi - 1), false) || crosses(valueAt(xi + 1, yi), true) || crosses(valueAt(xi, yi + 1), true);
      output[y * dimX + x] = hit ? foreground : background;
    }
  }
  return output;
}

template <class T>
std::vector<uint8> ZeroCrossing3DOracle(const std::vector<T>& field, usize dimX, usize dimY, usize dimZ, uint8 foreground, uint8 background)
{
  std::vector<uint8> output(field.size(), background);
  const auto clamp = [](int64 value, int64 upper) { return value < 0 ? int64{0} : (value > upper ? upper : value); };
  // ITK stores the absolute value in the pixel type. Types narrower than int wrap the minimum to a negative value.
  // Wider integer types compare the exact magnitude, which the unsigned domain carries without undefined behavior.
  const auto absolute = [](T value) {
    if constexpr(std::is_integral_v<T>)
    {
      using Unsigned = std::make_unsigned_t<T>;
      const Unsigned magnitude = value < T{} ? static_cast<Unsigned>(Unsigned{0} - static_cast<Unsigned>(value)) : static_cast<Unsigned>(value);
      if constexpr(sizeof(T) < sizeof(int32))
      {
        return static_cast<T>(magnitude);
      }
      else
      {
        return magnitude;
      }
    }
    else
    {
      return value < T{} ? -value : value;
    }
  };
  const int64 maxX = static_cast<int64>(dimX) - 1;
  const int64 maxY = static_cast<int64>(dimY) - 1;
  const int64 maxZ = static_cast<int64>(dimZ) - 1;
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const T center = field[FlatIndex(x, y, z, dimX, dimY)];
        const auto valueAt = [&](int64 neighborX, int64 neighborY, int64 neighborZ) {
          return field[FlatIndex(static_cast<usize>(clamp(neighborX, maxX)), static_cast<usize>(clamp(neighborY, maxY)), static_cast<usize>(clamp(neighborZ, maxZ)), dimX, dimY)];
        };
        const auto crosses = [&](T neighbor, bool positiveDirection) {
          const bool signChange = ((center < T{}) && (neighbor > T{})) || ((center > T{}) && (neighbor < T{})) || ((center == T{}) && (neighbor != T{})) || ((center != T{}) && (neighbor == T{}));
          if(!signChange)
          {
            return false;
          }
          const auto centerMagnitude = absolute(center);
          const auto neighborMagnitude = absolute(neighbor);
          return centerMagnitude < neighborMagnitude || (centerMagnitude == neighborMagnitude && positiveDirection);
        };

        const int64 xi = static_cast<int64>(x);
        const int64 yi = static_cast<int64>(y);
        const int64 zi = static_cast<int64>(z);
        const bool hit = crosses(valueAt(xi - 1, yi, zi), false) || crosses(valueAt(xi, yi - 1, zi), false) || crosses(valueAt(xi, yi, zi - 1), false) || crosses(valueAt(xi + 1, yi, zi), true) ||
                         crosses(valueAt(xi, yi + 1, zi), true) || crosses(valueAt(xi, yi, zi + 1), true);
        output[FlatIndex(x, y, z, dimX, dimY)] = hit ? foreground : background;
      }
    }
  }
  return output;
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

template <class T>
std::vector<T> MakeSignedField(usize dimX, usize dimY, usize dimZ, uint32 seed)
{
  std::mt19937 generator(seed);
  std::vector<T> field(dimX * dimY * dimZ);
  if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<float64> distribution(-9.0, 9.0);
    for(T& value : field)
    {
      value = static_cast<T>(distribution(generator));
    }
    const std::array<T, 7> specialValues = {T{0.0}, T{-0.0}, std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(), T{0.5}, T{-0.5}};
    for(usize index = 0; index < field.size(); index += 5)
    {
      field[index] = specialValues[(index / 5) % specialValues.size()];
    }
  }
  else
  {
    std::uniform_int_distribution<int64> distribution(-9, 9);
    for(T& value : field)
    {
      value = static_cast<T>(distribution(generator));
    }
    const std::array<T, 5> specialValues = {T{0}, std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), T{-1}, T{1}};
    for(usize index = 0; index < field.size(); index += 5)
    {
      field[index] = specialValues[(index / 5) % specialValues.size()];
    }
  }
  // Mirrored neighbors create magnitude ties in both directions. The integer minimum is never negated.
  for(usize index = 11; index < field.size(); index += 11)
  {
    if constexpr(std::is_integral_v<T>)
    {
      if(field[index - 1] == std::numeric_limits<T>::min())
      {
        continue;
      }
    }
    field[index] = static_cast<T>(-field[index - 1]);
  }
  return field;
}

struct ZeroCrossingCountingResult
{
  std::vector<uint8> output;
  usize readCount = 0;
  usize maxReadValues = 0;
  usize writeCount = 0;
  usize maxWriteValues = 0;
};

template <class T>
ZeroCrossingCountingResult RunZeroCrossingCounting(const std::vector<T>& field, const SizeVec3& dims, uint8 foreground, uint8 background, uint64 budgetBytes)
{
  ScopedBudget budget(budgetBytes);
  OutOfCoreTransferCountingDataStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
  OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, uint8{255});
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> applyResult = ApplyZeroCrossing<T>(inputStore, outputStore, dims, foreground, background, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

  std::vector<uint8> output(field.size());
  for(usize index = 0; index < output.size(); ++index)
  {
    output[index] = outputStore.getValue(index);
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  return {std::move(output), inputStore.readCount(), inputStore.maxReadValues(), outputStore.writeCount(), outputStore.maxWriteValues()};
}

// The helper runs the engine with the specified marker values.
template <class T>
std::vector<uint8> RunZeroCrossing(const std::vector<T>& field, usize dx, usize dy, usize dz, uint8 foreground = 1, uint8 background = 0)
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, T{});
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  DataStore<uint8> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, background);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> r = ApplyZeroCrossing<T>(inStore, outStore, SizeVec3{dx, dy, dz}, foreground, background, shouldCancel, messageHandler);
  REQUIRE(r.valid());
  std::vector<uint8> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::ZeroCrossingEngine: direct route matches the axial oracle", "[ImageProcessing][ZeroCrossingEngine]", int32, float32, float64)
{
  using T = TestType;
  constexpr uint8 k_Foreground = 7;
  constexpr uint8 k_Background = 3;
  const std::array<SizeVec3, 10> dimensions = {SizeVec3{1, 1, 1}, SizeVec3{1, 1, 7}, SizeVec3{7, 1, 1},    SizeVec3{1, 5, 1},  SizeVec3{2, 2, 2},
                                               SizeVec3{3, 3, 3}, SizeVec3{7, 6, 5}, SizeVec3{17, 13, 11}, SizeVec3{33, 9, 7}, SizeVec3{5, 40, 3}};

  for(usize dimensionsIndex = 0; dimensionsIndex < dimensions.size(); ++dimensionsIndex)
  {
    const SizeVec3& dims = dimensions[dimensionsIndex];
    DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2])
    {
      const std::vector<T> field = MakeSignedField<T>(dims[0], dims[1], dims[2], uint32{0x5EED0001u} + static_cast<uint32>(dimensionsIndex));
      const std::vector<uint8> expected = ZeroCrossing3DOracle(field, dims[0], dims[1], dims[2], k_Foreground, k_Background);
      const std::vector<uint8> actual = RunZeroCrossing(field, dims[0], dims[1], dims[2], k_Foreground, k_Background);
      REQUIRE(actual.size() == expected.size());
      for(usize index = 0; index < actual.size(); ++index)
      {
        CAPTURE(index);
        REQUIRE(actual[index] == expected[index]);
      }
    }
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::ZeroCrossingEngine: slab route matches the axial oracle at every slab depth", "[ImageProcessing][ZeroCrossingEngine][WorkingMemory]", int32, float32)
{
  using T = TestType;
  constexpr uint8 k_Foreground = 7;
  constexpr uint8 k_Background = 3;
  const std::array<SizeVec3, 3> dimensions = {SizeVec3{7, 6, 5}, SizeVec3{17, 13, 11}, SizeVec3{5, 40, 6}};

  for(usize dimensionsIndex = 0; dimensionsIndex < dimensions.size(); ++dimensionsIndex)
  {
    const SizeVec3& dims = dimensions[dimensionsIndex];
    const usize sliceValues = dims[0] * dims[1];
    const uint64 planeInputBytes = sliceValues * sizeof(T);
    const uint64 planeOutputBytes = sliceValues * sizeof(uint8);
    auto usefulResult = zero_detail::ZeroCrossingSlabUsefulBytes<T>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(usefulResult);
    const uint64 usefulBytes = usefulResult.value();
    const uint64 minimumFootprintBytes = 3 * planeInputBytes + planeOutputBytes;
    const uint64 lowGrantBytes = minimumFootprintBytes / 2;
    const std::array<uint64, 3> budgets = {4 * lowGrantBytes, 4 * (4 * planeInputBytes + 2 * planeOutputBytes), 4 * usefulBytes};
    const std::array<usize, 3> expectedCorePlanes = {1, 2, dims[2]};
    const std::vector<T> field = MakeSignedField<T>(dims[0], dims[1], dims[2], uint32{0x5EED1001u} + static_cast<uint32>(dimensionsIndex));
    const std::vector<uint8> expected = ZeroCrossing3DOracle(field, dims[0], dims[1], dims[2], k_Foreground, k_Background);

    for(usize budgetIndex = 0; budgetIndex < budgets.size(); ++budgetIndex)
    {
      const uint64 budgetBytes = budgets[budgetIndex];
      DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2] << " with budget " << budgetBytes)
      {
        const zero_detail::ZeroCrossingSlabPlan plan = zero_detail::PlanZeroCrossingSlab<T>(dims, budgetBytes / 4);
        REQUIRE(plan.corePlanes == expectedCorePlanes[budgetIndex]);
        const ZeroCrossingCountingResult result = RunZeroCrossingCounting(field, dims, k_Foreground, k_Background, budgetBytes);
        REQUIRE(result.output == expected);
        const usize expectedTransferCount = (dims[2] + plan.corePlanes - 1) / plan.corePlanes;
        REQUIRE(result.readCount == expectedTransferCount);
        REQUIRE(result.writeCount == expectedTransferCount);
        if(plan.corePlanes == dims[2])
        {
          REQUIRE(result.maxReadValues == plan.inputPlanes * sliceValues);
        }
        else
        {
          REQUIRE(result.maxReadValues <= plan.inputPlanes * sliceValues);
        }
        REQUIRE(result.maxWriteValues == plan.corePlanes * sliceValues);
      }
    }
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: a 1D sign flip marks the closer-to-zero side", "[ImageProcessing][ZeroCrossingEngine]")
{
  // Row: -2 -1 1 2. Crossing between x=1 (val -1) and x=2 (val 1). |−1|==|1| -> tie -> the POSITIVE-direction neighbor
  // wins, so x=1 (whose +x neighbor is x=2) is marked; x=2's crossing neighbor is x=1 (negative direction) with equal
  // magnitude -> NOT marked by that tie. So only x=1 is a zero crossing.
  const std::vector<int32> field = {-2, -1, 1, 2};
  const std::vector<uint8> out = RunZeroCrossing<int32>(field, 4, 1, 1);
  REQUIRE(out[0] == 0u);
  REQUIRE(out[1] == 1u);
  REQUIRE(out[2] == 0u);
  REQUIRE(out[3] == 0u);
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: unequal magnitudes mark the smaller-|.| side on both directions", "[ImageProcessing][ZeroCrossingEngine]")
{
  // Row: -1 3. Crossing between them; |−1| < |3| -> x=0 marked (its +x neighbor 3). x=1 (|3|) is not closer -> not marked.
  const std::vector<int32> field = {-1, 3};
  const std::vector<uint8> out = RunZeroCrossing<int32>(field, 2, 1, 1);
  REQUIRE(out[0] == 1u);
  REQUIRE(out[1] == 0u);
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: exact-zero pixel adjacent to nonzero is a crossing", "[ImageProcessing][ZeroCrossingEngine]")
{
  // Row: 0 5. |0| < |5| -> x=0 (the zero) is marked (exact-zero vs nonzero counts as a sign change).
  const std::vector<int32> field = {0, 5};
  const std::vector<uint8> out = RunZeroCrossing<int32>(field, 2, 1, 1);
  REQUIRE(out[0] == 1u);
  REQUIRE(out[1] == 0u);
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: no crossing in a monotone-positive field", "[ImageProcessing][ZeroCrossingEngine]")
{
  const std::vector<int32> field = {1, 2, 3, 4};
  const std::vector<uint8> out = RunZeroCrossing<int32>(field, 4, 1, 1);
  for(uint8 v : out)
  {
    REQUIRE(v == 0u);
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: determinism + tall-Z analytic check (float plane crossing)", "[ImageProcessing][ZeroCrossingEngine]")
{
  const usize dx = 5, dy = 5, dz = 30;
  std::vector<float32> field(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        // signed distance from the plane z==15: crosses zero across the z-window (exercises the rolling z planes).
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float32>(static_cast<int64>(z) - 15);
      }
    }
  }
  const std::vector<uint8> a = RunZeroCrossing<float32>(field, dx, dy, dz);
  const std::vector<uint8> b = RunZeroCrossing<float32>(field, dx, dy, dz);
  REQUIRE(a == b);
  // field(z) = z - 15, so only z-direction neighbors differ (in-plane neighbors are equal -> no crossing). The single
  // marked plane is z==15 (val 0): its z-1 neighbor (z=14, val -1) and z+1 neighbor (z=16, val 1) each form a sign
  // change with the exact-zero center and |0| < |±1|. z=14 (val -1) is NOT marked -- vs z=15 (0) the center is not the
  // closer side (|-1| > |0|), and vs z=13 (-2) there is no sign change; likewise z=16. So exactly plane z==15 is foreground.
  for(usize z = 0; z < dz; ++z)
  {
    const uint8 expected = (z == 15) ? 1u : 0u;
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        REQUIRE(a[FlatIndex(x, y, z, dx, dy)] == expected);
      }
    }
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: slab plan derives the plane count from the grant", "[ImageProcessing][ZeroCrossingEngine]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 largeDims{1024, 1024, 256};
  const auto requirePlan = [](const zero_detail::ZeroCrossingSlabPlan& plan, usize corePlanes, usize inputPlanes, usize inputBytes, usize outputBytes) {
    REQUIRE(plan.corePlanes == corePlanes);
    REQUIRE(plan.inputPlanes == inputPlanes);
    REQUIRE(plan.inputBytes == inputBytes);
    REQUIRE(plan.outputBytes == outputBytes);
  };

  requirePlan(zero_detail::PlanZeroCrossingSlab<float32>(largeDims, 512 * k_MiB), 100, 102, 408 * k_MiB, 100 * k_MiB);
  requirePlan(zero_detail::PlanZeroCrossingSlab<float32>(largeDims, 20 * k_MiB), 2, 4, 16 * k_MiB, 2 * k_MiB);
  requirePlan(zero_detail::PlanZeroCrossingSlab<float32>(largeDims, 1 * k_MiB), 1, 3, 12 * k_MiB, 1 * k_MiB);
  requirePlan(zero_detail::PlanZeroCrossingSlab<float32>(largeDims, 1280 * k_MiB), 256, 256, 1024 * k_MiB, 256 * k_MiB);
  requirePlan(zero_detail::PlanZeroCrossingSlab<float32>(SizeVec3{5, 6, 7}, 1024), 5, 7, 840, 150);

  auto usefulFloatResult = zero_detail::ZeroCrossingSlabUsefulBytes<float32>(SizeVec3{512, 512, 128});
  SIMPLNX_RESULT_REQUIRE_VALID(usefulFloatResult);
  REQUIRE(usefulFloatResult.value() == 160 * k_MiB);
  auto overflowResult = zero_detail::ZeroCrossingSlabUsefulBytes<float64>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);
  REQUIRE(overflowResult.errors().front().code == -8761);
  auto emptyResult = zero_detail::ZeroCrossingSlabUsefulBytes<float32>(SizeVec3{4, 4, 0});
  SIMPLNX_RESULT_REQUIRE_VALID(emptyResult);
  REQUIRE(emptyResult.value() == 0);
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: slab reservation follows the grant", "[ImageProcessing][ZeroCrossingEngine][WorkingMemory]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{512, 512, 128};
  auto usefulResult = zero_detail::ZeroCrossingSlabUsefulBytes<float32>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(usefulResult);
  const usize usefulBytes = usefulResult.value();

  {
    ScopedBudget budget(512 * k_MiB);
    {
      auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
      REQUIRE(reservation.sizeBytes() == 128 * k_MiB);
      const zero_detail::ZeroCrossingSlabPlan plan = zero_detail::PlanZeroCrossingSlab<float32>(dims, reservation.sizeBytes());
      REQUIRE(plan.corePlanes == 100);
      REQUIRE(plan.inputPlanes == 102);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
  {
    ScopedBudget budget(1024 * k_MiB);
    {
      auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
      REQUIRE(reservation.sizeBytes() == 160 * k_MiB);
      const zero_detail::ZeroCrossingSlabPlan plan = zero_detail::PlanZeroCrossingSlab<float32>(dims, reservation.sizeBytes());
      REQUIRE(plan.corePlanes == 128);
      REQUIRE(plan.inputPlanes == 128);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: slab route transfers follow the plan", "[ImageProcessing][ZeroCrossingEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint8 k_Foreground = 7;
  constexpr uint8 k_Background = 3;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<float32> field = MakeSignedField<float32>(dimX, dimY, dimZ, 0x5EED2001u);
  const std::vector<uint8> expected = ZeroCrossing3DOracle(field, dimX, dimY, dimZ, k_Foreground, k_Background);

  const auto partialPlan = zero_detail::PlanZeroCrossingSlab<float32>(dims, 1024);
  REQUIRE(partialPlan.corePlanes == 5);
  const ZeroCrossingCountingResult partialResult = RunZeroCrossingCounting(field, dims, k_Foreground, k_Background, 4096);
  REQUIRE(partialResult.output == expected);
  REQUIRE(partialResult.readCount == 2);
  REQUIRE(partialResult.maxReadValues == 6 * dimX * dimY);
  REQUIRE(partialResult.writeCount == 2);
  REQUIRE(partialResult.maxWriteValues == 5 * dimX * dimY);

  const auto completePlan = zero_detail::PlanZeroCrossingSlab<float32>(dims, 2048);
  REQUIRE(completePlan.corePlanes == 7);
  const ZeroCrossingCountingResult completeResult = RunZeroCrossingCounting(field, dims, k_Foreground, k_Background, 8192);
  REQUIRE(completeResult.output == expected);
  REQUIRE(completeResult.readCount == 1);
  REQUIRE(completeResult.maxReadValues == field.size());
  REQUIRE(completeResult.writeCount == 1);
  REQUIRE(completeResult.maxWriteValues == field.size());
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: in-core direct route performs no bulk transfers", "[ImageProcessing][ZeroCrossingEngine]")
{
  constexpr usize dimX = 17;
  constexpr usize dimY = 13;
  constexpr usize dimZ = 11;
  constexpr uint8 k_Foreground = 7;
  constexpr uint8 k_Background = 3;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<float32> field = MakeSignedField<float32>(dimX, dimY, dimZ, 0x5EED3001u);
  const std::vector<uint8> expected = ZeroCrossing3DOracle(field, dimX, dimY, dimZ, k_Foreground, k_Background);
  TransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  TransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{255});
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const Result<> result = ApplyZeroCrossing(inputStore, outputStore, dims, k_Foreground, k_Background, shouldCancel, messageHandler);
  REQUIRE(result.valid());
  REQUIRE(inputStore.readCount() == 0);
  REQUIRE(outputStore.writtenValues() == 0);
  REQUIRE(outputStore.writeCount() == 0);
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected[index]);
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: pre-cancelled run leaves the output untouched", "[ImageProcessing][ZeroCrossingEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 8;
  constexpr usize dimZ = 7;
  constexpr uint8 k_Poison = 255;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::vector<float32> field = MakeSignedField<float32>(dimX, dimY, dimZ, 0x5EED4001u);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  SECTION("direct route")
  {
    TransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    TransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, k_Poison);
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }

    const Result<> result = ApplyZeroCrossing(inputStore, outputStore, dims, uint8{7}, uint8{3}, shouldCancel, messageHandler);
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 0);
    REQUIRE(outputStore.writtenValues() == 0);
    REQUIRE(outputStore.writeCount() == 0);
    for(usize index = 0; index < field.size(); ++index)
    {
      CAPTURE(index);
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("slab route")
  {
    ScopedBudget budget(8192);
    OutOfCoreTransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, k_Poison);
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }

    const Result<> result = ApplyZeroCrossing(inputStore, outputStore, dims, uint8{7}, uint8{3}, shouldCancel, messageHandler);
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 0);
    REQUIRE(outputStore.writtenValues() == 0);
    REQUIRE(outputStore.writeCount() == 0);
    for(usize index = 0; index < field.size(); ++index)
    {
      CAPTURE(index);
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}

TEST_CASE("ImageProcessing::ZeroCrossingEngine: bounded 2D blocks and tiles preserve axial order", "[ImageProcessing][ZeroCrossingEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize dimZ = 1;
  constexpr usize totalValues = dimX * dimY;
  constexpr uint8 foreground = 7;
  constexpr uint8 background = 3;
  std::vector<int32> input(totalValues);
  for(usize y = 0; y < dimY; ++y)
  {
    for(usize x = 0; x < dimX; ++x)
    {
      int32 value = static_cast<int32>((x * 5 + y * 3) % 11) - 5;
      if((x + y) % 6 == 0)
      {
        value = 0;
      }
      input[y * dimX + x] = value;
    }
  }
  const std::vector<uint8> expected = ZeroCrossing2DOracle(input, dimX, dimY, foreground, background);

  struct TransferCase
  {
    const char* label;
    usize targetBytes;
    usize maximumReadValues;
    usize maximumWriteValues;
  };
  const std::array<TransferCase, 2> transferCases = {{{"full-width row blocks", 162, 36, 18}, {"overwide X tiles", 50, 4, 2}}};

  for(const TransferCase& transferCase : transferCases)
  {
    DYNAMIC_SECTION(transferCase.label)
    {
      TransferCountingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
      TransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, background);
      const Result<> inputCopyResult = inputStore.copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(inputCopyResult);

      std::atomic_bool shouldCancel{false};
      IFilter::MessageHandler messageHandler{};
      const Result<> applyResult = ApplyZeroCrossing<int32>(inputStore, outputStore, SizeVec3{dimX, dimY, dimZ}, foreground, background, shouldCancel, messageHandler, transferCase.targetBytes);
      SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

      std::vector<uint8> actual(totalValues);
      const Result<> outputCopyResult = outputStore.copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(outputCopyResult);
      REQUIRE(actual == expected);
      CAPTURE(inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
      REQUIRE(inputStore.maxReadValues() == transferCase.maximumReadValues);
      REQUIRE(outputStore.maxWriteValues() == transferCase.maximumWriteValues);
      REQUIRE(outputStore.writtenValues() == totalValues);
    }
  }
}
