#include "MorphologyFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/BinaryMorphologicalOpeningImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>

using namespace nx::core;

namespace
{
using OpeningFilterType = BinaryMorphologicalOpeningImageFilter;

// Kernel + Foreground/Background param setter for Binary Morphological Opening.
morph_test::KernelParamSetterType OpeningKernelParamSetter(float64 foreground, float64 background)
{
  return [foreground, background](Arguments& args, uint64 kernelType, const std::vector<uint32>& radius) {
    args.insertOrAssign(OpeningFilterType::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
    args.insertOrAssign(OpeningFilterType::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(radius));
    args.insertOrAssign(OpeningFilterType::k_ForegroundValue_Key, std::make_any<float64>(foreground));
    args.insertOrAssign(OpeningFilterType::k_BackgroundValue_Key, std::make_any<float64>(background));
  };
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Computed-expected: an independent oracle composed from the fg-count SE-gather passes
//     (opening == dilate(erode(in))). It validates Annulus (the filter rasterizes the real thickness-1 shell)
//     and the (Box, 3D r{2,1,0}) cell, which has a zero-radius axis on a 3D image. fg=41/bg=0.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Computed-expected (Annulus + Box zero-radius)", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  using morph_test::BinaryCompositeKind;
  using morph_test::KernelType;
  const auto build = [](DataStructure& dataStructure, usize dx, usize dy, usize dz) { return morph_test::BuildBinaryLabelImage<uint8>(dataStructure, dx, dy, dz, uint8{41}, uint8{0}); };
  const auto setParams = OpeningKernelParamSetter(41.0, 0.0);
  constexpr auto k_Opening = BinaryCompositeKind::Opening;

  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilterType, uint8>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, k_Opening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilterType, uint8>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, k_Opening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilterType, uint8>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, k_Opening, uint8{41}, uint8{0}, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunBinaryMorphologyCompositeComputedExpected<OpeningFilterType, uint8>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, k_Opening, uint8{41}, uint8{0}, setParams);
  }
}

