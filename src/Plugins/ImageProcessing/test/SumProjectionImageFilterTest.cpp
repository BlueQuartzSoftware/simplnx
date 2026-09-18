#include "ProjectionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SumProjectionImageFilter.hpp"

#include <nonstd/span.hpp>

#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
// Closed-form column sum of the ramp value(x,y,z) = z*D^2 + y*D + x along the projected axis, decomposed
// by the collapsed-slot -> (non-projected coords) map (see AxisProjectionEngine.hpp). The column is an
// arithmetic sequence of D terms, so its sum is D*(constant part) + sum_{i=0}^{D-1} i * (axis stride factor).
// The legacy ITK Sum Projection filter is NOT registered at runtime (commented out of the ITKImageProcessing
// FilterList), so this independent closed form is the correctness oracle instead of legacy parity.
float64 SumExpectedForSlot(uint32 projDim, usize slot, usize dim)
{
  const auto Dd = static_cast<float64>(dim);
  const float64 sumIdx = Dd * (Dd - 1.0) / 2.0; // sum of {0,1,...,D-1}
  switch(projDim)
  {
  case 0: { // collapse X: slot = z*D + y ; sum over x of (z*D^2 + y*D + x)
    const auto z = static_cast<float64>(slot / dim);
    const auto y = static_cast<float64>(slot % dim);
    return Dd * (z * Dd * Dd + y * Dd) + sumIdx;
  }
  case 1: { // collapse Y: slot = z*D + x ; sum over y of (z*D^2 + y*D + x)
    const auto z = static_cast<float64>(slot / dim);
    const auto x = static_cast<float64>(slot % dim);
    return Dd * (z * Dd * Dd + x) + Dd * sumIdx;
  }
  default: { // collapse Z: slot = y*D + x ; sum over z of (z*D^2 + y*D + x)
    const auto y = static_cast<float64>(slot / dim);
    const auto x = static_cast<float64>(slot % dim);
    return Dd * (y * Dd + x) + Dd * Dd * sumIdx;
  }
  }
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Computed-expected correctness grid (in-core). The legacy ITK Sum Projection filter is not registered
//     at runtime, so correctness is gated on the independent closed form (the column sum of a ramp), over
//     ProjectionDimension in {0,1,2} x Perform-In-Place in {true,false}. A ramp input (value == flat index)
//     makes the per-axis sum differ by axis, so a wrong axis mapping or a wrong in-place/new-geometry output
//     shape is caught, and BOTH output modes of the shared façade are exercised. The output element type is
//     Float64. This is the correctness gate for the axis-projection façade (AllNumeric + AlwaysFloat64) +
//     engine + SumReduce.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SumProjectionImageFilter: Computed-expected grid (3D)", "[ImageProcessing][SumProjectionImageFilter]")
{
  constexpr usize D = 12;
  projection_test::RunProjectionComputedExpectedGrid<SumProjectionImageFilter>([](DataStructure& ds) { return ip_test::BuildRampImage<float32>(ds, D, 0.0, 1.0); },
                                                                               [](uint32 projDim, usize slot) { return SumExpectedForSlot(projDim, slot, D); });
}

// -----------------------------------------------------------------------------
// (1b) Type-dispatch breadth: an INTEGER (int16) input case for the projection façade's type dispatch (the grid
//      above exercises only float32). value == flat index holds for any element type, so the same closed-form
//      column-sum oracle applies; the sum accumulates in Float64. Small dim keeps the ramp within int16 range.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SumProjectionImageFilter: Computed-expected grid (int16 input)", "[ImageProcessing][SumProjectionImageFilter]")
{
  constexpr usize D = 8;
  projection_test::RunProjectionComputedExpectedGrid<SumProjectionImageFilter>([](DataStructure& ds) { return ip_test::BuildRampImage<int16>(ds, D, 0.0, 1.0); },
                                                                               [](uint32 projDim, usize slot) { return SumExpectedForSlot(projDim, slot, D); });
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden test (plan Task 5) -- duplicates ITKSumProjectionImageTest.cpp(z_projection):
// RA-Float.nrrd, projection dimension 2 (Z), compared against the stored Float64 baseline (double, 64x64x1) at
// tol 1e-4 (A). This is an (A)-only case: the legacy NX ITKSumProjectionImageFilter is commented out of the
// ITKImageProcessing FilterList (createFilter -> null / -9002), so no live-ITK (B) parity is available -- a real
// coexistence finding, recorded (not an error).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SumProjectionImageFilter: ITK real-image golden (z_projection)", "[ImageProcessing][ItkGolden][SumProjectionImageFilter]")
{
  projection_test::RunProjectionBaselineItkGolden<SumProjectionImageFilter>("RA-Float.nrrd", "BasicFilters_SumProjectionImageFilter_z_projection.nrrd", /*projDim=*/2, /*tolerance=*/0.0001,
                                                                            projection_test::LiveItkParity::OptionalRecordIfMissing);
}
