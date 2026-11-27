#include "AdaptiveHistogramEqualizationFilterTestUtils.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/AdaptiveHistogramEqualizationEngine.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;
using ahe_test::AdaptiveHistogramEqualizationOracle;
using ahe_test::MakeAhePattern;
using ahe_test::RequireAheClose;
using ahe_test::RunApplyAhe;

namespace
{
template <class T>
class OutOfCoreTransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_ReadCount++;
    if(m_ReadCount > 1)
    {
      m_MaxTransformReadValues = std::max(m_MaxTransformReadValues, buffer.size());
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WrittenValues += buffer.size();
    m_WriteCount++;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maxTransformReadValues() const noexcept
  {
    return m_MaxTransformReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

protected:
  mutable usize m_MaxTransformReadValues = 0;
  usize m_MaxWriteValues = 0;
  usize m_WrittenValues = 0;
  usize m_WriteCount = 0;
  mutable usize m_ReadCount = 0;
};

template <class T>
class CancelAfterReadDataStore : public OutOfCoreTransferCountingDataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel, usize triggerRead)
  : OutOfCoreTransferCountingDataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  , m_TriggerRead(triggerRead)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = OutOfCoreTransferCountingDataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && this->m_ReadCount == m_TriggerRead)
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  usize m_TriggerRead = 0;
};

