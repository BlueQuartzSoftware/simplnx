#include "OrientationAnalysis/Filters/ComputeShapesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/Orientation/Euler.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
// ============================================================================
// Oracle fixtures for ComputeShapes.
//
// Every expected value below is derived from a closed form obtained BY HAND
// from Algorithms/ComputeShapes.cpp and cross-checked against an independent
// brute-force numpy re-implementation of the same loops.  The closed form was
// first validated by reproducing all fifteen numbers asserted by SIMPL
// 6.5.171's own FindShapesTest.cpp (using that code's voxel-corner sample
// convention).  See the V&V report for the derivation.
//
// For an axis-aligned box of N_x x N_y x N_z voxels with spacing d:
//
//   G_i  = (N_i^2 - 1)/12 + 1/16     ("1/16" is the octant quadrature's
//                                     self-moment, NOT the exact 1/12 integral)
//   P_i  = V_mod * D_i^2 * G_i       (D_i = d_i * m_ScaleFactor,
//                                     V_mod = N_tot * D_x*D_y*D_z)
//   Ixx  = P_y + P_z,  Iyy = P_x + P_z,  Izz = P_x + P_y   (off-diagonals 0)
//   Q_i  = 15 P_i / (4 pi);  m = (Q_x Q_y Q_z)^(1/5)
//   a    = sqrt(Q_max/m), b = sqrt(Q_mid/m), c = sqrt(Q_min/m)   (/ scale)
//   Om3  = 9 N_tot^2 / (2000 pi^2 G_x G_y G_z)     (clamped at 1)
//
// 2D (one geometry dimension == 1), in-plane counts/spacings (n0, n1)/(e0, e1):
//   Ixx  = A_mod * E_1^2 * G(n1),  Iyy = A_mod * E_0^2 * G(n0),  Ixy = 0
//   a    = (4/pi)^(1/4) (r1^3/r2)^(1/8) / scale,  r1 = max(Ixx,Iyy)
//   b    = (4/pi)^(1/4) (r2^3/r1)^(1/8) / scale,  c = 0
//   b/a  = sqrt(r2/r1), second aspect ratio 0, Omega3 never written (0)
//   axis euler = (pi/2, 0, 0) when Ixx > Iyy, else (0, 0, 0)
// ============================================================================

const std::string k_GeomName = "Image Geometry";
const std::string k_CellAMName = "Cell Data";
const std::string k_FeatureAMName = "Cell Feature Data";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CentroidsName = "Centroids";
const std::string k_Omega3sName = "Omega3s";
const std::string k_AxisLengthsName = "AxisLengths";
const std::string k_AxisEulerAnglesName = "AxisEulerAngles";
const std::string k_AspectRatiosName = "AspectRatios";
const std::string k_ShapeVolumesName = "Shape Volumes";

// Tolerances, with the cause of each named.
//  - Volumes are an integer count times an exactly representable product of
//    dyadic spacings, so they are exact.
//  - Axis lengths / aspect ratios pass through Eigen::EigenSolver<Matrix3f>,
//    i.e. a float32 eigen solve, hence ~1e-6 relative; 1e-5 gives headroom.
//  - Omega3 is built from u200/u020/u002, which are float32 casts of the
//    double moments, then raised to the 5th power; 1e-5 relative covers it.
//  - The 2D axis Euler angle is a literal constant stored into float32.
//    CheckFeature2D passes k_AngleAbsTol as the absolute floor but also
//    passes k_AxisRelTol (1e-5) as the relative tolerance, so the effective
//    budget on a pi/2 angle is max(1e-6, 1e-5 * pi/2) =~ 1.57e-5, not 1e-6.
constexpr float64 k_VolumeRelTol = 1.0e-6;
constexpr float64 k_AxisRelTol = 1.0e-5;
constexpr float64 k_Omega3RelTol = 1.0e-5;
constexpr float64 k_AngleAbsTol = 1.0e-6;
constexpr float64 k_ZeroAbsTol = 1.0e-12;

