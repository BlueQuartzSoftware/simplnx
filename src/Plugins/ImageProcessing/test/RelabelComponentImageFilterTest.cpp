#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/RelabelComponentImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

#include <catch2/catch.hpp>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKRelabelComponentImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// streaming size-sort-relabel's bit-exact label-VALUE parity gate.
const Uuid k_LegacyRCUuid = *Uuid::FromString("37e29d16-1020-478c-a506-c121e8f670ad");

using RCFilter = RelabelComponentImageFilter;

template <class T>
class RelabelTransferCountingStore : public DataStore<T>
{
public:
  struct Transfer
  {
    usize start = 0;
    usize count = 0;
  };

  using DataStore<T>::DataStore;

  RelabelTransferCountingStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::optional<ShapeType> chunkShape = {},
                               IDataStore::StoreType storeType = IDataStore::StoreType::OutOfCore)
  : DataStore<T>(tupleShape, componentShape, initialValue)
  , m_ChunkShape(std::move(chunkShape))
  , m_StoreType(storeType)
  {
  }

  IDataStore::StoreType getStoreType() const override
  {
    return m_StoreType;
  }

  std::optional<ShapeType> getChunkShape() const override
  {
    return m_ChunkShape;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    m_ReadValues += buffer.size();
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    m_WrittenValues += buffer.size();
    m_Writes.push_back({startIndex, buffer.size()});
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

  [[nodiscard]] usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  [[nodiscard]] usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  [[nodiscard]] const std::vector<Transfer>& writes() const noexcept
  {
    return m_Writes;
  }

private:
  std::optional<ShapeType> m_ChunkShape;
  IDataStore::StoreType m_StoreType = IDataStore::StoreType::OutOfCore;
  mutable usize m_ReadCount = 0;
  mutable usize m_ReadValues = 0;
  usize m_WriteCount = 0;
  usize m_WrittenValues = 0;
  std::vector<Transfer> m_Writes;
};

