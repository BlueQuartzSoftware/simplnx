#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"
#include "WatershedPreflightTestUtils.hpp"

#include "ImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

// solely to compute the byte-exact reference for the new ITK-free flood engine (the legacy NX wrapper for this
// filter is disabled/broken, so there is no runtime-createable legacy filter to compare against).

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <atomic>
#include <cctype>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using WSFilter = MorphologicalWatershedFromMarkersImageFilter;

template <class T>
Arguments createMarkerLineArguments(DataStructure& dataStructure, const std::vector<T>& markers, bool markWatershedLine, bool fullyConnected,
                                    std::shared_ptr<AbstractDataStore<float32>> inputStorePtr = {}, std::shared_ptr<AbstractDataStore<T>> markerStorePtr = {})
{
  REQUIRE_FALSE(markers.empty());
  const ShapeType tupleShape{1, 1, markers.size()};
  const ShapeType componentShape{1};
  const DataPath geometryPath({"Image Geometry"});
  const auto cellPath = geometryPath.createChildPath("CellData");
  const auto inputPath = cellPath.createChildPath("Input");
  const auto markerPath = cellPath.createChildPath("Marker");
  auto* geometryPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({markers.size(), 1, 1});
  geometryPtr->setSpacing({1.0F, 1.0F, 1.0F});
  geometryPtr->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), tupleShape, geometryPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  geometryPtr->setCellData(*cellDataPtr);

  if(inputStorePtr == nullptr)
  {
    inputStorePtr = DataStoreUtilities::CreateDataStore<float32>(dataStructure, inputPath, tupleShape, componentShape);
  }
  if(markerStorePtr == nullptr)
  {
    markerStorePtr = DataStoreUtilities::CreateDataStore<T>(dataStructure, markerPath, tupleShape, componentShape);
  }
  REQUIRE(inputStorePtr != nullptr);
  REQUIRE(markerStorePtr != nullptr);
  REQUIRE(inputStorePtr->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(markerStorePtr->getStoreType() == IDataStore::StoreType::InMemory);
  auto* inputArrayPtr = Float32Array::Create(dataStructure, inputPath.getTargetName(), inputStorePtr, cellDataPtr->getId());
  auto* markerArrayPtr = DataArray<T>::Create(dataStructure, markerPath.getTargetName(), markerStorePtr, cellDataPtr->getId());
  REQUIRE(inputArrayPtr != nullptr);
  REQUIRE(markerArrayPtr != nullptr);
  for(usize index = 0; index < markers.size(); ++index)
  {
    inputStorePtr->setValue(index, 0.0F);
    markerStorePtr->setValue(index, markers.at(index));
  }
  REQUIRE(inputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(markerArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(inputArrayPtr->getComponentShape() == componentShape);
  REQUIRE(markerArrayPtr->getComponentShape() == componentShape);

  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  return args;
}

Arguments createSinglePixelMarkerArguments(DataStructure& dataStructure, uint64 marker, bool markWatershedLine, bool fullyConnected)
{
  return createMarkerLineArguments<uint64>(dataStructure, {marker}, markWatershedLine, fullyConnected);
}

void requireSinglePixelMarkerPreflight(const WSFilter& filter, const DataStructure& dataStructure, const Arguments& args)
{
  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  REQUIRE(result.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(result.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::uint64);
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 1});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
}

void requireSinglePixelMarkerOutput(const DataStructure& dataStructure)
{
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::uint64);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  REQUIRE(outputArrayPtr->getStoreType() == IDataStore::StoreType::InMemory);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

template <class T>
void requireMarkerLine(const std::vector<T>& markers, const std::vector<T>& expected, ChoicesParameter::ValueType handling, bool markWatershedLine, bool fullyConnected, usize changedCount = 0)
{
  DataStructure dataStructure;
  auto args = createMarkerLineArguments<T>(dataStructure, markers, markWatershedLine, fullyConnected);
  args.insertOrAssign(WSFilter::k_MarkerIdConversionHandling_Key, std::make_any<ChoicesParameter::ValueType>(handling));
  const WSFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<T>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, markers.size()});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  bool enteredFlood = false;
  const IFilter::MessageHandler handler{[&enteredFlood](const IFilter::Message& message) { enteredFlood |= message.message == "Computing Watershed From Markers"; }};
  const auto result = filter.execute(dataStructure, args, nullptr, handler);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
  if(handling == 0 && changedCount != 0)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors().front().code == -79073);
    REQUIRE(result.result.errors().front().message.find("Image Geometry/CellData/Marker") != std::string::npos);
    REQUIRE_FALSE(enteredFlood);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(enteredFlood);
  REQUIRE(result.result.warnings().size() == (changedCount == 0 ? 0 : 1));
  if(changedCount != 0)
  {
    REQUIRE(result.result.warnings().front().code == 79074);
    REQUIRE(result.result.warnings().front().message.find(std::to_string(changedCount) + " changed marker occurrence(s)") != std::string::npos);
  }
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const IDataArray* outputBasePtr = nullptr;
  REQUIRE_NOTHROW(outputBasePtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputBasePtr->getDataType() == GetDataType<T>());
  REQUIRE(outputBasePtr->getTupleShape() == ShapeType{1, 1, markers.size()});
  REQUIRE(outputBasePtr->getComponentShape() == ShapeType{1});
  REQUIRE(outputBasePtr->getStoreType() == IDataStore::StoreType::InMemory);
  const DataArray<T>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<T>>(outputPath));
  REQUIRE(expected.size() == markers.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(outputArrayPtr->getDataStoreRef().getValue(index) == expected.at(index));
  }
}

