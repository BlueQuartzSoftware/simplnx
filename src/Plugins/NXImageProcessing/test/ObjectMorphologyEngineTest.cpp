#include "ObjectMorphologyFilterTestUtils.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <limits>
#include <optional>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;
using object_morph_test::FlatIndex;
using object_morph_test::MakeObjectPattern;
using object_morph_test::ObjectMorphologyOracle;
using object_morph_test::RunApplyObjectMorphology;

namespace
{
const char* KernelName(KernelType kt)
{
  switch(kt)
  {
  case KernelType::Annulus:
    return "Annulus";
  case KernelType::Ball:
    return "Ball";
  case KernelType::Box:
    return "Box";
  case KernelType::Cross:
    return "Cross";
  }
  return "?";
}

const char* OpName(ObjectMorphOp op)
{
  return (op == ObjectMorphOp::Dilate) ? "Dilate" : "Erode";
}

template <class T>
class CountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_CopyIntoBufferCalls++;
    m_CopiedValueCount += buffer.size();
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    m_ReadStarts.push_back(startIndex);
    m_ReadCounts.push_back(buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_CopyFromBufferCalls++;
    m_CopiedWriteValueCount += buffer.size();
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WriteStarts.push_back(startIndex);
    m_WriteCounts.push_back(buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize copyIntoBufferCallCount() const
  {
    return m_CopyIntoBufferCalls;
  }

  usize copiedValueCount() const
  {
    return m_CopiedValueCount;
  }

  usize copyFromBufferCallCount() const
  {
    return m_CopyFromBufferCalls;
  }

  usize maxReadValues() const
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const
  {
    return m_MaxWriteValues;
  }

  usize copiedWriteValueCount() const
  {
    return m_CopiedWriteValueCount;
  }

  const std::vector<usize>& readStarts() const
  {
    return m_ReadStarts;
  }

  const std::vector<usize>& writeStarts() const
  {
    return m_WriteStarts;
  }

  const std::vector<usize>& readCounts() const
  {
    return m_ReadCounts;
  }

  const std::vector<usize>& writeCounts() const
  {
    return m_WriteCounts;
  }

private:
  mutable usize m_CopyIntoBufferCalls = 0;
  mutable usize m_CopiedValueCount = 0;
  mutable usize m_MaxReadValues = 0;
  mutable std::vector<usize> m_ReadStarts;
  mutable std::vector<usize> m_ReadCounts;
  usize m_CopyFromBufferCalls = 0;
  usize m_CopiedWriteValueCount = 0;
  usize m_MaxWriteValues = 0;
  std::vector<usize> m_WriteStarts;
  std::vector<usize> m_WriteCounts;
};

template <class T>
class OutOfCoreCountingDataStore : public CountingDataStore<T>
{
public:
  using CountingDataStore<T>::CountingDataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

template <class T>
std::vector<T> RunBoundedGather(const std::vector<T>& input, usize dimX, usize dimY, KernelType kernelType, const std::array<int32, 3>& radius, ObjectMorphOp op, T objectValue, T backgroundValue,
                                usize targetBytes)
{
  DataStructure dataStructure;
  auto* inputArrayPtr = unit_test::CreateTestDataArray<T>(dataStructure, "input", {1, dimY, dimX}, {1});
  auto* outputArrayPtr = unit_test::CreateTestDataArray<T>(dataStructure, "output", {1, dimY, dimX}, {1});
  REQUIRE(inputArrayPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(input.data(), input.size())).valid());

  const StructuringElement structuringElement = MakeStructuringElement(kernelType, radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  ObjectMorphGather<T> gather{inputArrayPtr->getDataStoreRef(),
                              outputArrayPtr->getDataStoreRef(),
                              SizeVec3Type{dimX, dimY, 1},
                              structuringElement,
                              boxOffsets,
                              op,
                              objectValue,
                              backgroundValue,
                              shouldCancel,
                              messageHandler,
                              targetBytes};
  REQUIRE(gather().valid());

  std::vector<T> output(input.size());
  REQUIRE(outputArrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(output.data(), output.size())).valid());
  return output;
}

// For every kernel x radius x op: (a) the ITK-faithful in-core Scatter path must equal the INDEPENDENT
// scatter oracle, and (b) the streamed OOC Gather path must equal the Scatter path BYTE-FOR-BYTE (the D3
// gate). The force guards select the path deterministically regardless of the build's storage type. The
// pattern is multi-object with edge-touching blocks and solid interiors (see MakeObjectPattern).
template <class T>
void CheckObjectMorphology(usize dimX, usize dimY, usize dimZ)
{
  const T objectValue = static_cast<T>(1);
  const T backgroundValue = static_cast<T>(0);
  const std::vector<T> input = MakeObjectPattern<T>(dimX, dimY, dimZ, objectValue, backgroundValue);

  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<std::array<int32, 3>> radii = {{1, 1, 1}, {2, 1, 1}, {2, 2, 2}};
  const std::vector<ObjectMorphOp> ops = {ObjectMorphOp::Dilate, ObjectMorphOp::Erode};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      for(ObjectMorphOp op : ops)
      {
        const std::vector<T> expected = ObjectMorphologyOracle<T>(input, dimX, dimY, dimZ, kt, radius, op, objectValue, backgroundValue);

        std::vector<T> inCore;
        {
          const ForceInCoreAlgorithmGuard guard;
          inCore = RunApplyObjectMorphology<T>(input, dimX, dimY, dimZ, kt, radius, op, objectValue, backgroundValue);
        }
        std::vector<T> ooc;
        {
          const ForceOocAlgorithmGuard guard(true);
          ooc = RunApplyObjectMorphology<T>(input, dimX, dimY, dimZ, kt, radius, op, objectValue, backgroundValue);
        }

        for(usize i = 0; i < input.size(); ++i)
        {
          INFO("kernel=" << KernelName(kt) << " radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} op=" << OpName(op) << " index=" << i);
          REQUIRE(inCore[i] == expected[i]); // in-core Scatter == independent oracle
          REQUIRE(ooc[i] == inCore[i]);      // OOC Gather == in-core Scatter (byte-for-byte D3 gate)
        }
      }
    }
  }
}
} // namespace