struct ShapesFixture
{
  DataStructure dataStructure;
  DataPath geomPath;
  DataPath cellAmPath;
  DataPath featureAmPath;
  DataPath featureIdsPath;
  DataPath centroidsPath;
  DataPath omega3sPath;
  DataPath axisLengthsPath;
  DataPath axisEulerAnglesPath;
  DataPath aspectRatiosPath;
  DataPath volumesPath;
};

/**
 * @brief Builds an ImageGeom + FeatureIds + Centroids fixture.
 * @param dims Geometry dimensions (x, y, z) in cells
 * @param spacing Cell spacing
 * @param origin Geometry origin
 * @param featureIds One id per cell, in (z, y, x) linear order
 * @param numFeatures Number of tuples in the feature Attribute Matrix (id 0 included)
 *
 * The Centroids are the exact arithmetic mean of the voxel CENTER coordinates
 * of each id, which is what ComputeFeatureCentroids produces. Computing them
 * here rather than by running that filter keeps this fixture an oracle for
 * ComputeShapes alone.
 */
ShapesFixture BuildFixture(const std::array<usize, 3>& dims, const std::array<float32, 3>& spacing, const std::array<float32, 3>& origin, const std::vector<int32>& featureIds, usize numFeatures)
{
  ShapesFixture fixture;

  auto* imageGeom = ImageGeom::Create(fixture.dataStructure, k_GeomName);
  imageGeom->setDimensions({dims[0], dims[1], dims[2]});
  imageGeom->setSpacing({spacing[0], spacing[1], spacing[2]});
  imageGeom->setOrigin({origin[0], origin[1], origin[2]});

  // AttributeMatrix tuple shape is (z, y, x)
  const ShapeType cellTupleShape = {dims[2], dims[1], dims[0]};
  auto* cellAm = AttributeMatrix::Create(fixture.dataStructure, k_CellAMName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAm);
  auto* featureAm = AttributeMatrix::Create(fixture.dataStructure, k_FeatureAMName, ShapeType{numFeatures}, imageGeom->getId());

  auto* featureIdsArray = CreateTestDataArray<int32>(fixture.dataStructure, k_FeatureIdsName, cellTupleShape, {1}, cellAm->getId());
  auto* centroidsArray = CreateTestDataArray<float32>(fixture.dataStructure, k_CentroidsName, ShapeType{numFeatures}, {3}, featureAm->getId());

  const usize numCells = dims[0] * dims[1] * dims[2];
  REQUIRE(featureIds.size() == numCells);

  std::vector<float64> sums(numFeatures * 3, 0.0);
  std::vector<usize> counts(numFeatures, 0);

  for(usize cellIdx = 0; cellIdx < numCells; cellIdx++)
  {
    const int32 featureId = featureIds[cellIdx];
    REQUIRE(featureId >= 0);
    REQUIRE(static_cast<usize>(featureId) < numFeatures);
    (*featureIdsArray)[cellIdx] = featureId;

    const usize xIdx = cellIdx % dims[0];
    const usize yIdx = (cellIdx / dims[0]) % dims[1];
    const usize zIdx = cellIdx / (dims[0] * dims[1]);
    const Point3D<float64> voxelCenter = imageGeom->getCoords(xIdx, yIdx, zIdx);

    const auto slot = static_cast<usize>(featureId) * 3;
    sums[slot + 0] += voxelCenter[0];
    sums[slot + 1] += voxelCenter[1];
    sums[slot + 2] += voxelCenter[2];
    counts[static_cast<usize>(featureId)]++;
  }

  for(usize featureId = 0; featureId < numFeatures; featureId++)
  {
    if(counts[featureId] == 0)
    {
      continue;
    }
    for(usize comp = 0; comp < 3; comp++)
    {
      (*centroidsArray)[featureId * 3 + comp] = static_cast<float32>(sums[featureId * 3 + comp] / static_cast<float64>(counts[featureId]));
    }
  }

  fixture.geomPath = DataPath({k_GeomName});
  fixture.cellAmPath = fixture.geomPath.createChildPath(k_CellAMName);
  fixture.featureAmPath = fixture.geomPath.createChildPath(k_FeatureAMName);
  fixture.featureIdsPath = fixture.cellAmPath.createChildPath(k_FeatureIdsName);
  fixture.centroidsPath = fixture.featureAmPath.createChildPath(k_CentroidsName);
  fixture.omega3sPath = fixture.featureAmPath.createChildPath(k_Omega3sName);
  fixture.axisLengthsPath = fixture.featureAmPath.createChildPath(k_AxisLengthsName);
  fixture.axisEulerAnglesPath = fixture.featureAmPath.createChildPath(k_AxisEulerAnglesName);
  fixture.aspectRatiosPath = fixture.featureAmPath.createChildPath(k_AspectRatiosName);
  fixture.volumesPath = fixture.featureAmPath.createChildPath(k_ShapeVolumesName);

  return fixture;
}

