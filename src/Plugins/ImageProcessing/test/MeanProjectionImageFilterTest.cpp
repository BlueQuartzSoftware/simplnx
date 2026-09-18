#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MeanProjectionImageFilter.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
// Legacy ITKMeanProjectionImageFilter, created at runtime by UUID.
const Uuid k_LegacyMeanProjectionUuid = *Uuid::FromString("62ffddba-cc57-45fc-a93a-27914eea11ad");

// Shared tolerance comparator for the Float64 mean output (only algebraically equal to ITK's two-pass).
void CompareMean(const IDataArray& newOut, const IDataArray& legacyOut)
{
  projection_test::CompareFloat64ArraysWithinTolerance(newOut, legacyOut);
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Legacy parity grid: the new filter must produce (within tolerance) the SAME output the legacy ITK
//     Mean Projection filter produces, over ProjectionDimension in {0,1,2} x Perform-In-Place in
//     {true,false}. The legacy Mean filter supports BOTH output modes (it declares the
//     RemoveOriginalGeometry / Created-Image-Geometry parameters), so the full shared parity grid applies.
//     A ramp input (value == flat index) makes the per-axis mean differ by axis, so a wrong axis mapping or
//     a wrong in-place/new-geometry output shape is caught. The output element type is Float64 for both
//     filters. This is the correctness gate for the axis-projection façade (AllNumeric + AlwaysFloat64)
//     + engine + MeanReduce, cross-checked directly against ITK.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MeanProjectionImageFilter: Legacy parity grid (3D)", "[ImageProcessing][MeanProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MeanProjectionImageFilter, float64>(
      k_LegacyMeanProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, 12, 0.0, 1.0); }, [](Arguments&) {}, CompareMean);
}

// -----------------------------------------------------------------------------
// (1b) Type-dispatch breadth: an INTEGER (int16) input case for the projection façade's type dispatch (the grid
//      above exercises only float32). The output is Float64 for both filters (the legacy Mean's FilterOutputType
//      is double, our Mean is AlwaysFloat64), so the same tolerant Float64 comparator applies. Small dim keeps
//      the ramp within int16 range.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MeanProjectionImageFilter: Legacy parity grid (int16 input)", "[ImageProcessing][MeanProjectionImageFilter]")
{
  projection_test::RunProjectionParityGrid<MeanProjectionImageFilter, float64>(
      k_LegacyMeanProjectionUuid, [](DataStructure& ds) { return ip_test::BuildRampImage<int16>(ds, 8, 0.0, 1.0); }, [](Arguments&) {}, CompareMean);
}