template <class T>
class MarkerTransferStore : public DataStore<T>
{
public:
  explicit MarkerTransferStore(usize size)
  : DataStore<T>(ShapeType{1, 1, size}, ShapeType{1}, T{})
  {
  }
  ~MarkerTransferStore() noexcept override = default;
  MarkerTransferStore(const MarkerTransferStore&) = delete;
  MarkerTransferStore(MarkerTransferStore&&) = delete;
  MarkerTransferStore& operator=(const MarkerTransferStore&) = delete;
  MarkerTransferStore& operator=(MarkerTransferStore&&) = delete;

  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    m_LargestRead = std::max(m_LargestRead, buffer.size());
    if(m_ReadCount == m_FailRead)
    {
      if(m_ThrowAllocationFailure)
      {
        throw std::bad_alloc{};
      }
      return MakeErrorResult(-1874151, "Injected marker-watershed read failure.");
    }
    auto result = DataStore<T>::copyIntoBuffer(start, buffer);
    if(m_ReadCount == m_CancelAfterRead && m_ShouldCancelPtr != nullptr)
    {
      *m_ShouldCancelPtr = true;
    }
    if(m_WarnOnRead)
    {
      result = MergeResults(std::move(result), MakeWarningVoidResult(1874153, "Delivered marker-watershed read warning."));
    }
    return result;
  }
  Result<> copyFromBuffer(usize start, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    if(m_WriteCount == m_FailWrite)
    {
      return MakeErrorResult(-1874152, "Injected marker-watershed write failure.");
    }
    auto result = DataStore<T>::copyFromBuffer(start, buffer);
    if(m_WarnOnWrite)
    {
      result = MergeResults(std::move(result), MakeWarningVoidResult(1874154, "Delivered marker-watershed write warning."));
    }
    return result;
  }
  void failRead(usize call, bool throwAllocationFailure = false) noexcept
  {
    m_FailRead = call;
    m_ThrowAllocationFailure = throwAllocationFailure;
  }
  void failWrite(usize call) noexcept
  {
    m_FailWrite = call;
  }
  void cancelAfterRead(usize call, std::atomic_bool& shouldCancel) noexcept
  {
    m_CancelAfterRead = call;
    m_ShouldCancelPtr = &shouldCancel;
  }
  void warnOnRead() noexcept
  {
    m_WarnOnRead = true;
  }
  void warnOnWrite() noexcept
  {
    m_WarnOnWrite = true;
  }
  usize readCount() const noexcept
  {
    return m_ReadCount;
  }
  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }
  usize largestRead() const noexcept
  {
    return m_LargestRead;
  }

private:
  mutable usize m_ReadCount = 0;
  mutable usize m_LargestRead = 0;
  usize m_WriteCount = 0;
  usize m_FailRead = 0;
  usize m_FailWrite = 0;
  bool m_ThrowAllocationFailure = false;
  usize m_CancelAfterRead = 0;
  std::atomic_bool* m_ShouldCancelPtr = nullptr;
  bool m_WarnOnRead = false;
  bool m_WarnOnWrite = false;
};

//------------------------------------------------------------------------------
// A deterministic MARKER (seed) field over the grid, exercising the interesting cases the flood must reproduce:
//  - interior seeds of distinct labels (1, 2),
//  - a seed touching the image border at x==0 (label 3),
//  - two ADJACENT seeds of DIFFERENT labels feeding one region (labels 4, 5) -> a collision / watershed line,
//  - a seed whose label == numeric_limits<TLabel>::max() (the sentinel-collision case: a real label equal to the
//    out-of-bounds border sentinel; the uint32 flood must still match ITK's native-TLabel flood here),
//  - (3D only) a seed in the top z-plane (label 6).
// Requires dimX,dimY >= 8. 0 = unmarked.
template <class TLabel>
std::vector<TLabel> MakeMarkerField(usize dx, usize dy, usize dz)
{
  std::vector<TLabel> m(dx * dy * dz, TLabel{0});
  auto set = [&](usize x, usize y, usize z, TLabel label) {
    if(x < dx && y < dy && z < dz)
    {
      m[rt::FlatIndex(x, y, z, dx, dy)] = label;
    }
  };
  set(1, 1, 0, TLabel{1});               // interior seed A
  set(dx - 2, dy - 2, 0, TLabel{2});     // interior seed B
  set(0, dy / 2, 0, TLabel{3});          // border-touching seed (x==0)
  set(dx / 2, dy / 2, 0, TLabel{4});     // two ADJACENT seeds of ...
  set(dx / 2 + 1, dy / 2, 0, TLabel{5}); // ... different labels feeding one region
  // Sentinel-collision seed: for marker types whose max fits in uint32 (int8..uint32), the type max IS the border
  // sentinel and both ITK (native-TLabel flood) and the uint32 flood collide on it identically -- an EXACT sentinel
  // test, label value included. For 64-bit types the type max exceeds the uint32 flood's documented exact domain
  // (labels < 2^32) and would truncate/diverge, so use a large IN-DOMAIN label (< 2^32, no sentinel collision, still
  // exact vs ITK). This keeps the parity gate EXACT for every marker type while covering the sentinel collision
  // everywhere it is representable.
  const TLabel sentinelSeed = (static_cast<uint64>(std::numeric_limits<TLabel>::max()) <= 0xFFFFFFFFull) ? std::numeric_limits<TLabel>::max() : static_cast<TLabel>(0xFFFFFFFEu);
  set(2, dy - 2, 0, sentinelSeed);
  if(dz > 1)
  {
    set(dx - 2, 1, dz - 1, TLabel{6}); // 3D seed in the top z-plane
  }
  return m;
}

