#include "MorphologyFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryMorphologicalClosingImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string_view>

using namespace nx::core;

namespace
{
using ClosingFilter = BinaryMorphologicalClosingImageFilter;

// Legacy ITKBinaryMorphologicalClosingImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyBinaryClosingUuid = *Uuid::FromString("abb27e0c-b049-4f60-8355-178d86bb1de4");

// The (Box, 3D r{2,1,0}) parity cell is excluded from LIVE legacy comparison (same as the other morphology
// filters): the legacy decomposable-Box anchor path is broken for a zero-radius axis on a 3D image.
const auto k_ExcludeBoxZeroRadius = [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; };

// Kernel + Foreground + Safe Border param setter for Binary Morphological Closing. Shares the legacy filter's
// key strings, so the parity grid drives BOTH the new and the legacy filter with it. Closing has no user
// Background Value (it derives its internal background from the foreground).
morph_test::KernelParamSetter ClosingKernelParamSetter(float64 foreground, bool safeBorder)
{
  return [foreground, safeBorder](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(ClosingFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(ClosingFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(ClosingFilter::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(ClosingFilter::k_SafeBorder_Key, std::make_any<bool>(safeBorder));
  };
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}, across
//     Safe Border in {true (the ITK default), false} and varied foreground. Closing =
//     binaryDilate(boundaryToForeground=false) then binaryErode(boundaryToForeground=true). Since closing is
//     extensive, its internal background is 0 for a non-zero foreground, so the strictly-binary input uses
//     background 0 (matching the internal background). On such input the new filter must reproduce the legacy
//     ITK Binary Morphological Closing output EXACTLY. Annulus is excluded (empty legacy kernel); the
//     (Box, 3D r{2,1,0}) cell is excluded from live parity and gated by a computed-expected case in (2).
//     Storage is forced in-core (so legacy ITK can run) which also exercises the new filter's in-core moving
//     fg-count Direct path.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  const auto build410 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto build1000 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{100}, uint8{0}); };
  // Foreground 0 forces the internal background to the type maximum (255 for uint8), so the strictly-binary
  // input uses background 255. The runner's internalBg derivation matches ITK's closing exactly here, so this
  // case IS live-legacy parity (not merely computed-expected) -- guarding the fg==0 branch against a regression.
  const auto build0 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{0}, uint8{255}); };

  SECTION("SafeBorder=true fg=41")
  {
    morph_test::RunMorphologyParityGrid<ClosingFilter, uint8>(k_LegacyBinaryClosingUuid, build410, ClosingKernelParamSetter(41.0, true), k_ExcludeBoxZeroRadius);
  }
  SECTION("SafeBorder=false fg=41")
  {
    morph_test::RunMorphologyParityGrid<ClosingFilter, uint8>(k_LegacyBinaryClosingUuid, build410, ClosingKernelParamSetter(41.0, false), k_ExcludeBoxZeroRadius);
  }
  SECTION("SafeBorder=true fg=100 (varied fg)")
  {
    morph_test::RunMorphologyParityGrid<ClosingFilter, uint8>(k_LegacyBinaryClosingUuid, build1000, ClosingKernelParamSetter(100.0, true), k_ExcludeBoxZeroRadius);
  }
  SECTION("SafeBorder=true fg=0 (internal background = type max)")
  {
    morph_test::RunMorphologyParityGrid<ClosingFilter, uint8>(k_LegacyBinaryClosingUuid, build0, ClosingKernelParamSetter(0.0, true), k_ExcludeBoxZeroRadius);
  }
}

// -----------------------------------------------------------------------------
// (1b) Live-legacy parity on a 2D (Z=1) image with a NON-ZERO Z-radius under Safe Border == true -- the exact cell
//      the fixed-config parity grid in (1) cannot reach (its only 2D case is r{2,2,0}, rz==0, so no out-of-plane
//      SE offsets). This is the BINARY regression for the composite Safe Border 2D size-1-axis pad defect (fixed by
//      SafeBorderPadRadius in ImageProcessingFilterUtilities.hpp; the grayscale side is gated by the ITK real-image
//      goldens). On a Z=1 image a Ball/Cross SE with rz>0 has out-of-plane offsets; before the fix, padding the Z
//      axis by the SE Z-radius gave those offsets real (contaminated) planes to read, diverging from ITK. ITK
//      collapses the Z=1 image to a 2D image with a 2D kernel; with the pad clamped to 0 on the size-1 axis the new
//      filter reproduces the legacy ITK binary closing EXACTLY. Storage forced in-core so legacy ITK can run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Legacy parity 2D SafeBorder=true nonzero-Z-radius", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyBinaryClosingUuid) != nullptr);

