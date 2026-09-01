#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include <algorithm>
#include <limits>
#include <new>

using namespace nx::core;

TEST_CASE("MemorySafety: WouldExceedAvailableMemory pure overload", "[MemorySafety]")
{
  const uint64 oneGiB = uint64{1} * 1024 * 1024 * 1024;

  // Fits: 1 GiB used + 1 GiB required < 4 GiB available.
  REQUIRE_FALSE(ArrayCreationUtilities::WouldExceedAvailableMemory(oneGiB, oneGiB, 4 * oneGiB));

  // Exactly at the limit does NOT trigger (strictly-greater semantics):
  // 2 GiB used + 2 GiB required == 4 GiB available.
  REQUIRE_FALSE(ArrayCreationUtilities::WouldExceedAvailableMemory(2 * oneGiB, 2 * oneGiB, 4 * oneGiB));

  // Over by one byte triggers.
  REQUIRE(ArrayCreationUtilities::WouldExceedAvailableMemory(2 * oneGiB, 2 * oneGiB + 1, 4 * oneGiB));

  // Required alone exceeds available (and the sum would overflow if added naively).
  REQUIRE(ArrayCreationUtilities::WouldExceedAvailableMemory(oneGiB, std::numeric_limits<uint64>::max(), 4 * oneGiB));

  // Available of 0 (query failure) -> any non-zero requirement exceeds it.
  REQUIRE(ArrayCreationUtilities::WouldExceedAvailableMemory(0, 1, 0));

  // Boundary: required == available with non-zero current usage -> exceeds.
  REQUIRE(ArrayCreationUtilities::WouldExceedAvailableMemory(1, 4 * oneGiB, 4 * oneGiB));

  // NOTE: the DataStructure overload is intentionally not unit-tested here because it
  // calls Memory::GetSystemMemoryInfo() (the live OS); its arithmetic is covered by the
  // pure overload above.
}

TEST_CASE("MemorySafety: EmptyDataStore::memoryUsage in-core vs OOC", "[MemorySafety]")
{
  using StoreT = EmptyDataStore<float32>;
  const ShapeType tupleShape{100}; // 100 tuples
  const ShapeType compShape{4};    // x4 components
  const uint64 logicalBytes = uint64{100} * 4 * sizeof(float32);

  // Empty format (in-core) -> logical size.
  StoreT inCore(tupleShape, compShape, "");
  REQUIRE(inCore.memoryUsage() == logicalBytes);

  // Non-empty format (OOC placeholder) -> 0 (data lives on disk).
  StoreT ooc(tupleShape, compShape, "HDF5-OOC");
  REQUIRE(ooc.memoryUsage() == 0);

  // Zero tuples -> zero bytes regardless of storage strategy.
  StoreT empty(ShapeType{0}, compShape, "");
  REQUIRE(empty.memoryUsage() == 0);
}

