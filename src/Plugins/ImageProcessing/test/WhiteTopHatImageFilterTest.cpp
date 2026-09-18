#include "MorphologyFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/WhiteTopHatImageFilter.hpp"

#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKWhiteTopHatImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyWhiteTopHatUuid = *Uuid::FromString("2f377682-d0a8-4dea-8c68-60c2c523a074");
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: Ball/Box/Cross x {3D r{1,1,1}, 3D r{2,1,1}, 3D r{2,1,0}, 2D Z=1 r{2,2,0}}. The new
//     filter must reproduce the legacy ITK White Top Hat output EXACTLY (in - opening; the opening folds
//     select existing values and the subtract is a single exact op). Annulus is excluded (empty legacy
//     kernel). The (Box, 3D r{2,1,0}) cell is excluded from LIVE parity -- the legacy decomposable-Box anchor
//     path produces garbage for a zero-radius axis on a 3D image -- and is instead gated by a Box r{2,1,0}
//     computed-expected case in test (2); Ball/Cross at r{2,1,0} DO match live legacy.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::WhiteTopHatImageFilter: Legacy parity grid (Ball/Box/Cross)", "[ImageProcessing][WhiteTopHatImageFilter]")
{
  morph_test::RunMorphologyParityGrid<WhiteTopHatImageFilter, float32>(
      k_LegacyWhiteTopHatUuid, [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); },
      morph_test::CompositeKernelParamSetter<WhiteTopHatImageFilter>(/*safeBorder=*/true),
      [](std::string_view kernelName, const morph_test::MorphCase& cfg) { return kernelName == "Box" && std::string_view(cfg.label) == "3D r{2,1,0}"; });
}

// -----------------------------------------------------------------------------
// (2) Computed-expected (NOT legacy parity), independent oracle composed from the skip-OOB SE-gather folds
//     (white top-hat == in - dilate(erode(in))). Driven with Safe Border == FALSE so the skip-OOB oracle
//     applies; the Safe Border == true (padded) convention is covered by the parity grid in (1). This validates:
//     - a standard non-degenerate kernel under Safe Border == false (Ball + Box), and
//     - Annulus: this filter ships ITK's real thickness-1 shell, whereas the legacy SimpleITK Annulus was empty.
//     - Box r{2,1,0}: the excluded parity cell from (1); the legacy ITK anchor path is broken for a zero-radius
//       axis on a 3D image, so the correct result is test-documented here rather than left silently absent.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::WhiteTopHatImageFilter: Computed-expected (safeBorder=false; Annulus + Box zero-radius)", "[ImageProcessing][WhiteTopHatImageFilter]")
{
  using morph_test::CompositeKind;
  using morph_test::KernelType;
  auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return morph_test::BuildMorphologyImage<float32>(ds, dx, dy, dz); };
  const auto setParams = morph_test::CompositeKernelParamSetter<WhiteTopHatImageFilter>(/*safeBorder=*/false);
  constexpr auto kWhiteTopHat = CompositeKind::WhiteTopHat;

  SECTION("safeBorder=false Ball 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Ball, {1, 1, 1}, kWhiteTopHat, setParams);
  }
  SECTION("safeBorder=false Box 3D r{2,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 1}, kWhiteTopHat, setParams);
  }
  SECTION("Annulus 3D r{1,1,1}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {1, 1, 1}, kWhiteTopHat, setParams);
  }
  SECTION("Annulus 3D r{2,2,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {2, 2, 2}, kWhiteTopHat, setParams);
  }
  SECTION("Annulus 3D asymmetric r{3,1,2}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Annulus, {3, 1, 2}, kWhiteTopHat, setParams);
  }
  SECTION("Annulus 2D Z=1 r{2,2,0}")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 20, 16, 1, KernelType::Annulus, {2, 2, 0}, kWhiteTopHat, setParams);
  }
  SECTION("Box 3D zero-radius r{2,1,0} (legacy anchor path broken)")
  {
    morph_test::RunMorphologyCompositeComputedExpected<WhiteTopHatImageFilter, float32>(build, 12, 12, 12, KernelType::Box, {2, 1, 0}, kWhiteTopHat, setParams);
  }
}