TEMPLATE_TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: Scatter/Gather match the independent oracle (3D + 2D)", "[NXImageProcessing][ObjectMorphologyEngine]", int32, uint8, float32)
{
  using T = TestType;
  SECTION("3D non-cubic 10x11x9")
  {
    CheckObjectMorphology<T>(10, 11, 9);
  }
  SECTION("2D single-plane 12x10x1 (rz kernel clipped to plane)")
  {
    CheckObjectMorphology<T>(12, 10, 1);
  }
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: 3D Gather reads each input value once", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  constexpr usize k_DimX = 13;
  constexpr usize k_DimY = 11;
  constexpr usize k_DimZ = 9;
  constexpr int16 k_ObjectValue = 5;
  constexpr int16 k_BackgroundValue = 2;
  constexpr std::array<int32, 3> k_Radius = {2, 2, 2};

  const std::vector<int16> input = MakeObjectPattern<int16>(k_DimX, k_DimY, k_DimZ, k_ObjectValue, int16{0});
  const std::vector<int16> expected = ObjectMorphologyOracle<int16>(input, k_DimX, k_DimY, k_DimZ, KernelType::Box, k_Radius, ObjectMorphOp::Erode, k_ObjectValue, k_BackgroundValue);

  CountingDataStore<int16> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, std::optional<int16>{int16{0}});
  CountingDataStore<int16> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, std::optional<int16>{int16{0}});
  REQUIRE(inputStore.copyFromBuffer(0, nonstd::span<const int16>(input.data(), input.size())).valid());

  const StructuringElement structuringElement = MakeStructuringElement(KernelType::Box, k_Radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  ObjectMorphGather<int16> gather{inputStore,   outputStore,   SizeVec3Type{k_DimX, k_DimY, k_DimZ}, structuringElement, boxOffsets, ObjectMorphOp::Erode, k_ObjectValue, k_BackgroundValue,
                                  shouldCancel, messageHandler};
  REQUIRE(gather().valid());

  std::vector<int16> output(input.size());
  REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<int16>(output.data(), output.size())).valid());
  REQUIRE(output == expected);

  CAPTURE(inputStore.copyIntoBufferCallCount(), inputStore.copiedValueCount(), input.size());
  REQUIRE(inputStore.copiedValueCount() == input.size());
  REQUIRE(outputStore.copyFromBufferCallCount() == 1);
  REQUIRE(outputStore.maxWriteValues() == input.size());
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: adaptive 3D batches preserve non-box object morphology transfers", "[NXImageProcessing][ObjectMorphologyEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 13;
  constexpr usize k_DimY = 11;
  constexpr usize k_DimZ = 9;
  constexpr usize k_SliceValues = k_DimX * k_DimY;
  constexpr int16 k_ObjectValue = 5;
  constexpr int16 k_BackgroundValue = 2;
  constexpr std::array<int32, 3> k_Radius = {2, 2, 2};
  constexpr usize k_AdaptiveTargetBytes = 5000;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::vector<int16> input = MakeObjectPattern<int16>(k_DimX, k_DimY, k_DimZ, k_ObjectValue, int16{0});
  const StructuringElement structuringElement = MakeStructuringElement(KernelType::Ball, k_Radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();

  auto planResult = ImageProcessing::detail::CreateObjectMorphology3DPlan<int16>(dims, static_cast<usize>(k_Radius[2]), k_AdaptiveTargetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(planResult);
  const auto& plan = planResult.value();
  REQUIRE(plan.output_batch_depth > 1);
  REQUIRE(plan.output_batch_depth == 3);
  REQUIRE(plan.compute_batch_depth == plan.output_batch_depth);
  REQUIRE(plan.resident_bytes == 2 * (plan.output_batch_depth + 2 * static_cast<usize>(k_Radius[2])) * k_SliceValues * sizeof(int16));
  REQUIRE(plan.resident_bytes <= k_AdaptiveTargetBytes);

  for(const ObjectMorphOp op : {ObjectMorphOp::Dilate, ObjectMorphOp::Erode})
  {
    CountingDataStore<int16> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
    CountingDataStore<int16> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
    Result<> copyFromBufferResult = inputStore.copyFromBuffer(0, nonstd::span<const int16>(input.data(), input.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    ObjectMorphGather<int16> gather{inputStore, outputStore, dims, structuringElement, boxOffsets, op, k_ObjectValue, k_BackgroundValue, shouldCancel, messageHandler, k_AdaptiveTargetBytes};
    Result<> gatherResult = gather();
    SIMPLNX_RESULT_REQUIRE_VALID(gatherResult);

    const std::vector<int16> expected = ObjectMorphologyOracle<int16>(input, k_DimX, k_DimY, k_DimZ, KernelType::Ball, k_Radius, op, k_ObjectValue, k_BackgroundValue);
    std::vector<int16> actual(input.size());
    Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int16>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
    INFO("op=" << OpName(op));
    REQUIRE(actual == expected);
    REQUIRE(inputStore.copiedValueCount() == input.size());
    REQUIRE(inputStore.readStarts() == std::vector<usize>{0, 5 * k_SliceValues, 8 * k_SliceValues});
    REQUIRE(inputStore.readCounts() == std::vector<usize>{5 * k_SliceValues, 3 * k_SliceValues, k_SliceValues});
    REQUIRE(inputStore.maxReadValues() == 5 * k_SliceValues);
    REQUIRE(outputStore.copiedWriteValueCount() == input.size());
    REQUIRE(outputStore.writeStarts() == std::vector<usize>{0, k_SliceValues, 4 * k_SliceValues});
    REQUIRE(outputStore.writeCounts() == std::vector<usize>{k_SliceValues, 3 * k_SliceValues, 5 * k_SliceValues});
    REQUIRE(outputStore.copyFromBufferCallCount() == 3);
    REQUIRE(outputStore.maxWriteValues() == 5 * k_SliceValues);
  }

  auto boundedPlanResult = ImageProcessing::detail::CreateObjectMorphology3DPlan<int16>(dims, static_cast<usize>(k_Radius[2]), /*targetBytes=*/3003);
  SIMPLNX_RESULT_REQUIRE_VALID(boundedPlanResult);
  REQUIRE(boundedPlanResult.value().output_batch_depth == 1);
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: resident state requires a complete dataset-scaled reservation", "[NXImageProcessing][ObjectMorphologyEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 512;
  constexpr usize k_DimY = 512;
  constexpr usize k_DimZ = 128;
  constexpr usize k_ValueCount = k_DimX * k_DimY * k_DimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};

  auto requiredResult = ImageProcessing::detail::CalculateObjectMorphologyResidentWorkingMemoryBytes<int16>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == k_ValueCount * 2 * sizeof(int16));
  REQUIRE(requiredResult.value() == 128 * k_MiB);
  Result<usize> overflowDimsResult = ImageProcessing::detail::CalculateObjectMorphologyResidentWorkingMemoryBytes<int32>(SizeVec3Type{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimsResult);

  REQUIRE(ImageProcessing::detail::ShouldUseObjectMorphologyResidentState(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseObjectMorphologyResidentState(SizeVec3Type{k_DimX, k_DimY, 1}));

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(384 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveObjectMorphologyResidentWorkingMemory<int16>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 96 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveObjectMorphologyResidentWorkingMemory<int16>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: real-OOC selector uses resident state only after a complete grant", "[NXImageProcessing][ObjectMorphologyEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 8;
  constexpr usize k_DimY = 7;
  constexpr usize k_DimZ = 6;
  constexpr int16 k_ObjectValue = 5;
  constexpr int16 k_BackgroundValue = 2;
  constexpr uint64 k_PartialBudgetBytes = 4096;
  constexpr uint64 k_CompleteBudgetBytes = 8192;
  const SizeVec3Type dims{k_DimX, k_DimY, k_DimZ};
  const std::array<int32, 3> radius = {1, 1, 1};
  const std::vector<int16> input = MakeObjectPattern<int16>(k_DimX, k_DimY, k_DimZ, k_ObjectValue, int16{0});
  const std::vector<int16> expected = ObjectMorphologyOracle<int16>(input, k_DimX, k_DimY, k_DimZ, KernelType::Box, radius, ObjectMorphOp::Erode, k_ObjectValue, k_BackgroundValue);
  const StructuringElement structuringElement = MakeStructuringElement(KernelType::Box, radius);
  const std::vector<SEOffset> boxOffsets = ImageProcessing::detail::MakeFullBoxNeighborOffsets();

  DataStructure dispatchDataStructure;
  auto* dispatchInputPtr = unit_test::CreateTestDataArray<int16>(dispatchDataStructure, "dispatch input", {k_DimZ, k_DimY, k_DimX}, {1});
  auto* dispatchOutputPtr = unit_test::CreateTestDataArray<int16>(dispatchDataStructure, "dispatch output", {k_DimZ, k_DimY, k_DimX}, {1});

  auto& manager = CacheMemoryBudgetManager::Instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    OutOfCoreCountingDataStore<int16> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{0});
    OutOfCoreCountingDataStore<int16> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, int16{-1});
    for(usize index = 0; index < input.size(); ++index)
    {
      inputStore.setValue(index, input[index]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    const ForceOocAlgorithmGuard forceOoc(true);
    Result<> applyResult = ApplyObjectMorphology(inputStore, outputStore, dims, structuringElement, boxOffsets, ObjectMorphOp::Erode, k_ObjectValue, k_BackgroundValue, *dispatchInputPtr,
                                                 *dispatchOutputPtr, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return std::array<usize, 6>{inputStore.copyIntoBufferCallCount(), inputStore.maxReadValues(),    outputStore.copyFromBufferCallCount(),
                                outputStore.maxWriteValues(),         inputStore.copiedValueCount(), outputStore.copiedWriteValueCount()};
  };

  const auto partialTransfers = run(k_PartialBudgetBytes);
  auto partialPlanResult = ImageProcessing::detail::CreateObjectMorphology3DPlan<int16>(dims, static_cast<usize>(radius[2]), k_PartialBudgetBytes / 4);
  SIMPLNX_RESULT_REQUIRE_VALID(partialPlanResult);
  const auto& partialPlan = partialPlanResult.value();
  REQUIRE(partialTransfers[0] > 1);
  REQUIRE(partialTransfers[1] <= 3 * k_DimX * k_DimY);
  REQUIRE(partialTransfers[2] == (k_DimZ + partialPlan.output_batch_depth - 1) / partialPlan.output_batch_depth);
  REQUIRE(partialTransfers[3] == (partialPlan.output_batch_depth + static_cast<usize>(radius[2])) * k_DimX * k_DimY);

  auto undersizedPlanResult = ImageProcessing::detail::CreateObjectMorphology3DPlan<int16>(dims, static_cast<usize>(radius[2]), /*targetBytes=*/671);
  SIMPLNX_RESULT_REQUIRE_INVALID(undersizedPlanResult);
  REQUIRE(undersizedPlanResult.errors().front().code == -8765);
  const auto undersizedTransfers = run(/*budgetBytes=*/512);
  REQUIRE(undersizedTransfers[4] == input.size());
  REQUIRE(undersizedTransfers[0] == 1);
  REQUIRE(undersizedTransfers[1] == input.size());
  REQUIRE(undersizedTransfers[2] == 1);
  REQUIRE(undersizedTransfers[5] == input.size());

  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers[0] == 1);
  REQUIRE(completeTransfers[1] == input.size());
  REQUIRE(completeTransfers[2] == 1);
  REQUIRE(completeTransfers[3] == input.size());
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: bounded 2D planner accounts for input, mask, and output", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  using ImageProcessing::detail::CreateObjectMorphology2DPlan;

  SECTION("full-width blocks")
  {
    const auto result = CreateObjectMorphology2DPlan(/*dimX=*/7, /*dimY=*/11, /*radiusX=*/2, /*radiusY=*/1, /*valueBytes=*/2, /*targetBytes=*/175);
    REQUIRE(result.valid());
    const auto& plan = result.value();
    REQUIRE(plan.full_width);
    REQUIRE(plan.core_rows == 3);
    REQUIRE(plan.core_columns == 7);
    REQUIRE(plan.resident_bytes == 175);
  }

  SECTION("overwide X tiles")
  {
    const auto result = CreateObjectMorphology2DPlan(/*dimX=*/31, /*dimY=*/4, /*radiusX=*/2, /*radiusY=*/1, /*valueBytes=*/2, /*targetBytes=*/132);
    REQUIRE(result.valid());
    const auto& plan = result.value();
    REQUIRE_FALSE(plan.full_width);
    REQUIRE(plan.core_rows == 1);
    REQUIRE(plan.core_columns == 4);
    REQUIRE(plan.resident_bytes == 132);
  }

  SECTION("insufficient and overflowing budgets are rejected")
  {
    REQUIRE(CreateObjectMorphology2DPlan(/*dimX=*/31, /*dimY=*/4, /*radiusX=*/2, /*radiusY=*/1, /*valueBytes=*/2, /*targetBytes=*/86).invalid());
    REQUIRE(CreateObjectMorphology2DPlan(/*dimX=*/31, /*dimY=*/4, /*radiusX=*/std::numeric_limits<usize>::max(), /*radiusY=*/1, /*valueBytes=*/2, /*targetBytes=*/132).invalid());
  }
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: bounded 2D full-width blocks and overwide tiles match the independent oracle", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  constexpr usize k_DimX = 31;
  constexpr usize k_DimY = 9;
  constexpr int16 k_ObjectValue = 5;
  constexpr int16 k_BackgroundValue = 2;
  const std::vector<int16> input = MakeObjectPattern<int16>(k_DimX, k_DimY, 1, k_ObjectValue, int16{0});
  constexpr std::array<int32, 3> k_Radius = {2, 1, 1};

  for(const ObjectMorphOp op : {ObjectMorphOp::Dilate, ObjectMorphOp::Erode})
  {
    const std::vector<int16> expected = ObjectMorphologyOracle<int16>(input, k_DimX, k_DimY, 1, KernelType::Box, k_Radius, op, k_ObjectValue, k_BackgroundValue);
    const std::vector<int16> blocked = RunBoundedGather<int16>(input, k_DimX, k_DimY, KernelType::Box, k_Radius, op, k_ObjectValue, k_BackgroundValue, /*targetBytes=*/835);
    const std::vector<int16> tiled = RunBoundedGather<int16>(input, k_DimX, k_DimY, KernelType::Box, k_Radius, op, k_ObjectValue, k_BackgroundValue, /*targetBytes=*/132);
    INFO("op=" << OpName(op));
    REQUIRE(blocked == expected);
    REQUIRE(tiled == expected);
  }
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: explicit hand-computed single-object dilation", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  // A single object voxel at the center of a 3x3x3 image (all else background). The center is a boundary
  // object pixel (all 26 neighbors are non-object), so a Box r{1,1,1} Dilate paints the whole 3x3x3 box:
  // every voxel becomes the object value.
  constexpr usize k_D = 3;
  std::vector<int32> input(k_D * k_D * k_D, 0);
  input[FlatIndex(1, 1, 1, k_D, k_D)] = 1;

  const std::vector<int32> expected(k_D * k_D * k_D, 1);

  const ForceInCoreAlgorithmGuard guard;
  const std::vector<int32> out = RunApplyObjectMorphology<int32>(input, k_D, k_D, k_D, KernelType::Box, {1, 1, 1}, ObjectMorphOp::Dilate, 1, 0);
  for(usize i = 0; i < out.size(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(out[i] == expected[i]);
  }
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: Erode with BackgroundValue colliding with an existing label", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  // Three-label volume: object=5 (a coarse block checkerboard), and among the non-object voxels a mix of an
  // existing label 2 and background 0. Erode paints the Background Value over the SE of each boundary object
  // pixel. Set backgroundValue=2 so it COLLIDES with the pre-existing label 2, pinning that: (a) the boundary
  // test treats EVERY value != objectValue (both 0 and 2) as non-object; (b) erosion writes the Background
  // Value regardless of whether it already exists; and (c) the OOC Gather still equals the in-core Scatter
  // byte-for-byte. Validated against the independent oracle over the same input.
  constexpr usize k_DimX = 10;
  constexpr usize k_DimY = 11;
  constexpr usize k_DimZ = 9;
  const int32 objectValue = 5;
  const int32 backgroundValue = 2; // collides with the pre-existing label 2 below

  std::vector<int32> input(k_DimX * k_DimY * k_DimZ, 0);
  for(usize z = 0; z < k_DimZ; ++z)
  {
    for(usize y = 0; y < k_DimY; ++y)
    {
      for(usize x = 0; x < k_DimX; ++x)
      {
        int32 value = 0;
        if((((x / 4) + (y / 4) + (z / 4)) % 2) == 0)
        {
          value = objectValue;
        }
        else
        {
          value = ((x * 131 + y * 57 + z * 29) % 3 == 0) ? int32{2} : int32{0};
        }
        input[FlatIndex(x, y, z, k_DimX, k_DimY)] = value;
      }
    }
  }

  const std::vector<KernelType> kernels = {KernelType::Box, KernelType::Cross, KernelType::Ball, KernelType::Annulus};
  const std::vector<std::array<int32, 3>> radii = {{1, 1, 1}, {2, 1, 1}, {2, 2, 2}};

  for(KernelType kt : kernels)
  {
    for(const std::array<int32, 3>& radius : radii)
    {
      const std::vector<int32> expected = ObjectMorphologyOracle<int32>(input, k_DimX, k_DimY, k_DimZ, kt, radius, ObjectMorphOp::Erode, objectValue, backgroundValue);

      std::vector<int32> inCore;
      {
        const ForceInCoreAlgorithmGuard guard;
        inCore = RunApplyObjectMorphology<int32>(input, k_DimX, k_DimY, k_DimZ, kt, radius, ObjectMorphOp::Erode, objectValue, backgroundValue);
      }
      std::vector<int32> ooc;
      {
        const ForceOocAlgorithmGuard guard(true);
        ooc = RunApplyObjectMorphology<int32>(input, k_DimX, k_DimY, k_DimZ, kt, radius, ObjectMorphOp::Erode, objectValue, backgroundValue);
      }

      for(usize i = 0; i < input.size(); ++i)
      {
        INFO("kernel=" << KernelName(kt) << " radius={" << radius[0] << "," << radius[1] << "," << radius[2] << "} index=" << i);
        REQUIRE(inCore[i] == expected[i]); // in-core Scatter == independent oracle
        REQUIRE(ooc[i] == inCore[i]);      // OOC Gather == in-core Scatter (byte-for-byte D3 gate)
      }
    }
  }
}

TEST_CASE("NXImageProcessing::ObjectMorphologyEngine: solid all-object block is unchanged (no boundary voxels)", "[NXImageProcessing][ObjectMorphologyEngine]")
{
  // A fully-object image has NO boundary object pixels: every voxel's in-bounds neighbors are all object, and
  // out-of-bounds neighbors are ignored. So nothing paints and Dilate leaves the image unchanged. Exercises
  // the boundary test's negative case + the OOB-neighbor-ignored rule at the image edge, on both paths.
  constexpr usize k_D = 5;
  const std::vector<int32> input(k_D * k_D * k_D, 7);

  auto check = [&](const char* pathName, const std::vector<int32>& out) {
    for(usize i = 0; i < out.size(); ++i)
    {
      INFO(pathName << " index=" << i);
      REQUIRE(out[i] == input[i]);
    }
  };

  {
    const ForceInCoreAlgorithmGuard guard;
    check("in-core", RunApplyObjectMorphology<int32>(input, k_D, k_D, k_D, KernelType::Ball, {2, 2, 2}, ObjectMorphOp::Dilate, 7, 0));
  }
  {
    const ForceOocAlgorithmGuard guard(true);
    check("ooc", RunApplyObjectMorphology<int32>(input, k_D, k_D, k_D, KernelType::Ball, {2, 2, 2}, ObjectMorphOp::Dilate, 7, 0));
  }
}
