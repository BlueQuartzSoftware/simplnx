#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/RegularGridSampleSurfaceMeshFilter.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_TriGeomName = "Input Triangle Geometry";
const DataPath k_TriGeomPath = DataPath({k_TriGeomName});
const DataPath k_FaceLabelsPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath(Constants::k_FaceLabels);

const std::string k_ExemplarImageGeomName = "Exemplar Sample Triangle Geometry on Regular Grid";
const DataPath k_ExemplarImageGeomPath = DataPath({k_ExemplarImageGeomName});
const DataPath k_ExemplarFeatureIdsPath = k_ExemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_FeatureIds);

const DataPath k_GeneratedImageGeomPath = DataPath({Constants::k_ImageGeometry});
const DataPath k_GeneratedCellDataPath = k_GeneratedImageGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_GeneratedFeatureIdsPath = k_GeneratedCellDataPath.createChildPath(Constants::k_FeatureIds);

const std::vector<uint64> k_Dims{171, 200, 150};
const std::vector<float32> k_Origin{1.0f, 1.99f, 0.0f};
const std::vector<float32> k_Spacing{0.1f, 0.1f, 0.02f};

/**
 * @brief Builds vertical quads with two triangles and two Face Labels per quad.
 * @param dataStructure Receives the Triangle Geometry and its arrays.
 * @param positions Specifies each quad's X coordinate.
 * @param labels Specifies the pair of Face Labels for each quad.
 */
void BuildVerticalQuads(DataStructure& dataStructure, const std::vector<float32>& positions, const std::vector<std::array<int32, 2>>& labels)
{
  REQUIRE(positions.size() == labels.size());
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, k_TriGeomName);
  REQUIRE(triangleGeomPtr != nullptr);
  const usize numQuads = positions.size();
  const auto verticesPath = k_TriGeomPath.createChildPath("Vertices");
  const auto facesPath = k_TriGeomPath.createChildPath("Faces");
  auto* verticesPtr = Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, verticesPath, {numQuads * 4}, {3}), triangleGeomPtr->getId());
  auto* facesPtr = IGeometry::MeshIndexArrayType::Create(dataStructure, "Faces", DataStoreUtilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, facesPath, {numQuads * 2}, {3}),
                                                         triangleGeomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  triangleGeomPtr->setVertices(*verticesPtr);
  triangleGeomPtr->setFaceList(*facesPtr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, Constants::k_FaceData, ShapeType{numQuads * 2}, triangleGeomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  triangleGeomPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto* labelsPtr = Int32Array::Create(dataStructure, Constants::k_FaceLabels, DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FaceLabelsPath, {numQuads * 2}, {2}), faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  for(usize quadIdx = 0; quadIdx < numQuads; ++quadIdx)
  {
    const float32 x = positions[quadIdx];
    const std::array<float32, 12> coords = {x, 0.0F, 0.0F, x, 1.0F, 0.0F, x, 1.0F, 1.0F, x, 0.0F, 1.0F};
    const std::array<IGeometry::MeshIndexType, 6> indices = {0, 1, 2, 0, 2, 3};
    for(usize compIdx = 0; compIdx < coords.size(); ++compIdx)
    {
      (*verticesPtr)[quadIdx * 12 + compIdx] = coords[compIdx];
    }
    for(usize compIdx = 0; compIdx < indices.size(); ++compIdx)
    {
      (*facesPtr)[quadIdx * 6 + compIdx] = quadIdx * 4 + indices[compIdx];
    }
    for(usize compIdx = 0; compIdx < 4; ++compIdx)
    {
      (*labelsPtr)[quadIdx * 4 + compIdx] = labels[quadIdx][compIdx % 2];
    }
  }
}

Arguments MakeSmallGridArguments()
{
  Arguments args;
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{5, 1, 1}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0, 0, 0}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriGeomPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_GeneratedImageGeomPath));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));
  return args;
}