void RunShapes(ShapesFixture& fixture)
{
  ComputeShapesFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(fixture.geomPath));
  args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(fixture.featureIdsPath));
  args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(fixture.centroidsPath));
  args.insertOrAssign(ComputeShapesFilter::k_Omega3sArrayName_Key, std::make_any<std::string>(k_Omega3sName));
  args.insertOrAssign(ComputeShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<std::string>(k_AxisLengthsName));
  args.insertOrAssign(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<std::string>(k_AxisEulerAnglesName));
  args.insertOrAssign(ComputeShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<std::string>(k_AspectRatiosName));
  args.insertOrAssign(ComputeShapesFilter::k_VolumesArrayName_Key, std::make_any<std::string>(k_ShapeVolumesName));

  auto preflightResult = filter.preflight(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(fixture.dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

/**
 * @brief Compares one component of one tuple against an expected value with a
 * mixed relative/absolute tolerance, capturing everything needed to identify
 * the offending element on failure.
 */
void CheckComponent(const std::string& arrayName, usize featureId, usize component, float32 computed, float64 expected, float64 relTol, float64 absTol)
{
  const float64 tolerance = std::max(absTol, relTol * std::abs(expected));
  const float64 delta = std::abs(static_cast<float64>(computed) - expected);
  CAPTURE(arrayName, featureId, component, computed, expected, tolerance, delta);
  REQUIRE(std::isfinite(computed));
  REQUIRE(delta <= tolerance);
}

void CheckTuple(const DataStructure& dataStructure, const DataPath& path, usize featureId, const std::vector<float64>& expected, float64 relTol, float64 absTol)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(path));
  const auto& array = dataStructure.getDataRefAs<Float32Array>(path);
  REQUIRE(array.getNumberOfComponents() == expected.size());
  for(usize component = 0; component < expected.size(); component++)
  {
    CheckComponent(path.getTargetName(), featureId, component, array[featureId * expected.size() + component], expected[component], relTol, absTol);
  }
}

/**
 * @brief Asserts the outputs of one feature of a fully three-dimensional fixture.
 */
void CheckFeature3D(const ShapesFixture& fixture, usize featureId, float64 volume, const std::vector<float64>& axisLengths, const std::vector<float64>& aspectRatios, float64 omega3)
{
  CheckTuple(fixture.dataStructure, fixture.volumesPath, featureId, {volume}, k_VolumeRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.axisLengthsPath, featureId, axisLengths, k_AxisRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.aspectRatiosPath, featureId, aspectRatios, k_AxisRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.omega3sPath, featureId, {omega3}, k_Omega3RelTol, k_ZeroAbsTol);
}

/**
 * @brief Asserts the outputs of one feature of a two-dimensional fixture.
 * Omega3 is always zero because findMoments2D never writes it, and the third
 * axis length is always zero for the same reason.
 */
