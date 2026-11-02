#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/SweepTemporaryStore.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <limits>
#include <numeric>
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
class TransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_MaximumReadValues = std::max(m_MaximumReadValues, buffer.size());
    m_ReadCount++;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaximumWriteValues = std::max(m_MaximumWriteValues, buffer.size());
    m_WriteCount++;
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maximumReadValues() const noexcept
  {
    return m_MaximumReadValues;
  }

  usize maximumWriteValues() const noexcept
  {
    return m_MaximumWriteValues;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

private:
  mutable usize m_MaximumReadValues = 0;
  mutable usize m_ReadCount = 0;
  usize m_MaximumWriteValues = 0;
  usize m_WriteCount = 0;
};

template <class T>
class OutOfCoreReportingDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  std::string getDataFormat() const override
  {
    return "Connected-Component-Test-OOC";
  }
};

template <class T>
class CancelAfterFirstReadDataStore : public DataStore<T>
{
public:
  CancelAfterFirstReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    m_ShouldCancel = true;
    return result;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable usize m_ReadCount = 0;
};

template <class T>
class FailOnSecondReadDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    if(m_ReadCount == 2)
    {
      return MakeErrorResult(-8390, "Injected connected-component input read failure.");
    }
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

private:
  mutable usize m_ReadCount = 0;
};

std::vector<uint32> ConnectedComponent2DOracle(const std::vector<int32>& mask, usize dimX, usize dimY, bool fullyConnected)
{
  std::vector<uint32> labels(mask.size(), 0u);
  std::vector<usize> frontier;
  uint32 nextLabel = 0;
  for(usize seed = 0; seed < mask.size(); ++seed)
  {
    if(mask[seed] == 0 || labels[seed] != 0)
    {
      continue;
    }
    ++nextLabel;
    labels[seed] = nextLabel;
    frontier.clear();
    frontier.push_back(seed);
    for(usize head = 0; head < frontier.size(); ++head)
    {
      const usize flatIndex = frontier[head];
      const int64 x = static_cast<int64>(flatIndex % dimX);
      const int64 y = static_cast<int64>(flatIndex / dimX);
      for(int64 offsetY = -1; offsetY <= 1; ++offsetY)
      {
        for(int64 offsetX = -1; offsetX <= 1; ++offsetX)
        {
          if((offsetX == 0 && offsetY == 0) || (!fullyConnected && std::abs(offsetX) + std::abs(offsetY) != 1))
          {
            continue;
          }
          const int64 neighborX = x + offsetX;
          const int64 neighborY = y + offsetY;
          if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
          {
            continue;
          }
          const usize neighborIndex = static_cast<usize>(neighborY) * dimX + static_cast<usize>(neighborX);
          if(mask[neighborIndex] != 0 && labels[neighborIndex] == 0)
          {
            labels[neighborIndex] = nextLabel;
            frontier.push_back(neighborIndex);
          }
        }
      }
    }
  }
  return labels;
}