// -----------------------------------------------------------------------------
// (3) Both-algorithm-paths computed-expected on a ramp (value == flat index) with a Box structuring element. The
//     composite routes through ApplyMorphology's DispatchAlgorithm, so the AlgorithmTestScope runs the filter
//     under BOTH the in-core moving-histogram Direct path and the out-of-core Scanline path on in-memory stores
//     (selected by SIMPLNX_TEST_ALGORITHM_PATH). The legacy ITK filter rejects out-of-core arrays and is NOT run
//     here; instead the closed form of a Box white top-hat of a strictly-increasing ramp is asserted directly:
//     white top-hat == in - opening, and the Box opening of a monotone ramp is flat( max(min(x+rx,D-1)-rx,0),
//     ... ) (identity in the interior, a clamped shift near the boundary), so the result is (flat index) - (that
//     opening index). (D^3 - 1 < 2^24, so float32 represents every index -- and every difference -- exactly.)
//     Real-disk out-of-core coverage is retired to the generic SimplnxOoc store tests.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::WhiteTopHatImageFilter: Ramp computed-expected (both algorithm paths)", "[ImageProcessing][WhiteTopHatImageFilter]")
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

  WhiteTopHatImageFilter filter;
  Arguments args;
  args.insertOrAssign(WhiteTopHatImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(WhiteTopHatImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(WhiteTopHatImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(WhiteTopHatImageFilter::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<ChoicesParameter::ValueType>(morph_test::KernelType::Box)));
  args.insertOrAssign(WhiteTopHatImageFilter::k_KernelRadius_Key,
                      std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{static_cast<uint32>(rx), static_cast<uint32>(ry), static_cast<uint32>(rz)}));
  // Safe Border == false so the skip-OOB closed form below applies; this exercises the composite's OOC
  // streaming of the plain two-pass path (plus the subtract) on both algorithm iterations.
  args.insertOrAssign(WhiteTopHatImageFilter::k_SafeBorder_Key, std::make_any<bool>(false));

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
    const int64 ox = std::max<int64>(std::min<int64>(x + rx, Di - 1) - rx, 0);
    const int64 oy = std::max<int64>(std::min<int64>(y + ry, Di - 1) - ry, 0);
    const int64 oz = std::max<int64>(std::min<int64>(z + rz, Di - 1) - rz, 0);
    const int64 openingIdx = (oz * Di + oy) * Di + ox;
    return static_cast<float32>(static_cast<int64>(s) - openingIdx);
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
// ITK-sourced real-image md5 golden: duplicates the Ball r{1,1,1} case of ITKWhiteTopHatImageTest on STAPLE1.png
// (uint8, 200x150x1), Safe Border default (true). (A) md5 == committed ITK hash + (B) live-ITK bit-exact. White
// top-hat == in - opening; this is a regression for the composite Safe Border 2D size-1-axis pad defect (fixed by
// SafeBorderPadRadius in ImageProcessingFilterUtilities.hpp): before the fix the underlying opening's Safe Border
// path diverged on the 2D Ball SE, so this filter produced fddab5c7dd784e4aec54b67560cfd052 instead of the committed
// e784daff43d09a18e20556729afc0c9d; clamping the pad to 0 on the size-1 axis reproduces ITK exactly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::WhiteTopHatImageFilter: ITK real-image golden (Ball r{1,1,1})", "[ImageProcessing][ItkGolden][WhiteTopHatImageFilter]")
{
  morph_test::RunMorphologyMd5ItkGolden<WhiteTopHatImageFilter>("STAPLE1.png", "e784daff43d09a18e20556729afc0c9d", static_cast<uint64>(morph_test::KernelType::Ball), {1, 1, 1},
                                                                morph_test::CompositeKernelParamSetter<WhiteTopHatImageFilter>(/*safeBorder=*/true));
}

// -----------------------------------------------------------------------------
// Preflight rejects a NON-SCALAR (multi-component) input. The filter's own getNumberOfComponents() != 1 guard
// (error -8410) fires before the shared PreflightImageFilter, so a 3-component input is rejected at preflight.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::WhiteTopHatImageFilter: Preflight rejects non-scalar (multi-component) input", "[ImageProcessing][WhiteTopHatImageFilter]")
{
  morph_test::RequirePreflightRejectsNonScalar<WhiteTopHatImageFilter, float32>(-8410, morph_test::CompositeKernelParamSetter<WhiteTopHatImageFilter>(/*safeBorder=*/true));
}
