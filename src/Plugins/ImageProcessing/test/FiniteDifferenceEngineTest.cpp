#include "simplnx/Utilities/ImageProcessing/FiniteDifferenceEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <cmath>
#include <limits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
template <class T>
class OutOfCoreTransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadCount++;
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_WriteCount++;
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

private:
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
  mutable usize m_MaxReadValues = 0;
  usize m_MaxWriteValues = 0;
};

template <class T>
std::vector<T> RunCurvatureFlow(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 timeStep, uint32 nIter, FloatVec3 spacing = {1.0f, 1.0f, 1.0f})
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  DataStore<T> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const nx::core::ImageProcessing::detail::CurvatureFlowFn fn{};
  const Result<> r = ApplyFiniteDifference<T>(inStore, outStore, SizeVec3{dx, dy, dz}, spacing, fn, timeStep, nIter, shouldCancel, messageHandler);
  REQUIRE(r.valid());
  std::vector<T> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: resident state requires a complete dataset-scaled reservation", "[ImageProcessing][FiniteDifferenceEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr uint64 k_GiB = 1024ULL * k_MiB;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto float64WorkResult = ImageProcessing::detail::CalculateFiniteDifferenceResidentWorkingMemoryBytes<float32, float64>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(float64WorkResult);
  REQUIRE(float64WorkResult.value() == 774 * k_MiB);
  auto float32WorkResult = ImageProcessing::detail::CalculateFiniteDifferenceResidentWorkingMemoryBytes<float32, float32>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(float32WorkResult);
  REQUIRE(float32WorkResult.value() == 515 * k_MiB);
  auto overflowResult = ImageProcessing::detail::CalculateFiniteDifferenceResidentWorkingMemoryBytes<float64, float64>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  REQUIRE(ImageProcessing::detail::ShouldUseFiniteDifferenceResidentState(dims, 1));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseFiniteDifferenceResidentState(SizeVec3{dimX, dimY, 1}, 1));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseFiniteDifferenceResidentState(dims, 0));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(3 * k_GiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveFiniteDifferenceResidentWorkingMemory<float32, float64>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 768 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(4 * k_GiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveFiniteDifferenceResidentWorkingMemory<float32, float64>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == float64WorkResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: real-OOC selector uses resident state only after a complete grant", "[ImageProcessing][FiniteDifferenceEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint64 k_PartialBudgetBytes = 16384;
  constexpr uint64 k_CompleteBudgetBytes = 32768;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<float32> field(dimX * dimY * dimZ);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX));
  }
  const std::vector<float32> expected = RunCurvatureFlow(field, dimX, dimY, dimZ, 0.05, 2);

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    OutOfCoreTransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
    OutOfCoreTransferCountingDataStore<float32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, -12345.0f);
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    const ImageProcessing::detail::CurvatureFlowFn function{};
    auto applyFiniteDifferenceResult = ApplyFiniteDifference(inputStore, outputStore, dims, FloatVec3{1.0f, 1.0f, 1.0f}, function, 0.05, 2, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(applyFiniteDifferenceResult);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return std::array<usize, 4>{inputStore.readCount(), inputStore.maxReadValues(), outputStore.writeCount(), outputStore.maxWriteValues()};
  };

  if(DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
  {
    const auto partialTransfers = run(k_PartialBudgetBytes);
    REQUIRE(partialTransfers == std::array<usize, 4>{dimZ, dimX * dimY, dimZ, dimX * dimY});
  }
  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers == std::array<usize, 4>{1, field.size(), 1, field.size()});
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: final 3D iteration writes output without a scratch round trip", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<float32> field(dimX * dimY * dimZ);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX));
  }
  const std::vector<float32> expected = RunCurvatureFlow(field, dimX, dimY, dimZ, 0.05, 1);

  DataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  DataStore<float32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, -12345.0f);
  auto copyFromBufferResult = inputStore.copyFromBuffer(0, field);
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  OutOfCoreTransferCountingDataStore<float64> firstStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  OutOfCoreTransferCountingDataStore<float64> secondStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const ImageProcessing::detail::CurvatureFlowFn function{};
  const std::array<double, 3> scales{1.0, 1.0, 1.0};
  const Result<> result = ImageProcessing::ApplyFiniteDifference3DPingPong<float32, ImageProcessing::detail::CurvatureFlowFn, float64>(inputStore, outputStore, firstStore, secondStore, dims, scales,
                                                                                                                                       function, 0.05, 1, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::vector<float32> actual(field.size());
  auto copyIntoBufferResult = outputStore.copyIntoBuffer(0, actual);
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
  REQUIRE(firstStore.readCount() == 0);
  REQUIRE(firstStore.writeCount() == 0);
  REQUIRE(secondStore.readCount() == 0);
  REQUIRE(secondStore.writeCount() == 0);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: first anisotropic conductance reduction reuses the input initialization stream", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  const SizeVec3 dims{dimX, dimY, dimZ};
  OutOfCoreTransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, -12345.0f);
  OutOfCoreTransferCountingDataStore<float64> firstStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  OutOfCoreTransferCountingDataStore<float64> secondStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  for(usize index = 0; index < dimX * dimY * dimZ; ++index)
  {
    inputStore.setValue(index, static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX)));
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const ImageProcessing::detail::GradientAnisoFn function{/*m_K=*/0.0, /*conductance=*/3.0, /*interval=*/1u};
  const Result<> firstIterationResult = ImageProcessing::ApplyFiniteDifference3DPingPong<float32, ImageProcessing::detail::GradientAnisoFn, float64>(
      inputStore, outputStore, firstStore, secondStore, dims, {1.0, 1.0, 1.0}, function, 0.05, /*numberOfIterations=*/1, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(firstIterationResult);

  REQUIRE(inputStore.readCount() == dimZ);
  REQUIRE(firstStore.writeCount() == dimZ);
  REQUIRE(firstStore.readCount() == dimZ);
  REQUIRE(secondStore.readCount() == 0);
  REQUIRE(secondStore.writeCount() == 0);
  REQUIRE(outputStore.writeCount() == dimZ);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: later anisotropic conductance refresh retains its frozen-state reduction pass", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  const SizeVec3 dims{dimX, dimY, dimZ};
  OutOfCoreTransferCountingDataStore<float32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, -12345.0f);
  OutOfCoreTransferCountingDataStore<float64> firstStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  OutOfCoreTransferCountingDataStore<float64> secondStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  for(usize index = 0; index < dimX * dimY * dimZ; ++index)
  {
    inputStore.setValue(index, static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX)));
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const ImageProcessing::detail::GradientAnisoFn function{/*m_K=*/0.0, /*conductance=*/3.0, /*interval=*/1u};
  const Result<> secondIterationResult = ImageProcessing::ApplyFiniteDifference3DPingPong<float32, ImageProcessing::detail::GradientAnisoFn, float64>(
      inputStore, outputStore, firstStore, secondStore, dims, {1.0, 1.0, 1.0}, function, 0.05, /*numberOfIterations=*/2, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(secondIterationResult);

  REQUIRE(inputStore.readCount() == dimZ);
  REQUIRE(firstStore.writeCount() == dimZ);
  REQUIRE(firstStore.readCount() == dimZ);
  REQUIRE(secondStore.writeCount() == dimZ);
  REQUIRE(secondStore.readCount() == 2 * dimZ);
  REQUIRE(outputStore.writeCount() == dimZ);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: interior range excludes exactly the clamped stencil edge", "[ImageProcessing][FiniteDifferenceEngine]")
{
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, 1).begin == 1);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, 1).end == 4);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(6, 2).begin == 2);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(6, 2).end == 4);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, 3).begin == 0);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, 3).end == 0);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, -1).begin == 0);
  REQUIRE(ImageProcessing::detail::CreateFiniteDifferenceInteriorRange(5, -1).end == 0);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: direct interior neighbors remain bit-exact with clamped edges", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<float32> values(dimX * dimY * dimZ);
  for(usize index = 0; index < values.size(); ++index)
  {
    values[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX));
  }

  DataStore<float32> residentInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  DataStore<float32> residentOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  auto copyFromBufferResult = residentInput.copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const ImageProcessing::detail::CurvatureAnisoFn function{/*m_K=*/0.0, /*conductance=*/3.0, /*interval=*/1u};
  const Result<> residentResult =
      ImageProcessing::ApplyFiniteDifference(residentInput, residentOutput, dims, FloatVec3{1.0f, 1.0f, 1.0f}, function, 0.05, /*numberOfIterations=*/2, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(residentResult);

  OutOfCoreTransferCountingDataStore<float32> oocInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> oocOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> firstStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> secondStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  auto copyFromBufferResult2 = oocInput.copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);
  const Result<> oocResult = ImageProcessing::ApplyFiniteDifference3DPingPong<float32, ImageProcessing::detail::CurvatureAnisoFn, float32>(
      oocInput, oocOutput, firstStore, secondStore, dims, {1.0, 1.0, 1.0}, function, 0.05, /*numberOfIterations=*/2, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(oocResult);

  std::vector<float32> expected(values.size());
  std::vector<float32> actual(values.size());
  auto copyIntoBufferResult = residentOutput.copyIntoBuffer(0, nonstd::span<float32>(expected.data(), expected.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  auto copyIntoBufferResult2 = oocOutput.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
  REQUIRE(actual == expected);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: radius-two direct interior neighbors remain bit-exact with clamped edges", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr usize dimX = 7;
  constexpr usize dimY = 8;
  constexpr usize dimZ = 9;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<float32> values(dimX * dimY * dimZ);
  for(usize index = 0; index < values.size(); ++index)
  {
    values[index] = static_cast<float32>(std::sin(static_cast<double>(index) * 0.19) + 0.25 * static_cast<double>(index % dimX));
  }

  DataStore<float32> residentInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  DataStore<float32> residentOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  auto copyFromBufferResult = residentInput.copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const ImageProcessing::detail::MinMaxCurvatureFlowFn function{/*radius=*/2};
  const Result<> residentResult =
      ImageProcessing::ApplyFiniteDifference(residentInput, residentOutput, dims, FloatVec3{1.0f, 1.0f, 1.0f}, function, 0.05, /*numberOfIterations=*/1, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(residentResult);

  OutOfCoreTransferCountingDataStore<float32> oocInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float32> oocOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0f);
  OutOfCoreTransferCountingDataStore<float64> firstStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  OutOfCoreTransferCountingDataStore<float64> secondStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, 0.0);
  auto copyFromBufferResult2 = oocInput.copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult2);
  const Result<> oocResult = ImageProcessing::ApplyFiniteDifference3DPingPong<float32, ImageProcessing::detail::MinMaxCurvatureFlowFn, float64>(
      oocInput, oocOutput, firstStore, secondStore, dims, {1.0, 1.0, 1.0}, function, 0.05, /*numberOfIterations=*/1, shouldCancel, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(oocResult);

  std::vector<float32> expected(values.size());
  std::vector<float32> actual(values.size());
  auto copyIntoBufferResult = residentOutput.copyIntoBuffer(0, nonstd::span<float32>(expected.data(), expected.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  auto copyIntoBufferResult2 = oocOutput.copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult2);
  REQUIRE(actual == expected);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: MinMax stencil offsets are precomputed in ITK iteration order", "[ImageProcessing][FiniteDifferenceEngine]")
{
  for(const int64 radius : {int64{1}, int64{2}, int64{3}})
  {
    const ImageProcessing::detail::MinMaxCurvatureFlowFn function{radius};
    for(const uint32 effectiveDimensions : {uint32{2}, uint32{3}})
    {
      std::vector<std::array<int64, 3>> expected;
      const int64 squaredRadius = radius * radius;
      const int64 zBegin = effectiveDimensions == 3 ? -radius : 0;
      const int64 zEnd = effectiveDimensions == 3 ? radius : 0;
      for(int64 z = zBegin; z <= zEnd; ++z)
      {
        for(int64 y = -radius; y <= radius; ++y)
        {
          for(int64 x = -radius; x <= radius; ++x)
          {
            if(x * x + y * y + z * z <= squaredRadius)
            {
              expected.push_back({x, y, z});
            }
          }
        }
      }

      const auto& actual = function.stencilOffsets(effectiveDimensions);
      REQUIRE(actual == expected);
      REQUIRE(function.inverseStencilPixelCount(effectiveDimensions) == 1.0 / static_cast<double>(expected.size()));
    }
  }

  const ImageProcessing::detail::MinMaxCurvatureFlowFn boundedFunction{ImageProcessing::detail::k_MaxPrecomputedMinMaxStencilRadius + 1};
  REQUIRE(boundedFunction.stencilOffsets(2).empty());
  REQUIRE(boundedFunction.stencilOffsets(3).empty());
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: interior selector respects radius and effective dimensions", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const SizeVec3 dims{8, 9, 10};
  REQUIRE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(2, 2, 2, dims, 2, 3));
  REQUIRE_FALSE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(1, 2, 2, dims, 2, 3));
  REQUIRE_FALSE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(2, 7, 2, dims, 2, 3));
  REQUIRE_FALSE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(2, 2, 8, dims, 2, 3));
  REQUIRE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(2, 2, 0, dims, 2, 2));
  REQUIRE_FALSE(ImageProcessing::detail::FiniteDifferenceNeighborhoodIsInterior(2, 2, 0, dims, -1, 2));
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: anisotropic updates reuse the 19-value radius-one neighborhood", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const std::array<double, 3> scales = {1.0, 0.75, 1.25};

  SECTION("gradient diffusion")
  {
    ImageProcessing::detail::GradientAnisoFn function{};
    function.template setGlobalK<float64>(2.0);
    usize readCount = 0;
    auto get = [&](int64 x, int64 y, int64 z) {
      ++readCount;
      return static_cast<float64>(3 * x - 5 * y + 7 * z + x * y - y * z + 11);
    };
    REQUIRE(std::isfinite(function.computeUpdate(get, scales, 3)));
    REQUIRE(readCount == 19);
  }

  SECTION("curvature diffusion")
  {
    ImageProcessing::detail::CurvatureAnisoFn function{};
    function.template setGlobalK<float32>(2.0);
    usize readCount = 0;
    auto get = [&](int64 x, int64 y, int64 z) {
      ++readCount;
      return static_cast<float32>(3 * x - 5 * y + 7 * z + x * y - y * z + 11);
    };
    REQUIRE(std::isfinite(function.computeUpdate(get, scales, 3)));
    REQUIRE(readCount == 19);
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: zero iterations is the identity", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const usize d = 8;
  std::vector<float32> field(d * d * d);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 17) % 53);
  }
  const std::vector<float32> out = RunCurvatureFlow<float32>(field, d, d, d, 0.05, 0);
  REQUIRE(out == field); // out = cast<T>(in), no updates
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: constant image is preserved by curvature flow", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const usize d = 10;
  const std::vector<float32> field(d * d * d, 42.0f);
  const std::vector<float32> out = RunCurvatureFlow<float32>(field, d, d, d, 0.05, 5);
  for(float32 v : out)
  {
    REQUIRE(v == Approx(42.0f).margin(1e-5)); // zero gradient everywhere -> update 0 -> constant preserved incl. edges
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: deterministic + tall-Z range sanity", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const usize dx = 8, dy = 8, dz = 40;
  std::vector<float32> field(dx * dy * dz);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>(std::sin(0.3 * static_cast<double>(i)));
  }
  const std::vector<float32> a = RunCurvatureFlow<float32>(field, dx, dy, dz, 0.05, 3);
  const std::vector<float32> b = RunCurvatureFlow<float32>(field, dx, dy, dz, 0.05, 3);
  REQUIRE(a == b);
  // range sanity: curvature flow does not blow up a bounded input
  for(float32 v : a)
  {
    REQUIRE(std::isfinite(v));
    REQUIRE(v >= -2.0f);
    REQUIRE(v <= 2.0f);
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: planar level sets do not move (ramp update is zero)", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const usize dx = 10, dy = 10, dz = 10;
  auto at = [&](usize x, usize y, usize z) { return (z * dy + y) * dx + x; };
  SECTION("axis-aligned ramp along X")
  {
    std::vector<float64> field(dx * dy * dz);
    for(usize z = 0; z < dz; ++z)
      for(usize y = 0; y < dy; ++y)
        for(usize x = 0; x < dx; ++x)
          field[at(x, y, z)] = 3.0 * static_cast<float64>(x);
    // one iteration of curvature flow; interior voxels have planar level sets -> update 0 -> value unchanged.
    const std::vector<float64> out = RunCurvatureFlow<float64>(field, dx, dy, dz, 0.1, 1);
    for(usize z = 1; z < dz - 1; ++z)
      for(usize y = 1; y < dy - 1; ++y)
        for(usize x = 1; x < dx - 1; ++x)
          REQUIRE(out[at(x, y, z)] == Approx(3.0 * static_cast<float64>(x)).margin(1e-9));
  }
  SECTION("diagonal ramp exercises the cross-derivative cancellation")
  {
    std::vector<float64> field(dx * dy * dz);
    for(usize z = 0; z < dz; ++z)
      for(usize y = 0; y < dy; ++y)
        for(usize x = 0; x < dx; ++x)
          field[at(x, y, z)] = 2.0 * static_cast<float64>(x) + 3.0 * static_cast<float64>(y);
    const std::vector<float64> out = RunCurvatureFlow<float64>(field, dx, dy, dz, 0.1, 1);
    for(usize z = 1; z < dz - 1; ++z)
      for(usize y = 1; y < dy - 1; ++y)
        for(usize x = 1; x < dx - 1; ++x)
          REQUIRE(out[at(x, y, z)] == Approx(2.0 * static_cast<float64>(x) + 3.0 * static_cast<float64>(y)).margin(1e-9));
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: validates stores and cancellation", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const nx::core::ImageProcessing::detail::CurvatureFlowFn fn{};
  std::atomic_bool shouldCancel{false};

  SECTION("rejects mismatched stores")
  {
    DataStore<float32> shortStore(ShapeType{63}, ShapeType{1}, 0.0f);
    DataStore<float32> fullStore(ShapeType{64}, ShapeType{1}, 0.0f);
    Result<> result = ApplyFiniteDifference(shortStore, fullStore, SizeVec3{4, 4, 4}, FloatVec3{1.0f, 1.0f, 1.0f}, fn, 0.05, 1, shouldCancel, {});
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8632);

    result = ApplyFiniteDifference(fullStore, shortStore, SizeVec3{4, 4, 4}, FloatVec3{1.0f, 1.0f, 1.0f}, fn, 0.05, 1, shouldCancel, {});
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8633);
  }

  SECTION("pre-cancel preserves output")
  {
    constexpr float32 kPoison = -12345.0f;
    DataStore<float32> inputStore(ShapeType{4, 4, 4}, ShapeType{1}, 1.0f);
    DataStore<float32> outputStore(ShapeType{4, 4, 4}, ShapeType{1}, kPoison);
    shouldCancel = true;
    const Result<> result = ApplyFiniteDifference(inputStore, outputStore, SizeVec3{4, 4, 4}, FloatVec3{1.0f, 1.0f, 1.0f}, fn, 0.05, 1, shouldCancel, {});
    REQUIRE(result.valid());
    for(const float32 value : outputStore)
    {
      REQUIRE(value == kPoison);
    }
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: mixed storage selects an OOC working format", "[ImageProcessing][FiniteDifferenceEngine]")
{
  constexpr const char* kInputOocFormat = "HDF5-OOC-Input";
  constexpr const char* kOutputOocFormat = "HDF5-OOC-Output";

  SECTION("resident input and resident output preserve the input format")
  {
    const auto selectedFormat = nx::core::ImageProcessing::detail::SelectFiniteDifferenceWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore,
                                                                                                           IDataStore::StoreType::InMemory, "Alternate-DataStore");
    REQUIRE(selectedFormat == DataStore<float32>::k_DataStore);
  }

  SECTION("resident input and OOC output select the output format")
  {
    const auto selectedFormat = nx::core::ImageProcessing::detail::SelectFiniteDifferenceWorkingDataFormat(IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore,
                                                                                                           IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(selectedFormat == kOutputOocFormat);
  }

  SECTION("OOC input and resident output select the input format")
  {
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectFiniteDifferenceWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::InMemory, DataStore<float32>::k_DataStore);
    REQUIRE(selectedFormat == kInputOocFormat);
  }

  SECTION("OOC input takes precedence when both endpoints are OOC")
  {
    // The endpoint suffixes make the precedence observable without registering either backend in this helper-level test.
    const auto selectedFormat =
        nx::core::ImageProcessing::detail::SelectFiniteDifferenceWorkingDataFormat(IDataStore::StoreType::OutOfCore, kInputOocFormat, IDataStore::StoreType::OutOfCore, kOutputOocFormat);
    REQUIRE(selectedFormat == kInputOocFormat);
  }
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: 2D planner bounds row blocks and overwide tiles", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const auto benchmark = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(5888, 5888, 1, sizeof(float64));
  REQUIRE(benchmark.valid);
  REQUIRE(benchmark.coreCols == 5888);
  REQUIRE(benchmark.coreRows > 1);
  REQUIRE(benchmark.residentBytes <= ImageProcessing::detail::k_FiniteDifference2DResidentLimit);

  for(const usize radius : {usize{1}, usize{2}, usize{3}})
  {
    const auto stress = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(16385, 1025, radius, sizeof(float64));
    CAPTURE(radius);
    REQUIRE(stress.valid);
    REQUIRE(stress.coreCols == 16385);
    REQUIRE(stress.coreRows > 0);
    REQUIRE(stress.residentBytes <= ImageProcessing::detail::k_FiniteDifference2DResidentLimit);
  }

  for(const auto dimensions : {std::array<usize, 2>{1, 16385}, std::array<usize, 2>{16385, 1}})
  {
    const auto oneDimensional = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(dimensions[0], dimensions[1], 2, sizeof(float64));
    CAPTURE(dimensions[0], dimensions[1]);
    REQUIRE(oneDimensional.valid);
    REQUIRE(oneDimensional.residentBytes <= ImageProcessing::detail::k_FiniteDifference2DResidentLimit);
  }

  const auto overwide = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(100000000, 2, 2, sizeof(float64));
  REQUIRE(overwide.valid);
  REQUIRE(overwide.coreCols < 100000000);
  REQUIRE(overwide.coreRows == 1);
  REQUIRE(overwide.residentBytes <= ImageProcessing::detail::k_FiniteDifference2DResidentLimit);

  constexpr usize kSmallLimit = 5000;
  const auto forcedTiled = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(100, 8, 2, sizeof(float64), kSmallLimit);
  REQUIRE(forcedTiled.valid);
  REQUIRE(forcedTiled.coreCols < 100);
  REQUIRE(forcedTiled.coreRows == 1);
  REQUIRE(forcedTiled.residentBytes <= kSmallLimit);

  constexpr usize kOneColumnTileLimit = ImageProcessing::detail::k_FiniteDifference2DFixedStateBytes + ((1 + 2 * 2) * (1 + 2 * 2) + 1) * sizeof(float64);
  const auto oneColumnTiles = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(9, 7, 2, sizeof(float64), kOneColumnTileLimit);
  REQUIRE(oneColumnTiles.valid);
  REQUIRE(oneColumnTiles.coreCols == 1);
  REQUIRE(oneColumnTiles.residentBytes == kOneColumnTileLimit);

  const auto oneCell = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(1, 1, 0, sizeof(float32));
  REQUIRE(oneCell.valid);
  REQUIRE(oneCell.residentBytes <= ImageProcessing::detail::k_FiniteDifference2DResidentLimit);
}

TEST_CASE("ImageProcessing::FiniteDifferenceEngine: 2D planner rejects invalid and overflow inputs", "[ImageProcessing][FiniteDifferenceEngine]")
{
  const auto overflow = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(), 1, sizeof(float64));
  const auto radiusOverflow = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(1, 1, std::numeric_limits<usize>::max(), sizeof(float64));
  const auto zeroX = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(0, 1, 1, sizeof(float64));
  const auto zeroY = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(1, 0, 1, sizeof(float64));
  const auto zeroBytes = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(1, 1, 1, 0);
  REQUIRE_FALSE(overflow.valid);
  REQUIRE(overflow.overflow);
  REQUIRE_FALSE(radiusOverflow.valid);
  REQUIRE(radiusOverflow.overflow);
  REQUIRE_FALSE(zeroX.valid);
  REQUIRE(zeroX.overflow);
  REQUIRE_FALSE(zeroY.valid);
  REQUIRE(zeroY.overflow);
  REQUIRE_FALSE(zeroBytes.valid);
  REQUIRE(zeroBytes.overflow);
}