//------------------------------------------------------------------------------
// Build "Image Geometry"/"CellData" with a scalar grayscale array "Input" (type TInput) and a scalar marker array
// "Marker" (type TLabel) sharing the grid. Returns {grayscalePath, markerPath}.
template <class TInput, class TLabel>
std::pair<DataPath, DataPath> BuildGrayscaleAndMarker(DataStructure& ds, usize dx, usize dy, usize dz, const std::vector<TInput>& gray, const std::vector<TLabel>& markers)
{
  const DataPath inputPath = rt::BuildImageFromPattern<TInput>(ds, dx, dy, dz, gray);

  const ShapeType cellShape = {dz, dy, dx};
  const DataPath markerPath({"Image Geometry", "CellData", "Marker"});
  auto markerStore = DataStoreUtilities::CreateDataStore<TLabel>(ds, markerPath, cellShape, {1});
  const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
  auto* markerArray = DataArray<TLabel>::Create(ds, "Marker", markerStore, cellAM.getId());
  auto& ref = markerArray->getDataStoreRef();
  constexpr usize k_ChunkValues = 65536;
  for(usize start = 0; start < markers.size(); start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, markers.size() - start);
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const TLabel>(markers.data() + start, count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  return {inputPath, markerPath};
}

// Set the standard keys + the two bool params, run preflight + execute, require both succeed.
void RunWatershed(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const DataPath& markerPath, bool markWatershedLine, bool fullyConnected, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A signed marker field using NEGATIVE label values (plus one positive, for a mixed-sign collision), to exercise the
// marker widen->uint32->narrow round-trip for negatives (static_cast<uint32>(int8{-1}) == 0xFFFFFFFF, narrowed back
// to -1). None of the chosen labels equal the per-type border sentinel (NumericTraits<TLabel>::max()), so there is no
// sentinel collision here -- purely the negative round-trip. Requires dx,dy >= 4. 0 = unmarked.
template <class TLabel>
std::vector<TLabel> MakeNegativeMarkerField(usize dx, usize dy, usize dz)
{
  static_assert(std::is_signed_v<TLabel>, "MakeNegativeMarkerField requires a signed marker type.");
  std::vector<TLabel> m(dx * dy * dz, TLabel{0});
  auto set = [&](usize x, usize y, usize z, TLabel v) {
    if(x < dx && y < dy && z < dz)
    {
      m[rt::FlatIndex(x, y, z, dx, dy)] = v;
    }
  };
  set(1, 1, 0, static_cast<TLabel>(-1));               // interior negative seed
  set(dx - 2, dy - 2, 0, static_cast<TLabel>(-2));     // interior negative seed
  set(0, dy / 2, 0, static_cast<TLabel>(-3));          // border-touching negative seed (x==0)
  set(dx / 2, dy / 2, 0, static_cast<TLabel>(4));      // a POSITIVE seed and ...
  set(dx / 2 + 1, dy / 2, 0, static_cast<TLabel>(-4)); // ... an adjacent NEGATIVE seed -> a mixed-sign collision/line
  if(dz > 1)
  {
    set(dx - 2, 1, dz - 1, static_cast<TLabel>(-5)); // 3D negative seed in the top z-plane
  }
  return m;
}
} // namespace

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker conversion policy defaults to Error", "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const WSFilter filter;
  REQUIRE(filter.parametersVersion() == 2);
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "marker_id_conversion_handling_index";

  // The public schema can fail safely before any image allocation or future parameter-symbol lookup.
  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE(policyPtr != nullptr);
  REQUIRE(policyPtr->humanName() == "Marker ID Conversion Handling");
  REQUIRE(policyPtr->choices() == ChoicesParameter::Choices{"Error", "Warn and Continue (Legacy Behavior)"});
  REQUIRE(policyPtr->defaultIndex() == 0);
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE(defaultArguments.value<ChoicesParameter::ValueType>(policyKey) == 0);
  std::string tooltip = policyPtr->helpText();
  std::ranges::transform(tooltip, tooltip.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  for(const auto* termPtr : {"default", "before", "flood", "uint32", "seed", "merge", "boundaries", "region", "measurements", "4294967295", "4294967296", "4294967297", "-1"})
  {
    INFO(termPtr);
    REQUIRE(tooltip.find(termPtr) != std::string::npos);
  }
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: default Error rejects a changed uint64 marker before flooding",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(markWatershedLine, fullyConnected);
  DataStructure dataStructure;
  // Unsigned narrowing is defined: legacy uint64 4294967297 becomes uint32 1. No large ID-sized allocation is needed.
  const Arguments args = createSinglePixelMarkerArguments(dataStructure, uint64{4294967297ULL}, markWatershedLine, fullyConnected);
  const WSFilter filter;
  requireSinglePixelMarkerPreflight(filter, dataStructure, args);
  bool enteredFlood = false;
  const IFilter::MessageHandler messageHandler{[&enteredFlood](const IFilter::Message& message) {
    if(message.message == "Computing Watershed From Markers")
    {
      enteredFlood = true;
    }
  }};
  // Omit the new choice deliberately: existing pipelines must receive the safe default.
  const auto result = filter.execute(dataStructure, args, nullptr, messageHandler);
  requireSinglePixelMarkerOutput(dataStructure);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -79073);
  REQUIRE(result.result.errors().front().message.find("Marker") != std::string::npos);
  REQUIRE(result.result.errors().front().message.find("4294967297") != std::string::npos);
  REQUIRE_FALSE(enteredFlood);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: UINT32_MAX marker remains valid with uint64 output",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(markWatershedLine, fullyConnected);
  DataStructure dataStructure;
  // A seeded one-pixel image keeps its seed. The valid upper boundary is literal, independent of the conversion helper.
  const Arguments args = createSinglePixelMarkerArguments(dataStructure, uint64{4294967295ULL}, markWatershedLine, fullyConnected);
  const WSFilter filter;
  requireSinglePixelMarkerPreflight(filter, dataStructure, args);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  requireSinglePixelMarkerOutput(dataStructure);
  const UInt64Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<UInt64Array>(DataPath({"Image Geometry", "CellData", "Output"})));
  REQUIRE(outputArrayPtr->getDataStoreRef().getValue(0) == uint64{4294967295ULL});
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: native lossless seeds propagate exactly",
                   "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  std::vector<TestType> seeds{0, 1};
  if constexpr(sizeof(TestType) <= sizeof(uint32))
  {
    seeds.push_back(std::numeric_limits<TestType>::max());
    if constexpr(std::is_signed_v<TestType>)
    {
      seeds.push_back(std::numeric_limits<TestType>::lowest());
      seeds.push_back(-1);
    }
  }
  else
  {
    seeds.push_back(TestType{4294967295ULL});
  }
  for(const TestType seed : seeds)
  {
    CAPTURE(seed, handling, markWatershedLine, fullyConnected);
    requireMarkerLine<TestType>({seed, 0, 0}, {seed, seed, seed}, handling, markWatershedLine, fullyConnected);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: wide marker bounds have literal legacy results",
                   "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  requireMarkerLine<TestType>({TestType{4294967296ULL}, 0, 0}, {0, 0, 0}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<TestType>({TestType{4294967297ULL}, 0, 0}, {1, 1, 1}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<TestType>({std::numeric_limits<TestType>::max(), 0, 0}, {TestType{4294967295ULL}, TestType{4294967295ULL}, TestType{4294967295ULL}}, handling, markWatershedLine, fullyConnected,
                              1);
  if constexpr(std::is_signed_v<TestType>)
  {
    requireMarkerLine<TestType>({-1, 0, 0}, {TestType{4294967295ULL}, TestType{4294967295ULL}, TestType{4294967295ULL}}, handling, markWatershedLine, fullyConnected, 1);
    requireMarkerLine<TestType>({std::numeric_limits<TestType>::lowest(), 0, 0}, {0, 0, 0}, handling, markWatershedLine, fullyConnected, 1);
  }
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: alias and lost seed choices have exact flood consequences",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  requireMarkerLine<uint64>({1, 0, 4294967297ULL}, {1, 1, 1}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<uint64>({4294967296ULL, 0, 9}, {9, 9, 9}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<uint64>({4294967297ULL, 0, 4294967297ULL}, {1, 1, 1}, handling, markWatershedLine, fullyConnected, 2);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker copy checks chunk boundaries before writing",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize changedIndex = GENERATE(usize{65535}, usize{65536}, usize{65537});
  const auto handling = GENERATE(ImageProcessing::MarkerIdConversionHandling::Error, ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  constexpr usize valueCount = 65538;
  const auto markerStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  std::vector<uint64> markers(valueCount, 0);
  markers.at(changedIndex) = 4294967297ULL;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, markerStorePtr);
  MarkerTransferStore<uint32> destination(valueCount);
  std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel, handling);
  const std::string* messagePtr = nullptr;
  if(handling == ImageProcessing::MarkerIdConversionHandling::Error)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -79073);
    REQUIRE(destination.writeCount() == changedIndex / 65536);
    messagePtr = &result.errors().front().message;
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings().front().code == 79074);
    REQUIRE(destination.writeCount() == 2);
    REQUIRE(destination.getValue(changedIndex) == 1);
    messagePtr = &result.warnings().front().message;
  }
  REQUIRE(messagePtr->find("flat index " + std::to_string(changedIndex)) != std::string::npos);
  REQUIRE(messagePtr->find("native ID 4294967297 restores as 1") != std::string::npos);
  REQUIRE(messagePtr->find("1 changed marker occurrence(s)") != std::string::npos);
  REQUIRE(markerStorePtr->readCount() == (handling == ImageProcessing::MarkerIdConversionHandling::Error ? changedIndex / 65536 + 1 : 2));
  REQUIRE(markerStorePtr->largestRead() <= 65536);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: invalid policy is rejected before execution", "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  auto args = createSinglePixelMarkerArguments(dataStructure, 1, true, false);
  args.insertOrAssign(WSFilter::k_MarkerIdConversionHandling_Key, std::make_any<ChoicesParameter::ValueType>(2));
  const WSFilter filter;
  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == FilterParameter::Constants::k_Validate_OutOfRange_Error);
  REQUIRE(result.outputActions.errors().front().message.find("Marker ID Conversion Handling") != std::string::npos);
  REQUIRE(dataStructure.getData(DataPath({"Image Geometry", "CellData", "Output"})) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: diagnostic context keeps exact 64-bit extrema",
                   "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  createMarkerLineArguments<TestType>(dataStructure, {0, std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::max()}, true, false);
  MarkerTransferStore<uint32> destination(3);
  const auto handling = GENERATE(ImageProcessing::MarkerIdConversionHandling::Error, ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel, handling);
  const std::string* messagePtr = nullptr;
  if(handling == ImageProcessing::MarkerIdConversionHandling::Error)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -79073);
    REQUIRE(destination.writeCount() == 0);
    messagePtr = &result.errors().front().message;
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings().front().code == 79074);
    REQUIRE(result.warnings().front().message.find("2 changed marker occurrence(s)") != std::string::npos);
    messagePtr = &result.warnings().front().message;
  }
  const std::string nativeText = std::is_signed_v<TestType> ? "9223372036854775807" : "18446744073709551615";
  REQUIRE(messagePtr->find("native ID " + nativeText + " restores as 4294967295") != std::string::npos);
  REQUIRE(messagePtr->find("First change at flat index 1") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: copy cancellation retains observed warning only",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valueCount = 65537;
  const auto markerStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  DataStructure dataStructure;
  std::vector<uint64> markers(valueCount, 0);
  markers.at(0) = 4294967297ULL;
  markers.at(valueCount - 1) = 4294967296ULL;
  createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, markerStorePtr);
  MarkerTransferStore<uint32> destination(valueCount);
  std::atomic_bool shouldCancel = false;
  markerStorePtr->cancelAfterRead(1, shouldCancel);
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel,
                                                                         ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel.load());
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == 79074);
  REQUIRE(result.warnings().front().message.find("1 changed marker occurrence(s)") != std::string::npos);
  REQUIRE(markerStorePtr->readCount() == 1);
  REQUIRE(destination.writeCount() == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: copy warning survives transfer failure or allocation failure",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valueCount = 65537;
  const auto markerStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  std::vector<uint64> markers(valueCount, 0);
  markers.at(0) = 4294967297ULL;
  markers.at(valueCount - 1) = 4294967296ULL;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, markerStorePtr);
  MarkerTransferStore<uint32> destination(valueCount);
  int32 expectedCode = 0;
  SECTION("later marker read fails")
  {
    markerStorePtr->failRead(2);
    expectedCode = -1874151;
  }
  SECTION("scratch chunk write fails")
  {
    destination.failWrite(1);
    expectedCode = -1874152;
  }
  SECTION("later marker read throws allocation failure")
  {
    markerStorePtr->failRead(2, true);
    expectedCode = -79045;
  }
  std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel,
                                                                         ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == expectedCode);
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == 79074);
  REQUIRE(result.warnings().front().message.find("1 changed marker occurrence(s)") != std::string::npos);
  REQUIRE(markerStorePtr->readCount() <= 2);
  REQUIRE(destination.writeCount() == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: conversion warning survives later facade outcomes",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto inputStorePtr = std::make_shared<MarkerTransferStore<float32>>(3);
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, {4294967297ULL, 0, 0}, true, false, inputStorePtr);
  const auto outputStorePtr = std::make_shared<MarkerTransferStore<uint64>>(3);
  const AttributeMatrix* cellDataPtr = nullptr;
  REQUIRE_NOTHROW(cellDataPtr = &dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})));
  REQUIRE(UInt64Array::Create(dataStructure, "Output", outputStorePtr, cellDataPtr->getId()) != nullptr);
  std::atomic_bool shouldCancel = false;
  bool cancelAtFlood = false;
  int32 expectedCode = 0;
  SECTION("grayscale read error after conversion")
  {
    inputStorePtr->failRead(1);
    expectedCode = -1874151;
  }
  SECTION("grayscale allocation failure after conversion")
  {
    inputStorePtr->failRead(1, true);
    expectedCode = -79045;
  }
  SECTION("native restoration write error after conversion")
  {
    outputStorePtr->failWrite(1);
    expectedCode = -1874152;
  }
  SECTION("facade cancellation after conversion retains valid Result warning")
  {
    cancelAtFlood = true;
  }
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    if(cancelAtFlood && message.message == "Computing Watershed From Markers")
    {
      shouldCancel = true;
    }
  }};
  // Call the facade directly: generic IFilter cancellation can replace a valid result and drop its warnings.
  const auto result = ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter(dataStructure, DataPath({"Image Geometry"}), DataPath({"Image Geometry", "CellData", "Input"}),
                                                                                           DataPath({"Image Geometry", "CellData", "Marker"}), DataPath({"Image Geometry", "CellData", "Output"}), true,
                                                                                           false, shouldCancel, handler, ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  if(cancelAtFlood)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(shouldCancel.load());
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == expectedCode);
  }
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == 79074);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: successful transfer warnings survive later marker-copy failure",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valueCount = 65537;
  const auto markerStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  std::vector<uint64> markers(valueCount, 0);
  markers.at(0) = 4294967297ULL;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, markerStorePtr);
  markerStorePtr->warnOnRead();
  markerStorePtr->failRead(2);
  MarkerTransferStore<uint32> destination(valueCount);
  destination.warnOnWrite();
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel,
                                                                         ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -1874151);
  REQUIRE(result.warnings().size() == 3);
  REQUIRE(std::ranges::count_if(result.warnings(), [](const Warning& warning) { return warning.code == 79074; }) == 1);
  REQUIRE(std::ranges::count_if(result.warnings(), [](const Warning& warning) { return warning.code == 1874153; }) == 1);
  REQUIRE(std::ranges::count_if(result.warnings(), [](const Warning& warning) { return warning.code == 1874154; }) == 1);
  REQUIRE(markerStorePtr->readCount() == 2);
  REQUIRE(destination.writeCount() == 1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: restoration retains transfer warnings through a short final chunk",
          "[ImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valueCount = 65537;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, std::vector<uint64>(valueCount, 0), true, false);
  const auto outputStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  const AttributeMatrix* cellDataPtr = nullptr;
  REQUIRE_NOTHROW(cellDataPtr = &dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})));
  REQUIRE(UInt64Array::Create(dataStructure, "Output", outputStorePtr, cellDataPtr->getId()) != nullptr);
  MarkerTransferStore<uint32> source(valueCount);
  source.setValue(0, 1);
  source.setValue(valueCount - 1, 4294967295U);
  source.warnOnRead();
  outputStorePtr->warnOnWrite();
  bool expectFailure = false;
  SECTION("all delivered warnings survive success")
  {
  }
  SECTION("earlier warnings survive final write failure")
  {
    outputStorePtr->failWrite(2);
    expectFailure = true;
  }
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::NarrowUInt32StoreToIntegerArray(source, dataStructure, DataPath({"Image Geometry", "CellData", "Output"}), shouldCancel);
  if(expectFailure)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -1874152);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(outputStorePtr->getValue(valueCount - 1) == 4294967295ULL);
  }
  REQUIRE(outputStorePtr->getValue(0) == 1);
  REQUIRE(std::ranges::count_if(result.warnings(), [](const Warning& warning) { return warning.code == 1874153; }) == 2);
  REQUIRE(std::ranges::count_if(result.warnings(), [](const Warning& warning) { return warning.code == 1874154; }) == (expectFailure ? 1 : 2));
  REQUIRE(source.readCount() == 2);
  REQUIRE(outputStorePtr->writeCount() == 2);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (4) Preflight guards: a multi-component grayscale is rejected (requireScalar); a non-integer (float32) marker is