template <typename T>
void RequireSmallGridFeatureIds(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(k_GeneratedFeatureIdsPath));
  const auto& featureIdsRef = dataStructure.getDataRefAs<DataArray<T>>(k_GeneratedFeatureIdsPath);
  const std::vector<T> actual(featureIdsRef.begin(), featureIdsRef.end());
  const std::vector<T> expected = {0, 2, 2, 2, 0};
  REQUIRE(actual == expected);
}

} // namespace

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject nonpositive or nonfinite spacing when creating geometry", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto axis = GENERATE(usize{0}, usize{1}, usize{2});
  const auto invalidSpacing = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity());
  CAPTURE(axis, invalidSpacing);
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  auto validResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(validResult.outputActions);

  auto* imageGeom = ImageGeom::Create(dataStructure, "Existing Image Geometry");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({5, 1, 1});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellData = AttributeMatrix::Create(dataStructure, Constants::k_CellData, ShapeType{1, 1, 5}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  VectorFloat32Parameter::ValueType spacing{1.0F, 1.0F, 1.0F};
  spacing[axis] = invalidSpacing;
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, spacing);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key,
                      std::make_any<ChoicesParameter::ValueType>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, DataPath({"Existing Image Geometry"}));
  auto existingResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(existingResult.outputActions);

  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key,
                      std::make_any<ChoicesParameter::ValueType>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11802);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject Face Labels outside Face Attribute Matrix", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  auto* otherFaceData = AttributeMatrix::Create(dataStructure, "Other Face Data", ShapeType{4});
  REQUIRE(otherFaceData != nullptr);
  const DataPath otherLabelsPath({"Other Face Data", "Face Labels"});
  auto* otherLabels = Int32Array::Create(dataStructure, "Face Labels", DataStoreUtilities::CreateDataStore<int32>(dataStructure, otherLabelsPath, {4}, {2}), otherFaceData->getId());
  REQUIRE(otherLabels != nullptr);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, otherLabelsPath);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11803);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Reject missing Face Attribute Matrix without throwing", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  auto* triangleGeom = TriangleGeom::Create(dataStructure, "Without Face Data");
  REQUIRE(triangleGeom != nullptr);
  auto* sourceGeom = dataStructure.getDataAs<TriangleGeom>(k_TriGeomPath);
  REQUIRE(sourceGeom != nullptr);
  triangleGeom->setVertices(*sourceGeom->getVertices());
  triangleGeom->setFaceList(*sourceGeom->getFaces());
  REQUIRE(triangleGeom->getFaceAttributeMatrix() == nullptr);
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, DataPath({"Without Face Data"}));
  IFilter::PreflightResult preflightResult;
  REQUIRE_NOTHROW(preflightResult = filter.preflight(dataStructure, args));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -11803);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:CreateImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  std::string sectionTitle = "Exemplar Image Geometry Face Labels Create";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers int32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers uint8 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

    DataPath exemplarImageGeomPath({sectionTitle});
    UnitTest::CompareImageGeometry(dataStructure, exemplarImageGeomPath, ::k_GeneratedImageGeomPath);
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarImageGeomPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds"), ::k_GeneratedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "Exemplar Image Geometry Part Numbers float32 Create";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (float32)");

    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create)));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Dimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(k_Dims));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Origin));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(k_Spacing));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(6ULL));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(::k_GeneratedImageGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(Constants::k_FeatureIds));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter:ExistingImageGeom", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "sample_surface_mesh_regular_grid_v1.tar.gz", "sample_surface_mesh_regular_grid_v1");
  auto baseDataFilePath = fs::path(fmt::format("{}/sample_surface_mesh_regular_grid_v1/sample_surface_mesh_regular_grid_v1.dream3d", unit_test::k_TestFilesDir));

  DataPath exemplarImageGeometryPath({"Exemplar Image Geometry Existing"});
  std::string sectionTitle = "FeatureIds (FaceLabels)";
  SECTION(sectionTitle)
  {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(::k_FaceLabelsPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<int32>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  sectionTitle = "FeatureIds (PartNumber uint8)";
  SECTION(sectionTitle)
  {
    DataPath partNumberPath = k_TriGeomPath.createChildPath(Constants::k_FaceData).createChildPath("PartNumber (uint8)");
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

    // Instantiate the filter, a DataStructure object and an Arguments Object
    RegularGridSampleSurfaceMeshFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseExistingGeometry_Key, std::make_any<uint64>(to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting)));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(::k_TriGeomPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(partNumberPath));

    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_ExistingImageGeomPath_Key, std::make_any<DataPath>(exemplarImageGeometryPath));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_regular_grid_sample_surface_mesh_create_geometry.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
    DataPath exemplarFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath(sectionTitle);
    DataPath computedFeatureIdsPath = exemplarImageGeometryPath.createChildPath(Constants::k_CellData).createChildPath("FeatureIds");
    UnitTest::CompareArrays<uint8>(dataStructure, exemplarFeatureIdsPath, computedFeatureIdsPath);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RegularGridSampleSurfaceMeshFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RegularGridSampleSurfaceMeshFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<ChoicesParameter::ValueType>(RegularGridSampleSurfaceMeshFilter::k_LengthUnit_Key) == 0);
        // Complex type (UInt64ToVec3FilterParameterConverter) - verified by successful pipeline loading
      }
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_TriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(RegularGridSampleSurfaceMeshFilter::k_ImageGeomPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(RegularGridSampleSurfaceMeshFilter::k_CellAMName_Key) == "TestName");
      CHECK(args.value<std::string>(RegularGridSampleSurfaceMeshFilter::k_FeatureIdsArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Custom Output Type", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  const auto outputType = GENERATE(DataType::uint8, DataType::int64);
  CAPTURE(outputType);
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(outputType)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath).getDataType() == outputType);
  if(outputType == DataType::uint8)
  {
    RequireSmallGridFeatureIds<uint8>(dataStructure);
  }
  else
  {
    RequireSmallGridFeatureIds<int64>(dataStructure);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Output type ignored when Use Custom Output Type is off", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 2}, {0, 2}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, false);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DataType::uint8)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(k_GeneratedFeatureIdsPath).getDataType() == dataStructure.getDataRefAs<IDataArray>(k_FaceLabelsPath).getDataType());
  RequireSmallGridFeatureIds<int32>(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Custom Output Type value out of range", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{0, 300}, {0, 300}});
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
  args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(DataType::uint8)));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == -11801);
  REQUIRE(result.result.errors()[0].message.find("300") != std::string::npos);
  REQUIRE(result.result.errors()[0].message.find("uint8") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Exterior -1 Labels", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  SECTION("A: exterior and Feature 2")
  {
    BuildVerticalQuads(dataStructure, {1.2F, 3.7F}, {{-1, 2}, {-1, 2}});
  }
  SECTION("B: exterior and Feature 0 before Feature 2")
  {
    BuildVerticalQuads(dataStructure, {0.2F, 1.2F, 3.7F}, {{-1, 0}, {0, 2}, {0, 2}});
  }
  RegularGridSampleSurfaceMeshFilter filter;
  auto args = MakeSmallGridArguments();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  RequireSmallGridFeatureIds<int32>(dataStructure);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RegularGridSampleSurfaceMeshFilter: Use Custom Output Type parameter layout", "[SimplnxCore][RegularGridSampleSurfaceMeshFilter]")
{
  RegularGridSampleSurfaceMeshFilter filter;
  const auto params = filter.parameters();
  SECTION("Order")
  {
    const auto keys = params.getKeys();
    const auto customIter = std::find(keys.begin(), keys.end(), RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key);
    REQUIRE(customIter != keys.end());
    REQUIRE(std::next(customIter) != keys.end());
    REQUIRE(*std::next(customIter) == RegularGridSampleSurfaceMeshFilter::k_OutputType_Key);
  }
  SECTION("Link")
  {
    const auto& groups = params.getLinkedGroups(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key);
    REQUIRE(groups.size() == 1);
    REQUIRE(groups[0].first == RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key);
    REQUIRE(std::any_cast<bool>(groups[0].second));
    Arguments args;
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, false);
    REQUIRE_FALSE(params.isParameterActive(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, args));
    args.insertOrAssign(RegularGridSampleSurfaceMeshFilter::k_UseCustomOutputType_Key, true);
    REQUIRE(params.isParameterActive(RegularGridSampleSurfaceMeshFilter::k_OutputType_Key, args));
  }
}
