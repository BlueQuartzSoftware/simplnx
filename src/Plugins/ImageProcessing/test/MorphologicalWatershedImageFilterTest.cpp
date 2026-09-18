#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MorphologicalWatershedImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"

// Test-only: a DIRECT itk:: oracle for the COMPOSITE (mirrors the FromMarkers oracle). The shipped ImageProcessing
// plugin is ITK-free; this test target links ITK solely to compute a REMOVAL-PROOF byte-exact reference for the
// ITK-free composite (HMinima -> RegionalMinima -> ConnectedComponent -> FAH flood), independent of the legacy NX
// wrapper created by UUID above. itk::ImageRegionIterator walks x fastest, matching simplnx's flat index.
#include "itkImage.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkMorphologicalWatershedImageFilter.h"

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKMorphologicalWatershedImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// ITK-free composite (HMinima -> RegionalMinima -> ConnectedComponent -> FAH flood). Label VALUES must byte-match.
const Uuid k_LegacyWatershedUuid = *Uuid::FromString("f70337e5-4435-41f7-aecc-d79b4b1faccd");

using WSFilter = MorphologicalWatershedImageFilter;

// Sets the standard geom/input/output keys + Level/MarkWatershedLine/FullyConnected on a shared Arguments, runs
// preflight + execute, and requires both succeed. Reuses WSFilter::k_*_Key for BOTH the new and the legacy ITK
// filter -- correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_
// path", "input_image_data_path", "output_array_name", "level", "mark_watershed_line", "fully_connected").
void RunWatershed(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 level, bool markWatershedLine, bool fullyConnected, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(WSFilter::k_Level_Key, std::make_any<float64>(level));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// Run new + legacy on the caller-supplied @p field/params and require the uint32 label outputs match EXACTLY
// (byte-identical label VALUES -- the live-ITK gate on the composite).
template <class T>
void RequireParityWithField(usize dx, usize dy, usize dz, const std::vector<T>& field, float64 level, bool markWatershedLine, bool fullyConnected)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  WSFilter newFilter;
  RunWatershed(newFilter, newDs, newInput, level, markWatershedLine, fullyConnected);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyWatershedUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunWatershed(*legacyFilter, legacyDs, legacyInput, level, markWatershedLine, fullyConnected);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<uint32>(newOut, legacyOut); // EXACT -- label values must byte-match live ITK
}

// Run new + legacy on the same field/params and require the uint32 label outputs match EXACTLY (byte-identical label
// VALUES -- the live-ITK gate on the composite). The grayscale is a multi-plateau field (multiple isolated minima ->
// several basins + watershed lines), so both connectivities, both flood variants, and the h-minima Level path are
// exercised.
template <class T>
void RequireParity(usize dx, usize dy, usize dz, float64 level, bool markWatershedLine, bool fullyConnected)
{
  RequireParityWithField<T>(dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz), level, markWatershedLine, fullyConnected);
}

// A signed multi-plateau field shifted entirely into NEGATIVE territory (values in [-90, 0]) so the h-minima marker
// (input + Level) is evaluated where input + Level < 0 -- the regime where truncating a FRACTIONAL Level toward zero
// diverges from ITK's (InputImagePixelType)m_Level truncation. The shift is constant, so the multi-plateau topology
// (relative minima structure) is preserved and the field still yields several basins + watershed lines.
template <class T>
std::vector<T> MakeNegativePlateauPattern(usize dx, usize dy, usize dz)
{
  static_assert(std::is_signed_v<T>, "MakeNegativePlateauPattern requires a signed element type.");
  std::vector<T> field = rt::MakePlateauPattern<T>(dx, dy, dz); // values in [0, 90]
  for(T& v : field)
  {
    v = static_cast<T>(v - static_cast<T>(90)); // shift to [-90, 0]
  }
  return field;
}

//------------------------------------------------------------------------------
// Direct-itk:: oracle for the COMPOSITE (removal-proof: independent of the legacy-UUID wrapper used by tests 1/6).
// itk::ImageRegionIterator walks dimension 0 (x) fastest, which matches simplnx's flat index ((z*nY)+y)*nX + x, so a
// buffer laid out x-fastest round-trips through ITK unchanged.
template <class T>
typename itk::Image<T, 3>::Pointer MakeItkImage(const std::vector<T>& buf, usize dx, usize dy, usize dz)
{
  using ImageType = itk::Image<T, 3>;
  auto image = ImageType::New();
  typename ImageType::RegionType region;
  region.SetIndex({{0, 0, 0}});
  region.SetSize({{static_cast<itk::SizeValueType>(dx), static_cast<itk::SizeValueType>(dy), static_cast<itk::SizeValueType>(dz)}});
  image->SetRegions(region);
  image->Allocate();
  itk::ImageRegionIterator<ImageType> it(image, region);
  usize i = 0;
  for(it.GoToBegin(); !it.IsAtEnd(); ++it, ++i)
  {
    it.Set(buf[i]);
  }
  return image;
}

