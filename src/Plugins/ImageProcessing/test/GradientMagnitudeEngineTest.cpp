#include "simplnx/Utilities/ImageProcessing/GradientMagnitudeEngine.hpp"

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
namespace gradient_detail = nx::core::ImageProcessing::detail;

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

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

private:
  mutable usize m_MaxReadValues = 0;
  mutable usize m_ReadCount = 0;
  usize m_MaxWriteValues = 0;
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
class CancelAfterReadDataStore : public TransferCountingDataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel)
  : TransferCountingDataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = TransferCountingDataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
};

template <class T>
class FailingReadDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(-9750, "Injected gradient-magnitude input read failure.");
  }
};

template <class T>
class FailingWriteDataStore : public TransferCountingDataStore<T>
{
public:
  using TransferCountingDataStore<T>::TransferCountingDataStore;

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(-9751, "Injected gradient-magnitude output write failure.");
  }
};

template <class T>
std::vector<float32> GradientMagnitude2DOracle(const std::vector<T>& field, usize dimX, usize dimY, bool useSpacing, FloatVec3 spacing)
{
  const double invSpacingX = useSpacing ? 1.0 / static_cast<double>(spacing[0]) : 1.0;
  const double invSpacingY = useSpacing ? 1.0 / static_cast<double>(spacing[1]) : 1.0;
  std::vector<float32> output(field.size(), 0.0f);
  for(usize y = 0; y < dimY; ++y)
  {
    const usize previousY = y == 0 ? 0 : y - 1;
    const usize nextY = y + 1 == dimY ? y : y + 1;
    for(usize x = 0; x < dimX; ++x)
    {
      const usize previousX = x == 0 ? 0 : x - 1;
      const usize nextX = x + 1 == dimX ? x : x + 1;
      const double gradientX = 0.5 * (static_cast<double>(field[y * dimX + previousX]) - static_cast<double>(field[y * dimX + nextX])) * invSpacingX;
      const double gradientY = 0.5 * (static_cast<double>(field[previousY * dimX + x]) - static_cast<double>(field[nextY * dimX + x])) * invSpacingY;
      output[y * dimX + x] = static_cast<float32>(std::sqrt(gradientX * gradientX + gradientY * gradientY));
    }
  }
  return output;
}