void CheckFeature2D(const ShapesFixture& fixture, usize featureId, float64 area, float64 axisA, float64 axisB, float64 bOverA, float64 axisEuler0)
{
  CheckTuple(fixture.dataStructure, fixture.volumesPath, featureId, {area}, k_VolumeRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.axisLengthsPath, featureId, {axisA, axisB, 0.0}, k_AxisRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.aspectRatiosPath, featureId, {bOverA, 0.0}, k_AxisRelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.omega3sPath, featureId, {0.0}, k_Omega3RelTol, k_ZeroAbsTol);
  CheckTuple(fixture.dataStructure, fixture.axisEulerAnglesPath, featureId, {axisEuler0, 0.0, 0.0}, k_AxisRelTol, k_AngleAbsTol);
}

/**
 * @brief Checks the invariants that must hold for every feature of every fixture.
 */
void CheckInvariants(const ShapesFixture& fixture, bool is2D)
{
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisLengthsPath));
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.aspectRatiosPath));
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.omega3sPath));
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.volumesPath));
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisEulerAnglesPath));
  const auto& axisLengths = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisLengthsPath);
  const auto& aspectRatios = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.aspectRatiosPath);
  const auto& omega3s = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.omega3sPath);
  const auto& volumes = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.volumesPath);
  const auto& axisEulerAngles = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisEulerAnglesPath);

  const usize numFeatures = omega3s.getNumberOfTuples();
  for(usize featureId = 1; featureId < numFeatures; featureId++)
  {
    CAPTURE(featureId);
    // No NaN or Inf anywhere
    for(usize component = 0; component < 3; component++)
    {
      CAPTURE(component);
      REQUIRE(std::isfinite(axisLengths[featureId * 3 + component]));
      REQUIRE(std::isfinite(axisEulerAngles[featureId * 3 + component]));
    }
    REQUIRE(std::isfinite(aspectRatios[featureId * 2 + 0]));
    REQUIRE(std::isfinite(aspectRatios[featureId * 2 + 1]));
    REQUIRE(std::isfinite(omega3s[featureId]));
    REQUIRE(std::isfinite(volumes[featureId]));

    const float32 a = axisLengths[featureId * 3 + 0];
    const float32 b = axisLengths[featureId * 3 + 1];
    const float32 c = axisLengths[featureId * 3 + 2];
    // a >= b >= c
    REQUIRE(a >= b);
    REQUIRE(b >= c);
    // Semi-axis lengths and Omega3 are never negative
    REQUIRE(c >= 0.0f);
    REQUIRE(omega3s[featureId] >= 0.0f);
    REQUIRE(omega3s[featureId] <= 1.0f);
    REQUIRE(volumes[featureId] >= 0.0f);
    // Aspect ratios are in (0, 1] for non-empty features, exactly 0 for empty ones
    const bool isEmpty = volumes[featureId] == 0.0f;
    REQUIRE(aspectRatios[featureId * 2 + 0] <= 1.0f);
    if(isEmpty)
    {
      REQUIRE(aspectRatios[featureId * 2 + 0] == 0.0f);
      REQUIRE(a == 0.0f);
    }
    else
    {
      REQUIRE(aspectRatios[featureId * 2 + 0] > 0.0f);
      REQUIRE(a > 0.0f);
    }
    // The 2D path never populates the third semi-axis or the second aspect ratio
    if(is2D)
    {
      REQUIRE(c == 0.0f);
      REQUIRE(aspectRatios[featureId * 2 + 1] == 0.0f);
      REQUIRE(omega3s[featureId] == 0.0f);
    }
  }
}