// Run itk::MorphologicalWatershedImageFilter<Image<T,3>, Image<uint32,3>> directly -> the oracle label buffer (flat,
// x-fastest). ITK stores m_Level as InputImagePixelType, so pass the Level already truncated to T -- exactly what the
// composite does before the h-minima gate/height (see test 6). The output label type is fixed uint32 to match the
// composite's FilterOutputType.
template <class T>
std::vector<uint32> ItkCompositeOracle(const std::vector<T>& gray, usize dx, usize dy, usize dz, float64 level, bool markWatershedLine, bool fullyConnected)
{
  using InImg = itk::Image<T, 3>;
  using LabImg = itk::Image<uint32, 3>;
  auto in = MakeItkImage<T>(gray, dx, dy, dz);
  auto filter = itk::MorphologicalWatershedImageFilter<InImg, LabImg>::New();
  filter->SetInput(in);
  filter->SetLevel(static_cast<T>(level));
  filter->SetMarkWatershedLine(markWatershedLine);
  filter->SetFullyConnected(fullyConnected);
  filter->Update();
  std::vector<uint32> out(gray.size());
  itk::ImageRegionConstIterator<LabImg> it(filter->GetOutput(), filter->GetOutput()->GetLargestPossibleRegion());
  usize i = 0;
  for(it.GoToBegin(); !it.IsAtEnd(); ++it, ++i)
  {
    out[i] = it.Get();
  }
  return out;
}