template <class T>
std::vector<float32> RunGradMag(const std::vector<T>& field, usize dx, usize dy, usize dz, bool useSpacing, FloatVec3 spacing, UnitTest::AlgorithmTestScope* algorithmTestScope = nullptr)
{
  DataStore<T> inStore(ShapeType{dz, dy, dx}, ShapeType{1}, static_cast<T>(0));
  for(usize i = 0; i < field.size(); ++i)
  {
    inStore.setValue(i, field[i]);
  }
  DataStore<float32> outStore(ShapeType{dz, dy, dx}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  auto executeEngine = [&]() { return ApplyGradientMagnitude<T>(inStore, outStore, SizeVec3{dx, dy, dz}, useSpacing, spacing, shouldCancel, messageHandler); };
  const Result<> r = algorithmTestScope == nullptr ? executeEngine() : algorithmTestScope->execute(executeEngine);
  REQUIRE(r.valid());
  std::vector<float32> out(field.size());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = outStore.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: gradient of an X ramp is the slope", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const usize dx = 10, dy = 6, dz = 6;
  std::vector<float32> field(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float32>(3 * x); // slope 3 along X
      }
    }
  }
  const std::vector<float32> out = RunGradMag<float32>(field, dx, dy, dz, false, {1.0f, 1.0f, 1.0f});
  // Interior: central diff of a slope-3 ramp = |0.5*((3(x-1))-(3(x+1)))| = 3. (Boundaries differ under edge-clamp.)
  for(usize z = 1; z < dz - 1; ++z)
  {
    for(usize y = 1; y < dy - 1; ++y)
    {
      for(usize x = 1; x < dx - 1; ++x)
      {
        REQUIRE(out[FlatIndex(x, y, z, dx, dy)] == Approx(3.0f).margin(1e-4));
      }
    }
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: gradient of a constant image is zero", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const usize d = 8;
  const std::vector<int16> field(d * d * d, int16{42});
  const std::vector<float32> out = RunGradMag<int16>(field, d, d, d, false, {1.0f, 1.0f, 1.0f});
  for(float32 v : out)
  {
    REQUIRE(v == Approx(0.0f).margin(1e-6));
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: UseImageSpacing scales the gradient", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const usize dx = 10, dy = 6, dz = 6;
  std::vector<float32> field(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float32>(x);
  const std::vector<float32> out = RunGradMag<float32>(field, dx, dy, dz, true, {2.0f, 1.0f, 1.0f}); // spacing 2 along X
  for(usize z = 1; z < dz - 1; ++z)
    for(usize y = 1; y < dy - 1; ++y)
      for(usize x = 1; x < dx - 1; ++x)
        REQUIRE(out[FlatIndex(x, y, z, dx, dy)] == Approx(0.5f).margin(1e-4)); // slope 1 / spacing 2 = 0.5
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: deterministic + tall-Z streaming", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const usize dx = 8, dy = 8, dz = 64;
  std::vector<float32> field(dx * dy * dz);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<float32>((i * 31) % 97);
  }
  const std::vector<float32> a = RunGradMag<float32>(field, dx, dy, dz, false, {1.0f, 1.0f, 1.0f});
  const std::vector<float32> b = RunGradMag<float32>(field, dx, dy, dz, false, {1.0f, 1.0f, 1.0f});
  REQUIRE(a == b);
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: 2D X ramp — interior slope and edge-clamped boundary", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const usize dx = 10, dy = 6, dz = 1;
  std::vector<float32> field(dx * dy * dz);
  for(usize y = 0; y < dy; ++y)
    for(usize x = 0; x < dx; ++x)
      field[FlatIndex(x, y, 0, dx, dy)] = static_cast<float32>(3 * x); // slope 3 along X, constant in Y
  const std::vector<float32> out = RunGradMag<float32>(field, dx, dy, dz, false, {1.0f, 1.0f, 1.0f}, &algorithmTestScope);
  // Interior: gx=3, gy=0 -> magnitude 3.
  for(usize y = 1; y < dy - 1; ++y)
    for(usize x = 1; x < dx - 1; ++x)
      REQUIRE(out[FlatIndex(x, y, 0, dx, dy)] == Approx(3.0f).margin(1e-4));
  // ZeroFluxNeumann X boundaries: at x=0, gx=0.5*(f(0)-f(1))=0.5*(0-3)=-1.5 -> |.|=1.5; same at x=dx-1.
  for(usize y = 0; y < dy; ++y)
  {
    REQUIRE(out[FlatIndex(0, y, 0, dx, dy)] == Approx(1.5f).margin(1e-4));
    REQUIRE(out[FlatIndex(dx - 1, y, 0, dx, dy)] == Approx(1.5f).margin(1e-4));
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: nZ==2 Z ramp with edge-clamped rolling window", "[ImageProcessing][GradientMagnitudeEngine]")
{
  const usize dx = 5, dy = 5, dz = 2;
  std::vector<float32> field(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
    for(usize y = 0; y < dy; ++y)
      for(usize x = 0; x < dx; ++x)
        field[FlatIndex(x, y, z, dx, dy)] = static_cast<float32>(3 * z); // slope 3 along Z, constant in X/Y
  const std::vector<float32> out = RunGradMag<float32>(field, dx, dy, dz, false, {1.0f, 1.0f, 1.0f});
  // Only 2 Z-planes: every voxel's z-neighbors clamp to {0,1}: gz=0.5*(f_z0-f_z1)=0.5*(0-3)=-1.5 -> 1.5.
  // gx=gy=0 (constant in X/Y) -> magnitude 1.5 everywhere (validates nZ==2 window init + z edge-clamp + effDim==3).
  for(float32 v : out)
    REQUIRE(v == Approx(1.5f).margin(1e-4));
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: validates stores and cancellation", "[ImageProcessing][GradientMagnitudeEngine]")
{
  IFilter::MessageHandler messageHandler{};
  std::atomic_bool shouldCancel{false};

  SECTION("rejects zero dimensions")
  {
    DataStore<float32> inputStore(ShapeType{1}, ShapeType{1}, 0.0f);
    DataStore<float32> outputStore(ShapeType{1}, ShapeType{1}, 0.0f);
    const Result<> result = ApplyGradientMagnitude<float32>(inputStore, outputStore, SizeVec3{0, 1, 1}, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8356);
  }

  SECTION("rejects mismatched input and output sizes")
  {
    DataStore<float32> shortStore(ShapeType{63}, ShapeType{1}, 0.0f);
    DataStore<float32> fullStore(ShapeType{64}, ShapeType{1}, 0.0f);
    Result<> result = ApplyGradientMagnitude<float32>(shortStore, fullStore, SizeVec3{4, 4, 4}, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8358);

    result = ApplyGradientMagnitude<float32>(fullStore, shortStore, SizeVec3{4, 4, 4}, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8359);
  }

  SECTION("rejects degenerate active-axis spacing")
  {
    DataStore<float32> inputStore(ShapeType{4, 4, 4}, ShapeType{1}, 1.0f);
    DataStore<float32> outputStore(ShapeType{4, 4, 4}, ShapeType{1}, 0.0f);
    const Result<> result = ApplyGradientMagnitude<float32>(inputStore, outputStore, SizeVec3{4, 4, 4}, true, {1.0f, 0.0f, 1.0f}, shouldCancel, messageHandler);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_GradientMagnitudeZeroSpacing);
  }

  SECTION("pre-cancel preserves output")
  {
    constexpr float32 k_Poison = -12345.0f;
    DataStore<float32> inputStore(ShapeType{4, 4, 4}, ShapeType{1}, 1.0f);
    DataStore<float32> outputStore(ShapeType{4, 4, 4}, ShapeType{1}, k_Poison);
    shouldCancel = true;
    const Result<> result = ApplyGradientMagnitude<float32>(inputStore, outputStore, SizeVec3{4, 4, 4}, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    REQUIRE(result.valid());
    for(const float32 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: checked true-2-D plans stay bounded", "[ImageProcessing][GradientMagnitudeEngine]")
{
  constexpr usize k_TargetBytes = 1024ULL * 1024ULL;

  SECTION("full-width row blocks")
  {
    const SizeVec3 dims = {4097, 61, 1};
    const auto planResult = gradient_detail::CreateGradientMagnitude2DPlan<float32>(dims, k_TargetBytes);
    REQUIRE(planResult.valid());
    const auto& plan = planResult.value();
    REQUIRE(plan.route == gradient_detail::GradientMagnitude2DRoute::Rows);
    REQUIRE(plan.blockRows > 0);
    REQUIRE(plan.blockRows < dims[1]);
    REQUIRE(plan.tileColumns == dims[0]);
    REQUIRE(plan.inputValues < dims[0] * dims[1]);
    REQUIRE(plan.residentBytes <= k_TargetBytes);
  }

  SECTION("overwide X tiles")
  {
    const SizeVec3 dims = {100000, 3, 1};
    const auto planResult = gradient_detail::CreateGradientMagnitude2DPlan<float64>(dims, k_TargetBytes);
    REQUIRE(planResult.valid());
    const auto& plan = planResult.value();
    REQUIRE(plan.route == gradient_detail::GradientMagnitude2DRoute::Tiles);
    REQUIRE(plan.blockRows == 1);
    REQUIRE(plan.tileColumns > 0);
    REQUIRE(plan.tileColumns < dims[0]);
    REQUIRE(plan.inputValues < dims[0] * dims[1]);
    REQUIRE(plan.residentBytes <= k_TargetBytes);
  }

  SECTION("invalid plans report distinct errors")
  {
    const auto zeroPlan = gradient_detail::CreateGradientMagnitude2DPlan<float32>(SizeVec3{0, 4, 1}, k_TargetBytes);
    REQUIRE(zeroPlan.invalid());
    REQUIRE(zeroPlan.errors().front().code == -8754);

    const auto overflowPlan = gradient_detail::CreateGradientMagnitude2DPlan<float64>(SizeVec3{std::numeric_limits<usize>::max(), 4, 1}, k_TargetBytes);
    REQUIRE(overflowPlan.invalid());
    REQUIRE(overflowPlan.errors().front().code == -8755);

    const auto smallTargetPlan = gradient_detail::CreateGradientMagnitude2DPlan<float32>(SizeVec3{4, 4, 1}, gradient_detail::k_GradientMagnitude2DMetadataBytes);
    REQUIRE(smallTargetPlan.invalid());
    REQUIRE(smallTargetPlan.errors().front().code == -8756);
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: resident state requires a complete dataset-scaled reservation", "[ImageProcessing][GradientMagnitudeEngine][WorkingMemory]")
{
  constexpr usize dimX = 512;
  constexpr usize dimY = 512;
  constexpr usize dimZ = 128;
  constexpr usize sliceValues = dimX * dimY;
  constexpr usize valueCount = sliceValues * dimZ;
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 dims{dimX, dimY, dimZ};

  auto requiredResult = gradient_detail::CalculateGradientMagnitudeResidentWorkingMemoryBytes<float32>(dims);
  SIMPLNX_RESULT_REQUIRE_VALID(requiredResult);
  REQUIRE(requiredResult.value() == valueCount * (sizeof(float32) + sizeof(float32)) + sliceValues * (3 * sizeof(float32) + sizeof(float32)));
  REQUIRE(requiredResult.value() == 260 * k_MiB);
  const auto overflowResult = gradient_detail::CalculateGradientMagnitudeResidentWorkingMemoryBytes<float64>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowResult);

  REQUIRE(gradient_detail::ShouldUseGradientMagnitudeResidentState(dims));
  REQUIRE_FALSE(gradient_detail::ShouldUseGradientMagnitudeResidentState(SizeVec3{dimX, dimY, 1}));

  auto& manager = CacheMemoryBudgetManager::instance();
  const uint64 previousBudget = manager.budgetBytes();
  manager.clear();
  manager.setBudgetBytes(1024 * k_MiB);
  {
    auto allocationResult = gradient_detail::ReserveGradientMagnitudeResidentWorkingMemory<float32>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE_FALSE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == 256 * k_MiB);
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);

  manager.setBudgetBytes(2 * 1024 * k_MiB);
  {
    auto allocationResult = gradient_detail::ReserveGradientMagnitudeResidentWorkingMemory<float32>(dims);
    SIMPLNX_RESULT_REQUIRE_VALID(allocationResult);
    REQUIRE(allocationResult.value().holdsCompleteState());
    REQUIRE(allocationResult.value().reservation.sizeBytes() == requiredResult.value());
  }
  REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: real-OOC selector uses resident state only after a complete grant", "[ImageProcessing][GradientMagnitudeEngine][WorkingMemory]")
{
  constexpr usize dimX = 5;
  constexpr usize dimY = 6;
  constexpr usize dimZ = 7;
  constexpr uint64 k_PartialBudgetBytes = 4096;
  constexpr uint64 k_CompleteBudgetBytes = 16384;
  const SizeVec3 dims{dimX, dimY, dimZ};
  std::vector<float32> field(dimX * dimY * dimZ);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<float32>((index * 19 + 3) % 127 - 63);
  }
  const std::vector<float32> expected = RunGradMag<float32>(field, dimX, dimY, dimZ, true, {0.5f, 1.5f, 2.0f});

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
    const Result<> applyResult = ApplyGradientMagnitude(inputStore, outputStore, dims, true, {0.5f, 1.5f, 2.0f}, shouldCancel, messageHandler);
    SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(manager.reservedWorkingMemoryBytes() == 0);
    return std::array<usize, 3>{inputStore.readCount(), inputStore.maxReadValues(), outputStore.maxWriteValues()};
  };

  const auto partialTransfers = run(k_PartialBudgetBytes);
  REQUIRE(partialTransfers[0] > 1);
  REQUIRE(partialTransfers[1] == dimX * dimY);
  REQUIRE(partialTransfers[2] == dimX * dimY);
  const auto completeTransfers = run(k_CompleteBudgetBytes);
  REQUIRE(completeTransfers[0] == 1);
  REQUIRE(completeTransfers[1] == field.size());
  REQUIRE(completeTransfers[2] == field.size());
  manager.setBudgetBytes(previousBudget);
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: bounded true-2-D row and tile routes match an independent oracle", "[ImageProcessing][GradientMagnitudeEngine]")
{
  constexpr usize k_TargetBytes = 70ULL * 1024ULL;
  std::atomic_bool shouldCancel{false};

  SECTION("full-width row blocks")
  {
    constexpr usize k_DimX = 31;
    constexpr usize k_DimY = 257;
    const SizeVec3 dims = {k_DimX, k_DimY, 1};
    std::vector<int16> field(k_DimX * k_DimY);
    TransferCountingDataStore<int16> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int16{});
    for(usize index = 0; index < field.size(); ++index)
    {
      field[index] = static_cast<int16>((index * 13 + 7) % 101 - 50);
      inputStore.setValue(index, field[index]);
    }
    TransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, -12345.0f);

    const auto planResult = gradient_detail::CreateGradientMagnitude2DPlan<int16>(dims, k_TargetBytes);
    REQUIRE(planResult.valid());
    const auto& plan = planResult.value();
    REQUIRE(plan.route == gradient_detail::GradientMagnitude2DRoute::Rows);
    REQUIRE(plan.blockRows < k_DimY);

    const Result<> result = gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, false, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel, k_TargetBytes);
    REQUIRE(result.valid());
    const std::vector<float32> expected = GradientMagnitude2DOracle(field, k_DimX, k_DimY, false, {1.0f, 1.0f, 1.0f});
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(inputStore.maxReadValues() <= plan.inputValues);
    REQUIRE(outputStore.maxWriteValues() <= plan.outputValues);
    REQUIRE(outputStore.writtenValues() == expected.size());
  }

  SECTION("overwide X tiles")
  {
    constexpr usize k_DimX = 10000;
    constexpr usize k_DimY = 3;
    const SizeVec3 dims = {k_DimX, k_DimY, 1};
    std::vector<int64> field(k_DimX * k_DimY);
    TransferCountingDataStore<int64> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int64{});
    for(usize index = 0; index < field.size(); ++index)
    {
      field[index] = static_cast<int64>((index * 17 + 11) % 251 - 125);
      inputStore.setValue(index, field[index]);
    }
    TransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, -12345.0f);
    const FloatVec3 spacing = {2.0f, 0.5f, 1.0f};

    const auto planResult = gradient_detail::CreateGradientMagnitude2DPlan<int64>(dims, k_TargetBytes);
    REQUIRE(planResult.valid());
    const auto& plan = planResult.value();
    REQUIRE(plan.route == gradient_detail::GradientMagnitude2DRoute::Tiles);
    REQUIRE(plan.tileColumns < k_DimX);

    const Result<> result = gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, true, spacing, shouldCancel, k_TargetBytes);
    REQUIRE(result.valid());
    const std::vector<float32> expected = GradientMagnitude2DOracle(field, k_DimX, k_DimY, true, spacing);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
    REQUIRE(inputStore.maxReadValues() <= plan.tileColumns + 2);
    REQUIRE(inputStore.readCount() > k_DimY);
    REQUIRE(outputStore.maxWriteValues() <= plan.outputValues);
    REQUIRE(outputStore.writtenValues() == expected.size());
  }

  SECTION("one-cell-wide and one-cell-tall images preserve clamped boundaries")
  {
    for(const SizeVec3 dims : {SizeVec3{1, 17, 1}, SizeVec3{19, 1, 1}})
    {
      const usize valueCount = dims[0] * dims[1];
      std::vector<float64> field(valueCount);
      TransferCountingDataStore<float64> inputStore(ShapeType{1, dims[1], dims[0]}, ShapeType{1}, float64{});
      for(usize index = 0; index < valueCount; ++index)
      {
        field[index] = static_cast<float64>((index * 7 + 5) % 23);
        inputStore.setValue(index, field[index]);
      }
      TransferCountingDataStore<float32> outputStore(ShapeType{1, dims[1], dims[0]}, ShapeType{1}, -12345.0f);
      const FloatVec3 spacing = {0.5f, 2.0f, 1.0f};
      REQUIRE(gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, true, spacing, shouldCancel, k_TargetBytes).valid());
      const std::vector<float32> expected = GradientMagnitude2DOracle(field, dims[0], dims[1], true, spacing);
      for(usize index = 0; index < expected.size(); ++index)
      {
        REQUIRE(outputStore.getValue(index) == expected[index]);
      }
    }
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: bounded true-2-D cancellation and I/O failures preserve their contracts", "[ImageProcessing][GradientMagnitudeEngine]")
{
  constexpr usize k_DimX = 31;
  constexpr usize k_DimY = 257;
  constexpr usize k_TargetBytes = 70ULL * 1024ULL;
  constexpr float32 k_Poison = -12345.0f;
  const SizeVec3 dims = {k_DimX, k_DimY, 1};

  SECTION("cancellation after the input read leaves output unchanged")
  {
    std::atomic_bool shouldCancel{false};
    CancelAfterReadDataStore<int16> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int16{7}, shouldCancel);
    TransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, false, {1.0f, 1.0f, 1.0f}, shouldCancel, k_TargetBytes);
    REQUIRE(result.valid());
    REQUIRE(shouldCancel.load());
    REQUIRE(outputStore.writtenValues() == 0);
    for(usize index = 0; index < outputStore.getSize(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == k_Poison);
    }
  }

  SECTION("input read failure propagates")
  {
    std::atomic_bool shouldCancel{false};
    FailingReadDataStore<int16> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int16{7});
    TransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, false, {1.0f, 1.0f, 1.0f}, shouldCancel, k_TargetBytes);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -9750);
    REQUIRE(outputStore.writtenValues() == 0);
  }

  SECTION("output write failure propagates")
  {
    std::atomic_bool shouldCancel{false};
    TransferCountingDataStore<int16> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int16{7});
    FailingWriteDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, k_Poison);
    const Result<> result = gradient_detail::ApplyGradientMagnitude2D(inputStore, outputStore, dims, false, {1.0f, 1.0f, 1.0f}, shouldCancel, k_TargetBytes);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -9751);
  }
}