template <class T>
std::vector<T> RunBoundedAhe(const std::vector<T>& input, usize dimX, usize dimY, const std::array<usize, 3>& radius, float32 alpha, float32 beta, usize targetBytes, usize maximumReadValues,
                             usize maximumWriteValues, usize expectedWriteCount)
{
  OutOfCoreTransferCountingDataStore<T> inputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, T{});
  OutOfCoreTransferCountingDataStore<T> outputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, T{});
  for(usize index = 0; index < input.size(); ++index)
  {
    inputStore.setValue(index, input[index]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  Result<> applyResult = ApplyAdaptiveHistogramEqualization(inputStore, outputStore, SizeVec3Type{dimX, dimY, 1}, radius, alpha, beta, shouldCancel, messageHandler, targetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

  std::vector<T> output(input.size());
  Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<T>(output.data(), output.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  CAPTURE(inputStore.maxTransformReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues(), outputStore.writeCount());
  REQUIRE(inputStore.maxTransformReadValues() == maximumReadValues);
  REQUIRE(outputStore.maxWriteValues() == maximumWriteValues);
  REQUIRE(outputStore.writtenValues() == input.size());
  REQUIRE(outputStore.writeCount() == expectedWriteCount);
  return output;
}

// Engine-vs-independent-double-oracle over a set of (alpha,beta,radius) on 3D and 2D shapes. The oracle uses
// the same float/double split as the engine, so the tolerance is tiny; integer types still allow the rare
// truncation-boundary +/-1 flip (RequireAheClose).
template <class T>
void CheckAheAgainstOracle(usize dimX, usize dimY, usize dimZ)
{
  const std::vector<T> input = MakeAhePattern<T>(dimX, dimY, dimZ);
  const std::vector<std::array<int32, 3>> radii = {{1, 1, 1}, {2, 1, 1}, {2, 2, 2}};
  const std::vector<std::array<float32, 2>> ab = {{0.3f, 0.3f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {0.6f, 0.2f}};

  for(const std::array<int32, 3>& r : radii)
  {
    for(const std::array<float32, 2>& p : ab)
    {
      const std::array<usize, 3> radius = {static_cast<usize>(r[0]), static_cast<usize>(r[1]), static_cast<usize>(r[2])};
      const std::vector<T> expected = AdaptiveHistogramEqualizationOracle<T>(input, dimX, dimY, dimZ, r, p[0], p[1]);
      const std::vector<T> actual = RunApplyAhe<T>(input, dimX, dimY, dimZ, radius, p[0], p[1]);
      INFO("radius={" << r[0] << "," << r[1] << "," << r[2] << "} alpha=" << p[0] << " beta=" << p[1]);
      // Engine and oracle share the float/double split, so integers should match exactly and floats to ~ULP,
      // but keep a tiny tolerance for any pow()/reassociation drift.
      RequireAheClose<T>(actual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.01);
    }
  }
}
} // namespace

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: resident state requires a complete dataset-scaled reservation",
          "[NXImageProcessing][AdaptiveHistogramEqualizationEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 512;
  constexpr usize k_DimY = 512;
  constexpr usize k_DimZ = 128;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::array<usize, 3> radius = {10, 10, 10};

  auto requiredResult = ImageProcessing::detail::CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes<uint8>(dims, radius, /*useLinearUint8=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == 163 * k_MiB + 512ULL * 1024ULL);
  Result<usize> overflowDimsResult =
      ImageProcessing::detail::CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes<uint8>(SizeVec3Type{std::numeric_limits<usize>::max(), 2, 2}, radius, /*useLinearUint8=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimsResult);

  REQUIRE(ImageProcessing::detail::ShouldUseAdaptiveHistogramEqualizationResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseAdaptiveHistogramEqualizationResidentState(SizeVec3Type{k_DimX, k_DimY, 1}));

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveAdaptiveHistogramEqualizationResidentWorkingMemory<uint8>(dims, radius, /*useLinearUint8=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 128 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(1024 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveAdaptiveHistogramEqualizationResidentWorkingMemory<uint8>(dims, radius, /*useLinearUint8=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: real-OOC selector uses resident state only after a complete grant",
          "[NXImageProcessing][AdaptiveHistogramEqualizationEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 5;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr uint64 k_PartialBudgetBytes = 64 * k_MiB;
  constexpr uint64 k_CompleteBudgetBytes = 128 * k_MiB;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::array<usize, 3> radius = {1, 1, 1};
  const std::vector<uint8> input = MakeAhePattern<uint8>(k_DimX, k_DimY, k_DimZ);
  const std::vector<uint8> expected = RunApplyAhe<uint8>(input, k_DimX, k_DimY, k_DimZ, radius, 1.0f, 0.25f);

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    OutOfCoreTransferCountingDataStore<uint8> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{0});
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, uint8{255});
    for(usize index = 0; index < input.size(); ++index)
    {
      inputStore.setValue(index, input[index]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    Result<> applyResult = ApplyAdaptiveHistogramEqualization(inputStore, outputStore, dims, radius, 1.0f, 0.25f, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
    std::vector<uint8> actual(input.size());
    Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.0);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return std::array<usize, 4>{inputStore.readCount(), inputStore.maxTransformReadValues(), outputStore.writeCount(), outputStore.maxWriteValues()};
  };

  // The partial grant routes to the streamed ring, whose transfers are band-batched: the gray-range pass
  // issues one bulk read, and a 64 MiB grant lets the ring's band cover this tiny volume, so all input
  // planes arrive in one further read and all output planes leave in one ordered write. The resident route
  // below stays distinguishable by its zero transform reads.
  const auto partialTransfers = run(k_PartialBudgetBytes);
  REQUIRE(partialTransfers == std::array<usize, 4>{2, k_DimX * k_DimY * k_DimZ, 1, k_DimX * k_DimY * k_DimZ});
  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers == std::array<usize, 4>{1, 0, 1, input.size()});
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: bounded 2D fast and generic transfers", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  constexpr usize k_TotalValues = k_DimX * k_DimY;
  constexpr std::array<usize, 3> k_Radius = {2, 1, 3};
  constexpr std::array<int32, 3> k_OracleRadius = {2, 1, 3};

  SECTION("uint8 linear fast path")
  {
    const std::vector<uint8> input = MakeAhePattern<uint8>(k_DimX, k_DimY, 1);
    const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
    const usize span = static_cast<usize>(*maximum - *minimum);
    const usize lutBytes = (2 * span + 1) * sizeof(float);
    const std::vector<uint8> expected = AdaptiveHistogramEqualizationOracle<uint8>(input, k_DimX, k_DimY, 1, k_OracleRadius, 1.0f, 0.25f);

    struct TransferCase
    {
      const char* label;
      usize buffer_bytes;
      bool full_width;
      usize core_rows;
      usize core_columns;
      usize maximum_read_values;
      usize maximum_write_values;
      usize write_count;
    };
    const std::array<TransferCase, 2> transferCases = {
        {{.label = "full-width row blocks", .buffer_bytes = 342, .full_width = true, .core_rows = 2, .core_columns = 9, .maximum_read_values = 36, .maximum_write_values = 18, .write_count = 4},
         {.label = "overwide X tiles", .buffer_bytes = 68, .full_width = false, .core_rows = 1, .core_columns = 2, .maximum_read_values = 6, .maximum_write_values = 2, .write_count = 35}}};
    for(const TransferCase& transferCase : transferCases)
    {
      DYNAMIC_SECTION(transferCase.label)
      {
        const usize targetBytes = transferCase.buffer_bytes + lutBytes;
        const auto planResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(uint8), targetBytes, lutBytes,
                                                                                                   /*useHorizontalSums=*/true);
        SIMPLNX_RESULT_REQUIRE_VALID(planResult);
        const auto& plan = planResult.value();
        REQUIRE(plan.full_width == transferCase.full_width);
        REQUIRE(plan.core_rows == transferCase.core_rows);
        REQUIRE(plan.core_columns == transferCase.core_columns);
        REQUIRE(plan.resident_bytes == targetBytes);

        const std::vector<uint8> actual =
            RunBoundedAhe(input, k_DimX, k_DimY, k_Radius, 1.0f, 0.25f, targetBytes, transferCase.maximum_read_values, transferCase.maximum_write_values, transferCase.write_count);
        RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
      }
    }

    const auto fixedPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(uint8),
                                                                                                    ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, lutBytes,
                                                                                                    /*useHorizontalSums=*/true, /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(fixedPlanResult);
    REQUIRE(fixedPlanResult.value().fixed_output);
    const std::vector<uint8> fixedOutputActual = RunBoundedAhe(input, k_DimX, k_DimY, k_Radius, 1.0f, 0.25f, ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes,
                                                               /*maximumReadValues=*/k_TotalValues, /*maximumWriteValues=*/k_TotalValues, /*expectedWriteCount=*/1);
    RequireAheClose<uint8>(fixedOutputActual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);

    const usize overwideFixedScratchBytes = ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes - ImageProcessing::detail::k_AdaptiveHistogramEqualization2DFixedOutputBytes - 68;
    const auto overwideFixedPlan = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(uint8),
                                                                                                      ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, overwideFixedScratchBytes,
                                                                                                      /*useHorizontalSums=*/true, /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(overwideFixedPlan);
    REQUIRE(overwideFixedPlan.value().fixed_output);
    REQUIRE_FALSE(overwideFixedPlan.value().full_width);
    REQUIRE(overwideFixedPlan.value().core_columns == 2);
  }

  SECTION("float32 generic path")
  {
    const std::vector<float32> input = MakeAhePattern<float32>(k_DimX, k_DimY, 1);
    const std::vector<float32> expected = AdaptiveHistogramEqualizationOracle<float32>(input, k_DimX, k_DimY, 1, k_OracleRadius, 0.3f, 0.3f);
    struct TransferCase
    {
      const char* label;
      usize target_bytes;
      bool full_width;
      usize core_rows;
      usize core_columns;
      usize maximum_read_values;
      usize maximum_write_values;
      usize write_count;
    };
    const std::array<TransferCase, 2> transferCases = {
        {{.label = "full-width row blocks", .target_bytes = 216, .full_width = true, .core_rows = 2, .core_columns = 9, .maximum_read_values = 36, .maximum_write_values = 18, .write_count = 4},
         {.label = "overwide X tiles", .target_bytes = 80, .full_width = false, .core_rows = 1, .core_columns = 2, .maximum_read_values = 6, .maximum_write_values = 2, .write_count = 35}}};
    for(const TransferCase& transferCase : transferCases)
    {
      DYNAMIC_SECTION(transferCase.label)
      {
        const auto planResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(float32), transferCase.target_bytes,
                                                                                                   /*fixedScratchBytes=*/0, /*useHorizontalSums=*/false);
        SIMPLNX_RESULT_REQUIRE_VALID(planResult);
        const auto& plan = planResult.value();
        REQUIRE(plan.full_width == transferCase.full_width);
        REQUIRE(plan.core_rows == transferCase.core_rows);
        REQUIRE(plan.core_columns == transferCase.core_columns);
        REQUIRE(plan.resident_bytes == transferCase.target_bytes);

        const std::vector<float32> actual =
            RunBoundedAhe(input, k_DimX, k_DimY, k_Radius, 0.3f, 0.3f, transferCase.target_bytes, transferCase.maximum_read_values, transferCase.maximum_write_values, transferCase.write_count);
        RequireAheClose<float32>(actual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.0);
      }
    }

    const auto fixedPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(float32),
                                                                                                    ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes,
                                                                                                    /*fixedScratchBytes=*/0, /*useHorizontalSums=*/false,
                                                                                                    /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(fixedPlanResult);
    REQUIRE(fixedPlanResult.value().fixed_output);
    const std::vector<float32> fixedOutputActual =
        RunBoundedAhe(input, k_DimX, k_DimY, k_Radius, 0.3f, 0.3f, ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, /*maximumReadValues=*/k_TotalValues,
                      /*maximumWriteValues=*/k_TotalValues, /*expectedWriteCount=*/1);
    RequireAheClose<float32>(fixedOutputActual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.0);
  }

  auto smallTargetPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(k_DimX, k_DimY, k_Radius[0], k_Radius[1], sizeof(uint8), /*targetBytes=*/1, /*fixedScratchBytes=*/0,
                                                                                                  /*useHorizontalSums=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(smallTargetPlanResult);
  auto overflowDimPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(std::numeric_limits<usize>::max(), k_DimY, k_Radius[0], k_Radius[1], sizeof(uint8),
                                                                                                  /*targetBytes=*/342, /*fixedScratchBytes=*/0, /*useHorizontalSums=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimPlanResult);
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: bounded 2D constant and cancellation preserve output", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  constexpr usize k_TotalValues = k_DimX * k_DimY;
  constexpr std::array<usize, 3> k_Radius = {2, 1, 3};

  SECTION("constant input uses bounded copy chunks")
  {
    const std::vector<int32> input(k_TotalValues, 42);
    const std::vector<int32> output = RunBoundedAhe(input, k_DimX, k_DimY, k_Radius, 0.3f, 0.3f, /*targetBytes=*/32, /*maximumReadValues=*/8, /*maximumWriteValues=*/8, /*expectedWriteCount=*/8);
    REQUIRE(output == input);
  }

  SECTION("cancel after transform read performs no output write")
  {
    constexpr uint8 k_Poison = 73;
    const std::vector<uint8> input = MakeAhePattern<uint8>(k_DimX, k_DimY, 1);
    std::atomic_bool shouldCancel{false};
    CancelAfterReadDataStore<uint8> cancellingInputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, uint8{0}, shouldCancel, /*triggerRead=*/2);
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    for(usize index = 0; index < input.size(); ++index)
    {
      cancellingInputStore.setValue(index, input[index]);
    }
    IFilter::MessageHandler messageHandler{};
    const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
    const usize span = static_cast<usize>(*maximum - *minimum);
    const usize lutBytes = (2 * span + 1) * sizeof(float);
    Result<> applyResult = ApplyAdaptiveHistogramEqualization(cancellingInputStore, outputStore, SizeVec3Type{k_DimX, k_DimY, 1}, k_Radius, 1.0f, 0.25f, shouldCancel, messageHandler,
                                                              /*target2DBytes=*/342 + lutBytes);
    SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
    REQUIRE(shouldCancel);
    REQUIRE(cancellingInputStore.readCount() == 2);
    REQUIRE(outputStore.writtenValues() == 0);
    for(const uint8 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: matches independent oracle (3D + 2D)", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]", uint8, int16, int32,
                   float32, float64)
{
  using T = TestType;
  SECTION("3D non-cubic 10x11x9")
  {
    CheckAheAgainstOracle<T>(10, 11, 9);
  }
  SECTION("2D single-plane 12x10x1 (z-neighbors clipped to plane)")
  {
    CheckAheAgainstOracle<T>(12, 10, 1);
  }
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: constant image passes through (ITK would NaN)", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // max == min => iscale == 0. ITK computes 0/0 = NaN; the engine passes the input through unchanged.
  constexpr usize k_D = 6;
  const std::vector<int32> input(k_D * k_D * k_D, 42);
  const std::vector<int32> out = RunApplyAhe<int32>(input, k_D, k_D, k_D, {2, 2, 2}, 0.3f, 0.3f);
  for(usize i = 0; i < out.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(out[i] == 42);
  }

  // Float constant too.
  const std::vector<float32> finput(k_D * k_D * k_D, 3.5f);
  const std::vector<float32> fout = RunApplyAhe<float32>(finput, k_D, k_D, k_D, {1, 1, 1}, 0.3f, 0.3f);
  for(usize i = 0; i < fout.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(fout[i] == 3.5f);
  }
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: integer fallback (span exceeds LUT cap)", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // Force the NON-LUT integer path. The difference-keyed LUT fast path is gated at kMaxAheLutEntries = 2^22
  // table entries (an integer gray span <= 2^21). The TEMPLATE grid above never trips the gate (its patterns
  // are in [0,100] => span 100 => LUT path), so this case stretches an int32 image's range with two extreme
  // voxels (0 and 5,000,000 => 2*span+1 ~ 1e7 > 2^22) to drop the engine onto the exact per-neighbor body.
  // That pins the fallback body's boundary logic (which is duplicated from the LUT body) and confirms the gate.
  // The bulk of the voxels sit in a small band so the image is still non-constant and exercises accumulation.
  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 8;
  constexpr usize k_DimZ = 4;
  std::vector<int32> input(k_DimX * k_DimY * k_DimZ);
  for(usize i = 0; i < input.size(); ++i)
  {
    input[i] = static_cast<int32>(i % 37); // small band [0, 36]
  }
  input[0] = 0;                        // global min
  input[input.size() - 1] = 5'000'000; // global max => span 5,000,000 > 2^21 => LUT gated off, fallback path

  const std::vector<std::array<int32, 3>> radii = {{1, 1, 1}, {2, 2, 2}};
  const std::vector<std::array<float32, 2>> ab = {{0.3f, 0.3f}, {1.0f, 0.0f}};
  for(const std::array<int32, 3>& r : radii)
  {
    for(const std::array<float32, 2>& p : ab)
    {
      const std::array<usize, 3> radius = {static_cast<usize>(r[0]), static_cast<usize>(r[1]), static_cast<usize>(r[2])};
      const std::vector<int32> expected = AdaptiveHistogramEqualizationOracle<int32>(input, k_DimX, k_DimY, k_DimZ, r, p[0], p[1]);
      const std::vector<int32> actual = RunApplyAhe<int32>(input, k_DimX, k_DimY, k_DimZ, radius, p[0], p[1]);
      INFO("radius={" << r[0] << "," << r[1] << "," << r[2] << "} alpha=" << p[0] << " beta=" << p[1]);
      RequireAheClose<int32>(actual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.01);
    }
  }
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: alpha=beta=1 is the identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // Closed form independent of the CF internals: with alpha=beta=1, cf = 0.5*s*ad - 0.5*s*ad + u = u for
  // every neighbor, so sum/ikernel = u and out = iscale*(u+0.5)+min = the original pixel. This is a strong
  // anchor that does NOT depend on how the cumulative function is spelled. Float32 input avoids the extra
  // integer-truncation slack while still allowing tiny pow()-reassociation drift.
  const std::vector<float32> input = MakeAhePattern<float32>(9, 8, 7);
  const std::vector<float32> out = RunApplyAhe<float32>(input, 9, 8, 7, {2, 2, 2}, 1.0f, 1.0f);
  RequireAheClose<float32>(out, input, /*atol=*/1e-2, /*rtol=*/1e-4, /*maxDiffFraction=*/0.0);
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: uint8 alpha=1 beta=0.25 moving-box path", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize k_DimX = 17;
  constexpr usize k_DimY = 13;
  constexpr usize k_DimZ = 9;
  constexpr std::array<int32, 3> k_OracleRadius = {3, 2, 4};
  constexpr std::array<usize, 3> k_Radius = {3, 2, 4};
  const std::vector<uint8> input = MakeAhePattern<uint8>(k_DimX, k_DimY, k_DimZ);
  const std::vector<uint8> expected = AdaptiveHistogramEqualizationOracle<uint8>(input, k_DimX, k_DimY, k_DimZ, k_OracleRadius, 1.0f, 0.25f);
  const std::vector<uint8> actual = RunApplyAhe<uint8>(input, k_DimX, k_DimY, k_DimZ, k_Radius, 1.0f, 0.25f);
  RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: validates dimensions and stores", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  IFilter::MessageHandler messageHandler{};
  std::atomic_bool shouldCancel{false};

  SECTION("rejects zero dimensions")
  {
    DataStore<uint8> inputStore(ShapeType{1}, ShapeType{1}, 0);
    DataStore<uint8> outputStore(ShapeType{1}, ShapeType{1}, 0);
    const Result<> result = ApplyAdaptiveHistogramEqualization<uint8>(inputStore, outputStore, SizeVec3Type{0, 1, 1}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8541);
  }

  SECTION("rejects mismatched input and output stores")
  {
    DataStore<uint8> shortStore(ShapeType{63}, ShapeType{1}, 0);
    DataStore<uint8> fullStore(ShapeType{64}, ShapeType{1}, 0);
    Result<> result = ApplyAdaptiveHistogramEqualization<uint8>(shortStore, fullStore, SizeVec3Type{4, 4, 4}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8543);

    result = ApplyAdaptiveHistogramEqualization<uint8>(fullStore, shortStore, SizeVec3Type{4, 4, 4}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8544);
  }
}