// Strict EXACT comparison of the uint32 label output against the ITK oracle vector (no tolerance -- integer label
// VALUES must byte-match live ITK). Reports the first mismatch.
inline void RequireExactLabels(const IDataArray& newOut, const std::vector<uint32>& oracle)
{
  const auto& store = newOut.getIDataStoreRefAs<AbstractDataStore<uint32>>();
  REQUIRE(store.getSize() == oracle.size());
  const usize total = oracle.size();
  constexpr usize k_ChunkSize = 40000;
  auto buf = std::make_unique<uint32[]>(k_ChunkSize);
  bool failed = false;
  usize failIndex = 0;
  uint32 failNew = 0;
  uint32 failOracle = 0;
  for(usize offset = 0; offset < total && !failed; offset += k_ChunkSize)
  {
    const usize count = std::min(k_ChunkSize, total - offset);
    Result<> readResult = store.copyIntoBuffer(offset, nonstd::span<uint32>(buf.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    for(usize i = 0; i < count; ++i)
    {
      if(buf[i] != oracle[offset + i])
      {
        failed = true;
        failIndex = offset + i;
        failNew = buf[i];
        failOracle = oracle[offset + i];
        break;
      }
    }
  }
  if(failed)
  {
    UNSCOPED_INFO(fmt::format("Label mismatch @{}: new={} oracle={}", failIndex, failNew, failOracle));
  }
  REQUIRE(!failed);
}

// Run the NEW composite on @p field and require EXACT uint32-label parity with the direct-itk:: oracle.
template <class T>
void RequireItkParityWithField(usize dx, usize dy, usize dz, const std::vector<T>& field, float64 level, bool markWatershedLine, bool fullyConnected)
{
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<T>(ds, dx, dy, dz, field);
  WSFilter filter;
  RunWatershed(filter, ds, inputPath, level, markWatershedLine, fullyConnected);

  const std::vector<uint32> oracle = ItkCompositeOracle<T>(field, dx, dy, dz, level, markWatershedLine, fullyConnected);
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  RequireExactLabels(ds.getDataRefAs<IDataArray>(outputPath), oracle);
}

// Multi-plateau convenience overload (several isolated minima -> basins + watershed lines).
template <class T>
void RequireItkParity(usize dx, usize dy, usize dz, float64 level, bool markWatershedLine, bool fullyConnected)
{
  RequireItkParityWithField<T>(dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz), level, markWatershedLine, fullyConnected);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL scalar type grid x Level{0, >0} x MarkWatershedLine{off,on} x FullyConnected
//     {off,on} x {3D, 2D}. The composite is a fixed uint32 output; the new filter must reproduce legacy ITK's label
//     VALUES EXACTLY (no tolerance -- see the Parity model). This is the primary gate on the whole composite (the
//     HMinima Level path, the RegionalMinima fg/bg + flat handling, the ConnectedComponent predicate, and the flood).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: Live-ITK parity", "[ImageProcessing][MorphologicalWatershedImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64,
                   int64, float32, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyWatershedUuid) != nullptr);

  const float64 level = GENERATE(0.0, 5.0); // 0 -> no h-minima suppression; 5 -> exercise the HMinima path (T(5)!=0 for every type)
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(level, markWatershedLine, fullyConnected);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(DX, DY, DZ, level, markWatershedLine, fullyConnected);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(DX, DY, DZ, level, markWatershedLine, fullyConnected);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint32 output array (AlwaysUInt32).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: preflight guards", "[ImageProcessing][MorphologicalWatershedImageFilter]")
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
    WSFilter filter;
    Arguments args;
    args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(WSFilter::k_Level_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
    args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a uint32 output")
  {
    DataStructure ds;
    const std::vector<float32> field = rt::MakePlateauPattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    WSFilter filter;
    RunWatershed(filter, ds, inputPath, /*level=*/0.0, /*markWatershedLine=*/true, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint32);
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight RAM-fit guard: ValidateWatershedFitsInMemory hard-errors (with the code this filter passes, -79042)
//     for an absurd tuple count whose working set exceeds ANY machine's RAM, WITHOUT allocating an array; a small
//     tuple count is accepted on the test machine. No specific available-RAM number is asserted -- only that an absurd
//     size is rejected and a plausibly-small size is accepted.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: preflight RAM-fit guard", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  // ~1e11 voxels * >=11 bytes/voxel ~= 1.1 TB working set: larger than any machine's RAM, so it must be rejected with
  // this filter's memory code (-79042). The guard is a pure numeric estimate -- nothing is allocated. The ERROR takes
  // precedence over the OOC warning, so inputIsOutOfCore is irrelevant here.
  const Result<OutputActions> tooLarge = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(100'000'000'000ULL), /*inputIsOutOfCore=*/true, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_INVALID(tooLarge);
  REQUIRE(tooLarge.errors().front().code == -79042);

  // A tiny volume trivially fits on any test machine. In-core input: accepted with NO warning (RAM is the expected
  // backing -- no surprise to report).
  const Result<OutputActions> tinyInCore = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/false, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyInCore);
  REQUIRE(tinyInCore.warnings().empty());

  // Same tiny volume, but the input is stored out-of-core: still VALID (it fits), yet now carries the OOC-override
  // warning with this filter's warning code (-79044) -- watershed will load its working set into RAM regardless.
  const Result<OutputActions> tinyOoc = ImageProcessing::ValidateWatershedFitsInMemory(DataType::int16, static_cast<usize>(64), /*inputIsOutOfCore=*/true, -79042, -79044);
  SIMPLNX_RESULT_REQUIRE_VALID(tinyOoc);
  REQUIRE(tinyOoc.warnings().size() == 1);
  REQUIRE(tinyOoc.warnings().front().code == -79044);
}

// -----------------------------------------------------------------------------
// (4) FromSIMPLJson: the legacy SIMPL parameter set (Level + the two bools + geom/array/name) round-trips onto the
//     new filter's Arguments.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: FromSIMPLJson", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  const nlohmann::json json = {{"Level", 4.5},
                               {"MarkWatershedLine", false},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Watershed"}};

  Result<Arguments> result = MorphologicalWatershedImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(MorphologicalWatershedImageFilter::k_Level_Key) == 4.5);
  REQUIRE(args.value<bool>(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key) == false);
  REQUIRE(args.value<bool>(MorphologicalWatershedImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(MorphologicalWatershedImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(MorphologicalWatershedImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(MorphologicalWatershedImageFilter::k_OutputImageArrayName_Key) == "Watershed");
}

// -----------------------------------------------------------------------------
// (6) Live-ITK EXACT parity on a SIGNED int16 field containing NEGATIVE values with a FRACTIONAL Level -- the
//     regression guard for the Level-truncation bug. ITK stores m_Level as InputImagePixelType, so it truncates Level
//     to the input type BEFORE both the h-minima gate (m_Level != 0) and the h-minima height (SetHeight(m_Level)); the
//     composite must do the same. Passing the RAW double made the h-minima marker trunc(v + Level), which for
//     v + Level < 0 rounds toward zero instead of down and so diverges from ITK's v + (int16)Level by one across the
//     whole negative field -- different h-minima -> different regional minima -> different labels. The standard grid
//     (test 1) only uses integer Level on a non-negative field, so it never exercised this. Both sections below FAIL
//     before the fix and PASS after.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: fractional Level on a signed-negative field", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyWatershedUuid) != nullptr);

  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);

  // Level 4.5 truncates to a NON-zero height: ITK runs h-minima with height (int16)4.5 == 4, so its marker is v + 4;
  // the raw-double marker trunc(v + 4.5) is v + 5 for every v <= -5 -- an effective height of 5 across the negative
  // field, which suppresses the depth-5 ramps ITK keeps.
  SECTION("Level 4.5 truncates to 4")
  {
    constexpr float64 level = 4.5;
    CAPTURE(level, markWatershedLine, fullyConnected);
    SECTION("3D")
    {
      RequireParityWithField<int16>(12, 12, 4, MakeNegativePlateauPattern<int16>(12, 12, 4), level, markWatershedLine, fullyConnected);
    }
    SECTION("2D (Z==1)")
    {
      RequireParityWithField<int16>(12, 12, 1, MakeNegativePlateauPattern<int16>(12, 12, 1), level, markWatershedLine, fullyConnected);
    }
  }

  // Level 0.5 truncates to ZERO: ITK's gate (m_Level != 0) is false, so h-minima is SKIPPED entirely (identity),
  // whereas the raw-double gate (0.5 != 0.0) ran a non-identity h-minima pre-fix. After truncation the composite's
  // gate is levelT != 0, so it too skips h-minima -- matching ITK's untouched input.
  SECTION("Level 0.5 truncates to 0 (h-minima skipped)")
  {
    constexpr float64 level = 0.5;
    CAPTURE(level, markWatershedLine, fullyConnected);
    SECTION("3D")
    {
      RequireParityWithField<int16>(12, 12, 4, MakeNegativePlateauPattern<int16>(12, 12, 4), level, markWatershedLine, fullyConnected);
    }
    SECTION("2D (Z==1)")
    {
      RequireParityWithField<int16>(12, 12, 1, MakeNegativePlateauPattern<int16>(12, 12, 1), level, markWatershedLine, fullyConnected);
    }
  }
}

