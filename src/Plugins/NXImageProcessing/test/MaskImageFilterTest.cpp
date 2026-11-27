#include "NXImageProcessing/Filters/MaskImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/MaskEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
template <class T>
class MaskTransferCountingStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCalls;
    m_ReadValues += buffer.size();
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCalls;
    m_WriteValues += buffer.size();
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCalls() const noexcept
  {
    return m_ReadCalls;
  }

  [[nodiscard]] usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  [[nodiscard]] usize writeCalls() const noexcept
  {
    return m_WriteCalls;
  }

  [[nodiscard]] usize writeValues() const noexcept
  {
    return m_WriteValues;
  }

private:
  mutable usize m_ReadCalls = 0;
  mutable usize m_ReadValues = 0;
  usize m_WriteCalls = 0;
  usize m_WriteValues = 0;
};

class MaskCacheBudgetScope
{
public:
  explicit MaskCacheBudgetScope(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::Instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }

  ~MaskCacheBudgetScope()
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }

  MaskCacheBudgetScope(const MaskCacheBudgetScope&) = delete;
  MaskCacheBudgetScope& operator=(const MaskCacheBudgetScope&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget = 0;
};
} // namespace

TEST_CASE("NXImageProcessing::MaskEngine: adaptive OOC batches read the mask first and honor the live grant", "[NXImageProcessing][MaskImageFilter][WorkingMemory]")
{
  constexpr usize k_NumTuples = 8;
  constexpr usize k_NumComponents = 1;
  constexpr usize k_GrantedBytes = 2 * (sizeof(int32) + sizeof(uint8));
  const ShapeType tupleShape{4, 1, 2};
  MaskTransferCountingStore<int32> inputStore(tupleShape, ShapeType{1}, 0);
  MaskTransferCountingStore<int32> outputStore(tupleShape, ShapeType{1}, -1);
  MaskTransferCountingStore<uint8> maskStore(tupleShape, ShapeType{1}, 0);
  const std::array<uint8, k_NumTuples> mask = {0, 0, 1, 0, 1, 1, 0, 1};
  for(usize index = 0; index < k_NumTuples; ++index)
  {
    inputStore.setValue(index, static_cast<int32>(index + 1));
    maskStore.setValue(index, mask[index]);
  }

  const MaskCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const Result<> maskResult = ImageProcessing::ApplyTypedMask(inputStore, outputStore, maskStore, k_NumComponents, int32{-7}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(maskResult);

  REQUIRE(maskStore.readCalls() == 4);
  REQUIRE(maskStore.readValues() == k_NumTuples);
  REQUIRE(inputStore.readCalls() == 3); // The first two-tuple batch is fully outside and does not need input data.
  REQUIRE(inputStore.readValues() == 6);
  REQUIRE(outputStore.writeCalls() == 4);
  REQUIRE(outputStore.writeValues() == k_NumTuples);
  for(usize index = 0; index < k_NumTuples; ++index)
  {
    const int32 expected = mask[index] == 0 ? -7 : static_cast<int32>(index + 1);
    REQUIRE(outputStore.getValue(index) == expected);
  }
}

TEST_CASE("NXImageProcessing::MaskEngine: adaptive OOC batches keep multi-component tuples together", "[NXImageProcessing][MaskImageFilter][WorkingMemory]")
{
  constexpr usize k_NumTuples = 4;
  constexpr usize k_NumComponents = 3;
  constexpr usize k_GrantedBytes = 2 * (k_NumComponents * sizeof(int32) + sizeof(uint8));
  const ShapeType tupleShape{2, 1, 2};
  MaskTransferCountingStore<int32> inputStore(tupleShape, ShapeType{k_NumComponents}, 0);
  MaskTransferCountingStore<int32> outputStore(tupleShape, ShapeType{k_NumComponents}, -1);
  MaskTransferCountingStore<uint8> maskStore(tupleShape, ShapeType{1}, 0);
  const std::array<uint8, k_NumTuples> mask = {0, 0, 1, 0};
  for(usize tuple = 0; tuple < k_NumTuples; ++tuple)
  {
    maskStore.setValue(tuple, mask[tuple]);
    for(usize component = 0; component < k_NumComponents; ++component)
    {
      inputStore.setValue(tuple * k_NumComponents + component, static_cast<int32>(10 * tuple + component + 1));
    }
  }

  const MaskCacheBudgetScope budgetScope(4 * k_GrantedBytes);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride workingMemoryOverride(k_GrantedBytes);
  std::atomic_bool shouldCancel{false};
  const Result<> maskResult = ImageProcessing::ApplyTypedMask(inputStore, outputStore, maskStore, k_NumComponents, int32{-9}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(maskResult);

  REQUIRE(maskStore.readCalls() == 2);
  REQUIRE(maskStore.readValues() == k_NumTuples);
  REQUIRE(inputStore.readCalls() == 1); // The first two complete tuples are outside and need no input read.
  REQUIRE(inputStore.readValues() == 2 * k_NumComponents);
  REQUIRE(outputStore.writeCalls() == 2);
  REQUIRE(outputStore.writeValues() == k_NumTuples * k_NumComponents);
  for(usize tuple = 0; tuple < k_NumTuples; ++tuple)
  {
    for(usize component = 0; component < k_NumComponents; ++component)
    {
      const int32 expected = mask[tuple] == 0 ? -9 : static_cast<int32>(10 * tuple + component + 1);
      REQUIRE(outputStore.getValue(tuple * k_NumComponents + component) == expected);
    }
  }
}

TEST_CASE("NXImageProcessing::MaskEngine: cancellation before a chunk avoids all bulk transfers", "[NXImageProcessing][MaskImageFilter][WorkingMemory]")
{
  const ShapeType tupleShape{4, 1, 1};
  MaskTransferCountingStore<int32> inputStore(tupleShape, ShapeType{1}, 1);
  MaskTransferCountingStore<int32> outputStore(tupleShape, ShapeType{1}, -1);
  MaskTransferCountingStore<uint8> maskStore(tupleShape, ShapeType{1}, 1);
  std::atomic_bool shouldCancel{true};
  const Result<> maskResult = ImageProcessing::ApplyTypedMask(inputStore, outputStore, maskStore, /*numComponents=*/1, int32{-7}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(maskResult);

  REQUIRE(maskStore.readCalls() == 0);
  REQUIRE(inputStore.readCalls() == 0);
  REQUIRE(outputStore.writeCalls() == 0);
}

// Preflight guards added by the adversarial-review remediation: the Float64 Outside Value is cast to the input
// element type by the engine (undefined for a non-finite / out-of-range value on an integer type), and the mask
// engine reads the mask as a single-component scalar.
TEST_CASE("NXImageProcessing::MaskImageFilter: Preflight guards (Outside Value range + mask components)", "[NXImageProcessing][MaskImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // Build an int8 input + a uint8 mask (with maskComponents components) under one geometry.
  auto buildInt8 = [](DataStructure& dataStructure, usize maskComponents) {
    const ShapeType cellShape = {2, 2, 2};
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
    imageGeomPtr->setDimensions({2, 2, 2});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto inStore = data_store_utilities::CreateDataStore<int8>(dataStructure, DataPath({"Image Geometry", "CellData", "Input"}), cellShape, {1});
    DataArray<int8>::Create(dataStructure, "Input", inStore, cellAmPtr->getId());
    auto maskStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, DataPath({"Image Geometry", "CellData", "Mask"}), cellShape, {maskComponents});
    DataArray<uint8>::Create(dataStructure, "Mask", maskStore, cellAmPtr->getId());
  };

  auto makeArgs = [](float64 outsideValue) {
    Arguments args;
    args.insertOrAssign(MaskImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(MaskImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Input"})));
    args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Mask"})));
    args.insertOrAssign(MaskImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaskImageFilter::k_OutsideValue_Key, std::make_any<float64>(outsideValue));
    return args;
  };

  MaskImageFilter filter;

  SECTION("Outside Value out of int8 range -> error")
  {
    DataStructure dataStructure;
    buildInt8(dataStructure, 1);
    auto result = filter.preflight(dataStructure, makeArgs(300.0));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == -8103);
  }
  SECTION("Non-finite Outside Value -> error")
  {
    DataStructure dataStructure;
    buildInt8(dataStructure, 1);
    auto result = filter.preflight(dataStructure, makeArgs(std::numeric_limits<float64>::quiet_NaN()));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == -8102);
  }
  SECTION("Fractional Outside Value -> truncation warning, still valid")
  {
    DataStructure dataStructure;
    buildInt8(dataStructure, 1);
    auto result = filter.preflight(dataStructure, makeArgs(1.5));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    REQUIRE_FALSE(result.output_actions.warnings().empty());
    REQUIRE(result.output_actions.warnings().front().code == -8104);
  }
  SECTION("In-range Outside Value -> clean")
  {
    DataStructure dataStructure;
    buildInt8(dataStructure, 1);
    auto result = filter.preflight(dataStructure, makeArgs(5.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
    REQUIRE(result.output_actions.warnings().empty());
  }
  SECTION("Multi-component mask -> error")
  {
    DataStructure dataStructure;
    buildInt8(dataStructure, 2);
    auto result = filter.preflight(dataStructure, makeArgs(0.0));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == -8105);
  }
}

namespace
{
// Builds a scalar float32 "Input" (value(i) = i+1) and a scalar mask of element type MaskT (alternating
// 0 / nonzeroValue) under one geometry, for the integer-mask-type execute coverage below. The filter supports
// uint8/uint16/uint32 masks; only uint8 was exercised elsewhere, so this covers the uint16 and uint32 paths.
template <class MaskT>
void BuildTypedMaskInputs(DataStructure& dataStructure, usize dim, MaskT nonzeroValue)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({dim, dim, dim});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const usize numTuples = dim * dim * dim;
  auto inStore = data_store_utilities::CreateDataStore<float32>(dataStructure, DataPath({"Image Geometry", "CellData", "Input"}), cellShape, {1});
  auto* inArrayPtr = DataArray<float32>::Create(dataStructure, "Input", inStore, cellAmPtr->getId());
  auto& inRef = inArrayPtr->getDataStoreRef();
  for(usize i = 0; i < inRef.getSize(); ++i)
  {
    inRef.setValue(i, static_cast<float32>(i + 1));
  }
  auto maskStore = data_store_utilities::CreateDataStore<MaskT>(dataStructure, DataPath({"Image Geometry", "CellData", "Mask"}), cellShape, {1});
  auto* maskArrayPtr = DataArray<MaskT>::Create(dataStructure, "Mask", maskStore, cellAmPtr->getId());
  auto& maskRef = maskArrayPtr->getDataStoreRef();
  for(usize t = 0; t < numTuples; ++t)
  {
    maskRef.setValue(t, (t % 2 == 0) ? MaskT{0} : nonzeroValue); // alternating discard / keep
  }
}

// Runs MaskImageFilter with a mask of element type MaskT and asserts the keep/discard semantics directly
// (mask != 0 -> keep the input value; mask == 0 -> the Outside Value), the same rule the engine applies.
template <class MaskT>
void RunMaskTypeKeepDiscard(MaskT nonzeroValue)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  constexpr usize k_Dim = 6;
  const float64 outsideValue = -7.0;

  DataStructure dataStructure;
  BuildTypedMaskInputs<MaskT>(dataStructure, k_Dim, nonzeroValue);

  MaskImageFilter filter;
  Arguments args;
  args.insertOrAssign(MaskImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(MaskImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Input"})));
  args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Mask"})));
  args.insertOrAssign(MaskImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MaskImageFilter::k_OutsideValue_Key, std::make_any<float64>(outsideValue));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& inStore = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Image Geometry", "CellData", "Input"})).getDataStoreRef();
  const auto& maskStore = dataStructure.getDataRefAs<DataArray<MaskT>>(DataPath({"Image Geometry", "CellData", "Mask"})).getDataStoreRef();
  const auto& outStore = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Image Geometry", "CellData", "Output"})).getDataStoreRef();
  REQUIRE(outStore.getSize() == inStore.getSize());
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    const float32 expected = (maskStore.getValue(i) != MaskT{0}) ? inStore.getValue(i) : static_cast<float32>(outsideValue);
    INFO("index=" << i);
    REQUIRE(outStore.getValue(i) == expected);
  }
}
} // namespace

TEST_CASE("NXImageProcessing::MaskImageFilter: uint16 mask keep/discard", "[NXImageProcessing][MaskImageFilter]")
{
  RunMaskTypeKeepDiscard<uint16>(uint16{40000}); // nonzero beyond the uint8 range, exercising the uint16 mask path
}

TEST_CASE("NXImageProcessing::MaskImageFilter: uint32 mask keep/discard", "[NXImageProcessing][MaskImageFilter]")
{
  RunMaskTypeKeepDiscard<uint32>(uint32{3000000000U}); // nonzero beyond the uint16 range, exercising the uint32 mask path
}

// Additional preflight guards not covered by the Outside-Value / mask-component test above: the mask must be
// uint8/uint16/uint32 (a valid-but-wrong integer type -> -8100), and must have the same tuple count as the
// input (-8101). The mask param is restricted to integer scalar types, so int32 passes parameter validation
// yet trips the filter's uint8/uint16/uint32-only guard.
TEST_CASE("NXImageProcessing::MaskImageFilter: Preflight rejects wrong mask type (-8100) and tuple mismatch (-8101)", "[NXImageProcessing][MaskImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const auto makeArgs = []() {
    Arguments args;
    args.insertOrAssign(MaskImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(MaskImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "CellData", "Input"})));
    args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry", "MaskData", "Mask"})));
    args.insertOrAssign(MaskImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MaskImageFilter::k_OutsideValue_Key, std::make_any<float64>(0.0));
    return args;
  };

  MaskImageFilter filter;

  SECTION("wrong mask type (int32) -> -8100")
  {
    DataStructure dataStructure;
    const ShapeType cellShape = {2, 2, 2};
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
    imageGeomPtr->setDimensions({2, 2, 2});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto inStore = data_store_utilities::CreateDataStore<float32>(dataStructure, DataPath({"Image Geometry", "CellData", "Input"}), cellShape, {1});
    DataArray<float32>::Create(dataStructure, "Input", inStore, cellAmPtr->getId());
    // int32 mask with the SAME tuple count, so only the type guard fires (not the tuple guard).
    auto* maskAmPtr = AttributeMatrix::Create(dataStructure, "MaskData", cellShape, imageGeomPtr->getId());
    auto maskStore = data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"Image Geometry", "MaskData", "Mask"}), cellShape, {1});
    DataArray<int32>::Create(dataStructure, "Mask", maskStore, maskAmPtr->getId());

    auto result = filter.preflight(dataStructure, makeArgs());
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == -8100);
  }

  SECTION("mask/input tuple mismatch -> -8101")
  {
    DataStructure dataStructure;
    const ShapeType cellShape = {2, 2, 2};
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
    imageGeomPtr->setDimensions({2, 2, 2});
    auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
    imageGeomPtr->setCellData(*cellAmPtr);
    auto inStore = data_store_utilities::CreateDataStore<float32>(dataStructure, DataPath({"Image Geometry", "CellData", "Input"}), cellShape, {1});
    DataArray<float32>::Create(dataStructure, "Input", inStore, cellAmPtr->getId());
    // uint8 mask (valid type) but with a DIFFERENT tuple count (4 != 8), so the tuple guard fires.
    const ShapeType maskShape = {4};
    auto* maskAmPtr = AttributeMatrix::Create(dataStructure, "MaskData", maskShape, imageGeomPtr->getId());
    auto maskStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, DataPath({"Image Geometry", "MaskData", "Mask"}), maskShape, {1});
    DataArray<uint8>::Create(dataStructure, "Mask", maskStore, maskAmPtr->getId());

    auto result = filter.preflight(dataStructure, makeArgs());
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == -8101);
  }
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the ITK-sourced real-image md5 golden cases below. Mirrors ITKMaskImageTest.cpp: read the ITK case's
// primary input AND its mask image through OUR ITK-free readers (each into its own geometry, as the ITK test does),
// require their dimensions match, run OUR MaskImageFilter (SameAsInput output) with the mask wired via
// k_MaskImageDataPath_Key, then apply the (A) md5-validity-first golden (plan Sec.4). The cthead1 case's primary input
// is a .mha (MHA reader delivered). Pinned ForceInCore.
// -----------------------------------------------------------------------------
void RunMaskItkGolden(const std::string& inputFile, const std::string& maskFile, const std::string& committedMd5)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");
  const DataPath maskGeom({"Mask Geometry"});
  const DataPath mask = maskGeom.createChildPath("CellData").createChildPath("Mask");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  const auto readMask = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath(maskFile), maskGeom, "CellData", "Mask");
  SIMPLNX_RESULT_REQUIRE_VALID(readMask);
  REQUIRE(dataStructure.getDataRefAs<ImageGeom>(geom).getDimensions() == dataStructure.getDataRefAs<ImageGeom>(maskGeom).getDimensions());

  Arguments args;
  args.insertOrAssign(MaskImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(MaskImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(MaskImageFilter::k_MaskImageDataPath_Key, std::make_any<DataPath>(mask));
  args.insertOrAssign(MaskImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  MaskImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(dataStructure, output);
  INFO(fmt::format("MaskImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("NXImageProcessing::MaskImageFilter: ITK real-image golden (2d)", "[NXImageProcessing][ItkGolden][MaskImageFilter]")
{
  RunMaskItkGolden("STAPLE1.png", "STAPLE2.png", "c57d7fda3e42374881c3c3181d15bf90");
}

// MHA-input case (plan Sec.5 "MHA rung"): the primary input cthead1-Float.mha is read via the delivered ReadMhaFileFilter.
TEST_CASE("NXImageProcessing::MaskImageFilter: ITK real-image golden (cthead1)", "[NXImageProcessing][ItkGolden][MaskImageFilter]")
{
  RunMaskItkGolden("cthead1-Float.mha", "cthead1-mask.png", "0ef8943803bb4a21b2015b53f0164f1c");
}

// SKIPPED ITK Mask cases (plan Sec.5, no silent drop):
//  - `rgb` (VM1111Shrink-RGB.png + VM1111Shrink-mask.png): RGB/vector input, out of scope for the scalar port.
//  - `cthead1_maskvalue` (cthead1.png + 2th_cthead1.mha, md5 3eb703113d03f38e7b8db4b180079a39): this case is DISABLED
//    in the ITK source (`#if 0`) because it needs a "masking value" that the original ITK filter does not expose, so
//    its committed hash was never a validated golden. Not ported.