namespace
{
// Both traversals use the same LUT to isolate traversal differences.
template <class T>
std::vector<float32> MakeTraversalLut(const std::vector<T>& input, float64 iscale)
{
  const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
  const int64 span = static_cast<int64>(*maximum) - static_cast<int64>(*minimum);
  std::vector<float32> lookupTable(static_cast<usize>(2 * span + 1));
  for(usize index = 0; index < lookupTable.size(); ++index)
  {
    const int64 difference = static_cast<int64>(index) - span;
    const float64 normalizedDifference = static_cast<float64>(difference) / iscale;
    const float32 sign = static_cast<float32>(ImageProcessing::detail::AdaptiveEqualizationSgn(static_cast<float32>(normalizedDifference)));
    const float32 absoluteDifference = static_cast<float32>(std::fabs(2.0 * normalizedDifference));
    lookupTable[index] = 0.5f * sign * std::pow(absoluteDifference, 0.37f) - (0.21f * 0.5f) * sign * absoluteDifference;
  }
  return lookupTable;
}

template <class T>
void CheckPlaneTraversalIdentity()
{
  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 6;
  constexpr usize k_DimZ = 7;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  const std::vector<T> input = MakeAhePattern<T>(k_DimX, k_DimY, k_DimZ);
  const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
  const float64 minValue = static_cast<float64>(*minimum);
  const float64 iscale = static_cast<float64>(*maximum) - minValue;
  const std::array<std::array<usize, 3>, 2> radii = {{{2, 1, 1}, {1, 3, 0}}};
  for(const auto& radius : radii)
  {
    for(usize z = 0; z < k_DimZ; ++z)
    {
      const usize zLo = z > radius[2] ? z - radius[2] : 0;
      const usize zHi = std::min(k_DimZ - 1, z + radius[2]);
      CAPTURE(radius, z, zLo);
      const std::vector<T> slab(input.begin() + zLo * k_SliceValues, input.begin() + (zHi + 1) * k_SliceValues);
      std::vector<T> actual(k_SliceValues, T{73});
      std::vector<T> expected(k_SliceValues, T{91});
      if constexpr(std::is_integral_v<T>)
      {
        const std::vector<float32> lookupTable = MakeTraversalLut(input, iscale);
        ImageProcessing::detail::AdaptiveHistogramEqualizationLutPlaneBody<T> body{
            slab.data(), actual.data(), k_DimX, k_DimY, k_DimZ, radius[0], radius[1], radius[2], zLo, z, minValue, iscale, 0.21f, lookupTable.data(), -static_cast<int64>(iscale), lookupTable.size()};
        auto referenceBody = body;
        referenceBody.out_plane = expected.data();
        ahe_test::unclipped::AdaptiveHistogramEqualizationLutPlaneBody(referenceBody, Range{0, k_SliceValues});
        body(Range{0, k_SliceValues});
      }
      else
      {
        ImageProcessing::detail::AdaptiveHistogramEqualizationPlaneBody<T> body{slab.data(), actual.data(), k_DimX, k_DimY,   k_DimZ, radius[0], radius[1],
                                                                                radius[2],   zLo,           z,      minValue, iscale, 0.37f,     0.21f};
        auto referenceBody = body;
        referenceBody.out_plane = expected.data();
        ahe_test::unclipped::AdaptiveHistogramEqualizationPlaneBody(referenceBody, Range{0, k_SliceValues});
        body(Range{0, k_SliceValues});
      }
      REQUIRE(actual == expected);
    }
  }
}

template <class T>
void CheckBlockTraversalIdentity()
{
  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 8;
  const std::vector<T> input = MakeAhePattern<T>(k_DimX, k_DimY, 1);
  const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
  const float64 minValue = static_cast<float64>(*minimum);
  const float64 iscale = static_cast<float64>(*maximum) - minValue;
  const std::atomic_bool shouldCancel{false};
  const std::array<std::array<usize, 2>, 2> radii = {{{2, 1}, {1, 3}}};
  const std::array<std::array<usize, 2>, 2> outputOrigins = {{{0, 0}, {6, 5}}};
  for(const auto& radius : radii)
  {
    for(const auto& origin : outputOrigins)
    {
      constexpr usize k_OutputWidth = 3;
      constexpr usize k_OutputRows = 3;
      const usize inputXBegin = origin[0] > radius[0] ? origin[0] - radius[0] : 0;
      const usize inputYBegin = origin[1] > radius[1] ? origin[1] - radius[1] : 0;
      const usize inputXEnd = std::min(k_DimX, origin[0] + k_OutputWidth + radius[0]);
      const usize inputYEnd = std::min(k_DimY, origin[1] + k_OutputRows + radius[1]);
      const usize inputWidth = inputXEnd - inputXBegin;
      CAPTURE(radius, origin, inputXBegin, inputYBegin);
      std::vector<T> block;
      for(usize y = inputYBegin; y < inputYEnd; ++y)
      {
        block.insert(block.end(), input.begin() + y * k_DimX + inputXBegin, input.begin() + y * k_DimX + inputXEnd);
      }
      std::vector<T> actual(k_OutputWidth * k_OutputRows, T{73});
      std::vector<T> expected(k_OutputWidth * k_OutputRows, T{91});
      if constexpr(std::is_integral_v<T>)
      {
        const std::vector<float32> lookupTable = MakeTraversalLut(input, iscale);
        ImageProcessing::detail::AdaptiveHistogramEqualizationLut2DBlockBody<T> body{block.data(),
                                                                                     actual.data(),
                                                                                     k_DimX,
                                                                                     k_DimY,
                                                                                     radius[0],
                                                                                     radius[1],
                                                                                     inputXBegin,
                                                                                     inputYBegin,
                                                                                     inputWidth,
                                                                                     origin[0],
                                                                                     origin[1],
                                                                                     k_OutputWidth,
                                                                                     minValue,
                                                                                     iscale,
                                                                                     0.21f,
                                                                                     lookupTable.data(),
                                                                                     -static_cast<int64>(iscale),
                                                                                     lookupTable.size(),
                                                                                     shouldCancel};
        auto referenceBody = body;
        referenceBody.output = expected.data();
        ahe_test::unclipped::AdaptiveHistogramEqualizationLut2DBlockBody(referenceBody, Range{0, actual.size()});
        body(Range{0, actual.size()});
      }
      else
      {
        ImageProcessing::detail::AdaptiveHistogramEqualization2DBlockBody<T> body{block.data(), actual.data(), k_DimX,        k_DimY,   radius[0], radius[1], inputXBegin, inputYBegin, inputWidth,
                                                                                  origin[0],    origin[1],     k_OutputWidth, minValue, iscale,    0.37f,     0.21f,       shouldCancel};
        auto referenceBody = body;
        referenceBody.output = expected.data();
        ahe_test::unclipped::AdaptiveHistogramEqualization2DBlockBody(referenceBody, Range{0, actual.size()});
        body(Range{0, actual.size()});
      }
      REQUIRE(actual == expected);
    }
  }
}

template <class T>
void CheckBoundedTraversalIdentity()
{
  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  const std::vector<T> input = MakeAhePattern<T>(k_DimX, k_DimY, 1);
  const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
  const float64 minValue = static_cast<float64>(*minimum);
  const float64 iscale = static_cast<float64>(*maximum) - minValue;
  std::vector<T> expected(input.size(), T{91});
  usize lookupBytes = 0;
  if constexpr(std::is_integral_v<T>)
  {
    const std::vector<float32> lookupTable = MakeTraversalLut(input, iscale);
    lookupBytes = lookupTable.size() * sizeof(float32);
    const ImageProcessing::detail::AdaptiveHistogramEqualizationLutPlaneBody<T> body{
        input.data(), expected.data(), k_DimX, k_DimY, 1, 2, 1, 3, 0, 0, minValue, iscale, 0.21f, lookupTable.data(), -static_cast<int64>(iscale), lookupTable.size()};
    ahe_test::unclipped::AdaptiveHistogramEqualizationLutPlaneBody(body, Range{0, input.size()});
  }
  else
  {
    const ImageProcessing::detail::AdaptiveHistogramEqualizationPlaneBody<T> body{input.data(), expected.data(), k_DimX, k_DimY, 1, 2, 1, 3, 0, 0, minValue, iscale, 0.37f, 0.21f};
    ahe_test::unclipped::AdaptiveHistogramEqualizationPlaneBody(body, Range{0, input.size()});
  }
  // This test store advertises OutOfCore to select useBounded2D. It is resident, not a real disk-backed store.
  // Twenty values of scratch force 2-column tiles, including nonzero input and output origins.
  const std::vector<T> actual = RunBoundedAhe(input, k_DimX, k_DimY, {2, 1, 3}, 0.37f, 0.21f, 20 * sizeof(T) + lookupBytes,
                                              /*maximumReadValues=*/6, /*maximumWriteValues=*/2, /*expectedWriteCount=*/35);
  REQUIRE(actual == expected);
}
} // namespace

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: generic plane operation-order identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  CheckPlaneTraversalIdentity<float32>();
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: LUT plane operation-order identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  CheckPlaneTraversalIdentity<uint16>();
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: generic 2D block operation-order identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  CheckBlockTraversalIdentity<float32>();
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: LUT 2D block operation-order identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  CheckBlockTraversalIdentity<uint16>();
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: bounded-store operation-order identity", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  SECTION("generic tiled route")
  {
    CheckBoundedTraversalIdentity<float32>();
  }
  SECTION("LUT tiled route")
  {
    CheckBoundedTraversalIdentity<uint16>();
  }
}