// -----------------------------------------------------------------------------
// (7) Direct-itk:: EXACT parity: a REMOVAL-PROOF independent gate on the composite (tests 1/6 gate against the legacy
//     ITKMorphologicalWatershed wrapper created by UUID -- fragile once that wrapper is removed; this gates against a
//     directly-constructed itk::MorphologicalWatershedImageFilter instead). Representative scalar types (unsigned /
//     signed / float) x Level{0, >0} x MarkWatershedLine{off,on} x FullyConnected{off,on} x {3D, 2D}; the new filter
//     must reproduce the direct ITK label VALUES EXACTLY (no tolerance -- see the Parity model).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: direct-ITK composite parity", "[ImageProcessing][MorphologicalWatershedImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const float64 level = GENERATE(0.0, 3.0); // 0 -> no h-minima suppression; 3 -> exercise the HMinima path (T(3)!=0 for every type)
  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(level, markWatershedLine, fullyConnected);

  SECTION("3D")
  {
    RequireItkParity<T>(12, 12, 4, level, markWatershedLine, fullyConnected);
  }
  SECTION("2D (Z==1)")
  {
    RequireItkParity<T>(12, 12, 1, level, markWatershedLine, fullyConnected);
  }
}

// -----------------------------------------------------------------------------
// (8) Flat/constant input: a completely constant image drives the composite's flat-is-minima short-circuit
//     (ComputeArrayStatistics -> stats.min == stats.max -> FillStore(rmin, uint32-max)); the whole image is one
//     regional minimum -> one connected component -> a single basin. Must match the direct itk:: oracle EXACTLY
//     (ITK takes its own flat/GetFlat() branch). The standard grids (tests 1/6/7) all use a multi-plateau field, so
//     this branch was previously unexercised. Covered for a couple of types x params.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: flat/constant input matches ITK", "[ImageProcessing][MorphologicalWatershedImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const bool markWatershedLine = GENERATE(true, false);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(markWatershedLine, fullyConnected);

  // A single constant value everywhere -> stats.min == stats.max -> the flat branch. Level 0 (h-minima of a constant
  // is the identity anyway, so RegionalMinima still hits its flat short-circuit).
  SECTION("3D")
  {
    const std::vector<T> constant(12 * 12 * 4, static_cast<T>(42));
    RequireItkParityWithField<T>(12, 12, 4, constant, /*level=*/0.0, markWatershedLine, fullyConnected);
  }
  SECTION("2D (Z==1)")
  {
    const std::vector<T> constant(12 * 12 * 1, static_cast<T>(42));
    RequireItkParityWithField<T>(12, 12, 1, constant, /*level=*/0.0, markWatershedLine, fullyConnected);
  }
}

