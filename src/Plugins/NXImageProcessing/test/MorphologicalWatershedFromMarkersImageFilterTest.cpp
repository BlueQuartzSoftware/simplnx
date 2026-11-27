#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"
#include "WatershedFromMarkersTestUtils.hpp"
#include "WatershedPreflightTestUtils.hpp"

#include "NXImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"
#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"
#include "NXImageProcessing/utils/WatershedMarkerConversion.hpp"

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
#include <nlohmann/json.hpp>
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

Arguments watershed_marker_test::CreateArguments(const DataPath& inputPath, const DataPath& markerPath, bool markWatershedLine, bool fullyConnected, const std::string& outputName)
{
  using WSFilter = MorphologicalWatershedFromMarkersImageFilter;
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  return args;
}

namespace
{
using watershed_marker_test::BuildGrayscaleAndMarker;
using WsFilterType = MorphologicalWatershedFromMarkersImageFilter;

// The normal filter fixture uses the same array builder as the private store tests. Resident fault
// cases replace only the participating store, before any execution read or write is observed.
template <class T>
Arguments createMarkerLineArguments(DataStructure& dataStructure, const std::vector<T>& markers, bool markWatershedLine, bool fullyConnected,
                                    std::shared_ptr<AbstractDataStore<float32>> inputStorePtr = {}, std::shared_ptr<AbstractDataStore<T>> markerStorePtr = {})
{
  REQUIRE_FALSE(markers.empty());
  const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<float32, T>(dataStructure, markers.size(), 1, 1, std::vector<float32>(markers.size(), 0.0F), markers);
  auto* input = dataStructure.getDataAs<Float32ArrayType>(inputPath);
  auto* marker = dataStructure.getDataAs<DataArray<T>>(markerPath);
  REQUIRE((input != nullptr));
  REQUIRE((marker != nullptr));
  if(inputStorePtr != nullptr)
  {
    const auto result = input->setDataStore(std::move(inputStorePtr));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  if(markerStorePtr != nullptr)
  {
    for(usize index = 0; index < markers.size(); ++index)
    {
      markerStorePtr->setValue(index, markers[index]);
    }
    const auto result = marker->setDataStore(std::move(markerStorePtr));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  REQUIRE((input->getStoreType() == IDataStore::StoreType::InMemory));
  REQUIRE((marker->getStoreType() == IDataStore::StoreType::InMemory));
  return watershed_marker_test::CreateArguments(inputPath, markerPath, markWatershedLine, fullyConnected);
}
template <class T>
void requireMarkerLine(const std::vector<T>& markers, const std::vector<T>& expected, ChoicesParameter::ValueType handling, bool markWatershedLine, bool fullyConnected, usize changedCount = 0)
{
  DataStructure dataStructure;
  auto args = createMarkerLineArguments<T>(dataStructure, markers, markWatershedLine, fullyConnected);
  args.insertOrAssign(WsFilterType::k_MarkerIdConversionHandling_Key, std::make_any<ChoicesParameter::ValueType>(handling));
  const WsFilterType filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  REQUIRE((preflightResult.output_actions.value().actions.size() == 1));
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.output_actions.value().actions.front().get());
  REQUIRE((actionPtr != nullptr));
  REQUIRE((actionPtr->type() == GetDataType<T>()));
  REQUIRE((actionPtr->dims() == ShapeType{1, 1, markers.size()}));
  REQUIRE((actionPtr->componentDims() == ShapeType{1}));
  bool enteredFlood = false;
  bool enteredExternalSeeding = false;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    enteredFlood |= message.message == "Computing Watershed From Markers";
    enteredExternalSeeding |= message.message == "Seeding Watershed Regions";
  }};
  const auto result = filter.execute(dataStructure, args, nullptr, handler);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
  if(handling == 0 && changedCount != 0)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE((result.result.errors().size() == 1));
    REQUIRE((result.result.errors().front().code == -79073));
    REQUIRE((result.result.errors().front().message.find("Image Geometry/CellData/Marker") != std::string::npos));
    REQUIRE_FALSE(enteredFlood);
    return;
  }
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(enteredFlood);
  REQUIRE_FALSE(enteredExternalSeeding);
  REQUIRE((result.result.warnings().size() == (changedCount == 0 ? 0 : 1)));
  if(changedCount != 0)
  {
    REQUIRE((result.result.warnings().front().code == -79074));
    REQUIRE((result.result.warnings().front().message.find(std::to_string(changedCount) + " changed marker occurrence(s)") != std::string::npos));
  }
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const IDataArray* outputBasePtr = nullptr;
  REQUIRE_NOTHROW(outputBasePtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE((outputBasePtr->getDataType() == GetDataType<T>()));
  REQUIRE((outputBasePtr->getTupleShape() == ShapeType{1, 1, markers.size()}));
  REQUIRE((outputBasePtr->getComponentShape() == ShapeType{1}));
  REQUIRE((outputBasePtr->getStoreType() == IDataStore::StoreType::InMemory));
  const DataArray<T>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<T>>(outputPath));
  REQUIRE((expected.size() == markers.size()));
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE((outputArrayPtr->getDataStoreRef().getValue(index) == expected.at(index)));
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
    m_Reads.emplace_back(start, buffer.size());
    if(readCount() == m_CancelAfterRead && m_ShouldCancelPtr != nullptr)
    {
      *m_ShouldCancelPtr = true;
    }
    if(readCount() == m_FailRead)
    {
      if(m_ThrowAllocationFailure)
      {
        throw std::bad_alloc{};
      }
      return MakeErrorResult(-1874151, "Injected marker-watershed read failure.");
    }
    auto result = DataStore<T>::copyIntoBuffer(start, buffer);
    if(m_WarnOnRead)
    {
      result = MergeResults(std::move(result), MakeWarningVoidResult(1874153, "Delivered marker-watershed read warning."));
    }
    return result;
  }
  Result<> copyFromBuffer(usize start, nonstd::span<const T> buffer) override
  {
    m_Writes.emplace_back(start, buffer.size());
    if(writeCount() == m_FailWrite)
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
    return m_Reads.size();
  }
  usize writeCount() const noexcept
  {
    return m_Writes.size();
  }
  using Transfers = std::vector<std::pair<usize, usize>>;
  const Transfers& reads() const noexcept
  {
    return m_Reads;
  }
  const Transfers& writes() const noexcept
  {
    return m_Writes;
  }