  const auto setParams = ClosingKernelParamSetter(41.0, /*safeBorder=*/true);
  const std::vector<std::pair<const char*, std::pair<uint64, std::vector<uint32>>>> cases = {{"Ball r{2,2,1}", {static_cast<uint64>(morph_test::KernelType::Ball), {2, 2, 1}}},
                                                                                             {"Cross r{3,3,2}", {static_cast<uint64>(morph_test::KernelType::Cross), {3, 3, 2}}}};

  for(const auto& [label, kernelCase] : cases)
  {
    DYNAMIC_SECTION(label)
    {
      const auto build = [](DataStructure& ds) { return morph_test::BuildBinaryLabelImage<uint8>(ds, 20, 16, 1, uint8{41}, uint8{0}); };

      DataStructure newDs;
      const DataPath newInput = build(newDs);
      ClosingFilter newFilter;
      morph_test::detail::RunFilter<ClosingFilter>(newFilter, newDs, newInput, kernelCase.first, kernelCase.second, setParams);

      IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyBinaryClosingUuid);
      REQUIRE(legacyFilter != nullptr);
      DataStructure legacyDs;
      const DataPath legacyInput = build(legacyDs);
      morph_test::detail::RunFilter<ClosingFilter>(*legacyFilter, legacyDs, legacyInput, kernelCase.first, kernelCase.second, setParams);

      const DataPath outputPath({"Image Geometry", "CellData", "Output"});
      const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
      const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
      REQUIRE(newOut.getDataType() == legacyOut.getDataType());
      UnitTest::CompareDataArrays<uint8>(newOut, legacyOut);
    }
  }
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the fg-count SE-gather passes
//     (closing == erode(dilate(in))). Driven with Safe Border == FALSE so the non-padded oracle applies (the
//     padded Safe Border == true convention is covered by the parity grid in (1) and the cross-storage OOC
//     check in (5)). Validates Annulus (this filter ships ITK's real thickness-1 shell, whereas the legacy
//     SimpleITK Annulus was empty) and the (Box, 3D r{2,1,0}) cell excluded from (1). The fg=0 section exercises
//     the internal-background = type-max branch (input {0, 255}).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Computed-expected (safeBorder=false; Annulus + Box zero-radius)", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  using morph_test::BinaryCompositeKind;
  using morph_test::KernelType;
  const auto build410 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto setParams41 = ClosingKernelParamSetter(41.0, /*safeBorder=*/false);
  constexpr auto kClosing = BinaryCompositeKind::Closing;

  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<ClosingFilter, uint8>(build410, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kClosing, uint8{41}, uint8{0}, setParams41);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<ClosingFilter, uint8>(build410, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kClosing, uint8{41}, uint8{0}, setParams41);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<ClosingFilter, uint8>(build410, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kClosing, uint8{41}, uint8{0}, setParams41);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<ClosingFilter, uint8>(build410, 12, 12, 12, KernelType::Box, {2, 1, 0}, kClosing, uint8{41}, uint8{0}, setParams41);
  }
}