// -----------------------------------------------------------------------------
// (9) Both-algorithm-paths coverage: the composite on a multi-plateau field, run under BOTH the in-core and
//     out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//     UnitTest::AlgorithmTestScope. Scope-compatibility (verified by source analysis): with Level 0 the HMinima
//     reconstruction is skipped (levelT == 0) and the input is non-flat, so the composite performs EXACTLY ONE
//     DispatchAlgorithm call -- the RegionalMinima valued-extrema flood/sweep. The connected-component labeling and
//     the FAH watershed flood are single-implementation (no DispatchAlgorithm), and no step toggles the force
//     flags, so the scope witness sees exactly one expected-path execution and zero unexpected paths under either
//     scenario. The oracle is the direct-itk:: composite reference computed BEFORE the scope (independent of the
//     new filter -> non-circular for both paths). This subset config (Level 0, multi-plateau, mwl on, fc off, 3D)
//     lies within test (7)'s direct-ITK grid, so the oracle equals the established-correct output.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: multi-plateau computed-expected (both algorithm paths)", "[ImageProcessing][MorphologicalWatershedImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  using T = float32;
  constexpr usize DX = 12, DY = 12, DZ = 4;
  constexpr float64 level = 0.0;
  constexpr bool markWatershedLine = true;
  constexpr bool fullyConnected = false;
  const std::vector<T> field = rt::MakePlateauPattern<T>(DX, DY, DZ);

  // Direct-itk:: oracle, computed independent of the new filter and BEFORE the scope (which wraps only the
  // filter-under-test). This is NOT the new filter's own in-core run, so it is a non-circular oracle for both paths.
  const std::vector<uint32> oracle = ItkCompositeOracle<T>(field, DX, DY, DZ, level, markWatershedLine, fullyConnected);

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<T>(ds, DX, DY, DZ, field);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  WSFilter filter;
  Arguments args;
  args.insertOrAssign(WSFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(WSFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WSFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(WSFilter::k_Level_Key, std::make_any<float64>(level));
  args.insertOrAssign(WSFilter::k_MarkWatershedLine_Key, std::make_any<bool>(markWatershedLine));
  args.insertOrAssign(WSFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<IDataArray>(outputPath);
  scope.requireExpectedStore(outArray);
  RequireExactLabels(outArray, oracle);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKMorphologicalWatershedImageTest.cpp on OUR ITK-free filter
// (cthead1-grad-mag.nrrd input). Output is a FIXED uint32 label image (AlwaysUInt32); (A) DURABLE golden =
// md5-validity-first (plan Sec.4), (B) LIVE-ITK parity = BIT-EXACT (uint32 labels, CompareImages@0.0). The legacy ITK
// MorphologicalWatershed wrapper IS enabled, so LegacyUuidFor resolves and the md5 helper's normal (B) arm runs.
// Helper pins ForceInCore.
//   defaults: filter defaults (Level 0.0, MarkWatershedLine true, FullyConnected false -- identical vs legacy ITK).
//   level_1:  Level = 1.0, MarkWatershedLine = false.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MorphologicalWatershedImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<MorphologicalWatershedImageFilter>("cthead1-grad-mag.nrrd", "406079d7904d4e9ab0b5f29f7a3a1ea8");
}

TEST_CASE("ImageProcessing::MorphologicalWatershedImageFilter: ITK real-image golden (level_1)", "[ImageProcessing][ItkGolden][MorphologicalWatershedImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<MorphologicalWatershedImageFilter>("cthead1-grad-mag.nrrd", "a204ce7cf8ec4e7bc6538f0515a8910e", [](Arguments& args) {
    args.insertOrAssign(MorphologicalWatershedImageFilter::k_Level_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(MorphologicalWatershedImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(false));
  });
}