//     rejected; a marker whose tuple count mismatches the grayscale is rejected; a valid pair creates an output of
//     the MARKER's element type (NOT the grayscale's).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight guards", "[ImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto setArgs = [](Arguments& args, const DataPath& geomPath, const DataPath& inputPath, const DataPath& markerPath) {
    args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(WSFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
    args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  };

  SECTION("multi-component grayscale rejected")
  {
    DataStructure ds;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<uint32> markers = MakeMarkerField<uint32>(8, 8, 1);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, uint32>(ds, 8, 8, 1, gray, markers);
    // Add a multi-component grayscale array and select it as the input.
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int16>(ds, vecPath, {1, 8, 8}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int16>::Create(ds, "Vec", vecStore, cellAM.getId());
    WSFilter filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), vecPath, markerPath);
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float marker rejected")
  {
    DataStructure ds;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<float32> markers(8 * 8, 0.0f);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, float32>(ds, 8, 8, 1, gray, markers);
    WSFilter filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(ds, args);
    // A float32 marker is rejected: the marker ArraySelectionParameter admits only integer scalar types, so the
    // parameter layer rejects it before preflightImpl (the filter's own -8600 integer-type guard is the defensive
    // backstop for any non-GUI caller). Either way the marker cannot be non-integer.
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  }
  SECTION("multi-component marker rejected (-8601)")
  {
    DataStructure ds;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, gray); // scalar grayscale + geometry/CellData
    // A 3-component INTEGER marker: the marker ArraySelectionParameter admits integer types with NO component-shape
    // restriction, so it passes the parameter layer and reaches preflightImpl, which rejects it with -8601. int32 is
    // integer (so the -8600 integer-type guard passes), and the tuple count matches the grayscale (so the -8602
    // tuple-mismatch guard does not fire first) -- this isolates the non-scalar-marker guard, which had no coverage.
    const DataPath markerPath({"Image Geometry", "CellData", "Marker"});
    auto markerStore = DataStoreUtilities::CreateDataStore<int32>(ds, markerPath, {1, 8, 8}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Marker", markerStore, cellAM.getId());
    WSFilter filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8601); // non-scalar (multi-component) marker
  }
  SECTION("marker tuple-count mismatch rejected")
  {
    DataStructure ds;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1); // 64 tuples
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, gray);
    // Put the marker in its OWN AttributeMatrix with a different tuple count (so the AM tuple-shape check passes but
    // the filter's grayscale-vs-marker tuple check fails).
    const auto& imageGeom = ds.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"}));
    auto* otherAM = AttributeMatrix::Create(ds, "Other", ShapeType{50}, imageGeom.getId());
    const DataPath markerPath({"Image Geometry", "Other", "Marker"});
    auto markerStore = DataStoreUtilities::CreateDataStore<uint32>(ds, markerPath, ShapeType{50}, {1});
    DataArray<uint32>::Create(ds, "Marker", markerStore, otherAM->getId());
    WSFilter filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == -8602); // tuple-count mismatch
  }
  SECTION("valid pair creates an output of the MARKER's type")
  {
    DataStructure ds;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<int8> markers = MakeMarkerField<int8>(8, 8, 1);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, int8>(ds, 8, 8, 1, gray, markers);
    WSFilter filter;
    RunWatershed(filter, ds, inputPath, markerPath, /*markWatershedLine=*/true, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::int8); // marker type, not grayscale (int16)
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight RAM-fit guard: ValidateWatershedFitsInMemory hard-errors (with the code the FromMarkers filter passes,
//     -79041) for an absurd tuple count whose working set exceeds ANY machine's RAM, WITHOUT allocating an array; a
//     small tuple count is accepted on the test machine. No specific available-RAM number is asserted -- only that an
//     absurd size is rejected and a plausibly-small size is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight RAM-fit guard", "[ImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  // ~1e11 voxels * >=11 bytes/voxel ~= 1.1 TB working set: larger than any machine's RAM, so it must be rejected with
  // the FromMarkers filter's memory code (-79041). The guard is a pure numeric estimate -- nothing is allocated. The
  // ERROR takes precedence over the OOC warning, so inputIsOutOfCore is irrelevant here.
  const Result<OutputActions> tooLarge = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(100'000'000'000ULL), /*inputIsOutOfCore=*/true, -79041, -79043);
  SIMPLNX_RESULT_REQUIRE_INVALID(tooLarge);
  REQUIRE(tooLarge.errors().front().code == -79041);

  // A tiny volume trivially fits on any test machine. In-core input: accepted with NO warning (RAM is the expected
  // backing -- no surprise to report).
  const Result<OutputActions> tinyInCore = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/false, -79041, -79043);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyInCore);
  REQUIRE(tinyInCore.warnings().empty());

  // Same tiny volume, but the input is stored out-of-core: still VALID (it fits), yet now carries the OOC-override
  // warning with the FromMarkers filter's warning code (-79043) -- watershed will load its working set into RAM regardless.
  const Result<OutputActions> tinyOoc = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/true, -79041, -79043);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyOoc);
  REQUIRE(tinyOoc.warnings().size() == 1);
  REQUIRE(tinyOoc.warnings().front().code == -79043);
}

