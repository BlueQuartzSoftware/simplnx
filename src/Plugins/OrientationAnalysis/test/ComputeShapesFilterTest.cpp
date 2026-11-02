#include "OrientationAnalysis/Filters/ComputeShapesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/Orientation/Euler.hpp>

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <cmath>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("OrientationAnalysis::ComputeShapesFilter", "[SimplnxCore][ComputeShapesFilter]")
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
    std::vector<std::string> comparisonNames = {k_Omega3sArrayName, k_AxisLengthsArrayName, k_AspectRatiosArrayName, k_VolumesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, comparisonName + "NX"});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<float>(exemplarData, calculatedData);
    }

    const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayName});
    const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayNameNX});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(exemplarPath));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(calculatedPath));
    const auto& exemplar = dataStructure.getDataRefAs<Float32Array>(exemplarPath);
    const auto& calculated = dataStructure.getDataRefAs<Float32Array>(calculatedPath);
    REQUIRE(exemplar.getNumberOfComponents() == 3);
    REQUIRE(calculated.getNumberOfComponents() == 3);
    REQUIRE(exemplar.getNumberOfTuples() == calculated.getNumberOfTuples());
    usize equalFeatures = 0;
    usize mirroredFeatures = 0;
    for(usize feature = 0; feature < calculated.getNumberOfTuples(); feature++)
    {
      CAPTURE(feature);
      const usize offset = 3 * feature;
      const auto orientation = ebsdlib::EulerDType(calculated[offset], calculated[offset + 1], calculated[offset + 2]).toOrientationMatrix();
      OrientationUtilities::Matrix3dR frame = Eigen::Map<const OrientationUtilities::Matrix3dR>(orientation.data());
      CHECK(frame.determinant() == Approx(1.0).margin(1.0e-6));
      bool equalsExemplar = true;
      for(usize component = 0; component < 3; component++)
      {
        equalsExemplar = equalsExemplar && std::abs(exemplar[offset + component] - calculated[offset + component]) <= UnitTest::EPSILON;
      }
      if(equalsExemplar)
      {
        equalFeatures++;
        continue;
      }

      // The archive stores the old conversion of mirrored principal frames. Undo the row-2 mirror before that conversion.
      frame.row(2) *= -1.0;
      const auto legacyEuler = ebsdlib::OrientationMatrixDType(frame.data()).toEuler();
      for(usize component = 0; component < 3; component++)
      {
        CAPTURE(component);
        const float32 legacyValue = static_cast<float32>(legacyEuler[component]);
        CAPTURE(exemplar[offset + component], calculated[offset + component], legacyValue);
        REQUIRE(std::abs(exemplar[offset + component] - legacyValue) <= UnitTest::EPSILON);
      }
      mirroredFeatures++;
    }
    // The archive holds both proper and mirrored principal frames, so both branches must be exercised.
    CHECK(equalFeatures > 0);
    CHECK(mirroredFeatures > 0);
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

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Geometry cell tuple count", "[OrientationAnalysis][ComputeShapesFilter][TupleCount]")
{
  using FilterType = ComputeShapesFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Grid");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 1});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 3, 4}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Equal counts remain valid in another Attribute Matrix with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Grid"})));
  args.insertOrAssign(FilterType::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeomPtr->getId());
  REQUIRE(featureAMPtr != nullptr);
  REQUIRE(DataArray<float32>::Create(dataStructure, "Centroids", std::make_shared<DataStore<float32>>(ShapeType{2}, ShapeType{3}, 0), featureAMPtr->getId()) != nullptr);
  args.insertOrAssign(FilterType::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Feature Data", "Centroids"})));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -12806);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Grid' has 12 cells") != std::string::npos);
  }
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Cell Feature Ids requires 1 component", "[OrientationAnalysis][ComputeShapesFilter][ComponentShape]")
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({2, 1, 1});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", {1, 1, 2}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "Feature Ids", {1, 1, 2}, {2}, cellData->getId());
  (*featureIds)[0] = 1;
  (*featureIds)[1] = 0;
  (*featureIds)[2] = 2;
  (*featureIds)[3] = 0;
  auto* featureData = AttributeMatrix::Create(dataStructure, "Feature Data", {3}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
  auto* centroids = UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {3}, {3}, featureData->getId());
  (*centroids)[3] = 0.5F;
  (*centroids)[4] = 0.5F;
  (*centroids)[5] = 0.5F;
  (*centroids)[6] = 1.5F;
  (*centroids)[7] = 0.5F;
  (*centroids)[8] = 0.5F;

  ComputeShapesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "Feature Ids"})));
  args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Feature Data", "Centroids"})));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(std::any_of(preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end(), [](const Error& error) { return error.code == -208; }));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
