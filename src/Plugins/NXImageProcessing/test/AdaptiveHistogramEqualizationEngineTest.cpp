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
  Result<> applyResult = ApplyAdaptiveHistogramEqualization(inputStore, outputStore, SizeVec3{dimX, dimY, 1}, radius, alpha, beta, shouldCancel, messageHandler, targetBytes);
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

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: resident state requires a complete dataset-scaled reservation",
          "[ImageProcessing][AdaptiveHistogramEqualizationEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::array<usize, 3> radius = {10, 10, 10};

  auto requiredResult = ImageProcessing::detail::CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes<uint8>(dims, radius, /*useLinearUint8=*/true);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == 163 * k_MiB + 512ULL * 1024ULL);
  Result<usize> overflowDimsResult =
      ImageProcessing::detail::CalculateAdaptiveHistogramEqualizationResidentWorkingMemoryBytes<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2}, radius, /*useLinearUint8=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimsResult);

  REQUIRE(ImageProcessing::detail::ShouldUseAdaptiveHistogramEqualizationResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseAdaptiveHistogramEqualizationResidentState(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
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

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: real-OOC selector uses resident state only after a complete grant",
          "[ImageProcessing][AdaptiveHistogramEqualizationEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  constexpr uint64 k_PartialBudgetBytes = 64 * k_MiB;
  constexpr uint64 k_CompleteBudgetBytes = 128 * k_MiB;
  const SizeVec3 dims{dimX, dimY, dimZ};
  const std::array<usize, 3> radius = {1, 1, 1};
  const std::vector<uint8> input = MakeAhePattern<uint8>(dimX, dimY, dimZ);
  const std::vector<uint8> expected = RunApplyAhe<uint8>(input, dimX, dimY, dimZ, radius, 1.0f, 0.25f);

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    OutOfCoreTransferCountingDataStore<uint8> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{0});
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint8{255});
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
  REQUIRE(partialTransfers == std::array<usize, 4>{2, dimX * dimY * dimZ, 1, dimX * dimY * dimZ});
  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers == std::array<usize, 4>{1, 0, 1, input.size()});
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: bounded 2D fast and generic transfers", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize totalValues = dimX * dimY;
  constexpr std::array<usize, 3> radius = {2, 1, 3};
  constexpr std::array<int32, 3> oracleRadius = {2, 1, 3};

  SECTION("uint8 linear fast path")
  {
    const std::vector<uint8> input = MakeAhePattern<uint8>(dimX, dimY, 1);
    const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
    const usize span = static_cast<usize>(*maximum - *minimum);
    const usize lutBytes = (2 * span + 1) * sizeof(float);
    const std::vector<uint8> expected = AdaptiveHistogramEqualizationOracle<uint8>(input, dimX, dimY, 1, oracleRadius, 1.0f, 0.25f);

    struct TransferCase
    {
      const char* label;
      usize bufferBytes;
      bool fullWidth;
      usize coreRows;
      usize coreColumns;
      usize maximumReadValues;
      usize maximumWriteValues;
      usize writeCount;
    };
    const std::array<TransferCase, 2> transferCases = {{{"full-width row blocks", 342, true, 2, 9, 36, 18, 4}, {"overwide X tiles", 68, false, 1, 2, 6, 2, 35}}};
    for(const TransferCase& transferCase : transferCases)
    {
      DYNAMIC_SECTION(transferCase.label)
      {
        const usize targetBytes = transferCase.bufferBytes + lutBytes;
        const auto planResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(uint8), targetBytes, lutBytes,
                                                                                                   /*useHorizontalSums=*/true);
        SIMPLNX_RESULT_REQUIRE_VALID(planResult);
        const auto& plan = planResult.value();
        REQUIRE(plan.fullWidth == transferCase.fullWidth);
        REQUIRE(plan.coreRows == transferCase.coreRows);
        REQUIRE(plan.coreColumns == transferCase.coreColumns);
        REQUIRE(plan.residentBytes == targetBytes);

        const std::vector<uint8> actual = RunBoundedAhe(input, dimX, dimY, radius, 1.0f, 0.25f, targetBytes, transferCase.maximumReadValues, transferCase.maximumWriteValues, transferCase.writeCount);
        RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
      }
    }

    const auto fixedPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(uint8),
                                                                                                    ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, lutBytes,
                                                                                                    /*useHorizontalSums=*/true, /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(fixedPlanResult);
    REQUIRE(fixedPlanResult.value().fixedOutput);
    const std::vector<uint8> fixedOutputActual = RunBoundedAhe(input, dimX, dimY, radius, 1.0f, 0.25f, ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes,
                                                               /*maximumReadValues=*/totalValues, /*maximumWriteValues=*/totalValues, /*expectedWriteCount=*/1);
    RequireAheClose<uint8>(fixedOutputActual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);

    const usize overwideFixedScratchBytes = ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes - ImageProcessing::detail::k_AdaptiveHistogramEqualization2DFixedOutputBytes - 68;
    const auto overwideFixedPlan = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(uint8),
                                                                                                      ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, overwideFixedScratchBytes,
                                                                                                      /*useHorizontalSums=*/true, /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(overwideFixedPlan);
    REQUIRE(overwideFixedPlan.value().fixedOutput);
    REQUIRE_FALSE(overwideFixedPlan.value().fullWidth);
    REQUIRE(overwideFixedPlan.value().coreColumns == 2);
  }

  SECTION("float32 generic path")
  {
    const std::vector<float32> input = MakeAhePattern<float32>(dimX, dimY, 1);
    const std::vector<float32> expected = AdaptiveHistogramEqualizationOracle<float32>(input, dimX, dimY, 1, oracleRadius, 0.3f, 0.3f);
    struct TransferCase
    {
      const char* label;
      usize targetBytes;
      bool fullWidth;
      usize coreRows;
      usize coreColumns;
      usize maximumReadValues;
      usize maximumWriteValues;
      usize writeCount;
    };
    const std::array<TransferCase, 2> transferCases = {{{"full-width row blocks", 216, true, 2, 9, 36, 18, 4}, {"overwide X tiles", 80, false, 1, 2, 6, 2, 35}}};
    for(const TransferCase& transferCase : transferCases)
    {
      DYNAMIC_SECTION(transferCase.label)
      {
        const auto planResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(float32), transferCase.targetBytes,
                                                                                                   /*fixedScratchBytes=*/0, /*useHorizontalSums=*/false);
        SIMPLNX_RESULT_REQUIRE_VALID(planResult);
        const auto& plan = planResult.value();
        REQUIRE(plan.fullWidth == transferCase.fullWidth);
        REQUIRE(plan.coreRows == transferCase.coreRows);
        REQUIRE(plan.coreColumns == transferCase.coreColumns);
        REQUIRE(plan.residentBytes == transferCase.targetBytes);

        const std::vector<float32> actual =
            RunBoundedAhe(input, dimX, dimY, radius, 0.3f, 0.3f, transferCase.targetBytes, transferCase.maximumReadValues, transferCase.maximumWriteValues, transferCase.writeCount);
        RequireAheClose<float32>(actual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.0);
      }
    }

    const auto fixedPlanResult =
        ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(float32), ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes,
                                                                           /*fixedScratchBytes=*/0, /*useHorizontalSums=*/false,
                                                                           /*preferFixedOutput=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(fixedPlanResult);
    REQUIRE(fixedPlanResult.value().fixedOutput);
    const std::vector<float32> fixedOutputActual =
        RunBoundedAhe(input, dimX, dimY, radius, 0.3f, 0.3f, ImageProcessing::detail::k_AdaptiveHistogramEqualization2DTargetBytes, /*maximumReadValues=*/totalValues,
                      /*maximumWriteValues=*/totalValues, /*expectedWriteCount=*/1);
    RequireAheClose<float32>(fixedOutputActual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.0);
  }

  auto smallTargetPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(dimX, dimY, radius[0], radius[1], sizeof(uint8), /*targetBytes=*/1, /*fixedScratchBytes=*/0,
                                                                                                  /*useHorizontalSums=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(smallTargetPlanResult);
  auto overflowDimPlanResult = ImageProcessing::detail::CreateAdaptiveHistogramEqualization2DPlan(std::numeric_limits<usize>::max(), dimY, radius[0], radius[1], sizeof(uint8),
                                                                                                  /*targetBytes=*/342, /*fixedScratchBytes=*/0, /*useHorizontalSums=*/true);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimPlanResult);
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: bounded 2D constant and cancellation preserve output", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize dimX = 9;
  constexpr usize dimY = 7;
  constexpr usize totalValues = dimX * dimY;
  constexpr std::array<usize, 3> radius = {2, 1, 3};

  SECTION("constant input uses bounded copy chunks")
  {
    const std::vector<int32> input(totalValues, 42);
    const std::vector<int32> output = RunBoundedAhe(input, dimX, dimY, radius, 0.3f, 0.3f, /*targetBytes=*/32, /*maximumReadValues=*/8, /*maximumWriteValues=*/8, /*expectedWriteCount=*/8);
    REQUIRE(output == input);
  }

  SECTION("cancel after transform read performs no output write")
  {
    constexpr uint8 poison = 73;
    const std::vector<uint8> input = MakeAhePattern<uint8>(dimX, dimY, 1);
    std::atomic_bool shouldCancel{false};
    CancelAfterReadDataStore<uint8> cancellingInputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, uint8{0}, shouldCancel, /*triggerRead=*/2);
    OutOfCoreTransferCountingDataStore<uint8> outputStore(ShapeType{1, dimY, dimX}, ShapeType{1}, poison);
    for(usize index = 0; index < input.size(); ++index)
    {
      cancellingInputStore.setValue(index, input[index]);
    }
    IFilter::MessageHandler messageHandler{};
    const auto [minimum, maximum] = std::minmax_element(input.cbegin(), input.cend());
    const usize span = static_cast<usize>(*maximum - *minimum);
    const usize lutBytes = (2 * span + 1) * sizeof(float);
    Result<> applyResult = ApplyAdaptiveHistogramEqualization(cancellingInputStore, outputStore, SizeVec3{dimX, dimY, 1}, radius, 1.0f, 0.25f, shouldCancel, messageHandler,
                                                              /*target2DBytes=*/342 + lutBytes);
    SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
    REQUIRE(shouldCancel);
    REQUIRE(cancellingInputStore.readCount() == 2);
    REQUIRE(outputStore.writtenValues() == 0);
    for(const uint8 value : outputStore)
    {
      REQUIRE(value == poison);
    }
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: matches independent oracle (3D + 2D)", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]", uint8, int16, int32, float32,
                   float64)
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

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: constant image passes through (ITK would NaN)", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // max == min => iscale == 0. ITK computes 0/0 = NaN; the engine passes the input through unchanged.
  constexpr usize D = 6;
  const std::vector<int32> input(D * D * D, 42);
  const std::vector<int32> out = RunApplyAhe<int32>(input, D, D, D, {2, 2, 2}, 0.3f, 0.3f);
  for(usize i = 0; i < out.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(out[i] == 42);
  }

  // Float constant too.
  const std::vector<float32> finput(D * D * D, 3.5f);
  const std::vector<float32> fout = RunApplyAhe<float32>(finput, D, D, D, {1, 1, 1}, 0.3f, 0.3f);
  for(usize i = 0; i < fout.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(fout[i] == 3.5f);
  }
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: integer fallback (span exceeds LUT cap)", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // Force the NON-LUT integer path. The difference-keyed LUT fast path is gated at kMaxAheLutEntries = 2^22
  // table entries (an integer gray span <= 2^21). The TEMPLATE grid above never trips the gate (its patterns
  // are in [0,100] => span 100 => LUT path), so this case stretches an int32 image's range with two extreme
  // voxels (0 and 5,000,000 => 2*span+1 ~ 1e7 > 2^22) to drop the engine onto the exact per-neighbor body.
  // That pins the fallback body's boundary logic (which is duplicated from the LUT body) and confirms the gate.
  // The bulk of the voxels sit in a small band so the image is still non-constant and exercises accumulation.
  constexpr usize dimX = 8;
  constexpr usize dimY = 8;
  constexpr usize dimZ = 4;
  std::vector<int32> input(dimX * dimY * dimZ);
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
      const std::vector<int32> expected = AdaptiveHistogramEqualizationOracle<int32>(input, dimX, dimY, dimZ, r, p[0], p[1]);
      const std::vector<int32> actual = RunApplyAhe<int32>(input, dimX, dimY, dimZ, radius, p[0], p[1]);
      INFO("radius={" << r[0] << "," << r[1] << "," << r[2] << "} alpha=" << p[0] << " beta=" << p[1]);
      RequireAheClose<int32>(actual, expected, /*atol=*/1e-4, /*rtol=*/1e-5, /*maxDiffFraction=*/0.01);
    }
  }
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: alpha=beta=1 is the identity", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  // Closed form independent of the CF internals: with alpha=beta=1, cf = 0.5*s*ad - 0.5*s*ad + u = u for
  // every neighbor, so sum/ikernel = u and out = iscale*(u+0.5)+min = the original pixel. This is a strong
  // anchor that does NOT depend on how the cumulative function is spelled. Float32 input avoids the extra
  // integer-truncation slack while still allowing tiny pow()-reassociation drift.
  const std::vector<float32> input = MakeAhePattern<float32>(9, 8, 7);
  const std::vector<float32> out = RunApplyAhe<float32>(input, 9, 8, 7, {2, 2, 2}, 1.0f, 1.0f);
  RequireAheClose<float32>(out, input, /*atol=*/1e-2, /*rtol=*/1e-4, /*maxDiffFraction=*/0.0);
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: uint8 alpha=1 beta=0.25 moving-box path", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  constexpr usize dimX = 17;
  constexpr usize dimY = 13;
  constexpr usize dimZ = 9;
  constexpr std::array<int32, 3> oracleRadius = {3, 2, 4};
  constexpr std::array<usize, 3> radius = {3, 2, 4};
  const std::vector<uint8> input = MakeAhePattern<uint8>(dimX, dimY, dimZ);
  const std::vector<uint8> expected = AdaptiveHistogramEqualizationOracle<uint8>(input, dimX, dimY, dimZ, oracleRadius, 1.0f, 0.25f);
  const std::vector<uint8> actual = RunApplyAhe<uint8>(input, dimX, dimY, dimZ, radius, 1.0f, 0.25f);
  RequireAheClose<uint8>(actual, expected, /*atol=*/0.0, /*rtol=*/0.0, /*maxDiffFraction=*/0.005);
}