class CacheBudgetSentinel
{
public:
  explicit CacheBudgetSentinel(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~CacheBudgetSentinel()
  {
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  CacheBudgetSentinel(const CacheBudgetSentinel&) = delete;
  CacheBudgetSentinel& operator=(const CacheBudgetSentinel&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};

// Sets the standard geom/input/output keys + MinimumObjectSize/SortByObjectSize on a shared Arguments, runs
// preflight + execute, and requires both succeed. Reuses RCFilter::k_*_Key for BOTH the new and the legacy ITK
// filter -- correct only because the new filter deliberately reuses the legacy key strings
// ("input_image_geometry_path", "input_image_data_path", "output_array_name", "minimum_object_size",
// "sort_by_object_size").
void RunRelabel(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint64 minimumObjectSize, bool sortByObjectSize, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(RCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(RCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(RCFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(minimumObjectSize));
  args.insertOrAssign(RCFilter::k_SortByObjectSize_Key, std::make_any<bool>(sortByObjectSize));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic PRE-LABELED field with FOUR components under NON-CONSECUTIVE, UNSORTED original labels (2, 5, 8,
// 9) and, critically, a SIZE TIE between labels 2 and 8 (3 voxels each) -- std::sort is not stable, so
// itkRelabelComponentImageFilter.hxx's comparator tie-break ("equal size -> ascending original label") is the ONLY
// thing that makes the sorted order deterministic, and a field with no size ties would never actually exercise
// that tie-break clause. Sizes: label 5 -> 6 (largest), label 2 -> 3, label 8 -> 3 (tied with label 2), label 9 ->
// 1 (smallest, dropped when MinimumObjectSize > 1). RelabelComponent does not perform connectivity analysis (see
// itkRelabelComponentImageFilter.hxx::ParallelComputeLabels -- it only tallies pixel counts per label value), so
// the voxels of one label need not be spatially contiguous; the 6-voxel component's last voxel is placed on a
// second z-plane for dimZ>1 to also exercise the 3D case's raster layout. Every value fits in EVERY integer type
// incl. int8/uint8. Requires dimX>=10, dimY>=8, and (for dimZ>1) dimZ>=2.
template <class T>
std::vector<T> MakeLabeledField(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ, T{0});
  auto set = [&](usize x, usize y, usize z, T label) {
    if(x < dimX && y < dimY && z < dimZ)
    {
      v[rt::FlatIndex(x, y, z, dimX, dimY)] = label;
    }
  };

  // Label 5: 6 voxels -- the LARGEST component (first under size-descending sort).
  set(1, 1, 0, T{5});
  set(2, 1, 0, T{5});
  set(1, 2, 0, T{5});
  set(2, 2, 0, T{5});
  set(1, 3, 0, T{5});
  set(2, 3, dimZ > 1 ? 1 : 0, T{5});
  // Label 2: 3 voxels -- TIED in size with label 8 (ascending-label tie-break picks this one first when sorted).
  set(6, 1, 0, T{2});
  set(6, 2, 0, T{2});
  set(6, 3, 0, T{2});
  // Label 8: 3 voxels -- TIED in size with label 2 (higher original label, so sorts AFTER label 2 on a tie).
  set(6, 5, 0, T{8});
  set(6, 6, 0, T{8});
  set(6, 7, 0, T{8});
  // Label 9: 1 voxel -- the SMALLEST component (dropped when MinimumObjectSize > 1).
  set(9, 1, 0, T{9});
  return v;
}

// Deterministic large labeled field (values 0..6) for the 200^3 OOC-vs-in-core byte-match gate. Blocky (not
// single-voxel) regions of unequal size so the sort still has real work to do; the exact sizes are irrelevant to
// the D3 gate (OOC must byte-match in-core, not any particular ITK reference).
template <class T>
std::vector<T> MakeLargeLabeledField(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ);
  for(usize z = 0; z < dimZ; ++z)
  {
    for(usize y = 0; y < dimY; ++y)
    {
      for(usize x = 0; x < dimX; ++x)
      {
        const usize label = (x / 17 + y / 23 + z / 31) % 7; // 0..6, unevenly sized blocky regions
        v[rt::FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(label);
      }
    }
  }
  return v;
}

// Run new + legacy on the same field/params and require the outputs match EXACTLY (byte-identical label VALUES --
// the live-ITK gate on the streaming size-sort-relabel).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, uint64 minimumObjectSize, bool sortByObjectSize)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  RCFilter newFilter;
  RunRelabel(newFilter, newDs, newInput, minimumObjectSize, sortByObjectSize);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyRCUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunRelabel(*legacyFilter, legacyDs, legacyInput, minimumObjectSize, sortByObjectSize);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<T>(newOut, legacyOut); // EXACT -- label values must byte-match live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL integer type grid x {SortByObjectSize on/off} x {MinimumObjectSize 0,2} x
//     {3D, 2D}. RelabelComponent's output is SameAsInput; the new filter must reproduce legacy ITK's label VALUES
//     EXACTLY (no tolerance -- see the Parity model, in particular the std::sort comparator/tie-break). This is
//     the primary gate on the streaming size-sort-relabel.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::RelabelComponentImageFilter: Live-ITK parity", "[ImageProcessing][RelabelComponentImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyRCUuid) != nullptr);

  const bool sortByObjectSize = GENERATE(false, true);
  const uint64 minimumObjectSize = GENERATE(uint64{0}, uint64{2});
  CAPTURE(sortByObjectSize, minimumObjectSize);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(MakeLabeledField<T>(DX, DY, DZ), DX, DY, DZ, minimumObjectSize, sortByObjectSize);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(MakeLabeledField<T>(DX, DY, DZ), DX, DY, DZ, minimumObjectSize, sortByObjectSize);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
//     rejected (IntegerOnly); a valid integer scalar input produces a SameAsInput output array.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RelabelComponentImageFilter: preflight guards", "[ImageProcessing][RelabelComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    RCFilter filter;
    Arguments args;
    args.insertOrAssign(RCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(RCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(RCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(RCFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(uint64{0}));
    args.insertOrAssign(RCFilter::k_SortByObjectSize_Key, std::make_any<bool>(true));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float input rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    const DataPath geomPath = inputPath.getParent().getParent();
    RCFilter filter;
    Arguments args;
    args.insertOrAssign(RCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(RCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(RCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(RCFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(uint64{0}));
    args.insertOrAssign(RCFilter::k_SortByObjectSize_Key, std::make_any<bool>(true));
    const auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
  }
  SECTION("scalar integer input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeLabeledField<uint8>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 12, 12, 1, field);
    RCFilter filter;
    RunRelabel(filter, ds, inputPath, /*minimumObjectSize=*/0, /*sortByObjectSize=*/true);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// (3) Execute-time overflow guard: when the number of surviving components (>= MinimumObjectSize) exceeds the
//     output label type's maximum, ExecuteRelabelComponentImageFilter cannot assign a distinct consecutive label to
//     every survivor and returns k_RelabelComponentTooManyObjects (-8590). int8 (max label 127) is the ONLY integer
//     type that can actually trigger this at the filter level, because it is the only type whose count of distinct
//     non-zero label values (255) exceeds its own max (a uint8 image can hold at most 255 distinct labels, never
//     > 255). 130 distinct single-voxel labels (> 127) => assigning the 128th survivor overflows. Preflight passes;
//     the error only surfaces at execute (the survivor count is data-dependent, discovered mid-stream).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RelabelComponentImageFilter: too-many-objects overflow (int8)", "[ImageProcessing][RelabelComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 20, DY = 10, DZ = 1; // 200 voxels -- room for 130 single-voxel labels + background
  constexpr usize k_NumLabels = 130;        // > max<int8> (127): the 128th surviving label overflows
  static_assert(k_NumLabels <= DX * DY * DZ, "field must hold every distinct label voxel");
  std::vector<int8> field(DX * DY * DZ, int8{0});
  for(usize i = 0; i < k_NumLabels; ++i)
  {
    // 130 DISTINCT non-zero int8 labels: 1..127, then -1,-2,-3 (all representable, none equal to the background 0).
    const int labelValue = (i < 127) ? static_cast<int>(i) + 1 : 126 - static_cast<int>(i);
    field[i] = static_cast<int8>(labelValue);
  }

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int8>(ds, DX, DY, DZ, field);

  RCFilter filter;
  Arguments args;
  args.insertOrAssign(RCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(RCFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(uint64{0})); // keep every label (nothing dropped)
  args.insertOrAssign(RCFilter::k_SortByObjectSize_Key, std::make_any<bool>(false));

  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // the overflow is only detectable at execute
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().front().code == nx::core::ImageProcessing::k_RelabelComponentTooManyObjects);
}

// -----------------------------------------------------------------------------
// (4) A request cancelled before execution must not replace or partially overwrite the caller-provided output.
//     Exercise both the narrow dense-label path and the wider sparse-label path directly so this contract is not
//     hidden by the filter's output-creation action.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::RelabelComponentImageFilter: pre-cancel preserves output", "[ImageProcessing][RelabelComponentImageFilter]", uint8, int32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 4;
  constexpr T k_Poison = static_cast<T>(42);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<T>(ds, DX, DY, DZ, MakeLabeledField<T>(DX, DY, DZ));
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto outputStore = DataStoreUtilities::CreateDataStore<T>(ds, outputPath, {DZ, DY, DX}, {1});
  const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(outputPath.getParent());
  auto* outputArray = DataArray<T>::Create(ds, outputPath.getTargetName(), outputStore, cellAM.getId());
  REQUIRE(outputArray != nullptr);
  outputArray->getDataStoreRef().fill(k_Poison);

  std::atomic_bool shouldCancel = true;
  const Result<> result = ImageProcessing::ExecuteRelabelComponentImageFilter(ds, inputPath, outputPath, /*minimumObjectSize=*/0, /*sortByObjectSize=*/true, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  for(const T value : outputArray->getDataStoreRef())
  {
    REQUIRE(value == k_Poison);
  }
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: bounded OOC staging reads each input value twice", "[ImageProcessing][RelabelComponentImageFilter]")
{
  constexpr usize k_DimX = 12;
  constexpr usize k_DimY = 12;
  constexpr usize k_DimZ = 4;
  constexpr usize k_CacheBudgetBytes = 4096;
  constexpr usize k_WorkingBytes = k_CacheBudgetBytes / 4;
  constexpr usize k_StagingValues = k_WorkingBytes / sizeof(int32);
  static_assert(k_StagingValues > 0);

  const std::vector<int32> input = MakeLabeledField<int32>(k_DimX, k_DimY, k_DimZ);
  const std::vector<int32> expected = [] {
    std::vector<int32> values(k_DimX * k_DimY * k_DimZ, 0);
    values[rt::FlatIndex(1, 1, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 1, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(1, 2, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 2, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(1, 3, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 3, 1, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(6, 1, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 2, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 3, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 5, 0, k_DimX, k_DimY)] = 3;
    values[rt::FlatIndex(6, 6, 0, k_DimX, k_DimY)] = 3;
    values[rt::FlatIndex(6, 7, 0, k_DimX, k_DimY)] = 3;
    return values;
  }();
  CacheBudgetSentinel budgetSentinel(k_CacheBudgetBytes);

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_DimX, k_DimY, k_DimZ, input);
  auto& inputArray = dataStructure.getDataRefAs<DataArray<int32>>(inputPath);
  auto inputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 0);
  Result<> copyFromBufferResult = inputStore->copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  Result<> setStoreResult = inputArray.setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& cellAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(outputPath.getParent());
  auto outputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 0);
  auto* outputArray = DataArray<int32>::Create(dataStructure, outputPath.getTargetName(), outputStore, cellAttributeMatrix.getId());
  REQUIRE(outputArray != nullptr);

  std::atomic_bool shouldCancel{false};
  Result<> executeResult = ImageProcessing::ExecuteRelabelComponentImageFilter(dataStructure, inputPath, outputPath, /*minimumObjectSize=*/2, /*sortByObjectSize=*/true, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);

  REQUIRE(inputStore->readValues() == 2 * input.size());
  REQUIRE(inputStore->readCount() == 2 * ((input.size() + k_StagingValues - 1) / k_StagingValues));
  REQUIRE(outputStore->writtenValues() == input.size());
  REQUIRE(outputStore->writeCount() == (input.size() + k_StagingValues - 1) / k_StagingValues);

  std::vector<int32> actual(expected.size());
  Result<> copyIntoBufferResult = outputStore->copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: OOC staging aligns complete output slabs", "[ImageProcessing][RelabelComponentImageFilter]")
{
  const ShapeType tupleShape{4, 12, 12};
  const auto slabPlan = ImageProcessing::detail::CreateRelabelChunkSlabPlan(tupleShape, ShapeType{1, 12, 12}, /*numValues=*/576, sizeof(int32));
  REQUIRE(slabPlan.has_value());
  REQUIRE(slabPlan->slabValues == 144);
  REQUIRE(slabPlan->slabBytes == 144 * sizeof(int32));
  REQUIRE(ImageProcessing::detail::SelectRelabelStagingValues(/*numValues=*/576, /*grantedBytes=*/1024, sizeof(int32), slabPlan) == 144);
  REQUIRE(ImageProcessing::detail::SelectRelabelStagingValues(/*numValues=*/576, /*grantedBytes=*/575, sizeof(int32), slabPlan) == 143);
  REQUIRE_FALSE(ImageProcessing::detail::CreateRelabelChunkSlabPlan(tupleShape, ShapeType{0, 12, 12}, /*numValues=*/576, sizeof(int32)).has_value());
  REQUIRE_FALSE(ImageProcessing::detail::CreateRelabelChunkSlabPlan(tupleShape, ShapeType{5, 12, 12}, /*numValues=*/576, sizeof(int32)).has_value());
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: resident spans avoid staging transfers", "[ImageProcessing][RelabelComponentImageFilter]")
{
  constexpr usize k_DimX = 12;
  constexpr usize k_DimY = 12;
  constexpr usize k_DimZ = 4;
  const std::vector<int32> input = MakeLabeledField<int32>(k_DimX, k_DimY, k_DimZ);

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_DimX, k_DimY, k_DimZ, input);
  auto& inputArray = dataStructure.getDataRefAs<DataArray<int32>>(inputPath);
  auto inputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 0, std::nullopt, IDataStore::StoreType::InMemory);
  Result<> copyFromBufferResult = inputStore->copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  Result<> setStoreResult = inputArray.setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& cellAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(outputPath.getParent());
  auto outputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 73, std::nullopt, IDataStore::StoreType::InMemory);
  auto* outputArray = DataArray<int32>::Create(dataStructure, outputPath.getTargetName(), outputStore, cellAttributeMatrix.getId());
  REQUIRE(outputArray != nullptr);

  std::atomic_bool shouldCancel{false};
  Result<> executeResult = ImageProcessing::ExecuteRelabelComponentImageFilter(dataStructure, inputPath, outputPath, /*minimumObjectSize=*/2, /*sortByObjectSize=*/true, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);

  REQUIRE(inputStore->readValues() == 0);
  REQUIRE(outputStore->writtenValues() == 0);
  REQUIRE(&dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef() == outputStore.get());

  const std::vector<int32> expected = [] {
    std::vector<int32> values(k_DimX * k_DimY * k_DimZ, 0);
    values[rt::FlatIndex(1, 1, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 1, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(1, 2, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 2, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(1, 3, 0, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(2, 3, 1, k_DimX, k_DimY)] = 1;
    values[rt::FlatIndex(6, 1, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 2, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 3, 0, k_DimX, k_DimY)] = 2;
    values[rt::FlatIndex(6, 5, 0, k_DimX, k_DimY)] = 3;
    values[rt::FlatIndex(6, 6, 0, k_DimX, k_DimY)] = 3;
    values[rt::FlatIndex(6, 7, 0, k_DimX, k_DimY)] = 3;
    return values;
  }();
  std::vector<int32> actual(expected.size());
  Result<> copyIntoBufferResult = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
  REQUIRE(actual == expected);
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: an undersized OOC working-memory grant reports the actual values", "[ImageProcessing][RelabelComponentImageFilter]")
{
  constexpr usize k_DimX = 12;
  constexpr usize k_DimY = 12;
  constexpr usize k_DimZ = 4;
  constexpr uint64 k_CacheBudgetBytes = sizeof(int32) - 1;
  CacheBudgetSentinel budgetSentinel(k_CacheBudgetBytes);

  DataStructure dataStructure;
  const std::vector<int32> input = MakeLabeledField<int32>(k_DimX, k_DimY, k_DimZ);
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_DimX, k_DimY, k_DimZ, input);
  auto& inputArray = dataStructure.getDataRefAs<DataArray<int32>>(inputPath);
  auto inputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 0);
  Result<> copyFromBufferResult = inputStore->copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
  Result<> setStoreResult = inputArray.setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& cellAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(outputPath.getParent());
  auto outputStore = std::make_shared<RelabelTransferCountingStore<int32>>(inputArray.getTupleShape(), inputArray.getComponentShape(), 73);
  auto* outputArray = DataArray<int32>::Create(dataStructure, outputPath.getTargetName(), outputStore, cellAttributeMatrix.getId());
  REQUIRE(outputArray != nullptr);

  std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::ExecuteRelabelComponentImageFilter(dataStructure, inputPath, outputPath, /*minimumObjectSize=*/2, /*sortByObjectSize=*/true, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == ImageProcessing::detail::k_RelabelComponentInsufficientWorkingMemory);
  REQUIRE(result.errors().front().message.find(inputPath.toString()) != std::string::npos);
  REQUIRE(result.errors().front().message.find("granted bytes: 0") != std::string::npos);
  REQUIRE(outputStore->writtenValues() == 0);
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson: MinimumObjectSize (uint64), SortByObjectSize (bool), and the geometry/array/name DataPaths.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RelabelComponentImageFilter: FromSIMPLJson", "[ImageProcessing][RelabelComponentImageFilter]")
{
  const nlohmann::json json = {
      {"MinimumObjectSize", 5},
      {"SortByObjectSize", false},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "RelabelOut"},
  };
  const Result<Arguments> result = RelabelComponentImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<uint64>(RCFilter::k_MinimumObjectSize_Key) == 5u);
  REQUIRE(args.value<bool>(RCFilter::k_SortByObjectSize_Key) == false);
  REQUIRE(args.value<DataPath>(RCFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(RCFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(RCFilter::k_OutputImageArrayName_Key) == "RelabelOut");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKRelabelComponentImageTest.cpp on OUR ITK-free filter. Output is
// SameAsInput (uint8 for these PNG label inputs); (A) DURABLE golden = md5-validity-first (plan Sec.4), (B) LIVE-ITK
// parity = BIT-EXACT (integer labels, CompareImages@0.0). Helper pins ForceInCore.
//   default:    2th_cthead1.png, defaults (SortByObjectSize true, MinimumObjectSize 0).
//   no_sorting: simple-label-d.png, SortByObjectSize = false.
//   no_sorting2: simple-label-d.png, SortByObjectSize = false AND MinimumObjectSize = 140.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RelabelComponentImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][RelabelComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RelabelComponentImageFilter>("2th_cthead1.png", "58af064e929f08f9d5bacc8be44ed92e");
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: ITK real-image golden (no_sorting)", "[ImageProcessing][ItkGolden][RelabelComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RelabelComponentImageFilter>("simple-label-d.png", "0231da8387aa665ddff9c2645e71f213",
                                                                 [](Arguments& args) { args.insertOrAssign(RelabelComponentImageFilter::k_SortByObjectSize_Key, std::make_any<bool>(false)); });
}

TEST_CASE("ImageProcessing::RelabelComponentImageFilter: ITK real-image golden (no_sorting2)", "[ImageProcessing][ItkGolden][RelabelComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RelabelComponentImageFilter>("simple-label-d.png", "4c81464832a0270041334abd744b94b0", [](Arguments& args) {
    args.insertOrAssign(RelabelComponentImageFilter::k_SortByObjectSize_Key, std::make_any<bool>(false));
    args.insertOrAssign(RelabelComponentImageFilter::k_MinimumObjectSize_Key, std::make_any<uint64>(140));
  });
}