// -----------------------------------------------------------------------------
// (7) Marker value 0 is UNMARKED (flooded), not a distinct background seed. A 0 in the marker image means "flood me",
//     so a 0 punched into the middle of an otherwise-labeled block is flooded by the enclosing basin (not left as 0
//     and not treated as its own label). Gated EXACTLY against direct ITK, plus a direct assertion that the punched
//     center took the enclosing label.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker value 0 is unmarked (flooded)", "[ImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(markWatershedLine, fullyConnected);

  constexpr usize dx = 12, dy = 12, dz = 1;
  const std::vector<int16> gray = rt::MakePlateauPattern<int16>(dx, dy, dz);
  // A solid 3x3 block of label 5 with a single 0 hole punched at its center (2,2), plus a distant second seed. The
  // large remaining 0 field is likewise unmarked -> flooded between the two basins.
  std::vector<uint32> markers(dx * dy * dz, 0u);
  auto set = [&](usize x, usize y, uint32 v) { markers[rt::FlatIndex(x, y, 0, dx, dy)] = v; };
  for(usize yy = 1; yy <= 3; ++yy)
  {
    for(usize xx = 1; xx <= 3; ++xx)
    {
      set(xx, yy, 5u);
    }
  }
  set(2, 2, 0u);           // the 0 in the middle of the block (fully enclosed by label 5)
  set(dx - 2, dy - 2, 9u); // a second, distant seed

  // Directly confirm the punched 0 voxel was flooded to the enclosing basin (label 5), i.e. treated as unmarked --
  // never left at 0 and never a distinct seed. Enclosed on all four sides by label 5, so it is not a watershed line
  // regardless of MarkWatershedLine.
  DataStructure ds;
  const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, uint32>(ds, dx, dy, dz, gray, markers);
  WSFilter filter;
  RunWatershed(filter, ds, inputPath, markerPath, markWatershedLine, fullyConnected);
  const auto& store = ds.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})).getIDataStoreRefAs<AbstractDataStore<uint32>>();
  REQUIRE(store.getValue(rt::FlatIndex(2, 2, 0, dx, dy)) == 5u);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- MorphologicalWatershedFromMarkers duplicates the INPUTS of
