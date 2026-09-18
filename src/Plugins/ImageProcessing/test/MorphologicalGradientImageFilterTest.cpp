#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MorphologicalGradientImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKMorphologicalGradientImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyMorphologicalGradientUuid = *Uuid::FromString("9103009a-8884-4097-8c34-aec7019589ea");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK Morphological Gradient output EXACTLY (dilate - erode; both folds
//     select existing values and the subtract is a single exact op). Annulus is excluded (empty legacy
//     kernel). The (Box, 3D r{2,1,0}) cell is excluded from LIVE parity -- the legacy decomposable-Box anchor
//     path produces garbage for a zero-radius axis on a 3D image -- and is instead gated by a Box r{2,1,0}
//     computed-expected case in test (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][MorphologicalGradientImageFilter]")
{
  morph_test::RunMorphologyParityGrid<MorphologicalGradientImageFilter, float32>(
      k_LegacyMorphologicalGradientUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::DefaultKernelParamSetter<MorphologicalGradientImageFilter>(),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the skip-OOB SE-gather folds
//     (gradient == dilate(in) - erode(in)):
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: Computed-expected (Annulus + Box zero-radius)", "[ImageProcessing][MorphologicalGradientImageFilter]")
{
  using morph_test::CompositeKind;
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::DefaultKernelParamSetter<MorphologicalGradientImageFilter>();
  constexpr auto kGradient = CompositeKind::Gradient;

  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<MorphologicalGradientImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kGradient, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<MorphologicalGradientImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kGradient, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<MorphologicalGradientImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kGradient, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<MorphologicalGradientImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kGradient, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyCompositeComputedExpected<MorphologicalGradientImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kGradient, setParams);
  }
}

// -----------------------------------------------------------------------------
// (2b) Degenerate empty structuring element (Annulus r{0,0,0}) must produce an ALL-ZERO gradient. An Annulus of
//      radius {0,0,0} rasterizes to an EMPTY SE, and the morphological gradient (dilate - erode) over an empty
//      neighborhood is (in - in) == 0 at every voxel -- unlike GrayscaleDilate's empty-SE case, which passes the
//      INPUT through. The named scenario matrix requires both paths to fill the output with zero; the engine-level
//      bounded-transfer test separately constrains the true-2D Scanline writes.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: Empty-SE all-zero output (Annulus r{0,0,0})", "[ImageProcessing][MorphologicalGradientImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Sanity: confirm Annulus {0,0,0} really is the empty-SE case this test targets.
  REQUIRE(morph_test::MakeStructuringElement(morph_test::KernelType::Annulus, {0, 0, 0}).offsets.empty());

  constexpr usize D = 12;
  DataStructure ds;
  const DataPath inputPath = morph_test::BuildMorphologyImage<float32>(ds, D, D, D);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  MorphologicalGradientImageFilter filter;
  Arguments args;
  args.insertOrAssign(MorphologicalGradientImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Annulus)));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_KernelRadius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{0, 0, 0}));

  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<float32>>(outputPath).getDataStoreRef();
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(outputPath));
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("empty-SE gradient index=" << i);
    REQUIRE(outStore.getValue(i) == 0.0f);
  }
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected on a ramp (value == flat index) with a Box structuring element. The
//     morphology façade routes through ApplyMorphologicalGradient's DispatchAlgorithm, so the AlgorithmTestScope
//     runs the filter under BOTH the in-core moving-histogram Direct path and the out-of-core Scanline path on
//     in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core
//     arrays and is NOT run here; instead the closed form of a Box morphological gradient of a strictly-
//     increasing ramp is asserted directly: gradient == dilate - erode, and for a monotone ramp the box max is at
//     the +radius corner and the box min at the -radius corner (both clamped to the image), so the result is
//     (flat index of +corner) - (flat index of -corner). (D^3 - 1 < 2^24, so float32 represents every index --
//     and every difference -- exactly.) Real-disk out-of-core coverage is retired to the generic SimplnxOoc store
//     tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: Ramp computed-expected (both algorithm paths)", "[ImageProcessing][MorphologicalGradientImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize D = 32;
  constexpr int64 rx = 2;
  constexpr int64 ry = 1;
  constexpr int64 rz = 1;

  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, D, 0.0, 1.0);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  MorphologicalGradientImageFilter filter;
  Arguments args;
  args.insertOrAssign(MorphologicalGradientImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(MorphologicalGradientImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D * D;
  REQUIRE(outStore.getSize() == numSlots);

  auto expectedForSlot = [&](usize s) -> float32 {
    const int64 Di = static_cast<int64>(D);
    const int64 x = static_cast<int64>(s % D);
    const int64 y = static_cast<int64>((s / D) % D);
    const int64 z = static_cast<int64>(s / (D * D));
    const int64 dx = std::min<int64>(x + rx, Di - 1);
    const int64 dy = std::min<int64>(y + ry, Di - 1);
    const int64 dz = std::min<int64>(z + rz, Di - 1);
    const int64 ex = std::max<int64>(x - rx, 0);
    const int64 ey = std::max<int64>(y - ry, 0);
    const int64 ez = std::max<int64>(z - rz, 0);
    const int64 dilateIdx = (dz * Di + dy) * Di + dx;
    const int64 erodeIdx = (ez * Di + ey) * Di + ex;
    return static_cast<float32>(dilateIdx - erodeIdx);
  };

  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<float32[]>(k_ChunkValues);
  bool allMatch = true;
  usize badSlot = 0;
  float32 badGot = 0.0f;
  float32 badExp = 0.0f;
  for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<float32>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    for(usize i = 0; i < count; ++i)
    {
      if(buffer[i] != expectedForSlot(start + i))
      {
        allMatch = false;
        badSlot = start + i;
        badGot = buffer[i];
        badExp = expectedForSlot(start + i);
        break;
      }
    }
  }
  if(!allMatch)
  {
    UNSCOPED_INFO("slot=" << badSlot << " got=" << badGot << " expected=" << badExp);
  }
  REQUIRE(allMatch);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKMorphologicalGradientImageTest.cpp(MorphologicalGradient):
// STAPLE1.png (uint8), Ball radius {1,1,1}. (A) md5 == committed hash + (B) live-ITK bit-exact. MorphologicalGradient
// (dilate - erode) has no Safe Border parameter.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: ITK real-image golden (MorphologicalGradient)", "[ImageProcessing][ItkGolden][MorphologicalGradientImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<MorphologicalGradientImageFilter>("STAPLE1.png", "57167a1d86b60fbf9e040d9441676876", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                          morph_test::DefaultKernelParamSetter<MorphologicalGradientImageFilter>());
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8380) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MorphologicalGradientImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][MorphologicalGradientImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<MorphologicalGradientImageFilter, float32>(-8380, morph_test::DefaultKernelParamSetter<MorphologicalGradientImageFilter>());
}