// -----------------------------------------------------------------------------
// (2) Binary-input contract: the façade emits a strictly {fg, bg} image and therefore REJECTS a non-binary
//     input at execute. BuildMorphologyImage fills values in [0,199], so with fg=1/bg=0 many voxels are neither
//     foreground nor background: preflight passes (fg/bg are in range) but execute must fail with the
//     k_NonBinaryInput safeguard code (the composite runs the safeguard exactly once on the original input).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Rejects non-binary input at execute", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(dataStructure, 8, 8, 8); // values in [0,199] -> NOT binary

  OpeningFilterType filter;
  Arguments args;
  args.insertOrAssign(OpeningFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OpeningFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OpeningFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OpeningFilterType::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
  args.insertOrAssign(OpeningFilterType::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(OpeningFilterType::k_ForegroundValue_Key, std::make_any<float64>(1.0));
  args.insertOrAssign(OpeningFilterType::k_BackgroundValue_Key, std::make_any<float64>(0.0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions); // fg/bg are in range -> preflight is clean
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid()); // non-binary input rejected at execute
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors()[0].code == ImageProcessing::k_NonBinaryInput);
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected. The binary composite routes through ApplyBinaryMorphology's
//     DispatchAlgorithm, so the AlgorithmTestScope runs the filter under BOTH the in-core moving fg-count Direct
//     path and the out-of-core Scanline path on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The
//     test compares the output with the independent composite fg-count oracle computed over the actual input
//     (Opening never pads). Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Computed-expected (both algorithm paths)", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  unit_test::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  constexpr usize k_D = 32;
  constexpr uint8 k_Fg = 1;
  constexpr uint8 k_Bg = 0;
  const std::array<int32, 3> radius = {2, 1, 1};
  const auto kernelType = morph_test::KernelType::Box;

  DataStructure dataStructure;
  const DataPath inputPath = morph_test::BuildBinaryPatternImage<uint8>(dataStructure, k_D, k_Fg, k_Bg);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  OpeningFilterType filter;
  Arguments args;
  args.insertOrAssign(OpeningFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OpeningFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OpeningFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OpeningFilterType::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(kernelType)));
  args.insertOrAssign(OpeningFilterType::k_KernelRadius_Key,
                      std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{static_cast<uint32>(radius[0]), static_cast<uint32>(radius[1]), static_cast<uint32>(radius[2])}));
  args.insertOrAssign(OpeningFilterType::k_ForegroundValue_Key, std::make_any<float64>(static_cast<float64>(k_Fg)));
  args.insertOrAssign(OpeningFilterType::k_BackgroundValue_Key, std::make_any<float64>(static_cast<float64>(k_Bg)));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = k_D * k_D * k_D;
  REQUIRE(outStore.getSize() == numSlots);

  // Stream the ACTUAL input into memory (bulk) and compute the independent composite fg-count oracle over it.
  constexpr usize k_ChunkValues = 65536;
  std::vector<uint8> input(numSlots);
  const auto& inStore = dataStructure.getDataRefAs<DataArray<uint8>>(inputPath).getDataStoreRef();
  for(usize start = 0; start < numSlots; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> r = inStore.copyIntoBuffer(start, nonstd::span<uint8>(input.data() + start, count));
    SIMPLNX_RESULT_REQUIRE_VALID(r);
  }
  const morph_test::StructuringElement se = morph_test::MakeStructuringElement(kernelType, radius);
  const std::vector<uint8> expected = morph_test::BinaryMorphologyCompositeFgOracle<uint8>(input, k_D, k_D, k_D, se, morph_test::BinaryCompositeKind::Opening, k_Fg, k_Bg);

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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (4) Preflight guard: the Float64 foreground/background parameters are cast to the (integer) input element
//     type; a non-finite or out-of-range value would be an undefined conversion, so the filter must reject it
//     at preflight. In-range integer values must still preflight cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Preflight rejects non-finite / out-of-range fg/bg for integer input", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = morph_test::BuildMorphologyImage<uint8>(dataStructure, 8, 8, 8); // uint8 input, range [0, 255]

  const auto makeArgs = [&](float64 fg, float64 bg) {
    Arguments args;
    args.insertOrAssign(OpeningFilterType::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(OpeningFilterType::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(OpeningFilterType::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(OpeningFilterType::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Ball)));
    args.insertOrAssign(OpeningFilterType::k_KernelRadius_Key, std::make_any<VectorUInt32ParameterType::ValueType>(std::vector<uint32>{1, 1, 1}));
    args.insertOrAssign(OpeningFilterType::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(OpeningFilterType::k_BackgroundValue_Key, std::make_any<float64>(bg));
    return args;
  };

  OpeningFilterType filter;

  REQUIRE_FALSE(filter.preflight(dataStructure, makeArgs(99999.0, 0.0)).output_actions.valid());                                   // fg out of uint8 range
  REQUIRE_FALSE(filter.preflight(dataStructure, makeArgs(1.0, -5.0)).output_actions.valid());                                      // bg negative out of range
  REQUIRE_FALSE(filter.preflight(dataStructure, makeArgs(std::numeric_limits<float64>::quiet_NaN(), 0.0)).output_actions.valid()); // NaN fg
  REQUIRE_FALSE(filter.preflight(dataStructure, makeArgs(1.0, std::numeric_limits<float64>::infinity())).output_actions.valid());  // Inf bg
  auto preflightResult = filter.preflight(dataStructure, makeArgs(5.0, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions); // in-range integers preflight cleanly
}

// -----------------------------------------------------------------------------
// (5) ITK-sourced real-image md5 golden. The ITK BinaryMorphologicalOpening test uses STAPLE1.png, but STAPLE1 is a
//     195-value grayscale image (verified). This filter requires a strictly-{foreground, background} input and
//     rejects STAPLE1 at execute (k_NonBinaryInput), so STAPLE1 has no faithful analogue here (see the "Rejects
//     non-binary input at execute" test above). WhiteDots.png IS strictly binary (verified: only values
//     {0, 255}), so it satisfies the binary-input contract with fg=255/bg=0. Opening =
//     erode(boundaryToForeground=true) then dilate(boundaryToForeground=false); Ball radius {1,1,1}. The test
//     compares our output md5 with the committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: ITK real-image golden (WhiteDots)", "[NXImageProcessing][ItkGolden][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<OpeningFilterType>("WhiteDots.png", "3dcb8bce1c174f358608927212aa4cf7", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                           OpeningKernelParamSetter(255.0, 0.0));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (k_NonScalarInput == -8480) fires before the shared PreflightImageFilter, so a 3-component input is rejected.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Preflight rejects non-scalar (multi-component) input", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<OpeningFilterType, uint8>(-8480, OpeningKernelParamSetter(1.0, 0.0));
}

TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Kernel Radius representability", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RequireKernelRadiusPreflight<BinaryMorphologicalOpeningImageFilter>();
}

TEST_CASE("NXImageProcessing::BinaryMorphologicalOpeningImageFilter: Ordinary Kernel Radius control", "[NXImageProcessing][BinaryMorphologicalOpeningImageFilter]")
{
  morph_test::RequireOrdinaryKernelRadius<BinaryMorphologicalOpeningImageFilter>({0, 0, 0});
}