// -----------------------------------------------------------------------------
// (2) Independent closed-form oracle (small, in-memory). The Mean parity grid above only compares against the
//     legacy ITK filter (within tolerance, since our one-pass mean is only ALGEBRAICALLY equal to ITK's
//     two-pass), so this case keeps the UNIQUE independent oracle: for a ramp value(x,y,z) = z*D^2 + y*D + x,
//     the mean along the projected axis (an arithmetic sequence of D terms) is the term at the mid-value of
//     that axis (see expectedForSlot below), asserted directly across both output modes. Real-disk
//     out-of-core coverage is retired to the generic SimplnxOoc store tests (the adopted model), and the Mean
//     engine is single-implementation (no DispatchAlgorithm), so it cannot use AlgorithmTestScope; this is a
//     plain ForceInCore in-memory correctness check.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MeanProjectionImageFilter: Independent oracle (in-memory)", "[ImageProcessing][MeanProjectionImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const uint32 projDim = GENERATE(uint32{0}, uint32{1}, uint32{2});
  const bool performInPlace = GENERATE(false, true);
  CAPTURE(projDim, performInPlace);

  constexpr usize D = 32;
  DataStructure ds;
  const DataPath inputPath = ip_test::BuildRampImage<float32>(ds, D, 0.0, 1.0);

  MeanProjectionImageFilter filter;
  Arguments args;
  args.insertOrAssign(MeanProjectionImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(MeanProjectionImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(MeanProjectionImageFilter::k_ProjectionDimension_Key, std::make_any<uint32>(projDim));
  args.insertOrAssign(MeanProjectionImageFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(performInPlace));
  args.insertOrAssign(MeanProjectionImageFilter::k_OutputImageGeomName_Key, std::make_any<std::string>("Projected Image"));
  args.insertOrAssign(MeanProjectionImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputArrayPath = performInPlace ? DataPath({"Image Geometry", "CellData", "Output"}) : DataPath({"Projected Image", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<float64>>(outputArrayPath);
  const auto& outStore = outArray.getDataStoreRef();
  const usize numSlots = D * D;
  REQUIRE(outStore.getSize() == numSlots);

  // Mean of the arithmetic-sequence column, decomposed by the collapsed-slot -> (non-projected coords) map.
  constexpr float64 Dd = static_cast<float64>(D);
  const float64 midOffset = (Dd - 1.0) / 2.0; // mean of {0,1,...,D-1}
  auto expectedForSlot = [&](usize s) -> float64 {
    switch(projDim)
    {
    case 0: { // collapse X: slot = z*D + y ; mean over x of (z*D^2 + y*D + x)
      const auto z = static_cast<float64>(s / D);
      const auto y = static_cast<float64>(s % D);
      return z * Dd * Dd + y * Dd + midOffset;
    }
    case 1: { // collapse Y: slot = z*D + x ; mean over y of (z*D^2 + y*D + x)
      const auto z = static_cast<float64>(s / D);
      const auto x = static_cast<float64>(s % D);
      return z * Dd * Dd + x + Dd * midOffset;
    }
    default: { // collapse Z: slot = y*D + x ; mean over z of (z*D^2 + y*D + x)
      const auto y = static_cast<float64>(s / D);
      const auto x = static_cast<float64>(s % D);
      return y * Dd + x + Dd * Dd * midOffset;
    }
    }
  };

  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<float64[]>(k_ChunkValues);
  bool allMatch = true;
  usize badSlot = 0;
  float64 badGot = 0.0;
  float64 badExp = 0.0;
  for(usize start = 0; start < numSlots && allMatch; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, numSlots - start);
    Result<> copyResult = outStore.copyIntoBuffer(start, nonstd::span<float64>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
    for(usize i = 0; i < count; ++i)
    {
      const float64 expected = expectedForSlot(start + i);
      const float64 tol = 1.0e-6 + 1.0e-9 * std::abs(expected);
      if(std::abs(buffer[i] - expected) > tol)
      {
        allMatch = false;
        badSlot = start + i;
        badGot = buffer[i];
        badExp = expected;
        break;
      }
    }
  }
  if(!allMatch)
  {
    UNSCOPED_INFO(fmt::format("slot={} got={} expected={}", badSlot, badGot, badExp));
  }
  REQUIRE(allMatch);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden test (plan Task 5) -- duplicates ITKMeanProjectionImageTest.cpp(z_projection):
// RA-Float.nrrd, projection dimension 2 (Z), compared against the stored Float64 baseline (double, 64x64x1) at
// tol 1e-4 (A), plus live-ITK Float64 tolerant parity (B). Mean is a Float64 reducer (AlwaysFloat64), only
// ALGEBRAICALLY equal to ITK's two-pass, hence the baseline/tolerant comparators rather than md5/bit-exact.
// The projection collapses Z, so the OUTPUT (in-place, in the collapsed "Image Geometry") owns the reduced
// 64x64x1 array that matches the baseline's dims -- the geometry resolution for risk #2.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MeanProjectionImageFilter: ITK real-image golden (z_projection)", "[ImageProcessing][ItkGolden][MeanProjectionImageFilter]")
{
  projection_test::RunProjectionBaselineItkGolden<MeanProjectionImageFilter>("RA-Float.nrrd", "BasicFilters_MeanProjectionImageFilter_z_projection.nrrd", /*projDim=*/2, /*tolerance=*/0.0001,
                                                                             projection_test::LiveItkParity::Required);
}
