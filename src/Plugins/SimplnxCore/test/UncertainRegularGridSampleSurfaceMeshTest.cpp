#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/UncertainRegularGridSampleSurfaceMeshFilter.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <random>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_TriGeomName = "STL-Cylinder";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "UncertainRegularGrid";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);
} // namespace

TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: Valid Filter Execution", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  /**
   * THe generated test case file for this was generated on a temporary modification of the 6.6 DREAM3D-SIMPL fork where the
   * random generation was modified to use the same generator, engine, and distribution used in standard SIMPLNX random generation
   * utilizing the standard library. It was seeded with the std::mt19937::default_seed.
   */
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_sample_surface_mesh.tar.gz", "6_6_sample_surface_mesh");

  // Read Exemplar DREAM3D File Filter
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_sample_surface_mesh/6_6_grid_sample_surface_mesh.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    UncertainRegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, std::make_any<bool>(true));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, std::make_any<uint64>(std::mt19937::default_seed));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{179, 18, 2}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0f, 1.0f, 1.0f}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.25f, 0.25f, 0.25f}));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.1f, 0.1f, 0.1f}));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_uncertain_regular_grid_sample_surface_mesh.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, ::k_ExemplarImageGeomPath, ::k_GeneratedImageGeomPath);

  UnitTest::CompareArrays<int32>(dataStructure, ::k_ExemplarFeatureIdsPath, ::k_GeneratedFeatureIdsPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: SIMPL Backwards Compatibility", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "UncertainRegularGridSampleSurfaceMeshFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "UncertainRegularGridSampleSurfaceMeshFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(UncertainRegularGridSampleSurfaceMeshFilter::k_CellAMName_Key) == "TestName");
      CHECK(args.value<std::string>(UncertainRegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: Ray exits long thin feature", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter][SampleSurfaceMeshRay]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;

  // A sheared 1 x 1 x 20 box: 0 < x < 1, 0 < y - z/8 < 1, 0 < z < 20.
  // The shear keeps a long ray inside the thin feature and uses exact binary coordinates.
  // The first ray from IsPointInPolyhedron() is deterministic. With the current Ray basis its
  // direction is close to (0, 1/8, 1), so the shear y = z/8 follows it. A ray of half the box
  // diagonal then ends inside the feature at z = 1. If the Ray basis or the random numbers change
  // (see issue #1861), check again that this test fails with a half-diagonal ray.
  const std::array<float32, 24> vertices = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 2.5F, 20, 1, 2.5F, 20, 1, 3.5F, 20, 0, 3.5F, 20};
  const std::array<IGeometry::MeshIndexType, 36> faces = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7};
  const std::array<int32, 24> labels = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

  auto* triangleGeom = TriangleGeom::Create(dataStructure, k_TriGeomName);
  REQUIRE(triangleGeom != nullptr);
  auto* verticesArray =
      Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_TriGeomPath.createChildPath("Vertices"), {8}, {3}), triangleGeom->getId());
  auto* facesArray = IGeometry::MeshIndexArrayType::Create(
      dataStructure, "Faces", DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, k_TriGeomPath.createChildPath("Faces"), {12}, {3}), triangleGeom->getId());
  REQUIRE(verticesArray != nullptr);
  REQUIRE(facesArray != nullptr);
  auto verticesResult = verticesArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(vertices));
  SIMPLNX_RESULT_REQUIRE_VALID(verticesResult);
  auto facesResult = facesArray->getDataStoreRef().copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(faces));
  SIMPLNX_RESULT_REQUIRE_VALID(facesResult);
  triangleGeom->setVertices(*verticesArray);
  triangleGeom->setFaceList(*facesArray);
  auto* faceData = AttributeMatrix::Create(dataStructure, Constants::k_FaceData, ShapeType{12}, triangleGeom->getId());
  REQUIRE(faceData != nullptr);
  triangleGeom->setFaceAttributeMatrix(*faceData);
  auto* faceLabels = Int32Array::Create(dataStructure, Constants::k_FaceLabels, DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FaceLabelsPath, {12}, {2}), faceData->getId());
  REQUIRE(faceLabels != nullptr);
  auto labelsResult = faceLabels->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(labels));
  SIMPLNX_RESULT_REQUIRE_VALID(labelsResult);

  const float32 z = GENERATE(1.0F, 19.0F);
  CAPTURE(z);
  UncertainRegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, true);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, uint64{5489});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{3, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{-1, z / 8, z - 0.5F});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, k_TriGeomPath);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, k_FaceLabelsPath);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, k_GeneratedImageGeomPath);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::string(Constants::k_CellData));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::string(Constants::k_FeatureIds));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_GeneratedFeatureIdsPath));
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_GeneratedFeatureIdsPath);
  std::array<int32, 3> actual = {};
  auto readResult = featureIds.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(actual));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  // Centers have x = {-0.5, 0.5, 1.5}, y - z/8 = 0.5, and z = 1 or 19.
  // Only the middle center satisfies all three strict box inequalities.
  const std::array<int32, 3> expected = {0, 1, 0};
  CHECK(actual == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include <limits>

namespace
{
void CreateSamplingTetrahedron(DataStructure& ds)
{
  auto result = CreateTriangleGeometryAction(DataPath({"Mesh"}), 4, 4, "Vertex Data", "Face Data", "Vertices", "Faces").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<TriangleGeom>(DataPath({"Mesh"})));
  auto& geom = ds.getDataRefAs<TriangleGeom>(DataPath({"Mesh"}));
  const std::array<float32, 12> coords = {0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4};
  const std::array<uint64, 12> faces = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3};
  auto write = geom.getVerticesRef().getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  auto faceWrite = geom.getFacesRef().getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint64>(faces));
  SIMPLNX_RESULT_REQUIRE_VALID(faceWrite);
  auto labels = CreateArrayAction(DataType::int32, {4}, {2}, DataPath({"Mesh", "Face Data", "Labels"})).apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(labels);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Face Data", "Labels"})));
  auto& array = ds.getDataRefAs<Int32Array>(DataPath({"Mesh", "Face Data", "Labels"}));
  for(usize i = 0; i < 4; ++i)
  {
    array[2 * i] = 1;
    array[2 * i + 1] = 0;
  }
}
} // namespace

TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: Spatial parameter domains", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  UncertainRegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  int32 expectedCode;
  std::string parameter;
  const auto axis = GENERATE(0, 1, 2);
  const auto value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  CAPTURE(axis, value);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, true);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, uint64{5489});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::vector<float32>{0, 0, 0});
  SECTION("Spacing")
  {
    std::vector<float32> values = {1, 1, 1};
    values[axis] = value;
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, values);
    expectedCode = -86105;
    parameter = "Spacing";
  }
  SECTION("Origin")
  {
    std::vector<float32> values = {0, 0, 0};
    values[axis] = value;
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, values);
    expectedCode = -86107;
    parameter = "Origin";
  }
  SECTION("Uncertainty")
  {
    std::vector<float32> values = {0, 0, 0};
    values[axis] = value;
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, values);
    expectedCode = -86106;
    parameter = "Uncertainty";
  }

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);

  CHECK(result.outputActions.errors()[0].message.find(fmt::format("{}", value)) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(std::string(1, "XYZ"[axis])) != std::string::npos);
}
TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: Nonpositive sampling spacing", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  UncertainRegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  int32 expectedCode;
  std::string parameter;
  const auto axis = GENERATE(0, 1, 2);
  const auto value = GENERATE(0.0F, -1.0F);
  CAPTURE(axis, value);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, true);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, uint64{5489});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::vector<float32>{0, 0, 0});
  std::vector<float32> values = {1, 1, 1};
  values[axis] = value;
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, values);
  expectedCode = -86105;
  parameter = "Spacing";

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(std::string(1, "XYZ"[axis])) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(fmt::format("{}", value)) != std::string::npos);
}

TEST_CASE("SimplnxCore::UncertainRegularGridSampleSurfaceMeshFilter: Sampling boundary controls", "[SimplnxCore][UncertainRegularGridSampleSurfaceMeshFilter][SpatialSamplingControls]")
{

  DataStructure ds;
  CreateSamplingTetrahedron(ds);
  UncertainRegularGridSampleSurfaceMeshFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Mesh"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, DataPath({"Mesh", "Face Data", "Labels"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, DataPath({"Grid"}));
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0, 0});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::vector<float32>{1, 1, 1});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_UseSeed_Key, true);
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_SeedValue_Key, uint64{5489});
  args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::vector<float32>{0, 0, 0});
  SECTION("Signed uncertainty")
  {
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key, std::vector<float32>{-0.1F, 0, 0});
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Repeatable zero uncertainty")
  {
    for(usize run = 0; run < 2; ++run)
    {
      DataStructure repeated = ds;
      auto result = filter.execute(repeated, args);
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
      REQUIRE_NOTHROW(repeated.getDataRefAs<Int32Array>(DataPath({"Grid", "Cell Data", "Feature Ids"})));
      const auto& ids = repeated.getDataRefAs<Int32Array>(DataPath({"Grid", "Cell Data", "Feature Ids"}));
      REQUIRE(ids.getNumberOfTuples() == 1);
      CHECK(ids[0] == 1);
    }
  }
  SECTION("Spacing and uncertainty length")
  {
    const auto key = GENERATE(UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key, UncertainRegularGridSampleSurfaceMeshFilter::k_Uncertainty_Key);
    args.insertOrAssign(key, std::vector<float32>{1, 1});
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors()[0].code == -1);
    CHECK(filter.parameters().at(key)->humanName() == (key.view() == UncertainRegularGridSampleSurfaceMeshFilter::k_Spacing_Key.view() ? "Spacing" : "Uncertainty"));
    CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
  }
  SECTION("Origin length")
  {
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::vector<float32>{0, 0});
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors()[0].code == -1);
    CHECK(filter.parameters().at(UncertainRegularGridSampleSurfaceMeshFilter::k_Origin_Key)->humanName() == "Origin");
    CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
  }
  SECTION("Dimensions length")
  {
    args.insertOrAssign(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::vector<uint64>{1, 1});
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors()[0].code == -1);
    CHECK(filter.parameters().at(UncertainRegularGridSampleSurfaceMeshFilter::k_Dimensions_Key)->humanName() == "Number of Cells per Axis");
    CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
  }
}