// -----------------------------------------------------------------------------
// (3) Binary-input contract: the façade emits a strictly {fg, internal-bg} image and REJECTS a non-binary input
//     at execute. BuildMorphologyImage fills values in [0,199], so with fg=1 (internal bg=0) many voxels are
//     neither foreground nor background: preflight passes but execute must fail with the k_NonBinaryInput
//     safeguard code (the composite runs the safeguard exactly once on the original input).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Rejects non-binary input at execute", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // values in [0,199] -> NOT binary

  ClosingFilter filter;
  Arguments args;
  args.insertOrAssign(ClosingFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ClosingFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ClosingFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(ClosingFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
  args.insertOrAssign(ClosingFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(ClosingFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(ClosingFilter::k_SafeBorder_Key, std::make_any<bool>(true));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // fg is in range -> preflight is clean
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid()); // non-binary input rejected at execute
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
}

// -----------------------------------------------------------------------------
// (4) Both-algorithm-paths computed-expected (Safe Border == false). The binary composite routes through
//     ApplyBinaryMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the in-core
//     moving fg-count Direct path and the out-of-core Scanline path on in-memory stores (selected by
//     SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is NOT run; instead the
//     output is compared to the independent composite fg-count oracle computed over the actual input. Real-disk
//     out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Computed-expected (safeBorder=false; both algorithm paths)", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr uint8 fg = 1;
  constexpr uint8 bg = 0; // internal background for a non-zero foreground
  const std::array<int32, 3> radius = {2, 1, 1};
  const auto kernelType = morph_test::KernelType::Box;

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildBinaryPatternImage<uint8>(ds, D, fg, bg);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  ClosingFilter filter;
  Arguments args;
  args.insertOrAssign(ClosingFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ClosingFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ClosingFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(ClosingFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
  args.insertOrAssign(ClosingFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])}));
  args.insertOrAssign(ClosingFilter::k_ForegroundValue_Key, std::make_any<float64>(static_cast<float64>(fg)));
  args.insertOrAssign(ClosingFilter::k_SafeBorder_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  // Stream the ACTUAL input into memory (bulk) and compute the independent composite fg-count oracle over it.
  constexpr usize k_ChunkValues = 65536;
  std::vector<uint8> input(numSlots);
  const auto& inStore = ds.getDataRefAs<DataArray<uint8>>(inputPath).getDataStoreRef();
  for(usize start = 0; start < numSlots; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> r = inStore.copyIntoBuffer(start, nonstd::span<uint8>(input.data() + start, count));
    SIMPLNX_RESULT_REQUIRE_VALID(r);
  }
  const morph_test::StructuringElement se = morph_test::MakeStructuringElement(kernelType, radius);
  const std::vector<uint8> expected = morph_test::BinaryMorphologyCompositeFgOracle<uint8>(input, D, D, D, se, morph_test::BinaryCompositeKind::Closing, fg, bg);

  // Stream the output in bounded chunks and compare.
  auto buffer = std::make_unique<uint8[]>(k_ChunkValues);
  bool allMatch = true;
  usize badSlot = 0;
  for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<uint8>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    for(usize i = 0; i < count; ++i)
    {
      if(buffer[i] != expected[start + i])
      {
        allMatch = false;
        badSlot = start + i;
        break;
      }
    }
  }
  if(!allMatch)
  {
    UNSCOPED_INFO("slot=" << badSlot << " got=" << static_cast<int>(buffer[badSlot % k_ChunkValues]) << " expected=" << static_cast<int>(expected[badSlot]));
  }
  REQUIRE(allMatch);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (6) Preflight guard: the Float64 Foreground Value is cast to the (integer) input element type; a non-finite or
//     out-of-range value would be an undefined conversion, so the filter must reject it at preflight. In-range
//     integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Preflight rejects non-finite / out-of-range foreground", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 fg) {
    Arguments args;
    args.insertOrAssign(ClosingFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(ClosingFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ClosingFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ClosingFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
    args.insertOrAssign(ClosingFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(ClosingFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(ClosingFilter::k_SafeBorder_Key, std::make_any<bool>(true));
    return args;
  };

  ClosingFilter filter;

  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0)).outputActions.valid());                                   // fg out of uint8 range
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(-5.0)).outputActions.valid());                                      // fg negative out of range
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN())).outputActions.valid()); // NaN fg
  auto preflightResult = filter.preflight(ds, makeArgs(1.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // in-range integer preflights cleanly
}

// -----------------------------------------------------------------------------
// (7) ITK-sourced real-image md5 golden -- duplicates ITKBinaryMorphologicalClosingImageTest.cpp
//     (BinaryMorphologicalClosingWithBorder): WhiteDots.png (uint8), Ball radius {5,5,5}, Safe Border FALSE,
//     Foreground 255. WhiteDots.png is a STRICTLY binary image (verified: only values {0, 255}), so it satisfies
//     this filter's strictly-{foreground, internal-background} input contract (fg=255 -> internal bg=0, which is the
//     WhiteDots background), and the legacy ITK "preserve non-foreground original value" behavior coincides with our
//     internal-background emission. (A) md5 == committed hash + (B) live-ITK bit-exact.
//
//     NOTE (contract divergence, intentionally NOT ported): the ITK "BinaryMorphologicalClosing" default case uses
//     STAPLE1.png, which is a 195-value grayscale image (verified), not a binary image. ITK's binary morphology
//     binarizes at foreground==255 and PRESERVES every other pixel's original grayscale value (its committed hash
//     095f00a68a84df4396914fa758f34dcc is the unchanged-STAPLE1 identity hash), whereas this filter requires a
//     strictly-binary input and rejects STAPLE1 at execute (k_NonBinaryInput). That case therefore has no faithful
//     analogue here and is deliberately omitted (see the "Rejects non-binary input at execute" test above).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: ITK real-image golden (WhiteDots WithBorder)", "[ImageProcessing][ItkGolden][BinaryMorphologicalClosingImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<ClosingFilter>("WhiteDots.png", "506d365dd92db16c2ade264fca46890c", static_cast<uint64>(morph_test::KernelType::Ball), {5, 5, 5},
                                                       ClosingKernelParamSetter(255.0, /*safeBorder=*/false));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (k_NonScalarInput == -8490) fires before the shared PreflightImageFilter, so a 3-component input is rejected.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalClosingImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][BinaryMorphologicalClosingImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<ClosingFilter, uint8>(-8490, ClosingKernelParamSetter(1.0, /*safeBorder=*/true));
}
