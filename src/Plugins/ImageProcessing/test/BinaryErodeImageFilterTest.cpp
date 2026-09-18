#include "MorphologyFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BinaryErodeImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <string_view>

using namespace nx::core;

namespace
{
// Legacy ITKBinaryErodeImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyBinaryErodeUuid = *Uuid::FromString("243dd30b-d1f0-42ad-8b47-77d57f9fc262");

// The (Box, 3D r{2,1,0}) parity cell is excluded from LIVE legacy comparison (same as the grayscale filters):
// the legacy decomposable-Box anchor path is broken for a zero-radius axis on a 3D image.
const auto k_ExcludeBoxZeroRadius = [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; };
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}, across
//     BoundaryToForeground in {true,false} (true is the erosion default) and varied foreground/background. On a
//     strictly binary input the new filter must reproduce the legacy ITK Binary Erode output EXACTLY. Annulus is
//     excluded (empty legacy kernel); (Box, 3D r{2,1,0}) is excluded from live parity (broken legacy anchor
//     path). Storage is forced in-core (so legacy ITK can run) which also exercises the new filter's in-core
//     moving fg-count Direct path.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][BinaryErodeImageFilter]")
{
  // A STRICTLY binary input is required: this filter emits a strictly {fg, bg} image (and its façade rejects any
  // non-{fg, bg} input), matching legacy ITK exactly on a binary image (legacy preserves non-foreground voxels,
  // so they only agree when every non-foreground voxel already equals bg). fg/bg are varied across sections
  // (bg matches the input's background), as is BoundaryToForeground.
  const auto build410 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto build10013 = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{100}, uint8{13}); };

  SECTION("BoundaryToForeground=true fg=41 bg=0")
  {
    morph_test::RunMorphologyParityGrid<BinaryErodeImageFilter, uint8>(k_LegacyBinaryErodeUuid, build410, morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(41.0, 0.0, true),
                                                                       k_ExcludeBoxZeroRadius);
  }
  SECTION("BoundaryToForeground=false fg=41 bg=0")
  {
    morph_test::RunMorphologyParityGrid<BinaryErodeImageFilter, uint8>(k_LegacyBinaryErodeUuid, build410, morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(41.0, 0.0, false),
                                                                       k_ExcludeBoxZeroRadius);
  }
  SECTION("BoundaryToForeground=true fg=100 bg=13 (varied fg/bg)")
  {
    morph_test::RunMorphologyParityGrid<BinaryErodeImageFilter, uint8>(k_LegacyBinaryErodeUuid, build10013, morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(100.0, 13.0, true),
                                                                       k_ExcludeBoxZeroRadius);
  }
}

// -----------------------------------------------------------------------------
// (2) Binary-input contract: the façade emits a strictly {fg, bg} image and therefore REJECTS a non-binary
//     input at execute. BuildMorphologyImage fills values in [0,199], so with fg=1/bg=0 many voxels are neither
//     foreground nor background: preflight passes (fg/bg are in range) but execute must fail with the
//     k_NonBinaryInput safeguard code. MorphologyEngineTest covers Annulus and zero-radius kernel correctness
//     across all four kernels.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Rejects non-binary input at execute", "[ImageProcessing][BinaryErodeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // values in [0,199] -> NOT binary

  BinaryErodeImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryErodeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
  args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(true));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions); // fg/bg are in range -> preflight is clean
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid()); // non-binary input rejected at execute
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
}

TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Reports the first non-binary value and index", "[ImageProcessing][BinaryErodeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 16, 16, 16);
  constexpr uint8 fg = 1;
  constexpr uint8 bg = 0;
  constexpr uint8 value = 9;
  constexpr usize index = 1200;
  AbstractDataStore<uint8>* inputStorePtr = nullptr;
  REQUIRE_NOTHROW(inputStorePtr = &ds.getDataRefAs<DataArray<uint8>>(inputPath).getDataStoreRef());
  REQUIRE(inputStorePtr != nullptr);
  for(usize i = 0; i < inputStorePtr->getSize(); ++i)
  {
    inputStorePtr->setValue(i, (i % usize{3} == usize{0}) ? fg : bg);
  }
  inputStorePtr->setValue(usize{3000}, uint8{7});
  inputStorePtr->setValue(index, value);

  BinaryErodeImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryErodeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(static_cast<float64>(fg)));
  args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(bg)));
  args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
  const std::string expectedMessage =
      fmt::format("Binary morphology requires a binary image containing only the foreground ({}) or background ({}) value, but input array '{}' contains the value {} at index {}. Threshold or "
                  "relabel the input first.",
                  static_cast<int64>(fg), static_cast<int64>(bg), inputPath.toString(), static_cast<int64>(value), index);
  REQUIRE(executeResult.result.errors()[0].message == expectedMessage);
}