TEST_CASE("MemorySafety: CreateArray preflight emits -271 only for over-available in-core arrays", "[MemorySafety]")
{
  // A tuple count whose float32 size (~4 PB) exceeds available RAM on any machine,
  // but is far below uint64 overflow. Preflight never allocates, so this is safe.
  const std::vector<usize> hugeTupleShape{uint64{1} * 1000 * 1000 * 1000 * 1000}; // 1e12 tuples
  const std::vector<usize> compShape{4};
  const std::vector<usize> smallTupleShape{10};

  // 1) Huge IN-CORE array (empty dataFormat) -> exactly one -271 warning.
  {
    DataStructure ds;
    DataGroup::Create(ds, "Group");
    const DataPath path({"Group", "BigInCore"});
    Result<> r = ArrayCreationUtilities::CreateArray<float32>(ds, hugeTupleShape, compShape, path, IDataAction::Mode::Preflight, "");
    REQUIRE_FALSE(r.invalid());
    const auto count = std::count_if(r.warnings().cbegin(), r.warnings().cend(), [](const Warning& w) { return w.code == -271; });
    REQUIRE(count == 1);
  }

  // 2) Huge array with a non-empty (out-of-core) format -> NO -271 warning.
  //    A non-empty dataFormat makes isInCore false regardless of OOC build config,
  //    so the preflight warning branch is correctly skipped.
  {
    DataStructure ds;
    DataGroup::Create(ds, "Group");
    const DataPath path({"Group", "BigOoc"});
    Result<> r = ArrayCreationUtilities::CreateArray<float32>(ds, hugeTupleShape, compShape, path, IDataAction::Mode::Preflight, "HDF5-OOC");
    REQUIRE_FALSE(r.invalid());
    const auto count = std::count_if(r.warnings().cbegin(), r.warnings().cend(), [](const Warning& w) { return w.code == -271; });
    REQUIRE(count == 0);
  }

  // 3) Small in-core array -> NO -271 warning.
  {
    DataStructure ds;
    DataGroup::Create(ds, "Group");
    const DataPath path({"Group", "SmallInCore"});
    Result<> r = ArrayCreationUtilities::CreateArray<float32>(ds, smallTupleShape, compShape, path, IDataAction::Mode::Preflight, "");
    REQUIRE_FALSE(r.invalid());
    const auto count = std::count_if(r.warnings().cbegin(), r.warnings().cend(), [](const Warning& w) { return w.code == -271; });
    REQUIRE(count == 0);
  }
}

TEST_CASE("MemorySafety: DataStructure::memoryUsage counts OOC preflight arrays as 0", "[MemorySafety]")
{
  DataStructure ds;
  DataGroup::Create(ds, "Group");
  const std::vector<usize> tupleShape{1000};
  const std::vector<usize> compShape{1};
  const uint64 inCoreBytes = uint64{1000} * 1 * sizeof(float32);

  REQUIRE_FALSE(ArrayCreationUtilities::CreateArray<float32>(ds, tupleShape, compShape, DataPath({"Group", "InCore"}), IDataAction::Mode::Preflight, "").invalid());
  REQUIRE_FALSE(ArrayCreationUtilities::CreateArray<float32>(ds, tupleShape, compShape, DataPath({"Group", "Ooc"}), IDataAction::Mode::Preflight, "HDF5-OOC").invalid());

  // Only the in-core array contributes to the RAM total.
  REQUIRE(ds.memoryUsage() == inCoreBytes);
}

namespace
{
// Minimal in-test filter whose executeImpl throws std::bad_alloc, to exercise the
// IFilter::execute safety net.
class MockOomFilter : public IFilter
{
public:
  MockOomFilter() = default;
  ~MockOomFilter() noexcept override = default;

  std::string name() const override
  {
    return "MockOomFilter";
  }
  std::string className() const override
  {
    return "MockOomFilter";
  }
  Uuid uuid() const override
  {
    return *Uuid::FromString("11111111-1111-1111-1111-111111111111");
  }
  std::string humanName() const override
  {
    return "Mock OOM Filter";
  }
  Parameters parameters() const override
  {
    return {};
  }
  VersionType parametersVersion() const override
  {
    return 1;
  }
  UniquePointer clone() const override
  {
    return std::make_unique<MockOomFilter>();
  }

  PreflightResult preflightImpl(const DataStructure&, const Arguments&, const MessageHandler&, const std::atomic_bool&, const ExecutionContext&) const override
  {
    return {OutputActions{}};
  }

  Result<> executeImpl(DataStructure&, const Arguments&, const PipelineFilter*, const MessageHandler&, const std::atomic_bool&, const ExecutionContext&) const override
  {
    throw std::bad_alloc{};
  }
};
} // namespace

TEST_CASE("MemorySafety: IFilter::execute converts std::bad_alloc to a -272 error", "[MemorySafety]")
{
  MockOomFilter filter;
  DataStructure ds;
  Arguments args;
  std::atomic_bool shouldCancel{false};

  IFilter::ExecuteResult result = filter.execute(ds, args, nullptr, {}, shouldCancel, {});

  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == -272);
  REQUIRE(result.result.errors()[0].message.find("Mock OOM Filter") != std::string::npos);
}