TEST_CASE("ImageProcessing::GradientMagnitudeEngine: either OOC endpoint selects bounded true-2-D execution", "[ImageProcessing][GradientMagnitudeEngine]")
{
  constexpr usize k_DimX = 31;
  constexpr usize k_DimY = 257;
  constexpr usize k_TargetBytes = 70ULL * 1024ULL;
  const SizeVec3 dims = {k_DimX, k_DimY, 1};
  std::vector<int32> field(k_DimX * k_DimY);
  for(usize index = 0; index < field.size(); ++index)
  {
    field[index] = static_cast<int32>((index * 19 + 3) % 127 - 63);
  }
  const std::vector<float32> expected = GradientMagnitude2DOracle(field, k_DimX, k_DimY, false, {1.0f, 1.0f, 1.0f});
  const auto planResult = gradient_detail::CreateGradientMagnitude2DPlan<int32>(dims, k_TargetBytes);
  REQUIRE(planResult.valid());
  const auto& plan = planResult.value();
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  SECTION("OOC input and resident output")
  {
    OutOfCoreTransferCountingDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{});
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }
    TransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, -12345.0f);
    ResetAlgorithmPathExecutionCounts();
    REQUIRE(ApplyGradientMagnitude(inputStore, outputStore, dims, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler, k_TargetBytes).valid());
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnOutOfCoreStore == 1);
    REQUIRE(counts.InCore == 0);
    REQUIRE(inputStore.maxReadValues() <= plan.inputValues);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
  }

  SECTION("resident input and OOC output")
  {
    TransferCountingDataStore<int32> inputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, int32{});
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }
    OutOfCoreTransferCountingDataStore<float32> outputStore(ShapeType{1, k_DimY, k_DimX}, ShapeType{1}, -12345.0f);
    ResetAlgorithmPathExecutionCounts();
    REQUIRE(ApplyGradientMagnitude(inputStore, outputStore, dims, false, {1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler, k_TargetBytes).valid());
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnOutOfCoreStore == 1);
    REQUIRE(counts.InCore == 0);
    REQUIRE(outputStore.maxWriteValues() <= plan.outputValues);
    for(usize index = 0; index < expected.size(); ++index)
    {
      REQUIRE(outputStore.getValue(index) == expected[index]);
    }
  }
}
