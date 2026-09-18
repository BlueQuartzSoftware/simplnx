#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/BlackTopHatImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKBlackTopHatImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyBlackTopHatUuid = *Uuid::FromString("b7471b64-2282-449b-82b4-3ce359e9dda0");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK Black Top Hat output EXACTLY (closing - in; the closing folds
//     select existing values and the subtract is a single exact op). Annulus is excluded (empty legacy
//     kernel). The (Box, 3D r{2,1,0}) cell is excluded from LIVE parity -- the legacy decomposable-Box anchor
//     path produces garbage for a zero-radius axis on a 3D image -- and is instead gated by a Box r{2,1,0}
//     computed-expected case in test (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BlackTopHatImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][BlackTopHatImageFilter]")
{
  morph_test::RunMorphologyParityGrid<BlackTopHatImageFilter, float32>(
      k_LegacyBlackTopHatUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::CompositeKernelParamSetter<BlackTopHatImageFilter>(/*safeBorder=*/true),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the skip-OOB SE-gather folds
//     (black top-hat == erode(dilate(in)) - in). Driven with Safe Border == FALSE so the skip-OOB oracle
//     applies; the Safe Border == true (padded) convention is covered by the parity grid in (1). This validates:
//     - a standard non-degenerate kernel under Safe Border == false (Ball + Box), and
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BlackTopHatImageFilter: Computed-expected (safeBorder=false; Annulus + Box zero-radius)", "[ImageProcessing][BlackTopHatImageFilter]")
{
  using morph_test::CompositeKind;
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::CompositeKernelParamSetter<BlackTopHatImageFilter>(/*safeBorder=*/false);
  constexpr auto kBlackTopHat = CompositeKind::BlackTopHat;

  SECTION("safeBorder=false Ball 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {1, 1, 1}, kBlackTopHat, setParams);
  }
  SECTION("safeBorder=false Box 3D r{2,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 1}, kBlackTopHat, setParams);
  }
  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kBlackTopHat, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kBlackTopHat, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kBlackTopHat, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kBlackTopHat, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyCompositeComputedExpected<BlackTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kBlackTopHat, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected on a ramp (value == flat index) with a Box structuring element. The
//     composite routes through ApplyMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter
//     under BOTH the in-core moving-histogram Direct path and the out-of-core Scanline path on in-memory stores
//     (selected by SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is NOT run
//     here; instead the closed form of a Box black top-hat of a strictly-increasing ramp is asserted directly:
//     black top-hat == closing - in, and the Box closing of a monotone ramp is flat( min(max(x-rx,0)+rx,D-1),
//     ... ) (identity in the interior, a clamped shift near the boundary), so the result is (that closing index)
//     - (flat index). (D^3 - 1 < 2^24, so float32 represents every index -- and every difference -- exactly.)
//     Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BlackTopHatImageFilter: Ramp computed-expected (both algorithm paths)", "[ImageProcessing][BlackTopHatImageFilter]")
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

  BlackTopHatImageFilter filter;
  Arguments args;
  args.insertOrAssign(BlackTopHatImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(BlackTopHatImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(BlackTopHatImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(BlackTopHatImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(BlackTopHatImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));
  // Safe Border == false so the skip-OOB closed form below applies; this exercises the composite's OOC
  // streaming of the plain two-pass path (plus the subtract) on both algorithm iterations.
  args.insertOrAssign(BlackTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));

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
    const int64 cx = std::min<int64>(std::max<int64>(x - rx, 0) + rx, Di - 1);
    const int64 cy = std::min<int64>(std::max<int64>(y - ry, 0) + ry, Di - 1);
    const int64 cz = std::min<int64>(std::max<int64>(z - rz, 0) + rz, Di - 1);
    const int64 closingIdx = (cz * Di + cy) * Di + cx;
    return static_cast<float32>(closingIdx - static_cast<int64>(s));
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
// ITK-sourced real-image md5 golden: duplicates the Ball r{1,1,1} case of ITKBlackTopHatImageTest on STAPLE1.png
// (uint8, 200x150x1), Safe Border default (true). (A) md5 == committed ITK hash + (B) live-ITK bit-exact. Black
// top-hat == closing - in; this is a regression for the composite Safe Border 2D size-1-axis pad defect (fixed by
// SafeBorderPadRadius in ImageProcessingFilterUtilities.hpp): before the fix the underlying closing's Safe Border
// path diverged on the 2D Ball SE, so this filter produced 429243242c23867fefa7eab7438747f8 instead of the committed
// 445a5da6221f6d976d169b70c5538614; clamping the pad to 0 on the size-1 axis reproduces ITK exactly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BlackTopHatImageFilter: ITK real-image golden (Ball r{1,1,1})", "[ImageProcessing][ItkGolden][BlackTopHatImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<BlackTopHatImageFilter>("STAPLE1.png", "445a5da6221f6d976d169b70c5538614", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                morph_test::CompositeKernelParamSetter<BlackTopHatImageFilter>(/*safeBorder=*/true));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8420) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::BlackTopHatImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][BlackTopHatImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<BlackTopHatImageFilter, float32>(-8420, morph_test::CompositeKernelParamSetter<BlackTopHatImageFilter>(/*safeBorder=*/true));
}