TEST_CASE("ImageProcessing::AdaptiveHistogramEqualizationEngine: validates dimensions and stores", "[ImageProcessing][AdaptiveHistogramEqualizationEngine]")
{
  IFilter::MessageHandler messageHandler{};
  std::atomic_bool shouldCancel{false};

  SECTION("rejects zero dimensions")
  {
    DataStore<uint8> inputStore(ShapeType{1}, ShapeType{1}, 0);
    DataStore<uint8> outputStore(ShapeType{1}, ShapeType{1}, 0);
    const Result<> result = ApplyAdaptiveHistogramEqualization<uint8>(inputStore, outputStore, SizeVec3{0, 1, 1}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8541);
  }

  SECTION("rejects mismatched input and output stores")
  {
    DataStore<uint8> shortStore(ShapeType{63}, ShapeType{1}, 0);
    DataStore<uint8> fullStore(ShapeType{64}, ShapeType{1}, 0);
    Result<> result = ApplyAdaptiveHistogramEqualization<uint8>(shortStore, fullStore, SizeVec3{4, 4, 4}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8543);

    result = ApplyAdaptiveHistogramEqualization<uint8>(fullStore, shortStore, SizeVec3{4, 4, 4}, {1, 1, 1}, 0.3f, 0.3f, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8544);
  }
}
