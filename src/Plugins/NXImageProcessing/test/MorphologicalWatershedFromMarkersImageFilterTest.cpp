#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"
#include "WatershedPreflightTestUtils.hpp"

#include "NXImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

// solely to compute the byte-exact reference for the new ITK-free flood engine (the legacy NX wrapper for this
// filter is disabled/broken, so there is no runtime-createable legacy filter to compare against).

#include <catch2/catch.hpp>

#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using WsFilterType = MorphologicalWatershedFromMarkersImageFilter;

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
std::pair<DataPath, DataPath> BuildGrayscaleAndMarker(DataStructure& dataStructure, usize dx, usize dy, usize dz, const std::vector<TInput>& gray, const std::vector<TLabel>& markers)
{
  const DataPath inputPath = rt::BuildImageFromPattern<TInput>(dataStructure, dx, dy, dz, gray);

  const ShapeType cellShape = {dz, dy, dx};
  const DataPath markerPath({"Image Geometry", "CellData", "Marker"});
  auto markerStore = data_store_utilities::CreateDataStore<TLabel>(dataStructure, markerPath, cellShape, {1});
  const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
  auto* markerArrayPtr = DataArray<TLabel>::Create(dataStructure, "Marker", markerStore, cellAM.getId());
  auto& ref = markerArrayPtr->getDataStoreRef();
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
void RunWatershed(IFilter& filter, DataStructure& dataStructure, const DataPath& inputPath, const DataPath& markerPath, bool markWatershedLine, bool fullyConnected,
                  const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(WsFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WsFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WsFilterType::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markerPath));
  args.insertOrAssign(WsFilterType::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WsFilterType::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  args.insertOrAssign(WsFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
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

// -----------------------------------------------------------------------------
// (4) Preflight guards: a multi-component grayscale is rejected; a non-integer (float32) marker is
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
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue);
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
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateAllowedTypeError);
  }
  SECTION("multi-component marker rejected")
  {
    DataStructure dataStructure;
    const std::vector<int16> gray = rt::MakePlateauPattern<int16>(8, 8, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(dataStructure, 8, 8, 1, gray); // scalar grayscale + geometry/CellData
    // An integer marker image with the correct tuple count isolates component-shape validation.
    const DataPath markerPath({"Image Geometry", "CellData", "Marker"});
    auto markerStore = data_store_utilities::CreateDataStore<int32>(dataStructure, markerPath, {1, 8, 8}, {3});
    const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(dataStructure, "Marker", markerStore, cellAM.getId());
    WsFilterType filter;
    Arguments args;
    setArgs(args, DataPath({"Image Geometry"}), inputPath, markerPath);
    const auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
    REQUIRE(result.output_actions.errors().front().code == filter_parameter::constants::k_ValidateTupleShapeValue); // non-scalar (multi-component) marker
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