private:
  mutable Transfers m_Reads;
  Transfers m_Writes;
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

// Set the standard keys + the two bool params, run preflight + execute, require both succeed.
void RunWatershed(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, const DataPath& markerPath, bool markWatershedLine, bool fullyConnected,
                  const std::string& outputName = "Output")
{
  const auto args = watershed_marker_test::CreateArguments(inputPath, markerPath, markWatershedLine, fullyConnected, outputName);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
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

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker conversion policy defaults to Error", "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  const WsFilterType filter;
  REQUIRE((filter.parametersVersion() == 2));
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "marker_id_conversion_handling_index";

  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE((policyPtr != nullptr));
  REQUIRE((policyPtr->humanName() == "Marker ID Conversion Handling"));
  REQUIRE((policyPtr->choices() == ChoicesParameter::ChoicesType{"Error", "Warn and Continue (Legacy Behavior)"}));
  REQUIRE((policyPtr->defaultIndex() == 0));
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE((defaultArguments.value<ChoicesParameter::ValueType>(policyKey) == 0));
  std::string tooltip = policyPtr->helpText();
  std::ranges::transform(tooltip, tooltip.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  for(const auto* termPtr : {"default", "before", "flood", "seed", "merge", "measurements"})
  {
    INFO(termPtr);
    REQUIRE((tooltip.find(termPtr) != std::string::npos));
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: native lossless seeds propagate exactly",
                   "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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

TEMPLATE_TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: wide marker bounds have literal legacy results",
                   "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: alias and lost seed choices have exact flood consequences",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  requireMarkerLine<uint64>({1, 0, 4294967297ULL}, {1, 1, 1}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<uint64>({4294967296ULL, 0, 9}, {9, 9, 9}, handling, markWatershedLine, fullyConnected, 1);
  requireMarkerLine<uint64>({4294967297ULL, 0, 4294967297ULL}, {1, 1, 1}, handling, markWatershedLine, fullyConnected, 2);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker copy checks chunk boundaries before writing",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize changedIndex = GENERATE(usize{65535}, usize{65536}, usize{65537});
  const auto handling = GENERATE(ImageProcessing::MarkerIdConversionHandling::Error, ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  constexpr usize valueCount = 65538;
  const auto markerStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  std::vector<uint64> markers(valueCount, 0);
  markers.at(changedIndex) = 4294967297ULL;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, markerStorePtr);
  MarkerTransferStore<uint32> destination(valueCount);
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel, handling);
  const std::string* messagePtr = nullptr;
  if(handling == ImageProcessing::MarkerIdConversionHandling::Error)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE((result.errors().size() == 1));
    REQUIRE((result.errors().front().code == -79073));
    REQUIRE((destination.writeCount() == changedIndex / 65536));
    messagePtr = &result.errors().front().message;
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE((result.warnings().size() == 1));
    REQUIRE((result.warnings().front().code == -79074));
    REQUIRE((destination.writeCount() == 2));
    REQUIRE((destination.getValue(changedIndex) == 1));
    messagePtr = &result.warnings().front().message;
  }
  REQUIRE((messagePtr->find("flat index " + std::to_string(changedIndex)) != std::string::npos));
  REQUIRE((messagePtr->find("native ID 4294967297 restores as 1") != std::string::npos));
  REQUIRE((messagePtr->find("1 changed marker occurrence(s)") != std::string::npos));
  REQUIRE((markerStorePtr->readCount() == (handling == ImageProcessing::MarkerIdConversionHandling::Error ? (changedIndex / 65536) + 1 : 2)));
  MarkerTransferStore<uint64>::Transfers expectedReads{{0, 65536}};
  if(handling == ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy || changedIndex >= 65536)
  {
    expectedReads.emplace_back(65536, 2);
  }
  REQUIRE((markerStorePtr->reads() == expectedReads));
  if(handling == ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy)
  {
    REQUIRE((destination.writes() == expectedReads));
  }
  else
  {
    expectedReads.pop_back();
    REQUIRE((destination.writes() == expectedReads));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: invalid policy is rejected before execution",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  auto args = createMarkerLineArguments<uint64>(dataStructure, {1}, true, false);
  args.insertOrAssign(WsFilterType::k_MarkerIdConversionHandling_Key, std::make_any<ChoicesParameter::ValueType>(2));
  const WsFilterType filter;
  const auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE((result.output_actions.errors().size() == 1));
  REQUIRE((result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateOutOfRangeError));
  REQUIRE((result.output_actions.errors().front().message.find("Marker ID Conversion Handling") != std::string::npos));
  REQUIRE((dataStructure.getData(DataPath({"Image Geometry", "CellData", "Output"})) == nullptr));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: diagnostic context keeps exact 64-bit extrema",
                   "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]", int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
    REQUIRE((result.errors().size() == 1));
    REQUIRE((result.errors().front().code == -79073));
    REQUIRE((destination.writeCount() == 0));
    messagePtr = &result.errors().front().message;
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE((result.warnings().size() == 1));
    REQUIRE((result.warnings().front().code == -79074));
    REQUIRE((result.warnings().front().message.find("2 changed marker occurrence(s)") != std::string::npos));
    messagePtr = &result.warnings().front().message;
  }
  const std::string nativeText = std::is_signed_v<TestType> ? "9223372036854775807" : "18446744073709551615";
  REQUIRE((messagePtr->find("native ID " + nativeText + " restores as 4294967295") != std::string::npos));
  REQUIRE((messagePtr->find("First change at flat index 1") != std::string::npos));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: copy cancellation retains observed warning only",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
  REQUIRE((result.warnings().size() == 1));
  REQUIRE((result.warnings().front().code == -79074));
  REQUIRE((result.warnings().front().message.find("1 changed marker occurrence(s)") != std::string::npos));
  REQUIRE((markerStorePtr->readCount() == 1));
  REQUIRE((destination.writeCount() == 1));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: copy warning survives transfer failure or allocation failure",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
    expectedCode = -79075;
  }
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::WidenIntegerArrayToUInt32(dataStructure, DataPath({"Image Geometry", "CellData", "Marker"}), destination, shouldCancel,
                                                                         ImageProcessing::MarkerIdConversionHandling::WarnAndContinueLegacy);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE((result.errors().size() == 1));
  REQUIRE((result.errors().front().code == expectedCode));
  REQUIRE((result.warnings().size() == 1));
  REQUIRE((result.warnings().front().code == -79074));
  REQUIRE((result.warnings().front().message.find("1 changed marker occurrence(s)") != std::string::npos));
  REQUIRE((markerStorePtr->readCount() <= 2));
  REQUIRE((destination.writeCount() == 1));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: conversion warning survives later facade outcomes",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto inputStorePtr = std::make_shared<MarkerTransferStore<float32>>(3);
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, {4294967297ULL, 0, 0}, true, false, inputStorePtr);
  const auto outputStorePtr = std::make_shared<MarkerTransferStore<uint64>>(3);
  const AttributeMatrix* cellDataPtr = nullptr;
  REQUIRE_NOTHROW(cellDataPtr = &dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})));
  REQUIRE((UInt64ArrayType::Create(dataStructure, "Output", outputStorePtr, cellDataPtr->getId()) != nullptr));
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
  // The facade returns a valid partial result on cancellation. The public filter reports cancellation as an error.
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
    REQUIRE((result.errors().size() == 1));
    REQUIRE((result.errors().front().code == expectedCode));
  }
  REQUIRE((result.warnings().size() == 1));
  REQUIRE((result.warnings().front().code == -79074));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: successful transfer warnings survive later marker-copy failure",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
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
  REQUIRE((result.errors().size() == 1));
  REQUIRE((result.errors().front().code == -1874151));
  REQUIRE((result.warnings().size() == 3));
  REQUIRE((result.warnings().at(0).code == 1874153));
  REQUIRE((result.warnings().at(1).code == 1874154));
  REQUIRE((result.warnings().at(2).code == -79074));
  REQUIRE((markerStorePtr->readCount() == 2));
  REQUIRE((destination.writeCount() == 1));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: restoration retains transfer warnings through a short final chunk",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valueCount = 65537;
  DataStructure dataStructure;
  createMarkerLineArguments<uint64>(dataStructure, std::vector<uint64>(valueCount, 0), true, false);
  const auto outputStorePtr = std::make_shared<MarkerTransferStore<uint64>>(valueCount);
  const AttributeMatrix* cellDataPtr = nullptr;
  REQUIRE_NOTHROW(cellDataPtr = &dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})));
  REQUIRE((UInt64ArrayType::Create(dataStructure, "Output", outputStorePtr, cellDataPtr->getId()) != nullptr));
  MarkerTransferStore<uint32> source(valueCount);
  source.setValue(0, 1);
  source.setValue(valueCount - 1, 4294967295U);
  source.warnOnRead();
  outputStorePtr->warnOnWrite();
  bool failFinalWrite = false;
  bool failRestoreAllocation = false;
  SECTION("all delivered warnings survive success")
  {
  }
  SECTION("earlier warnings survive final write failure")
  {
    outputStorePtr->failWrite(2);
    failFinalWrite = true;
  }
  SECTION("allocation failure during the last restoration read")
  {
    source.failRead(2, true);
    failRestoreAllocation = true;
  }
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::NarrowUInt32StoreToIntegerArray(source, dataStructure, DataPath({"Image Geometry", "CellData", "Output"}), shouldCancel);
  if(failRestoreAllocation)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE((result.errors().size() == 1));
    REQUIRE((result.errors().front().code == -79076));
    REQUIRE((result.errors().front().message.find("while restoring marker IDs") != std::string::npos));
    REQUIRE((outputStorePtr->getValue(valueCount - 1) == 0));
  }
  else if(failFinalWrite)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE((result.errors().size() == 1));
    REQUIRE((result.errors().front().code == -1874152));
    REQUIRE((outputStorePtr->getValue(valueCount - 1) == 0));
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE((outputStorePtr->getValue(valueCount - 1) == 4294967295ULL));
  }
  REQUIRE((outputStorePtr->getValue(0) == 1));
  std::vector<int32> expectedWarnings{1874153, 1874154};
  if(!failRestoreAllocation)
  {
    expectedWarnings.push_back(1874153);
    if(!failFinalWrite)
    {
      expectedWarnings.push_back(1874154);
    }
  }
  REQUIRE((result.warnings().size() == expectedWarnings.size()));
  for(usize index = 0; index < expectedWarnings.size(); ++index)
  {
    REQUIRE((result.warnings().at(index).code == expectedWarnings.at(index)));
  }
  const MarkerTransferStore<uint32>::Transfers allReads{{0, 65536}, {65536, 1}};
  auto expectedWrites = allReads;
  if(failRestoreAllocation)
  {
    expectedWrites.pop_back();
  }
  REQUIRE((source.reads() == allReads));
  REQUIRE((outputStorePtr->writes() == expectedWrites));
  REQUIRE((source.readCount() == 2));
  REQUIRE((outputStorePtr->writeCount() == (failRestoreAllocation ? 1 : 2)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: saved policies and version 1 arguments", "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  const WsFilterType filter;
  const std::string key = WsFilterType::k_MarkerIdConversionHandling_Key.str();
  auto arguments = filter.getDefaultArguments();
  SECTION("version 1 supplies Error with a missing-key warning")
  {
    auto legacy = filter.toJson(arguments);
    legacy["parameters_version"] = 1;
    REQUIRE((legacy.erase(key) == 1));
    const auto loaded = filter.fromJson(legacy);
    SIMPLNX_RESULT_REQUIRE_VALID(loaded);
    REQUIRE((loaded.value().value<ChoicesParameter::ValueType>(key) == 0));
    REQUIRE((loaded.warnings().size() == 1));
    CHECK((loaded.warnings().front().code == -5432));
    CHECK((loaded.warnings().front().message.find(key) != std::string::npos));
  }
  SECTION("both choices round-trip in the persisted schema")
  {
    const auto choice = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
    arguments.insertOrAssign(key, std::make_any<ChoicesParameter::ValueType>(choice));
    const auto saved = filter.toJson(arguments);
    CHECK((saved.at("parameters_version") == 2));
    CHECK((saved.at(key).at("value") == choice));
    CHECK((saved.at(key).at("version") == 1));
    const auto loaded = filter.fromJson(nlohmann::json::parse(saved.dump()));
    SIMPLNX_RESULT_REQUIRE_VALID(loaded);
    REQUIRE(loaded.warnings().empty());
    CHECK((loaded.value().value<ChoicesParameter::ValueType>(key) == choice));
  }
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: omitted policy stops before flooding", "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto args = createMarkerLineArguments<uint64>(dataStructure, {0, 4294967297ULL, 0}, true, false);
  const DataPath input({"Image Geometry", "CellData", "Input"});
  const auto marker = input.getParent().createChildPath("Marker");
  const auto output = input.getParent().createChildPath("Output");
  bool enteredFlood = false;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) { enteredFlood |= message.message == "Computing Watershed From Markers"; }};
  Result<> result;
  SECTION("omitted filter argument")
  {
    result = WsFilterType{}.execute(dataStructure, args, nullptr, handler).result;
  }
  SECTION("omitted trailing argument at the exported C++ entry point")
  {
    const auto cellId = dataStructure.getId(input.getParent());
    REQUIRE(cellId.has_value());
    auto store = data_store_utilities::CreateDataStore<uint64>(dataStructure, output, ShapeType{1, 1, 3}, ShapeType{1});
    REQUIRE((UInt64ArrayType::Create(dataStructure, output.getTargetName(), store, *cellId) != nullptr));
    result = ImageProcessing::ExecuteMorphologicalWatershedFromMarkersImageFilter(dataStructure, input.getParent().getParent(), input, marker, output, true, false, false, handler);
  }
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE((result.errors().size() == 1));
  REQUIRE((result.errors().front().code == -79073));
  REQUIRE((result.errors().front().message.find("flat index 1") != std::string::npos));
  REQUIRE((result.errors().front().message.find("native ID 4294967297 restores as 1") != std::string::npos));
  REQUIRE_FALSE(enteredFlood);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: public execute retains conversion warnings on cancellation and transfer failure",
          "[NXImageProcessing][Issue1874][MorphologicalWatershedFromMarkersImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize count = 65537;
  std::vector<uint64> markers(count, 0);
  markers.at(4) = 4294967297ULL;
  markers.back() = 4294967296ULL;
  DataStructure dataStructure;
  const auto store = std::make_shared<MarkerTransferStore<uint64>>(count);
  auto args = createMarkerLineArguments<uint64>(dataStructure, markers, true, false, {}, store);
  args.insertOrAssign(WsFilterType::k_MarkerIdConversionHandling_Key, std::make_any<ChoicesParameter::ValueType>(1));
  std::atomic_bool shouldCancel = false;
  store->warnOnRead();
  int32 expectedCode = -1;
  SECTION("cancel while reading the first batch")
  {
    store->cancelAfterRead(1, shouldCancel);
  }
  SECTION("a genuine error remains primary when the second read also cancels")
  {
    store->failRead(2);
    store->cancelAfterRead(2, shouldCancel);
    expectedCode = -1874151;
  }
  bool enteredFlood = false;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) { enteredFlood |= message.message == "Computing Watershed From Markers"; }};
  const auto result = WsFilterType{}.execute(dataStructure, args, nullptr, handler, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(shouldCancel.load());
  REQUIRE((result.result.errors().size() == 1));
  REQUIRE((result.result.errors().front().code == expectedCode));
  REQUIRE((result.result.warnings().size() == 2));
  REQUIRE((result.result.warnings().at(0).code == 1874153));
  const auto& warning = result.result.warnings().at(1);
  REQUIRE((warning.code == -79074));
  REQUIRE((warning.message.find("1 changed marker occurrence(s)") != std::string::npos));
  REQUIRE((warning.message.find("flat index 4") != std::string::npos));
  REQUIRE_FALSE(enteredFlood);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (4) Preflight guards: a multi-component grayscale is rejected (requireScalar); a non-integer (float32) marker is
//     rejected; a marker whose tuple count mismatches the grayscale is rejected; a valid pair creates an output of
//     the MARKER's element type (NOT the grayscale's).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight guards", "[NXImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto setArgs = [](Arguments& args, const DataPath& geomPath, const DataPath& inputPath, const DataPath& markerPath) {
    args.insertOrAssign(WsFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WsFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(WsFilterType::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
    args.insertOrAssign(WsFilterType::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WsFilterType::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(WsFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  };

  SECTION("multi-component grayscale rejected")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<uint32> markers = MakeMarkerField<uint32>(8, 8, 1);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, uint32>(dataStructure, 8, 8, 1, gray, markers);
    // Add a multi-component grayscale array and select it as the input.
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = data_store_utilities::CreateDataStore<int16>(dataStructure, vecPath, {1, 8, 8}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int16>::Create(dataStructure, "Vec", vecStore, cellAM.getId());
    WsFilterType filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), vecPath, markerPath);
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float marker rejected")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<float32> markers(8 * 8, 0.0f);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, float32>(dataStructure, 8, 8, 1, gray, markers);
    WsFilterType filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(dataStructure, args);
    // A float32 marker is rejected: the marker ArraySelectionParameter admits only integer scalar types, so the
    // parameter layer rejects it before preflightImpl (the filter's own -8600 integer-type guard is the defensive
    // backstop for any non-GUI caller). Either way the marker cannot be non-integer.
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  }
  SECTION("multi-component marker rejected (-8601)")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(dataStructure, 8, 8, 1, gray); // scalar grayscale + geometry/CellData
    // A 3-component INTEGER marker: the marker ArraySelectionParameter admits integer types with NO component-shape
    // restriction, so it passes the parameter layer and reaches preflightImpl, which rejects it with -8601. int32 is
    // integer (so the -8600 integer-type guard passes), and the tuple count matches the grayscale (so the -8602
    // tuple-mismatch guard does not fire first) -- this isolates the non-scalar-marker guard, which had no coverage.
    const DataPath markerPath({"Image Geometry", "CellData", "Marker"});
    auto markerStore = data_store_utilities::CreateDataStore<int32>(dataStructure, markerPath, {1, 8, 8}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Marker", markerStore, cellAM.getId());
    WsFilterType filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == -8601); // non-scalar (multi-component) marker
  }
  SECTION("marker tuple-count mismatch rejected")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1); // 64 tuples
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(dataStructure, 8, 8, 1, gray);
    // Put the marker in its OWN AttributeMatrix with a different tuple count (so the AM tuple-shape check passes but
    // the filter's grayscale-vs-marker tuple check fails).
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"}));
    auto* otherAmPtr = AttributeMatrix::Create(dataStructure, "Other", ShapeType{50}, imageGeom.getId());
    const DataPath markerPath({"Image Geometry", "Other", "Marker"});
    auto markerStore = data_store_utilities::CreateDataStore<uint32>(dataStructure, markerPath, ShapeType{50}, {1});
    DataArray<uint32>::Create(dataStructure, "Marker", markerStore, otherAmPtr->getId());
    WsFilterType filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == -8602); // tuple-count mismatch
  }
  SECTION("valid pair creates an output of the MARKER's type")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const std::vector<int8> markers = MakeMarkerField<int8>(8, 8, 1);
    const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, int8>(dataStructure, 8, 8, 1, gray, markers);
    WsFilterType filter;
    RunWatershed(filter, dataStructure, inputPath, markerPath, /*markWatershedLine=*/true, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::int8); // marker type, not grayscale (int16)
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight RAM-fit guard: ValidateWatershedFitsInMemory hard-errors (with the code the FromMarkers filter passes,
//     -79041) for an absurd tuple count whose working set exceeds ANY machine's RAM, WITHOUT allocating an array; a
//     small tuple count is accepted on the test machine. No specific available-RAM number is asserted -- only that an
//     absurd size is rejected and a plausibly-small size is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight RAM-fit guard", "[NXImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
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
TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: marker value 0 is unmarked (flooded)", "[NXImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(markWatershedLine, fullyConnected);

  constexpr usize k_Dx = 12;
  constexpr usize k_Dy = 12;
  constexpr usize k_Dz = 1;
  const std::vector<int16> gray = rt::MakePlateauPattern<int16>(k_Dx, k_Dy, k_Dz);
  // A solid 3x3 block of label 5 with a single 0 hole punched at its center (2,2), plus a distant second seed. The
  // large remaining 0 field is likewise unmarked -> flooded between the two basins.
  std::vector<uint32> markers(k_Dx * k_Dy * k_Dz, 0u);
  auto set = [&](usize x, usize y, uint32 v) { markers[rt::FlatIndex(x, y, 0, k_Dx, k_Dy)] = v; };
  for(usize yy = 1; yy <= 3; ++yy)
  {
    for(usize xx = 1; xx <= 3; ++xx)
    {
      set(xx, yy, 5u);
    }
  }
  set(2, 2, 0u);               // the 0 in the middle of the block (fully enclosed by label 5)
  set(k_Dx - 2, k_Dy - 2, 9u); // a second, distant seed

  // Directly confirm the punched 0 voxel was flooded to the enclosing basin (label 5), i.e. treated as unmarked --
  // never left at 0 and never a distinct seed. Enclosed on all four sides by label 5, so it is not a watershed line
  // regardless of MarkWatershedLine.
  DataStructure dataStructure;
  const auto [inputPath, markerPath] = BuildGrayscaleAndMarker<int16, uint32>(dataStructure, k_Dx, k_Dy, k_Dz, gray, markers);
  WsFilterType filter;
  RunWatershed(filter, dataStructure, inputPath, markerPath, markWatershedLine, fullyConnected);
  const auto& store = dataStructure.getDataRefAs<IDataArray>(DataPath({"Image Geometry", "CellData", "Output"})).getIDataStoreRefAs<AbstractDataStore<uint32>>();
  REQUIRE(store.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == 5u);
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
TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][MorphologicalWatershedFromMarkersImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

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
  DataStructure dataStructure;
  const Result<> readGray = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("cthead1-grad-mag.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readGray);
  const Result<> readMarker = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("cthead1-marker.png"), markerGeom, "CellData", "Marker");
  SIMPLNX_RESULT_REQUIRE_VALID(readMarker);

  const auto& grayArray = dataStructure.getDataRefAs<IDataArray>(input);
  const auto& markerArray = dataStructure.getDataRefAs<IDataArray>(marker);
  REQUIRE(grayArray.getDataType() == DataType::uint8);
  REQUIRE(markerArray.getDataType() == DataType::uint8);
  REQUIRE(markerArray.getNumberOfComponents() == 1);
  REQUIRE(markerArray.getNumberOfTuples() == grayArray.getNumberOfTuples());

  const SizeVec3Type dims = dataStructure.getDataRefAs<ImageGeom>(geom).getDimensions();
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
  WsFilterType filter;
  RunWatershed(filter, dataStructure, input, marker, /*markWatershedLine=*/true, /*fullyConnected=*/false, "Output");

  // DURABLE golden: md5 of our output. It equals ITK's committed hash (see the comment above).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(dataStructure, output);
  UNSCOPED_INFO("MorphologicalWatershedFromMarkers md5 = " + ourMd5);
  REQUIRE(ourMd5 == "c32759ecb10cbff555c750b8b7a8d32e");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::MorphologicalWatershedFromMarkersImageFilter: preflight rejects missing temporary provider", "[NXImageProcessing][MorphologicalWatershedFromMarkersImageFilter]")
{
  unit_test::LoadPlugins();
  const auto application = Application::GetOrCreateInstance();
  auto& collection = application->getIOCollection();
  const DataIOCollection::MapType originalManagers(collection.begin(), collection.end());
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
    auto* imageGeomPtr = ImageGeom::Create(dataStructure, geomPath.getTargetName());
    REQUIRE(imageGeomPtr != nullptr);
    imageGeomPtr->setDimensions({3, 2, 1});
    REQUIRE(imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
    REQUIRE(imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
    auto* cellDataPtr = AttributeMatrix::Create(dataStructure, cellPath.getTargetName(), tupleShape, imageGeomPtr->getId());
    REQUIRE(cellDataPtr != nullptr);
    imageGeomPtr->setCellData(*cellDataPtr);

    // HDF5 identifies a registered metadata-only format here. Neither input contains HDF5 values.
    auto inputStoreResult = EmptyDataStore<int16>::Create(tupleShape, componentShape, plannedFormat);
    SIMPLNX_RESULT_REQUIRE_VALID(inputStoreResult);
    const auto* inputStorePtr = inputStoreResult.value().get();
    auto* inputArrayPtr = Int16ArrayType::Create(dataStructure, inputPath.getTargetName(), std::move(inputStoreResult.value()), cellDataPtr->getId());
    REQUIRE(inputArrayPtr != nullptr);
    auto markerStoreResult = EmptyDataStore<uint32>::Create(tupleShape, componentShape, "");
    SIMPLNX_RESULT_REQUIRE_VALID(markerStoreResult);
    const auto* markerStorePtr = markerStoreResult.value().get();
    auto* markerArrayPtr = UInt32ArrayType::Create(dataStructure, markerPath.getTargetName(), std::move(markerStoreResult.value()), cellDataPtr->getId());
    REQUIRE(markerArrayPtr != nullptr);

    const auto checkInputs = [&] {
      REQUIRE(dataStructure.getDataAs<Int16ArrayType>(inputPath) == inputArrayPtr);
      REQUIRE(inputArrayPtr->getIDataStore() == inputStorePtr);
      REQUIRE(inputArrayPtr->getDataType() == DataType::int16);
      REQUIRE(inputArrayPtr->getTupleShape() == tupleShape);
      REQUIRE(inputArrayPtr->getComponentShape() == componentShape);
      REQUIRE(inputArrayPtr->getStoreType() == IDataStore::StoreType::Empty);
      REQUIRE(inputStorePtr->getPlannedStoreType() == IDataStore::StoreType::OutOfCore);
      REQUIRE(inputArrayPtr->getDataFormat() == plannedFormat);
      REQUIRE(dataStructure.getDataAs<UInt32ArrayType>(markerPath) == markerArrayPtr);
      REQUIRE(markerArrayPtr->getIDataStore() == markerStorePtr);
      REQUIRE(markerArrayPtr->getDataType() == DataType::uint32);
      REQUIRE(markerArrayPtr->getTupleShape() == tupleShape);
      REQUIRE(markerArrayPtr->getComponentShape() == componentShape);
      REQUIRE(markerArrayPtr->getStoreType() == IDataStore::StoreType::Empty);
      REQUIRE(markerStorePtr->getPlannedStoreType() == IDataStore::StoreType::InMemory);
      REQUIRE(markerArrayPtr->getDataFormat().empty());
    };
    checkInputs();
    REQUIRE(dataStructure.getData(outputPath) == nullptr);
    const auto originalIds = dataStructure.getAllDataObjectIds();

    WsFilterType filter;
    Arguments args;
    args.insertOrAssign(WsFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WsFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(WsFilterType::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
    args.insertOrAssign(WsFilterType::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WsFilterType::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(WsFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputPath.getTargetName()));
    const auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().size() == 1);
    REQUIRE(result.output_actions.errors().front().code == -79047);
    REQUIRE(result.output_actions.errors().front().message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(result.output_actions.warnings().empty());
    REQUIRE(dataStructure.getData(outputPath) == nullptr);
    REQUIRE(dataStructure.getAllDataObjectIds() == originalIds);
    checkInputs();
    unit_test::CheckArraysInheritTupleDims(dataStructure);
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

  const DataIOCollection::MapType restoredManagers(collection.begin(), collection.end());
  REQUIRE(restoredManagers == originalManagers);
  REQUIRE(collection.hasTemporaryRecordStoreCapability() == originalTemporaryCapability);
  REQUIRE(collection.hasExternalSortCapability() == originalSortCapability);
  REQUIRE(collection.anyManagerFinalizesImport() == originalImportCapability);
  REQUIRE(collection.getFormatNames() == originalNumericFormats);
  REQUIRE(collection.getFormatDisplayNames() == originalDisplayNames);
}