Arguments ShapeArguments()
{
  ComputeShapesFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeShapesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Grid"})));
  args.insertOrAssign(ComputeShapesFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Cells", "Ids"})));
  args.insertOrAssign(ComputeShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Features", "Centroids"})));
  args.insertOrAssign(ComputeShapesFilter::k_VolumesArrayName_Key, std::make_any<std::string>("Areas"));
  args.insertOrAssign(ComputeShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<std::string>("Lengths"));
  args.insertOrAssign(ComputeShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<std::string>("Ratios"));
  args.insertOrAssign(ComputeShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<std::string>("Eulers"));
  return args;
}

void CheckOrderedAxisLines(const std::array<float64, 3>& euler, const OrientationUtilities::Matrix3dR& expected)
{
  const auto orientation = ebsdlib::EulerDType(euler[0], euler[1], euler[2]).toOrientationMatrix();
  const Eigen::Map<const OrientationUtilities::Matrix3dR> actual(orientation.data());
  CHECK(actual.determinant() == Approx(1.0).margin(1.0e-6));
  for(usize row = 0; row < 3; row++)
  {
    CAPTURE(row);
    CHECK(std::abs(actual.row(row).dot(expected.row(row))) == Approx(1.0).margin(1.0e-6));
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Planar rectangle uses active axes and cell centers", "[OrientationAnalysis][ComputeShapesFilter]")
{
  const usize plane = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(plane);
  // XY, YZ and XZ contain the same 2-by-3 rectangle with active spacings 2 and 4.
  const std::array<SizeVec3, 3> dims = {{{2, 3, 1}, {1, 2, 3}, {2, 1, 3}}};
  const std::array<FloatVec3, 3> spacing = {{{2, 4, 8}, {8, 2, 4}, {2, 8, 4}}};
  const std::array<FloatVec3, 3> origin = {{{10, 20, 30}, {30, 10, 20}, {10, 30, 20}}};
  const std::array<FloatVec3, 3> centroid = {{{12, 26, 34}, {34, 12, 26}, {12, 34, 26}}};
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Grid");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions(dims[plane]);
  geometry->setSpacing(spacing[plane]);
  geometry->setOrigin(origin[plane]);
  const ShapeType shape = {dims[plane][2], dims[plane][1], dims[plane][0]};
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", shape, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(*cells);
  auto* ids = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", shape, {1}, cells->getId());
  REQUIRE(ids != nullptr);
  ids->fill(1);
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {2}, geometry->getId());
  REQUIRE(features != nullptr);
  auto* centers = UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {2}, {3}, features->getId());
  REQUIRE(centers != nullptr);
  for(usize axis = 0; axis < 3; axis++)
  {
    (*centers)[3 + axis] = centroid[plane][axis];
  }
  ComputeShapesFilter filter;
  const auto args = ShapeArguments();
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const Float32Array* areas = nullptr;
  const Float32Array* lengths = nullptr;
  const Float32Array* ratios = nullptr;
  const Float32Array* eulers = nullptr;
  REQUIRE_NOTHROW(areas = &dataStructure.getDataRefAs<Float32Array>(DataPath({"Grid", "Features", "Areas"})));
  REQUIRE_NOTHROW(lengths = &dataStructure.getDataRefAs<Float32Array>(DataPath({"Grid", "Features", "Lengths"})));
  REQUIRE_NOTHROW(ratios = &dataStructure.getDataRefAs<Float32Array>(DataPath({"Grid", "Features", "Ratios"})));
  REQUIRE_NOTHROW(eulers = &dataStructure.getDataRefAs<Float32Array>(DataPath({"Grid", "Features", "Eulers"})));
  // A=6*2*4=48. Four quarter-cell samples give E[x^2]=2^2*(2^2-1)/12+2^2/16=1.25.
  // E[y^2]=4^2*(3^2-1)/12+4^2/16=35/3, hence Ixx=560, Iyy=60 and Ixy=0.
  // a=(4/pi)^(1/4)*(560^3/60)^(1/8); b=(4/pi)^(1/4)*(60^3/560)^(1/8).
  CHECK((*areas)[1] == Approx(48.0));
  CHECK((*lengths)[3] == Approx(6.83170545).margin(1.0e-5));
  CHECK((*lengths)[4] == Approx(2.23620053).margin(1.0e-5));
  CHECK((*lengths)[5] == 0.0F);
  CHECK((*ratios)[2] == Approx(0.327326835).margin(1.0e-6)); // sqrt(60/560).
  CHECK((*ratios)[3] == 0.0F);
  // Ixy=0 and Ixx>Iyy select the second axis: pi/2 radians, not pi/180.
  CHECK((*eulers)[3] == Approx(1.5707963267948966).margin(1.0e-6));
  CHECK((*eulers)[4] == 0.0F);
  CHECK((*eulers)[5] == 0.0F);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Principal Euler frame preserves ordered axes", "[OrientationAnalysis][ComputeShapesFilter]")
{
  OrientationUtilities::Matrix3dR frame;
  SECTION("Symmetric improper H frame")
  {
    // H=I-(2/3)*ones: H*H^T=I, det(H)=-1. Its rows are ordered axis lines.
    frame << 1, -2, -2, -2, 1, -2, -2, -2, 1;
    frame /= 3.0;
  }
  SECTION("Nonsymmetric proper frame")
  {
    // Rodrigues rotation about (1,2,2)/3 with cos(theta)=3/5, sin(theta)=4/5.
    // R=(1/45)*[[29,-20,28],[28,35,-4],[-20,20,35]], det(R)=1.
    frame << 29, -20, 28, 28, 35, -4, -20, 20, 35;
    frame /= 45.0;
  }
  SECTION("Nonsymmetric improper frame")
  {
    // Negating row 1 of the same proper frame gives determinant -1.
    frame << 29, -20, 28, -28, -35, 4, -20, 20, 35;
    frame /= 45.0;
  }
  const auto euler = OrientationUtilities::PrincipalAxisFrameToEuler(frame);
  CheckOrderedAxisLines(euler, frame);
}

TEST_CASE("OrientationAnalysis::ComputeShapesFilter: Discrete grain preserves ordered axes under lattice transforms", "[OrientationAnalysis][ComputeShapesFilter]")
{
  const usize transform = GENERATE(usize{0}, usize{1}, usize{2});
  CAPTURE(transform);
  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Grid");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({3, 3, 3});
  const std::array<FloatVec3, 3> spacings = {{{2, 4, 8}, {8, 2, 4}, {4, 2, 8}}};
  geometry->setSpacing(spacings[transform]);
  geometry->setOrigin({10, 20, 30});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", {3, 3, 3}, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(*cells);
  auto* ids = UnitTest::CreateTestDataArray<int32>(dataStructure, "Ids", {3, 3, 3}, {1}, cells->getId());
  REQUIRE(ids != nullptr);
  ids->fill(0);
  for(usize z = 0; z < 3; z++)
  {
    for(usize y = 0; y < 3; y++)
    {
      for(usize x = 0; x < 3; x++)
      {
        // Nine cells in x=0 and two cells extending from its center form an asymmetric grain.
        if(x != 0 && (y != 1 || z != 1))
        {
          continue;
        }
        std::array<usize, 3> point = {x, y, z};
        if(transform == 1)
        {
          point = {z, x, y}; // Cyclic axis permutation.
        }
        if(transform == 2)
        {
          point = {2 - y, x, z}; // Exact 90-degree rotation about Z.
        }
        (*ids)[point[0] + 3 * point[1] + 9 * point[2]] = 1;
      }
    }
  }
  auto* features = AttributeMatrix::Create(dataStructure, "Features", {2}, geometry->getId());
  REQUIRE(features != nullptr);
  auto* centers = UnitTest::CreateTestDataArray<float32>(dataStructure, "Centroids", {2}, {3}, features->getId());
  REQUIRE(centers != nullptr);
  // The eleven centers have mean (17/11,6,12) before translation.
  // Var(X)=43/11-(17/11)^2+2^2/16=857/484.
  // Var(Y)=96/11+4^2/16=107/11; Var(Z)=384/11+8^2/16=428/11.
  // Cross moments vanish by Y/Z symmetry.
  // Inertia is trace(C)*I-C, so its increasing eigenvalues have axis lines Z,Y,X.
  const std::array<FloatVec3, 3> means = {{{17.0F / 11.0F, 6, 12}, {12, 17.0F / 11.0F, 6}, {6, 17.0F / 11.0F, 12}}};
  for(usize axis = 0; axis < 3; axis++)
  {
    (*centers)[3 + axis] = geometry->getOrigin()[axis] + means[transform][axis];
  }
  ComputeShapesFilter filter;
  const auto result = filter.execute(dataStructure, ShapeArguments());
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const Float32Array* eulers = nullptr;
  REQUIRE_NOTHROW(eulers = &dataStructure.getDataRefAs<Float32Array>(DataPath({"Grid", "Features", "Eulers"})));
  OrientationUtilities::Matrix3dR expected;
  if(transform == 0)
  {
    expected << 0, 0, 1, 0, 1, 0, 1, 0, 0;
  }
  else if(transform == 1)
  {
    expected << 1, 0, 0, 0, 0, 1, 0, 1, 0;
  }
  else
  {
    expected << 0, 0, 1, -1, 0, 0, 0, 1, 0;
  }
  CheckOrderedAxisLines({(*eulers)[3], (*eulers)[4], (*eulers)[5]}, expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