// -----------------------------------------------------------------------------
// (2b) Degenerate empty structuring element (Annulus r{0,0,0}) on a binary input must pass the input through
//      unchanged (the passthrough copies the input verbatim), exercising the binary engine's offsets.empty()
//      branch at the filter level.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Empty-SE passthrough (Annulus r{0,0,0})", "[ImageProcessing][BinaryErodeImageFilter]")
{
  const auto buildBinary = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(ds, dx, dy, dz, uint8{41}, uint8{0}); };
  morph_test::RunMorphologyEmptySEPassthrough<BinaryErodeImageFilter, uint8>(buildBinary, 12, 12, 12, morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(41.0, 0.0, true));
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected. The binary morphology façade routes through ApplyBinaryMorphology's
//     DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the in-core moving fg-count Direct
//     path and the out-of-core Scanline path on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The
//     legacy ITK filter rejects out-of-core arrays and is NOT run; instead the output is compared to the
//     independent fgCount oracle computed over the actual input. Real-disk out-of-core coverage is retired to the
//     generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Computed-expected (both algorithm paths)", "[ImageProcessing][BinaryErodeImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr uint8 fg = 1;
  constexpr uint8 bg = 0;
  constexpr bool b2f = true; // erosion default
  const std::array<int32, 3> radius = {2, 1, 1};
  const auto kernelType = morph_test::KernelType::Box;

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildBinaryPatternImage<uint8>(ds, D, fg, bg);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  BinaryErodeImageFilter filter;
  Arguments args;
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BinaryErodeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BinaryErodeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
  args.insertOrAssign(BinaryErodeImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])}));
  args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(static_cast<float64>(fg)));
  args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(bg)));
  args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(b2f));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  // Stream the ACTUAL input into memory (bulk) and compute the independent fgCount oracle over it -- structurally
  // independent of the engine's slab streaming / moving accumulator.
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
  const std::vector<uint8> expected = morph_test::BinaryMorphologyFgOracle<uint8>(input, D, D, D, se, ImageProcessing::MorphOp::Erode, fg, bg, b2f);

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
// (4) Preflight guard: the Float64 foreground/background parameters are cast to the (integer) input element
//     type; a non-finite or out-of-range value would be an undefined conversion, so the filter must reject it
//     at preflight. In-range integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Preflight rejects non-finite / out-of-range fg/bg for integer input", "[ImageProcessing][BinaryErodeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(ds, 8, 8, 8); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(BinaryErodeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(BinaryErodeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(BinaryErodeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(BinaryErodeImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
    args.insertOrAssign(BinaryErodeImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(BinaryErodeImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(BinaryErodeImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    args.insertOrAssign(BinaryErodeImageFilter::k_BoundaryToForeground_Key, std::make_any<bool>(true));
    return args;
  };

  BinaryErodeImageFilter filter;

  // Foreground out of uint8 range (255 max) -> undefined cast -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(99999.0, 0.0)).outputActions.valid());
  // Background out of range on the negative side -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, -5.0)).outputActions.valid());
  // Non-finite (NaN) foreground -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(std::numeric_limits<float64>::quiet_NaN(), 0.0)).outputActions.valid());
  // Non-finite (Inf) background -> preflight error.
  REQUIRE_FALSE(filter.preflight(ds, makeArgs(1.0, std::numeric_limits<float64>::infinity())).outputActions.valid());
  // In-range integer values still preflight cleanly.
  auto preflightResult = filter.preflight(ds, makeArgs(5.0, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
}

// -----------------------------------------------------------------------------
// (5) ITK-sourced real-image md5 golden. The ITK BinaryErode test uses STAPLE1.png, but STAPLE1 is a 195-value
//     grayscale image (verified) that ITK's binary morphology binarizes at foreground==255 while PRESERVING every
//     other pixel's original value; this filter instead requires a strictly-{foreground, background} input and
//     rejects STAPLE1 at execute (k_NonBinaryInput), so STAPLE1 has no faithful analogue here (see the "Rejects
//     non-binary input at execute" test above). WhiteDots.png IS strictly binary (verified: only values {0, 255}),
//     so it satisfies the binary-input contract with fg=255/bg=0 and the legacy ITK "preserve non-foreground
//     original value" behavior coincides with our background emission (every non-foreground voxel already equals
//     bg). Ball radius {1,1,1}, BoundaryToForeground=true (the erosion default). (B) live-ITK parity FIRST
//     (bit-exact), then (A) md5 == committed hash (OUR ITK-free reader's computed hash, the durable pin).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: ITK real-image golden (WhiteDots)", "[ImageProcessing][ItkGolden][BinaryErodeImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<BinaryErodeImageFilter>("WhiteDots.png", "55f752c7208878f90ffb30a8df8f6382", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(255.0, 0.0, /*boundaryToForeground=*/true));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (k_NonScalarInput == -8440) fires before the shared PreflightImageFilter, so a 3-component input is rejected.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BinaryErodeImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][BinaryErodeImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<BinaryErodeImageFilter, uint8>(-8440, morph_test::BinaryKernelParamSetter<BinaryErodeImageFilter>(1.0, 0.0, /*boundaryToForeground=*/false));
}