std::vector<int32> FillIds(usize numCells, int32 value)
{
  return std::vector<int32>(numCells, value);
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeShapesFilter", "[OrientationAnalysis][ComputeShapesFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_stats_test_v2.tar.gz", "6_6_stats_test_v2.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test_v2.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_Omega3sArrayName("Omega3s");
  const std::string k_AxisLengthsArrayName("AxisLengths");
  const std::string k_AxisEulerAnglesArrayName("AxisEulerAngles");
  const std::string k_AspectRatiosArrayName("AspectRatios");
  const std::string k_VolumesArrayName("Shape Volumes");
  const std::string k_Omega3sArrayNameNX("Omega3sNX");
  const std::string k_AxisLengthsArrayNameNX("AxisLengthsNX");
  const std::string k_AxisEulerAnglesArrayNameNX("AxisEulerAnglesNX");
  const std::string k_AspectRatiosArrayNameNX("AspectRatiosNX");
  const std::string k_VolumesArrayNameNX("Shape VolumesNX");

  // Instantiate ComputeShapesFilter
  {
    ComputeShapesFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});

    const DataPath k_Omega3sArrayPath({k_DataContainer, k_CellFeatureData, k_Omega3sArrayNameNX});
    const DataPath k_AxisLengthsArrayPath({k_DataContainer, k_CellFeatureData, k_AxisLengthsArrayNameNX});
    const DataPath k_AxisEulerAnglesArrayPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayNameNX});
    const DataPath k_AspectRatiosArrayPath({k_DataContainer, k_CellFeatureData, k_AspectRatiosArrayNameNX});
    const DataPath k_VolumesArrayPath({k_DataContainer, k_CellFeatureData, k_VolumesArrayNameNX});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsArrayPath));
    args.insertOrAssign(ComputeShapesFilter::k_Omega3sArrayName_Key, std::make_any<std::string>(k_Omega3sArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<std::string>(k_AxisLengthsArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<std::string>(k_AxisEulerAnglesArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<std::string>(k_AspectRatiosArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_VolumesArrayName_Key, std::make_any<std::string>(k_VolumesArrayNameNX));
    args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_Omega3sArrayName, k_AxisLengthsArrayName, k_AxisEulerAnglesArrayName, k_AspectRatiosArrayName, k_VolumesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, comparisonName + "NX"});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<float>(exemplarData, calculatedData);
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_shapes.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeShapesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeShapesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeShapesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeShapesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeShapesFilter::k_CentroidsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeShapesFilter::k_Omega3sArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AxisLengthsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_AspectRatiosArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeShapesFilter::k_VolumesArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: 3D Oracle", "[OrientationAnalysis][ComputeShapesFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("F1 anisotropic box")
  {
    // 8 x 4 x 2 cells, spacing (0.75, 0.5, 0.25), origin 0, every cell in feature 1.
    // G = (5.3125, 1.3125, 0.3125); V = 64 * 0.09375 = 6
    const std::array<usize, 3> dims = {8, 4, 2};
    ShapesFixture fixture = BuildFixture(dims, {0.75f, 0.5f, 0.25f}, {0.0f, 0.0f, 0.0f}, FillIds(64, 1), 2);
    RunShapes(fixture);

    CheckFeature3D(fixture, 1, 6.0, {3.806257634215029, 1.2612672136598166, 0.30771769145455}, {0.33136674783180564, 0.08084520834544379}, 0.8570860756352066);
    CheckInvariants(fixture, false);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F2 two boxes plus an empty feature id")
  {
    // 8 x 4 x 2 cells, spacing (0.5, 0.25, 0.125). Feature 1 is x in [0, 4),
    // feature 2 is x in [4, 8) - both 4 x 4 x 2 boxes - and feature 3 has no
    // cells at all. The empty feature's axis outputs must all be zero rather
    // than NaN (the (A^4)/(B*C) division in findAxes is 0/0 for a feature with
    // no cells).
    const std::array<usize, 3> dims = {8, 4, 2};
    std::vector<int32> featureIds(64, 0);
    for(usize cellIdx = 0; cellIdx < 64; cellIdx++)
    {
      featureIds[cellIdx] = (cellIdx % 8) < 4 ? 1 : 2;
    }
    ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.25f, 0.125f}, {0.0f, 0.0f, 0.0f}, featureIds, 4);
    RunShapes(fixture);

    const std::vector<float64> axisLengths = {1.262760738580534, 0.631380369290267, 0.15404103711216124};
    const std::vector<float64> aspectRatios = {0.5, 0.12198750911856697};
    CheckFeature3D(fixture, 1, 0.5, axisLengths, aspectRatios, 0.8672894812975347);
    CheckFeature3D(fixture, 2, 0.5, axisLengths, aspectRatios, 0.8672894812975347);
    // The empty feature
    CheckFeature3D(fixture, 3, 0.0, {0.0, 0.0, 0.0}, {0.0, 0.0}, 0.0);
    CheckTuple(fixture.dataStructure, fixture.axisEulerAnglesPath, 3, {0.0, 0.0, 0.0}, k_AxisRelTol, k_AngleAbsTol);

    // Sum of the volumes over the labelled features equals the geometry volume
    const auto& volumes = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.volumesPath);
    const float64 volumeSum = static_cast<float64>(volumes[1]) + static_cast<float64>(volumes[2]) + static_cast<float64>(volumes[3]);
    CheckComponent("volume sum", 0, 0, static_cast<float32>(volumeSum), 8 * 4 * 2 * 0.5 * 0.25 * 0.125, k_VolumeRelTol, k_ZeroAbsTol);

    CheckInvariants(fixture, false);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F3 single voxel")
  {
    // 3 x 3 x 3 cells, spacing (0.5, 0.25, 0.125), non-zero origin. Only the
    // centre cell is labelled, so N_i == 1 on every axis and G_i is the bare
    // 1/16 quadrature self-moment. Omega3 comes out above 1 and must clamp.
    const std::array<usize, 3> dims = {3, 3, 3};
    std::vector<int32> featureIds(27, 0);
    featureIds[1 + 3 * 1 + 9 * 1] = 1;
    ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.25f, 0.125f}, {2.0f, -1.0f, 0.5f}, featureIds, 2);
    RunShapes(fixture);

    CheckFeature3D(fixture, 1, 0.015625, {0.2975241936287001, 0.14876209681435004, 0.07438104840717494}, {0.5, 0.24999999999999972}, 1.0);
    CheckInvariants(fixture, false);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F4 isotropic cube")
  {
    // 4 x 4 x 4 cells of isotropic spacing: all three eigenvalues are exactly
    // equal, which drives TripletSort down its all-ties path, and both aspect
    // ratios must be exactly 1.
    const std::array<usize, 3> dims = {4, 4, 4};
    ShapesFixture fixture = BuildFixture(dims, {0.25f, 0.25f, 0.25f}, {0.0f, 0.0f, 0.0f}, FillIds(64, 1), 2);
    RunShapes(fixture);

    const float64 axis = 0.6283073568383009;
    CheckFeature3D(fixture, 1, 1.0, {axis, axis, axis}, {1.0, 1.0}, 0.8259899821881282);

    // The three semi-axes must agree with each other bit for bit, not just
    // within tolerance, because the moment matrix is exactly isotropic.
    const auto& axisLengths = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisLengthsPath);
    REQUIRE(axisLengths[3] == axisLengths[4]);
    REQUIRE(axisLengths[4] == axisLengths[5]);

    CheckInvariants(fixture, false);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F8 multi-feature fully labelled grid")
  {
    // 6 x 4 x 2 cells, spacing (0.25, 0.125, 0.5). spacing[2] is the largest,
    // which exercises the third m_ScaleFactor branch. Three features of
    // differing widths, no unlabelled cells.
    const std::array<usize, 3> dims = {6, 4, 2};
    std::vector<int32> featureIds(48, 0);
    for(usize cellIdx = 0; cellIdx < 48; cellIdx++)
    {
      const usize xIdx = cellIdx % 6;
      featureIds[cellIdx] = xIdx < 2 ? 1 : (xIdx == 2 ? 2 : 3);
    }
    ShapesFixture fixture = BuildFixture(dims, {0.25f, 0.125f, 0.5f}, {0.0f, 0.0f, 0.0f}, featureIds, 4);
    RunShapes(fixture);

    CheckFeature3D(fixture, 1, 0.25, {0.6191777692188447, 0.3172342058281098, 0.3095888846094223}, {0.5123475382979799, 0.4999999999999999}, 0.9106539553624113);
    CheckFeature3D(fixture, 2, 0.125, {0.6331496286205737, 0.32439265359803116, 0.14157656095243495}, {0.51234753829798, 0.22360679774997905}, 1.0);
    CheckFeature3D(fixture, 3, 0.375, {0.6169300552204079, 0.47118811275680095, 0.31608259509421277}, {0.7637626158259734, 0.5123475382979799}, 0.8781305998137537);

    const auto& volumes = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.volumesPath);
    const float64 volumeSum = static_cast<float64>(volumes[1]) + static_cast<float64>(volumes[2]) + static_cast<float64>(volumes[3]);
    CheckComponent("volume sum", 0, 0, static_cast<float32>(volumeSum), 6 * 4 * 2 * 0.25 * 0.125 * 0.5, k_VolumeRelTol, k_ZeroAbsTol);

    CheckInvariants(fixture, false);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: 2D Oracle", "[OrientationAnalysis][ComputeShapesFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("F5 Z flat slab")
  {
    // 4 x 8 x 1 cells, isotropic spacing 0.5. The in-plane axes are X and Y,
    // so the in-plane axis mapping is already correct here; this section
    // isolates the voxel sample point convention and the degenerate axis Euler
    // branch. The slab is longer along Y, so Ixx > Iyy and the axis Euler
    // angle must be pi/2. Feature id 2 is left with no cells to pin the
    // empty-feature behaviour of the 2D path.
    const std::array<usize, 3> dims = {4, 8, 1};
    ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 0.0f}, FillIds(32, 1), 3);
    RunShapes(fixture);

    CheckFeature2D(fixture, 1, 8.0, 2.2840716229601994, 1.135298078272853, 0.4970501217477084, nx::core::numbers::pi / 2.0);
    // The empty feature: no cells means no axes, so every axis output is zero.
    // In particular the aspect ratio must not be the ratio of two spacings.
    CheckFeature2D(fixture, 2, 0.0, 0.0, 0.0, 0.0, 0.0);
    CheckInvariants(fixture, true);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F6a Y flat slab")
  {
    // 4 x 1 x 8 cells, spacing (0.5, 0.25, 0.125), non-zero origin. The
    // in-plane axes are X (4 cells, spacing 0.5) and Z (8 cells, spacing
    // 0.125), so the second in-plane axis must be remapped from Y to Z for the
    // spacing, the origin and the centroid component alike. X is the longer
    // extent, so Iyy > Ixx and the axis Euler angle must be 0.
    const std::array<usize, 3> dims = {4, 1, 8};
    ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.25f, 0.125f}, {0.5f, 1.0f, 2.0f}, FillIds(32, 1), 2);
    RunShapes(fixture);

    CheckFeature2D(fixture, 1, 2.0, 1.135298078272853, 0.5710179057400498, 0.5029673851018478, 0.0);
    CheckInvariants(fixture, true);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }

  SECTION("F6b X flat slab")
  {
    // 1 x 2 x 8 cells, spacing (0.5, 0.5, 0.25), non-zero origin. The in-plane
    // axes are Y (2 cells, spacing 0.5) and Z (8 cells, spacing 0.25), so both
    // in-plane axes must be remapped. Z is the longer extent, so Ixx > Iyy and
    // the axis Euler angle must be pi/2.
    const std::array<usize, 3> dims = {1, 2, 8};
    ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.5f, 0.25f}, {0.5f, 1.0f, 2.0f}, FillIds(16, 1), 2);
    RunShapes(fixture);

    CheckFeature2D(fixture, 1, 2.0, 1.1490221080765564, 0.5573575903258251, 0.485071250072666, nx::core::numbers::pi / 2.0);
    CheckInvariants(fixture, true);
    UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Rotated Feature", "[OrientationAnalysis][ComputeShapesFilter]")
{
  UnitTest::LoadPlugins();

  // F7: a plate whose short axis lies along the in-plane 45 degree direction.
  // On an 8 x 8 x 8 grid of isotropic spacing 0.5, feature 1 is every cell with
  // |i - j| <= 1. That set is invariant under i <-> j and under
  // (i, j) -> (7 - i, 7 - j), so the principal axes are exactly
  // u = (1, 1, 0)/sqrt(2), v = (-1, 1, 0)/sqrt(2) and z, with extents
  // u > z > v. The off-diagonal xy moment is therefore large and non-zero, so
  // this fixture is the only one that exercises a genuine (non-diagonal) eigen
  // problem. Expected values come from the independent numpy reference
  // (ww_work/ComputeShapes/oracle.py, numeric_3d).
  const std::array<usize, 3> dims = {8, 8, 8};
  std::vector<int32> featureIds(512, 0);
  usize cellCount = 0;
  for(usize cellIdx = 0; cellIdx < 512; cellIdx++)
  {
    const auto xIdx = static_cast<int64>(cellIdx % 8);
    const auto yIdx = static_cast<int64>((cellIdx / 8) % 8);
    if(std::abs(xIdx - yIdx) <= 1)
    {
      featureIds[cellIdx] = 1;
      cellCount++;
    }
  }
  REQUIRE(cellCount == 176);

  ShapesFixture fixture = BuildFixture(dims, {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 0.0f}, featureIds, 2);
  RunShapes(fixture);

  CheckFeature3D(fixture, 1, 22.0, {3.2659572370809347, 2.5131894874843153, 0.6727548391375626}, {0.7695108371138893, 0.2059900942667765}, 0.7784084896356491);

  // Axis DIRECTIONS, never element-wise Euler angles. Eigen picks the sign of
  // each eigenvector arbitrarily and the handedness correction in
  // findAxisEulers is dead code (OrientationMatrix::isValid never returns 0),
  // so the assembled matrix may be improper. om2eu only reads om[2], om[5] and
  // om[6..8], so the third ROW of the matrix rebuilt from the reported Euler
  // angles always reproduces the eigenvector of the largest moment eigenvalue,
  // which is the shortest semi-axis. Compare it up to sign with |dot| == 1.
  REQUIRE_NOTHROW(fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisEulerAnglesPath));
  const auto& axisEulerAngles = fixture.dataStructure.getDataRefAs<Float32Array>(fixture.axisEulerAnglesPath);
  const ebsdlib::EulerDType euler(static_cast<float64>(axisEulerAngles[3]), static_cast<float64>(axisEulerAngles[4]), static_cast<float64>(axisEulerAngles[5]));
  const ebsdlib::OrientationMatrixDType orientationMatrix = euler.toOrientationMatrix();

  const float64 invRoot2 = 1.0 / std::sqrt(2.0);
  const std::array<float64, 3> shortAxis = {-invRoot2, invRoot2, 0.0};
  const float64 dotProduct = orientationMatrix[6] * shortAxis[0] + orientationMatrix[7] * shortAxis[1] + orientationMatrix[8] * shortAxis[2];
  CAPTURE(axisEulerAngles[3], axisEulerAngles[4], axisEulerAngles[5], orientationMatrix[6], orientationMatrix[7], orientationMatrix[8], dotProduct);
  REQUIRE(std::abs(std::abs(dotProduct) - 1.0) < 1.0e-5);

  CheckInvariants(fixture, false);
  UnitTest::CheckArraysInheritTupleDims(fixture.dataStructure);
}