TEST_CASE("NXImageProcessing::AdaptiveHistogramEqualizationEngine: linear uint8 huge-radius positive control", "[NXImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize k_Huge = std::numeric_limits<uint32>::max();
  SECTION("3D ring")
  {
    const auto input = MakeAhePattern<uint8>(3, 3, 2);
    REQUIRE(RunApplyAhe(input, 3, 3, 2, {k_Huge, k_Huge, k_Huge}, 1.0f, 0.25f) == RunApplyAhe(input, 3, 3, 2, {2, 2, 1}, 1.0f, 0.25f));
  }
  SECTION("bounded 2D")
  {
    const auto input = MakeAhePattern<uint8>(9, 7, 1);
    const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
    const usize lookupBytes = (2 * static_cast<usize>(*maximum - *minimum) + 1) * sizeof(float32);
    const auto expected = RunApplyAhe(input, 9, 7, 1, {8, 6, 0}, 1.0f, 0.25f);
    const auto actual = RunBoundedAhe(input, 9, 7, {k_Huge, k_Huge, k_Huge}, 1.0f, 0.25f, 4096 + lookupBytes,
                                      /*maximumReadValues=*/63, /*maximumWriteValues=*/63, /*expectedWriteCount=*/1);
    REQUIRE(actual == expected);
  }
}