// Label an int32 mask (foreground = nonzero). Returns the uint32 label image.
std::vector<uint32> RunLabel(const std::vector<int32>& mask, usize dx, usize dy, usize dz, bool fullyConnected)
{
  DataStore<int32> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0);
  for(usize i = 0; i < mask.size(); ++i)
  {
    inStore.setValue(i, mask[i]);
  }
  DataStore<uint32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0u);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const auto pred = [](int32 v) { return v != 0; };
  const Result<> r = LabelConnectedComponents<int32>(inStore, outStore, SizeVec3{dx, dy, dz}, pred, fullyConnected, shouldCancel, messageHandler);
  REQUIRE(r.valid());
  std::vector<uint32> out(mask.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::ConnectedComponentEngine: resident provisional labels require a complete dataset-scaled reservation", "[ImageProcessing][ConnectedComponentEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr usize valueCount = dimX * dimY * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = ImageProcessing::detail::CalculateConnectedComponentResidentProvisionalBytes(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == valueCount * sizeof(uint32));
  REQUIRE(requiredResult.value() == 128 * k_MiB);
  const auto overflowResult = ImageProcessing::detail::CalculateConnectedComponentResidentProvisionalBytes(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  REQUIRE(ImageProcessing::detail::ShouldUseConnectedComponentResidentProvisional(dims));
  REQUIRE_FALSE(ImageProcessing::detail::ShouldUseConnectedComponentResidentProvisional(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(256 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveConnectedComponentResidentProvisional(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 64 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(512 * k_MiB);
  {
    auto allocationResult = ImageProcessing::detail::ReserveConnectedComponentResidentProvisional(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: provisional selection requires a complete resident grant", "[ImageProcessing][ConnectedComponentEngine][WorkingMemory]")
{
  using ImageProcessing::detail::TryCreateConnectedComponentResidentProvisionalSelection;

  constexpr usize k_DimX = 512;
  constexpr usize k_DimY = 512;
  constexpr usize k_DimZ = 128;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{k_DimX, k_DimY, k_DimZ};
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();

  manager.setBudgetBytes(512 * k_MiB);
  {
    auto residentResult = TryCreateConnectedComponentResidentProvisionalSelection(dims, /*usesOutOfCoreEndpoint=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(residentResult);
    REQUIRE(residentResult.value().holdsResidentState());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(256 * k_MiB);
  {
    auto partialResult = TryCreateConnectedComponentResidentProvisionalSelection(dims, /*usesOutOfCoreEndpoint=*/true);
    SIMPLNX_RESULT_REQUIRE_VALID(partialResult);
    REQUIRE_FALSE(partialResult.value().holdsResidentState());
    REQUIRE(partialResult.value().dataStore == nullptr);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: full resident state uses a conservative dataset-scaled reservation", "[ImageProcessing][ConnectedComponentEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 32;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = ImageProcessing::detail::CalculateConnectedComponentResidentWorkingMemoryBytes<uint8>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == 214 * k_MiB + 32);
  const auto overflowResult = ImageProcessing::detail::CalculateConnectedComponentResidentWorkingMemoryBytes<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  // Cache budgets of at most 1 GiB avoid the machine-dependent upper cap.
  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  REQUIRE_FALSE(manager.setBudgetBytes(768 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveConnectedComponentResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 192 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  REQUIRE_FALSE(manager.setBudgetBytes(1024 * k_MiB));
  {
    auto allocationResult = ImageProcessing::detail::ReserveConnectedComponentResidentWorkingMemory<uint8>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: bounded, resident-provisional, and full-resident tiers preserve exact labels", "[ImageProcessing][ConnectedComponentEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint64 k_BoundedBudgetBytes = 2048;
  constexpr uint64 k_ProvisionalBudgetBytes = 4096;
  constexpr uint64 k_FullBudgetBytes = 65536;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<int32> mask(dimX * dimY * dimZ);
  for(usize index = 0; index < mask.size(); ++index)
  {
    mask[index] = ((index * 37 + 5) % 11) < 4 ? 1 : 0;
  }
  const std::vector<uint32> expected = RunLabel(mask, dimX, dimY, dimZ, true);

  DataStore<int32> residentInput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
  OutOfCoreReportingDataStore<uint32> oocOutput(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint32{0});
  REQUIRE(ImageProcessing::detail::SelectConnectedComponentProvisionalDataFormat(residentInput, oocOutput) == oocOutput.getDataFormat());

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();

  auto run = [&](uint64 budgetBytes, bool expectResidentProvisional) {
    manager.clear();
    manager.setBudgetBytes(budgetBytes);
    {
      auto selectionResult = ImageProcessing::detail::CreateConnectedComponentProvisionalSelection(dims, /*usesOutOfCoreEndpoint=*/true, "Unregistered-Connected-Component-Test-Format");
      SIMPLNX_RESULT_REQUIRE_VALID(selectionResult);
      REQUIRE(selectionResult.value().dataStore != nullptr);
      REQUIRE(selectionResult.value().holdsResidentState() == expectResidentProvisional);
      REQUIRE(manager.reservedWorkingMemoryBytes() == (expectResidentProvisional ? mask.size() * sizeof(uint32) : 0));
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

    OutOfCoreReportingDataStore<int32> inputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, int32{0});
    OutOfCoreReportingDataStore<uint32> outputStore(ShapeType{dimZ, dimY, dimX}, ShapeType{1}, uint32{0});
    for(usize index = 0; index < mask.size(); ++index)
    {
      inputStore.setValue(index, mask[index]);
    }

    std::atomic_bool shouldCancel{false};
    IFilter::MessageHandler messageHandler{};
    const auto predicate = [](int32 value) { return value != 0; };
    const Result<> labelResult = LabelConnectedComponents(inputStore, outputStore, dims, predicate, true, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(labelResult);
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    return std::array<usize, 4>{inputStore.readCount(), inputStore.maximumReadValues(), outputStore.writeCount(), outputStore.maximumWriteValues()};
  };

  const auto boundedTransfers = run(k_BoundedBudgetBytes, false);
  REQUIRE(boundedTransfers == std::array<usize, 4>{2 * dimZ, dimX * dimY, dimZ, dimX * dimY});
  const auto provisionalTransfers = run(k_ProvisionalBudgetBytes, true);
  REQUIRE(provisionalTransfers == std::array<usize, 4>{dimZ, dimX * dimY, dimZ, dimX * dimY});
  const auto fullTransfers = run(k_FullBudgetBytes, true);
  REQUIRE(fullTransfers == std::array<usize, 4>{1, mask.size(), 1, mask.size()});
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: two disjoint blobs get labels 1 and 2 (raster order)", "[ImageProcessing][ConnectedComponentEngine]")
{
  const usize dx = 6, dy = 1, dz = 1;
  // mask: 1 1 0 0 1 1  -> runs [0..1]=label1, [4..5]=label2
  std::vector<int32> mask = {1, 1, 0, 0, 1, 1};
  const std::vector<uint32> out = RunLabel(mask, dx, dy, dz, false);
  REQUIRE(out[0] == 1u);
  REQUIRE(out[1] == 1u);
  REQUIRE(out[2] == 0u);
  REQUIRE(out[3] == 0u);
  REQUIRE(out[4] == 2u);
  REQUIRE(out[5] == 2u);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: connectivity distinguishes a diagonal-touching pair", "[ImageProcessing][ConnectedComponentEngine]")
{
  // 2x2: fg at (0,0) and (1,1) touch only diagonally.
  const usize dx = 2, dy = 2, dz = 1;
  std::vector<int32> mask(4, 0);
  mask[FlatIndex(0, 0, 0, dx, dy)] = 1;
  mask[FlatIndex(1, 1, 0, dx, dy)] = 1;
  const std::vector<uint32> face = RunLabel(mask, dx, dy, dz, false); // 4-conn -> 2 labels
  REQUIRE(face[FlatIndex(0, 0, 0, dx, dy)] == 1u);
  REQUIRE(face[FlatIndex(1, 1, 0, dx, dy)] == 2u);
  const std::vector<uint32> full = RunLabel(mask, dx, dy, dz, true); // 8-conn -> 1 label
  REQUIRE(full[FlatIndex(0, 0, 0, dx, dy)] == 1u);
  REQUIRE(full[FlatIndex(1, 1, 0, dx, dy)] == 1u);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: a U-shape is one component (merge across rows)", "[ImageProcessing][ConnectedComponentEngine]")
{
  // 3x3: a U (bottom row full, two side columns) -> single connected component under face connectivity.
  const usize dx = 3, dy = 3, dz = 1;
  std::vector<int32> mask = {1, 0, 1, 1, 0, 1, 1, 1, 1};
  const std::vector<uint32> out = RunLabel(mask, dx, dy, dz, false);
  for(usize i = 0; i < mask.size(); ++i)
  {
    REQUIRE(out[i] == (mask[i] != 0 ? 1u : 0u)); // all fg is one component -> label 1
  }
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: deterministic + tall-Z streaming + count matches", "[ImageProcessing][ConnectedComponentEngine]")
{
  const usize dx = 8, dy = 8, dz = 40;
  std::vector<int32> mask(dx * dy * dz);
  for(usize i = 0; i < mask.size(); ++i)
  {
    mask[i] = ((i * 37) % 5 == 0) ? 1 : 0; // scattered foreground
  }
  const std::vector<uint32> a = RunLabel(mask, dx, dy, dz, false);
  const std::vector<uint32> b = RunLabel(mask, dx, dy, dz, false);
  REQUIRE(a == b);
  // max label == number of components == CountConnectedComponents(minSize 0).
  uint32 maxLabel = 0;
  for(uint32 v : a)
  {
    maxLabel = std::max(maxLabel, v);
  }
  DataStore<int32> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0);
  for(usize i = 0; i < mask.size(); ++i)
  {
    inStore.setValue(i, mask[i]);
  }
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const auto pred = [](int32 v) { return v != 0; };
  const Result<uint32> countResult = CountConnectedComponents<int32>(inStore, SizeVec3{dx, dy, dz}, pred, false, /*minSize=*/1, shouldCancel);
  REQUIRE(countResult.valid());
  REQUIRE(countResult.value() == maxLabel);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: grouped three-dimensional replay preserves labels and transfer bounds", "[ImageProcessing][ConnectedComponentEngine][WorkingMemory]")
{
  constexpr usize k_DimX = 6;
  constexpr usize k_DimY = 4;
  constexpr usize k_DimZ = 40;
  constexpr usize k_ExpectedGroupedTransfers = 20;
  constexpr usize k_VolumeValues = k_DimX * k_DimY * k_DimZ;
  const uint64 tuningBytes = GENERATE(uint64{1}, uint64{1024 * 1024});
  CAPTURE(tuningBytes);

  TransferCountingDataStore<int32> inputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0);
  TransferCountingDataStore<uint32> outputStore(ShapeType{k_DimZ, k_DimY, k_DimX}, ShapeType{1}, 0u);
  std::vector<uint32> expected(k_VolumeValues, 0u);
  std::vector<uint32> finalLabel(2 * k_DimY * k_DimZ + 1, 0u);
  std::iota(finalLabel.begin(), finalLabel.end(), uint32{100});
  finalLabel[0] = 0u;

  usize provisionalLabel = 0;
  for(usize z = 0; z < k_DimZ; ++z)
  {
    for(usize y = 0; y < k_DimY; ++y)
    {
      ++provisionalLabel;
      for(usize x = 1; x <= 2; ++x)
      {
        const usize index = FlatIndex(x, y, z, k_DimX, k_DimY);
        inputStore.setValue(index, 1);
        expected[index] = finalLabel[provisionalLabel];
      }
      ++provisionalLabel;
      const usize index = FlatIndex(4, y, z, k_DimX, k_DimY);
      inputStore.setValue(index, 1);
      expected[index] = finalLabel[provisionalLabel];
    }
  }

  std::atomic_bool shouldCancel{false};
  const auto predicate = [](int32 value) { return value != 0; };
  ScopedWorkingMemoryTuningOverride tuningOverride(tuningBytes);
  const Result<> result = ImageProcessing::detail::ReplayThreeDimensionalLabels(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, k_DimZ}, predicate, finalLabel, shouldCancel);
  REQUIRE(result.valid());

  std::vector<uint32> actual(k_VolumeValues);
  REQUIRE(outputStore.copyIntoBuffer(0, nonstd::span<uint32>(actual.data(), actual.size())).valid());
  REQUIRE(actual == expected);
  const usize expectedTransfers = tuningBytes == 1 ? k_DimZ : k_ExpectedGroupedTransfers;
  REQUIRE(inputStore.readCount() == expectedTransfers);
  REQUIRE(outputStore.writeCount() == expectedTransfers);
  REQUIRE(inputStore.maximumReadValues() == k_VolumeValues / expectedTransfers);
  REQUIRE(outputStore.maximumWriteValues() == k_VolumeValues / expectedTransfers);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: a bar spanning z-planes is one component (cross-plane union)", "[ImageProcessing][ConnectedComponentEngine]")
{
  const usize dx = 3, dy = 3, dz = 3;
  std::vector<int32> mask(dx * dy * dz, 0);
  for(usize z = 0; z < dz; ++z) // a vertical bar at (1,1,z) -> face-connected along z
  {
    mask[FlatIndex(1, 1, z, dx, dy)] = 1;
  }
  const std::vector<uint32> out = RunLabel(mask, dx, dy, dz, false);
  for(usize z = 0; z < dz; ++z)
  {
    REQUIRE(out[FlatIndex(1, 1, z, dx, dy)] == 1u); // one component across all 3 planes
  }
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: 3D corner-diagonal pair (full connectivity crosses planes)", "[ImageProcessing][ConnectedComponentEngine]")
{
  const usize dx = 2, dy = 2, dz = 2;
  std::vector<int32> mask(8, 0);
  mask[FlatIndex(0, 0, 0, dx, dy)] = 1;
  mask[FlatIndex(1, 1, 1, dx, dy)] = 1;                               // touch only via the 3D corner diagonal
  const std::vector<uint32> face = RunLabel(mask, dx, dy, dz, false); // 6-conn -> 2 labels
  REQUIRE(face[FlatIndex(0, 0, 0, dx, dy)] == 1u);
  REQUIRE(face[FlatIndex(1, 1, 1, dx, dy)] == 2u);
  const std::vector<uint32> full = RunLabel(mask, dx, dy, dz, true); // 26-conn -> 1 label (via prevPlane diagonal, offset 1)
  REQUIRE(full[FlatIndex(0, 0, 0, dx, dy)] == 1u);
  REQUIRE(full[FlatIndex(1, 1, 1, dx, dy)] == 1u);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: degenerate masks (all-bg, all-fg, single voxel)", "[ImageProcessing][ConnectedComponentEngine]")
{
  SECTION("all background -> all zero")
  {
    const std::vector<int32> mask(4 * 4 * 4, 0);
    const std::vector<uint32> out = RunLabel(mask, 4, 4, 4, false);
    for(uint32 v : out)
    {
      REQUIRE(v == 0u);
    }
  }
  SECTION("all foreground -> single component")
  {
    const std::vector<int32> mask(4 * 4 * 4, 1);
    const std::vector<uint32> out = RunLabel(mask, 4, 4, 4, false);
    for(uint32 v : out)
    {
      REQUIRE(v == 1u);
    }
  }
  SECTION("single foreground voxel -> label 1")
  {
    std::vector<int32> mask(3 * 3 * 3, 0);
    mask[FlatIndex(1, 1, 1, 3, 3)] = 1;
    const std::vector<uint32> out = RunLabel(mask, 3, 3, 3, false);
    REQUIRE(out[FlatIndex(1, 1, 1, 3, 3)] == 1u);
  }
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: CountConnectedComponents honors minSize", "[ImageProcessing][ConnectedComponentEngine]")
{
  // dx=20: one 10-voxel run [0..9], then five isolated single-voxel components at 11,13,15,17,19.
  const usize dx = 20, dy = 1, dz = 1;
  std::vector<int32> mask(dx, 0);
  for(usize x = 0; x < 10; ++x)
  {
    mask[x] = 1;
  }
  mask[11] = 1;
  mask[13] = 1;
  mask[15] = 1;
  mask[17] = 1;
  mask[19] = 1;

  DataStore<int32> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0);
  for(usize i = 0; i < mask.size(); ++i)
  {
    inStore.setValue(i, mask[i]);
  }
  std::atomic_bool shouldCancel{false};
  const auto pred = [](int32 v) { return v != 0; };

  const Result<uint32> allResult = CountConnectedComponents<int32>(inStore, SizeVec3{dx, dy, dz}, pred, false, /*minSize=*/1, shouldCancel);
  REQUIRE(allResult.valid());
  REQUIRE(allResult.value() == 6u); // 1 large + 5 singles

  const Result<uint32> largeOnlyResult = CountConnectedComponents<int32>(inStore, SizeVec3{dx, dy, dz}, pred, false, /*minSize=*/2, shouldCancel);
  REQUIRE(largeOnlyResult.valid());
  REQUIRE(largeOnlyResult.value() == 1u); // only the 10-voxel run meets minSize=2
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: rejects mismatched store sizes", "[ImageProcessing][ConnectedComponentEngine]")
{
  DataStore<int32> inStore(ShapeType{2, 2, 2}, ShapeType{1}, 1);
  DataStore<uint32> outStore(ShapeType{2, 2, 2}, ShapeType{1}, 0u);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  const auto pred = [](int32 value) { return value != 0; };

  const Result<> labelResult = LabelConnectedComponents<int32>(inStore, outStore, SizeVec3{3, 2, 2}, pred, false, shouldCancel, messageHandler);
  REQUIRE(labelResult.invalid());
  REQUIRE(labelResult.errors()[0].code == -8372);

  const Result<uint32> countResult = CountConnectedComponents<int32>(inStore, SizeVec3{3, 2, 2}, pred, false, 1, shouldCancel);
  REQUIRE(countResult.invalid());
  REQUIRE(countResult.errors()[0].code == -8372);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: checked true-2-D plan", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::CcLine;
  using ImageProcessing::detail::CcRun;
  using ImageProcessing::detail::CreateConnectedComponent2DPlan;

  constexpr usize k_DimX = 7;
  constexpr usize k_DimY = 11;
  constexpr usize k_MaximumRuns = (k_DimX + 1) / 2;
  const usize labelFixedBytes =
      ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int16>) + sizeof(std::vector<uint32>) + 2 * sizeof(CcLine) + 2 * k_MaximumRuns * sizeof(CcRun);
  const usize countFixedBytes = ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int16>) + 2 * sizeof(CcLine) + 2 * k_MaximumRuns * sizeof(CcRun);
  const usize labelTargetBytes = labelFixedBytes + 3 * k_DimX * (sizeof(int16) + sizeof(uint32));
  const usize countTargetBytes = countFixedBytes + 3 * k_DimX * sizeof(int16);

  const auto labelResult = CreateConnectedComponent2DPlan<int16>(k_DimX, k_DimY, /*writesProvisional=*/true, labelTargetBytes);
  REQUIRE(labelResult.valid());
  REQUIRE_FALSE(labelResult.value().useTiles);
  REQUIRE(labelResult.value().coreRows == 3);
  REQUIRE(labelResult.value().coreColumns == k_DimX);
  REQUIRE(labelResult.value().maximumRunsPerLine == k_MaximumRuns);
  REQUIRE(labelResult.value().residentBytes == labelTargetBytes);

  const auto countResult = CreateConnectedComponent2DPlan<int16>(k_DimX, k_DimY, /*writesProvisional=*/false, countTargetBytes);
  REQUIRE(countResult.valid());
  REQUIRE_FALSE(countResult.value().useTiles);
  REQUIRE(countResult.value().coreRows == 3);
  REQUIRE(countResult.value().residentBytes == countTargetBytes);

  constexpr usize k_TileDimX = 31;
  const usize tileFixedBytes = ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int32>) + 2 * sizeof(std::vector<uint32>) + 2 * sizeof(uint32);
  const usize tileTargetBytes = tileFixedBytes + 5 * (sizeof(int32) + 2 * sizeof(uint32));
  const auto tileResult = CreateConnectedComponent2DPlan<int32>(k_TileDimX, /*dimY=*/4, /*writesProvisional=*/true, tileTargetBytes);
  REQUIRE(tileResult.valid());
  REQUIRE(tileResult.value().useTiles);
  REQUIRE(tileResult.value().coreRows == 1);
  REQUIRE(tileResult.value().coreColumns == 5);
  REQUIRE(tileResult.value().residentBytes == tileTargetBytes);

  const auto zeroWidthResult = CreateConnectedComponent2DPlan<int32>(/*dimX=*/0, /*dimY=*/4, true, tileTargetBytes);
  REQUIRE(zeroWidthResult.invalid());
  const auto zeroHeightResult = CreateConnectedComponent2DPlan<int32>(k_TileDimX, /*dimY=*/0, true, tileTargetBytes);
  REQUIRE(zeroHeightResult.invalid());
  const usize oneColumnTileBytes = tileFixedBytes + sizeof(int32) + 2 * sizeof(uint32);
  const auto insufficientResult = CreateConnectedComponent2DPlan<int32>(k_TileDimX, /*dimY=*/4, true, oneColumnTileBytes - 1);
  REQUIRE(insufficientResult.invalid());
  const auto overflowResult = CreateConnectedComponent2DPlan<int32>(std::numeric_limits<usize>::max(), /*dimY=*/4, true, std::numeric_limits<usize>::max());
  REQUIRE(overflowResult.invalid());
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: provisional adapter supports abstract and fixed-record stores", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::ConnectedComponentProvisionalStore;
  using ImageProcessing::detail::SweepTemporaryStore;

  std::atomic_bool shouldCancel{false};
  const std::array<uint32, 8> expected = {3, 1, 4, 1, 5, 9, 2, 6};
  std::array<uint32, 8> actual = {};

  DataStore<uint32> dataStore(ShapeType{8}, ShapeType{1}, 0u);
  ConnectedComponentProvisionalStore dataAdapter(dataStore);
  REQUIRE(dataAdapter.copyFromBuffer(0, nonstd::span<const uint32>(expected.data(), expected.size())).valid());
  REQUIRE(dataAdapter.copyIntoBuffer(0, nonstd::span<uint32>(actual.data(), actual.size())).valid());
  REQUIRE(actual == expected);

  TemporaryRecordStoreConfig config;
  config.recordSize = sizeof(uint32);
  config.maxRecordsPerBatch = 3;
  config.initialRecordCount = expected.size();
  auto recordResult = InMemoryTemporaryRecordStore::Create(config);
  REQUIRE(recordResult.valid());
  SweepTemporaryStore<uint32> fixedRecords(std::move(recordResult.value()), shouldCancel, "Connected-component test");
  ConnectedComponentProvisionalStore fixedAdapter(fixedRecords);
  actual.fill(0u);
  REQUIRE(fixedAdapter.copyFromBuffer(0, nonstd::span<const uint32>(expected.data(), expected.size())).valid());
  REQUIRE(fixedAdapter.copyIntoBuffer(0, nonstd::span<uint32>(actual.data(), actual.size())).valid());
  REQUIRE(actual == expected);
  REQUIRE_FALSE(fixedAdapter.getChunkShape().has_value());
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: true-2-D row blocks align to OOC chunk bands", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::AlignConnectedComponent2DRows;
  using ImageProcessing::detail::CreateConnectedComponent2DFinalChunkValues;

  const std::optional<ShapeType> twoRowChunks = ShapeType{1, 2, 9};
  const std::optional<ShapeType> fourRowChunks = ShapeType{1, 4, 9};
  REQUIRE(AlignConnectedComponent2DRows(/*maximumRows=*/7, /*dimX=*/9, twoRowChunks, fourRowChunks) == 4);
  REQUIRE(AlignConnectedComponent2DRows(/*maximumRows=*/3, /*dimX=*/9, twoRowChunks, std::nullopt) == 2);
  REQUIRE(AlignConnectedComponent2DRows(/*maximumRows=*/1, /*dimX=*/9, twoRowChunks, fourRowChunks) == 1);
  REQUIRE(AlignConnectedComponent2DRows(/*maximumRows=*/7, /*dimX=*/9, ShapeType{1, 2, 8}, fourRowChunks) == 4);
  REQUIRE(AlignConnectedComponent2DRows(/*maximumRows=*/7, /*dimX=*/9, std::nullopt, std::nullopt) == 7);
  const usize maximumRows = std::numeric_limits<usize>::max();
  const ShapeType overflowingFirstChunk = ShapeType{1, maximumRows / 2 + 1, 9};
  const ShapeType overflowingSecondChunk = ShapeType{1, 3, 9};
  REQUIRE(AlignConnectedComponent2DRows(maximumRows, /*dimX=*/9, overflowingFirstChunk, overflowingSecondChunk) == maximumRows);

  const usize finalTargetBytes = ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<uint32>) + 7 * 9 * sizeof(uint32);
  const auto finalChunkResult = CreateConnectedComponent2DFinalChunkValues(/*totalValues=*/9 * 11, /*dimX=*/9, finalTargetBytes, twoRowChunks, fourRowChunks);
  REQUIRE(finalChunkResult.valid());
  REQUIRE(finalChunkResult.value() == 4 * 9);
  const auto insufficientResult = CreateConnectedComponent2DFinalChunkValues(
      /*totalValues=*/9 * 11, /*dimX=*/9, ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<uint32>) + sizeof(uint32) - 1, twoRowChunks, fourRowChunks);
  REQUIRE(insufficientResult.invalid());
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: bounded true-2-D blocks preserve connectivity and transfer caps", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::CcLine;
  using ImageProcessing::detail::CcRun;
  using ImageProcessing::detail::CreateConnectedComponent2DPlan;

  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  std::vector<int32> mask(k_DimX * k_DimY, 0);
  for(usize y = 0; y < k_DimY; ++y)
  {
    mask[y * k_DimX + 7] = 1;
  }
  mask[0 * k_DimX + 1] = 1;
  mask[1 * k_DimX + 1] = 1;
  mask[2 * k_DimX + 1] = 1;
  mask[3 * k_DimX + 2] = 1;
  mask[4 * k_DimX + 3] = 1;
  mask[5 * k_DimX + 3] = 1;
  mask[6 * k_DimX + 3] = 1;
  mask[5 * k_DimX + 5] = 1;

  constexpr usize k_MaximumRuns = (k_DimX + 1) / 2;
  const usize labelFixedBytes =
      ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int32>) + sizeof(std::vector<uint32>) + 2 * sizeof(CcLine) + 2 * k_MaximumRuns * sizeof(CcRun);
  const usize labelTargetBytes = labelFixedBytes + 2 * k_DimX * (sizeof(int32) + sizeof(uint32));
  const auto labelPlanResult = CreateConnectedComponent2DPlan<int32>(k_DimX, k_DimY, /*writesProvisional=*/true, labelTargetBytes);
  REQUIRE(labelPlanResult.valid());
  REQUIRE_FALSE(labelPlanResult.value().useTiles);
  REQUIRE(labelPlanResult.value().coreRows == 2);
  const auto labelPass1PlanResult = CreateConnectedComponent2DPlan<int32>(k_DimX, k_DimY, /*writesProvisional=*/false, labelTargetBytes);
  REQUIRE(labelPass1PlanResult.valid());
  REQUIRE_FALSE(labelPass1PlanResult.value().useTiles);
  REQUIRE(labelPass1PlanResult.value().coreRows == 4);

  const usize countFixedBytes = ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int32>) + 2 * sizeof(CcLine) + 2 * k_MaximumRuns * sizeof(CcRun);
  const usize countTargetBytes = countFixedBytes + 2 * k_DimX * sizeof(int32);
  const auto countPlanResult = CreateConnectedComponent2DPlan<int32>(k_DimX, k_DimY, /*writesProvisional=*/false, countTargetBytes);
  REQUIRE(countPlanResult.valid());
  REQUIRE(countPlanResult.value().coreRows == 2);

  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  OutOfCoreReportingDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0);
  for(usize index = 0; index < mask.size(); ++index)
  {
    inputStore.setValue(index, mask[index]);
  }
  OutOfCoreReportingDataStore<uint32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0u);
  std::atomic_bool shouldCancel{false};
  const auto predicate = [](int32 value) { return value != 0; };
  const Result<> labelResult = LabelConnectedComponents<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, fullyConnected, shouldCancel, {}, labelTargetBytes);
  REQUIRE(labelResult.valid());

  std::vector<uint32> actual(mask.size());
  const Result<> readResult = outputStore.copyIntoBuffer(0, nonstd::span<uint32>(actual.data(), actual.size()));
  REQUIRE(readResult.valid());
  REQUIRE(actual == ConnectedComponent2DOracle(mask, k_DimX, k_DimY, fullyConnected));
  REQUIRE(inputStore.maximumReadValues() <= std::max(labelPlanResult.value().coreRows, labelPass1PlanResult.value().coreRows) * k_DimX);
  REQUIRE(inputStore.readCount() ==
          (k_DimY + labelPass1PlanResult.value().coreRows - 1) / labelPass1PlanResult.value().coreRows + (k_DimY + labelPlanResult.value().coreRows - 1) / labelPlanResult.value().coreRows);
  const auto finalChunkResult = ImageProcessing::detail::CreateConnectedComponent2DFinalChunkValues(mask.size(), k_DimX, labelTargetBytes, std::nullopt, std::nullopt);
  REQUIRE(finalChunkResult.valid());
  REQUIRE(outputStore.maximumWriteValues() <= finalChunkResult.value());

  TransferCountingDataStore<int32> countInputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0);
  for(usize index = 0; index < mask.size(); ++index)
  {
    countInputStore.setValue(index, mask[index]);
  }
  const Result<uint32> countResult = CountConnectedComponents<int32>(countInputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, fullyConnected, /*minSize=*/1, shouldCancel, countTargetBytes);
  REQUIRE(countResult.valid());
  const std::vector<uint32> expected = ConnectedComponent2DOracle(mask, k_DimX, k_DimY, fullyConnected);
  REQUIRE(countResult.value() == *std::max_element(expected.cbegin(), expected.cend()));
  REQUIRE(countInputStore.maximumReadValues() <= countPlanResult.value().coreRows * k_DimX);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: bounded true-2-D tiles preserve run and neighbor continuity", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::CreateConnectedComponent2DPlan;

  constexpr usize k_DimX = 13;
  constexpr usize k_DimY = 5;
  constexpr usize k_CoreColumns = 3;
  std::vector<int32> mask(k_DimX * k_DimY, 0);
  for(usize x = 1; x <= 6; ++x)
  {
    mask[0 * k_DimX + x] = 1;
  }
  mask[1 * k_DimX + 6] = 1;
  mask[2 * k_DimX + 7] = 1;
  mask[3 * k_DimX + 7] = 1;
  mask[4 * k_DimX + 7] = 1;
  mask[1 * k_DimX + 10] = 1;
  mask[2 * k_DimX + 10] = 1;
  mask[4 * k_DimX + 11] = 1;

  const usize tileFixedBytes = ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int32>) + 2 * sizeof(std::vector<uint32>) + 2 * sizeof(uint32);
  const usize targetBytes = tileFixedBytes + k_CoreColumns * (sizeof(int32) + 2 * sizeof(uint32));
  const auto planResult = CreateConnectedComponent2DPlan<int32>(k_DimX, k_DimY, /*writesProvisional=*/true, targetBytes);
  REQUIRE(planResult.valid());
  REQUIRE(planResult.value().useTiles);
  REQUIRE(planResult.value().coreColumns == k_CoreColumns);

  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  OutOfCoreReportingDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0);
  for(usize index = 0; index < mask.size(); ++index)
  {
    inputStore.setValue(index, mask[index]);
  }
  OutOfCoreReportingDataStore<uint32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0u);
  std::atomic_bool shouldCancel{false};
  const auto predicate = [](int32 value) { return value != 0; };
  const Result<> labelResult = LabelConnectedComponents<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, fullyConnected, shouldCancel, {}, targetBytes);
  REQUIRE(labelResult.valid());

  std::vector<uint32> actual(mask.size());
  const Result<> readResult = outputStore.copyIntoBuffer(0, nonstd::span<uint32>(actual.data(), actual.size()));
  REQUIRE(readResult.valid());
  const std::vector<uint32> expected = ConnectedComponent2DOracle(mask, k_DimX, k_DimY, fullyConnected);
  REQUIRE(actual == expected);
  REQUIRE(inputStore.maximumReadValues() <= k_CoreColumns);
  REQUIRE(inputStore.readCount() == 2 * k_DimY * ((k_DimX + k_CoreColumns - 1) / k_CoreColumns));

  TransferCountingDataStore<int32> countInputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 0);
  for(usize index = 0; index < mask.size(); ++index)
  {
    countInputStore.setValue(index, mask[index]);
  }
  const Result<uint32> countResult = CountConnectedComponents<int32>(countInputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, fullyConnected, /*minSize=*/1, shouldCancel, targetBytes);
  REQUIRE(countResult.valid());
  REQUIRE(countResult.value() == *std::max_element(expected.cbegin(), expected.cend()));
  REQUIRE(countInputStore.maximumReadValues() <= k_CoreColumns);
}

TEST_CASE("ImageProcessing::ConnectedComponentEngine: bounded true-2-D cancellation and input errors stop before final output", "[ImageProcessing][ConnectedComponentEngine]")
{
  using ImageProcessing::detail::CcLine;
  using ImageProcessing::detail::CcRun;

  constexpr usize k_DimX = 9;
  constexpr usize k_DimY = 7;
  constexpr uint32 k_Poison = 0xA5A5A5A5u;
  constexpr usize k_MaximumRuns = (k_DimX + 1) / 2;
  const usize fixedBytes =
      ImageProcessing::detail::k_ConnectedComponent2DMetadataHeadroomBytes + sizeof(std::vector<int32>) + sizeof(std::vector<uint32>) + 2 * sizeof(CcLine) + 2 * k_MaximumRuns * sizeof(CcRun);
  const usize targetBytes = fixedBytes + 2 * k_DimX * (sizeof(int32) + sizeof(uint32));
  const auto predicate = [](int32 value) { return value != 0; };

  SECTION("cancellation after the first block read")
  {
    std::atomic_bool shouldCancel{false};
    CancelAfterFirstReadDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 1, shouldCancel);
    DataStore<uint32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = LabelConnectedComponents<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, false, shouldCancel, {}, targetBytes);

    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 1);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("the second block read fails")
  {
    std::atomic_bool shouldCancel{false};
    FailOnSecondReadDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, 1);
    DataStore<uint32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = LabelConnectedComponents<int32>(inputStore, outputStore, SizeVec3{k_DimX, k_DimY, 1}, predicate, false, shouldCancel, {}, targetBytes);

    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -8390);
    REQUIRE(inputStore.readCount() == 2);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }
}
