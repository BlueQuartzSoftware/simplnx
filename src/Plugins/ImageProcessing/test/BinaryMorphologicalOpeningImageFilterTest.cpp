#include "MorphologyFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string_view>

using namespace nx::core;

namespace
{
using OpeningFilter = BinaryMorphologicalOpeningImageFilter;

// Legacy ITKBinaryMorphologicalOpeningImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyBinaryOpeningUuid = *Uuid::FromString("861ccb46-dbce-41bf-a66f-25cc18cd1073");

// The (Box, 3D r{2,1,0}) parity cell is excluded from LIVE legacy comparison (same as the other morphology
// filters): the legacy decomposable-Box anchor path is broken for a zero-radius axis on a 3D image.
const auto k_ExcludeBoxZeroRadius = [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; };

// Kernel + Foreground/Background param setter for Binary Morphological Opening. Shares the legacy filter's key
// strings, so the parity grid drives BOTH the new and the legacy filter with it.
morph_test::KernelParamSetter OpeningKernelParamSetter(float64 foreground, float64 background)
{
  return [foreground, background](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(OpeningFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(OpeningFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(radius));
    args.insertOrAssign(OpeningFilter::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(OpeningFilter::k_BackgroundValue_Key, std::make_any<float64>(background));
  };
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}, across
//     varied foreground/background. Opening = binaryErode(boundaryToForeground=true) then
//     binaryDilate(boundaryToForeground=false). On a strictly binary input the new filter must reproduce the
//     legacy ITK Binary Morphological Opening output EXACTLY. Annulus is excluded (empty legacy kernel);
//     (Box, 3D r{2,1,0}) is excluded from live parity (broken legacy anchor path) and is instead gated by a
//     computed-expected case in (2). Storage is forced in-core (so legacy ITK can run) which also exercises the
//     new filter's in-core moving fg-count Direct path.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  const auto build410 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto build10013 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{100}, uint8{13}); };

  SECTION("fg=41 bg=0")
  {
    morph_test::RunMorphologyParityGrid<OpeningFilter, uint8>(k_LegacyBinaryOpeningUuid, build410, OpeningKernelParamSetter(41.0, 0.0), k_ExcludeBoxZeroRadius);
  }
  SECTION("fg=100 bg=13 (varied fg/bg)")
  {
    morph_test::RunMorphologyParityGrid<OpeningFilter, uint8>(k_LegacyBinaryOpeningUuid, build10013, OpeningKernelParamSetter(100.0, 13.0), k_ExcludeBoxZeroRadius);
  }
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the fg-count SE-gather passes
//     (opening == dilate(erode(in))). Validates Annulus (this filter ships ITK's real thickness-1 shell, whereas
//     the legacy SimpleITK Annulus was empty) and the (Box, 3D r{2,1,0}) cell excluded from (1) (broken legacy
//     anchor path). fg=41/bg=0.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Computed-expected (Annulus + Box zero-radius)", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  using morph_test::BinaryCompositeKind;
  using morph_test::KernelType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto setParams = OpeningKernelParamSetter(41.0, 0.0);
  constexpr auto kOpening = BinaryCompositeKind::Opening;

  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilter, uint8>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kOpening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilter, uint8>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kOpening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilter, uint8>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kOpening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilter, uint8>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kOpening, uint8{41}, uint8{0}, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) Binary-input contract: the façade emits a strictly {fg, bg} image and therefore REJECTS a non-binary
//     input at execute. BuildMorphologyImage fills values in [0,199], so with fg=1/bg=0 many voxels are neither
//     foreground nor background: preflight passes (fg/bg are in range) but execute must fail with the
//     k_NonBinaryInput safeguard code (the composite runs the safeguard exactly once on the original input).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Rejects non-binary input at execute", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // values in [0,199] -> NOT binary

  OpeningFilter filter;
  Arguments args;
  args.insertOrAssign(OpeningFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OpeningFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OpeningFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OpeningFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
  args.insertOrAssign(OpeningFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(OpeningFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(OpeningFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // fg/bg are in range -> preflight is clean
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid()); // non-binary input rejected at execute
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
}

// -----------------------------------------------------------------------------
// (4) Both-algorithm-paths computed-expected. The binary composite routes through ApplyBinaryMorphology's
//     DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the in-core moving fg-count Direct
//     path and the out-of-core Scanline path on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The
//     legacy ITK filter rejects out-of-core arrays and is NOT run; instead the output is compared to the
//     independent composite fg-count oracle computed over the actual input (Opening never pads). Real-disk
//     out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Computed-expected (both algorithm paths)", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr uint8 fg = 1;
  constexpr uint8 bg = 0;
  const std::array<int32, 3> radius = {2, 1, 1};
  const auto kernelType = morph_test::KernelType::Box;

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildBinaryPatternImage<uint8>(ds, D, fg, bg);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  OpeningFilter filter;
  Arguments args;
  args.insertOrAssign(OpeningFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OpeningFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OpeningFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OpeningFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
  args.insertOrAssign(OpeningFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])}));
  args.insertOrAssign(OpeningFilter::k_ForegroundValue_Key, std::make_any<float64>(static_cast<float64>(fg)));
  args.insertOrAssign(OpeningFilter::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(bg)));

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
  const std::vector<uint8> expected = morph_test::BinaryMorphologyCompositeFgOracle<uint8>(input, D, D, D, se, morph_test::BinaryCompositeKind::Opening, fg, bg);

  // Stream the output in bounded chunks and compare (avoids per-element OOC access and per-voxel assertions).
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
// (5) Preflight guard: the Float64 foreground/background parameters are cast to the (integer) input element
//     type; a non-finite or out-of-range value would be an undefined conversion, so the filter must reject it
//     at preflight. In-range integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Preflight rejects non-finite / out-of-range fg/bg for integer input", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(OpeningFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(OpeningFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(OpeningFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(OpeningFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
    args.insertOrAssign(OpeningFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(OpeningFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(OpeningFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    return args;
  };

  OpeningFilter filter;

  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0, 0.0)).outputActions.valid());                                   // fg out of uint8 range
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, -5.0)).outputActions.valid());                                      // bg negative out of range
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN(), 0.0)).outputActions.valid()); // NaN fg
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, std::numeric_limits<float64>::infinity())).outputActions.valid());  // Inf bg
  auto preflightResult = filter.preflight(ds, makeArgs(5.0, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // in-range integers preflight cleanly
}

// -----------------------------------------------------------------------------
// (6) ITK-sourced real-image md5 golden. The ITK BinaryMorphologicalOpening test uses STAPLE1.png, but STAPLE1 is a
//     195-value grayscale image (verified) that ITK's binary morphology binarizes at foreground==255 while
//     PRESERVING every other pixel's original value; this filter instead requires a strictly-{foreground,
//     background} input and rejects STAPLE1 at execute (k_NonBinaryInput), so STAPLE1 has no faithful analogue here
//     (see the "Rejects non-binary input at execute" test above). WhiteDots.png IS strictly binary (verified: only
//     values {0, 255}), so it satisfies the binary-input contract with fg=255/bg=0 and the legacy ITK "preserve
//     non-foreground original value" behavior coincides with our background emission (every non-foreground voxel
//     already equals bg). Opening = erode(boundaryToForeground=true) then dilate(boundaryToForeground=false); Ball
//     radius {1,1,1}. (B) live-ITK parity FIRST (bit-exact), then (A) md5 == committed hash (OUR ITK-free reader's
//     computed hash, the durable pin).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: ITK real-image golden (WhiteDots)", "[ImageProcessing][ItkGolden][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<OpeningFilter>("WhiteDots.png", "3dcb8bce1c174f358608927212aa4cf7", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                       OpeningKernelParamSetter(255.0, 0.0));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (k_NonScalarInput == -8480) fires before the shared PreflightImageFilter, so a 3-component input is rejected.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryMorphologicalOpeningImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<OpeningFilter, uint8>(-8480, OpeningKernelParamSetter(1.0, 0.0));
}