// ITKMorphologicalWatershedFromMarkersImageTest.cpp (cthead1-grad-mag.nrrd grayscale + cthead1-marker.png marker), but
// NOT its committed md5.
//
// The durable golden is the md5 of our uint8 output. That md5 equals the ITK test's committed hash
// c32759ecb10cbff555c750b8b7a8d32e. A direct itk:: oracle validated this output bit-exact before the
// project removed ITK. The hash keeps that evidence without a call to ITK.
// Output element type == the marker's type (uint8). Pinned ForceInCore.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath markerGeom({"Marker Geometry"});
  const DataPath marker = markerGeom.createChildPath("CellData").createChildPath("Marker");
  const DataPath output({"Image Geometry", "CellData", "Output"});

  // Read the grayscale and the marker into DISTINCT geometries (our readers each create their own geom) -- unlike the
  // broken ITK test that overwrote the grayscale with the marker. The filter only requires the marker to be an integer
  // scalar array with a matching tuple count (checked below), not that it share the input's geometry.
  // NOTE: bind each side-effecting read to a named local (SIMPLNX_RESULT_REQUIRE_VALID double-evaluates its argument;
  // inlining a ds-mutating read would run it twice and the second would fail on the already-created geometry).
  DataStructure ds;
  const Result<> readGray = ip_golden::ReadInputImage(ds, ip_golden::InputPath("cthead1-grad-mag.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readGray);
  const Result<> readMarker = ip_golden::ReadInputImage(ds, ip_golden::InputPath("cthead1-marker.png"), markerGeom, "CellData", "Marker");
  SIMPLNX_RESULT_REQUIRE_VALID(readMarker);

  const auto& grayArray = ds.getDataRefAs<IDataArray>(input);
  const auto& markerArray = ds.getDataRefAs<IDataArray>(marker);
  REQUIRE(grayArray.getDataType() == DataType::uint8);
  REQUIRE(markerArray.getDataType() == DataType::uint8);
  REQUIRE(markerArray.getNumberOfComponents() == 1);
  REQUIRE(markerArray.getNumberOfTuples() == grayArray.getNumberOfTuples());

  const SizeVec3 dims = ds.getDataRefAs<ImageGeom>(geom).getDimensions();
  const usize dx = dims[0];
  const usize dy = dims[1];
  const usize dz = dims[2];

  auto materialize = [](const IDataArray& arr) {
    const auto& store = arr.getIDataStoreRefAs<AbstractDataStore<uint8>>();
    std::vector<uint8> v(store.getSize());
    Result<> readResult = store.copyIntoBuffer(0, nonstd::span<uint8>(v.data(), v.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    return v;
  };
  const std::vector<uint8> gray = materialize(grayArray);
  const std::vector<uint8> markers = materialize(markerArray);

  // Run OUR filter with the ITK defaults (MarkWatershedLine true, FullyConnected false).
  WSFilter filter;
  RunWatershed(filter, ds, input, marker, /*markWatershedLine=*/true, /*fullyConnected=*/false, "Output");

  // DURABLE golden: md5 of our output. It equals ITK's committed hash (see the comment above).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  UNSCOPED_INFO("MorphologicalWatershedFromMarkers md5 = " + ourMd5);
  REQUIRE(ourMd5 == "c32759ecb10cbff555c750b8b7a8d32e");

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight rejects missing temporary provider", "[ImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  auto& collection = application->getIOCollection();
  const DataIOCollection::map_type originalManagers(collection.begin(), collection.end());
  const bool originalTemporaryCapability = collection.hasTemporaryRecordStoreCapability();
  const bool originalSortCapability = collection.hasExternalSortCapability();
  const bool originalImportCapability = collection.anyManagerFinalizesImport();
  const auto originalNumericFormats = collection.getFormatNames();
  const auto originalDisplayNames = collection.getFormatDisplayNames();
  const std::string plannedFormat = "HDF5";
  REQUIRE(collection.getManager(plannedFormat) != nullptr);
  const auto originalInMemoryManager = collection.getManager(Preferences::k_InMemoryFormat.str());
  REQUIRE(originalInMemoryManager != nullptr);

  SECTION("planned grayscale reaches the missing-provider guard")
  {
    watershed_preflight_test::ScopedMissingTemporaryProvider guard(collection);
    REQUIRE_FALSE(collection.hasTemporaryRecordStoreCapability());
    REQUIRE(collection.getManager(Preferences::k_InMemoryFormat.str()) == originalInMemoryManager);
    REQUIRE(collection.hasDataStoreCreationFunction(plannedFormat));

    DataStructure dataStructure;
    const DataPath geomPath({"Planned Image"});
    const auto cellPath = geomPath.createChildPath("CellData");
    const auto inputPath = cellPath.createChildPath("Grayscale");
    const auto markerPath = cellPath.createChildPath("Markers");
    const auto outputPath = cellPath.createChildPath("Output");
    const ShapeType tupleShape{1, 2, 3};
    const ShapeType componentShape{1};
    auto* imageGeom = ImageGeom::Create(dataStructure, geomPath.getTargetName());
    REQUIRE(imageGeom != nullptr);
    imageGeom->setDimensions({3, 2, 1});
    imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
    imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
    auto* cellData = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), tupleShape, imageGeom->getId());
    REQUIRE(cellData != nullptr);
    imageGeom->setCellData(*cellData);

    // HDF5 identifies a registered metadata-only format here. Neither input contains HDF5 values.
    auto inputStoreResult = EmptyDataStore<int16>::Create(tupleShape, componentShape, plannedFormat);
    SIMPLNX_RESULT_REQUIRE_VALID(inputStoreResult);
    const auto* inputStore = inputStoreResult.value().get();
    auto* inputArray = Int16Array::Create(dataStructure, inputPath.getTargetName(), std::move(inputStoreResult.value()), cellData->getId());
    REQUIRE(inputArray != nullptr);
    auto markerStoreResult = EmptyDataStore<uint32>::Create(tupleShape, componentShape, "");
    SIMPLNX_RESULT_REQUIRE_VALID(markerStoreResult);
    const auto* markerStore = markerStoreResult.value().get();
    auto* markerArray = UInt32Array::Create(dataStructure, markerPath.getTargetName(), std::move(markerStoreResult.value()), cellData->getId());
    REQUIRE(markerArray != nullptr);

    const auto checkInputs = [&] {
      REQUIRE(dataStructure.getDataAs<Int16Array>(inputPath) == inputArray);
      REQUIRE(inputArray->getIDataStore() == inputStore);
      REQUIRE(inputArray->getDataType() == DataType::int16);
      REQUIRE(inputArray->getTupleShape() == tupleShape);
      REQUIRE(inputArray->getComponentShape() == componentShape);
      REQUIRE(inputArray->getStoreType() == IDataStore::StoreType::Empty);
      REQUIRE(inputStore->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
      REQUIRE(inputArray->getDataFormat() == plannedFormat);
      REQUIRE(dataStructure.getDataAs<UInt32Array>(markerPath) == markerArray);
      REQUIRE(markerArray->getIDataStore() == markerStore);
      REQUIRE(markerArray->getDataType() == DataType::uint32);
      REQUIRE(markerArray->getTupleShape() == tupleShape);
      REQUIRE(markerArray->getComponentShape() == componentShape);
      REQUIRE(markerArray->getStoreType() == IDataStore::StoreType::Empty);
      REQUIRE(markerStore->getPlannedStoreType() == IDataStore::StoreType::InMemory);
      REQUIRE(markerArray->getDataFormat().empty());
    };
    checkInputs();
    REQUIRE(dataStructure.getData(outputPath) == nullptr);
    const auto originalIds = dataStructure.getAllDataObjectIds();

    WSFilter filter;
    Arguments args;
    args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(WSFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
    args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputPath.getTargetName()));
    const auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().size() == 1);
    REQUIRE(result.outputActions.errors().front().code == -79047);
    REQUIRE(result.outputActions.errors().front().message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(result.outputActions.warnings().empty());
    REQUIRE(dataStructure.getData(outputPath) == nullptr);
    REQUIRE(dataStructure.getAllDataObjectIds() == originalIds);
    checkInputs();
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("manager slots restore after a controlled throw")
  {
    struct ControlledUnwind
    {
    };
    bool reachedGuardedBody = false;
    bool caughtControlledThrow = false;
    try
    {
      watershed_preflight_test::ScopedMissingTemporaryProvider guard(collection);
      REQUIRE_FALSE(collection.hasTemporaryRecordStoreCapability());
      REQUIRE(collection.getManager(Preferences::k_InMemoryFormat.str()) == originalInMemoryManager);
      reachedGuardedBody = true;
      throw ControlledUnwind{};
    } catch(const ControlledUnwind&)
    {
      caughtControlledThrow = true;
    }
    REQUIRE(reachedGuardedBody);
    REQUIRE(caughtControlledThrow);
  }

  const DataIOCollection::map_type restoredManagers(collection.begin(), collection.end());
  REQUIRE(restoredManagers == originalManagers);
  REQUIRE(collection.hasTemporaryRecordStoreCapability() == originalTemporaryCapability);
  REQUIRE(collection.hasExternalSortCapability() == originalSortCapability);
  REQUIRE(collection.anyManagerFinalizesImport() == originalImportCapability);
  REQUIRE(collection.getFormatNames() == originalNumericFormats);
  REQUIRE(collection.getFormatDisplayNames() == originalDisplayNames);
}
