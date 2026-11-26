#include "UnaryFilterTestUtils.hpp"

#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Core/Application.hpp"

namespace ip_test
{
namespace
{
const DataPath k_InputGeometryPath({"Image Geometry"});

DataPath RunItkRampFilter(DataStructure& ds, PointwiseFilterRunner& runFilter)
{
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), k_InputGeometryPath, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);
  return runFilter(ds, k_InputGeometryPath.createChildPath("CellData").createChildPath("Input"));
}
} // namespace

void CheckItkRampBaseline(PointwiseFilterRunner& runFilter, std::string_view baselineFile, float64 tolerance)
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath output = RunItkRampFilter(ds, runFilter);
  const DataPath baselineGeometry({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath(baselineFile), baselineGeometry, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = baselineGeometry.createChildPath("CellData").createChildPath("Baseline");
  const auto comparison = ip_golden::CompareImages(ds, baselineGeometry, baseline, k_InputGeometryPath, output, tolerance);
  SIMPLNX_RESULT_REQUIRE_VALID(comparison);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

void CheckItkRampMd5(PointwiseFilterRunner& runFilter, std::string_view expectedHash)
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath output = RunItkRampFilter(ds, runFilter);
  const auto actualHash = ip_golden::ComputeMd5Hash(ds, output);
  REQUIRE(actualHash == expectedHash);
  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace ip_test
