#include "SimplnxCore/Filters/FlyingEdges3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace ContourTest
{
const float64 k_IsoVal = 328;

const std::string k_ImageGeometryName = "Geometry";
const std::string k_DataName = "Data";
const std::string k_VertexNormals = "VertexNormals";

const DataPath k_GeometryPath = DataPath({k_ImageGeometryName});
const DataPath k_DataPath = k_GeometryPath.createChildPath(Constants::k_Cell_Data).createChildPath(k_DataName);

const std::string k_NewTriangleContourName = "Contouring Test";
const std::string k_ExemplarTriangleContourName = "Contouring Geometry";

const DataPath k_ExemplarContourPath = DataPath({k_ExemplarTriangleContourName});
const DataPath k_NewContourPath = DataPath({k_NewTriangleContourName});

const DataPath k_ExemplarNormals = k_ExemplarContourPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(k_VertexNormals);
const DataPath k_NewNormals = k_NewContourPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(k_VertexNormals);
} // namespace ContourTest

TEST_CASE("SimplnxCore::Image Contouring Valid Execution", "[SimplnxCore][FlyingEdges3D]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "flying_edges_exemplar_v2.tar.gz", "flying_edges_exemplar.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/flying_edges_exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    FlyingEdges3DFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FlyingEdges3DFilter::k_IsoVal_Key, std::make_any<float64>(ContourTest::k_IsoVal));
    // Selected Data Objects
    args.insertOrAssign(FlyingEdges3DFilter::k_SelectedImageGeometryPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(ContourTest::k_GeometryPath));
    args.insertOrAssign(FlyingEdges3DFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(ContourTest::k_DataPath));
    // Output Path
    args.insertOrAssign(FlyingEdges3DFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataGroupCreationParameter::ValueType>(ContourTest::k_NewContourPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // This is in here because the exemplar face attribute matrix is not sized correctly. This will
    // correct that value allowing the test to proceed normally.
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_ExemplarContourPath);
    auto faceAttributeMatrixResizeResult = exemplarContourTriGeom.getFaceAttributeMatrixRef().resizeTuples({70});
    SIMPLNX_RESULT_REQUIRE_VALID(faceAttributeMatrixResizeResult);

    // Execute the filter and check the result
    UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    auto& newContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_NewContourPath);
    auto& exemplarContourTriGeom = dataStructure.getDataRefAs<TriangleGeom>(ContourTest::k_ExemplarContourPath);

    const auto& kNxVertArray = newContourTriGeom.getVerticesRef();
    const auto& kExemplarsVertArray = exemplarContourTriGeom.getVerticesRef();

    CompareDataArrays<IGeometry::SharedVertexList::value_type>(kExemplarsVertArray, kNxVertArray);

    const auto& kNxTriArray = newContourTriGeom.getFacesRef();
    const auto& kExemplarsTriArray = exemplarContourTriGeom.getFacesRef();

    CompareDataArrays<IGeometry::MeshIndexType>(kExemplarsTriArray, kNxTriArray);

    const auto& kNxNormalsArray = dataStructure.getDataRefAs<Float32Array>(ContourTest::k_NewNormals);
    const auto& kExemplarsNormalsArray = dataStructure.getDataRefAs<Float32Array>(ContourTest::k_ExemplarNormals);

    CompareDataArrays<float32>(kExemplarsNormalsArray, kNxNormalsArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/image_contouring_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::Image Contouring FlyingEdges3D: Geometry cell tuple count", "[SimplnxCore][FlyingEdges3D][TupleCount]")
{
  using FilterType = FlyingEdges3DFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // A matching array is valid outside Cell Data; short and long arrays must fail preflight.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<float32>::Create(dataStructure, "Values", std::make_shared<DataStore<float32>>(otherAMPtr->getShape(), ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(inputPath));

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
    REQUIRE(errorRef.code == -10510);
    const DataPath expectedPath = inputPath;
    const usize expectedTuples = tuples;
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::FlyingEdges3D fractional edge interpolation", "[SimplnxCore][FlyingEdges3D][L17]", int32, uint8, uint32, float32)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  // Inverted puts the high value at corner (0,0,0), so every cut edge descends.
  // Unsigned types then wrap if the weight is subtracted in T.
  const bool inverted = GENERATE(false, true);
  CAPTURE(inverted);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  auto* image = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 2, 2});
  image->setOrigin({1.0F, 2.0F, 3.0F});
  image->setSpacing({2.0F, 4.0F, 6.0F});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 2, 2}, image->getId());
  REQUIRE(cells != nullptr);
  image->setCellData(*cells);
  auto* values = DataArray<TestType>::Create(dataStructure, "Values", std::make_shared<DataStore<TestType>>(cells->getShape(), ShapeType{1}, 0), cells->getId());
  REQUIRE(values != nullptr);
  values->fill(static_cast<TestType>(inverted ? 0 : 10));
  (*values)[0] = static_cast<TestType>(inverted ? 10 : 0);

  FlyingEdges3DFilter filter;
  Arguments args;
  args.insertOrAssign(FlyingEdges3DFilter::k_IsoVal_Key, std::make_any<float64>(3.0));
  args.insertOrAssign(FlyingEdges3DFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FlyingEdges3DFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "Values"})));
  args.insertOrAssign(FlyingEdges3DFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Contour"})));
  auto result = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath contourPath({"Contour"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(contourPath));
  const auto& contour = dataStructure.getDataRefAs<TriangleGeom>(contourPath);
  REQUIRE(contour.getNumberOfVertices() == 3);
  REQUIRE(contour.getNumberOfFaces() == 1);
  const auto& vertices = contour.getVerticesRef();
  const DataPath normalsPath = contourPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(ContourTest::k_VertexNormals);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(normalsPath));
  const auto& normals = dataStructure.getDataRefAs<Float32Array>(normalsPath);
  // getPosCube uses origin + index * spacing, without a half-cell offset.
  // Only corner (0,0,0) is on its own side of iso. Each incident edge crosses at
  // t = (3 - 0) / (10 - 0) = 0.3, or t = (3 - 10) / (0 - 10) = 0.7 when inverted.
  const std::array<std::array<float32, 3>, 3> ascendingVertices = {{{1.6F, 2.0F, 3.0F}, {1.0F, 3.2F, 3.0F}, {1.0F, 2.0F, 4.8F}}};
  const std::array<std::array<float32, 3>, 3> descendingVertices = {{{2.4F, 2.0F, 3.0F}, {1.0F, 4.8F, 3.0F}, {1.0F, 2.0F, 7.2F}}};
  // One-sided negative gradients: the crossing-axis component stays -/+10/spacing;
  // the other components interpolate from -/+10/spacing to zero.
  const std::array<std::array<float32, 3>, 3> ascendingNormals = {{{-5.0F, -1.75F, -7.0F / 6.0F}, {-3.5F, -2.5F, -7.0F / 6.0F}, {-3.5F, -1.75F, -10.0F / 6.0F}}};
  const std::array<std::array<float32, 3>, 3> descendingNormals = {{{5.0F, 0.75F, 0.5F}, {1.5F, 2.5F, 0.5F}, {1.5F, 0.75F, 10.0F / 6.0F}}};
  const auto& expectedVertices = inverted ? descendingVertices : ascendingVertices;
  const auto& expectedNormals = inverted ? descendingNormals : ascendingNormals;
  std::array<bool, 3> seen = {};
  for(usize vertex = 0; vertex < 3; ++vertex)
  {
    const usize axis = vertices[3 * vertex] > 1.0F ? 0 : (vertices[3 * vertex + 1] > 2.0F ? 1 : 2);
    REQUIRE_FALSE(seen[axis]);
    seen[axis] = true;
    for(usize component = 0; component < 3; ++component)
    {
      CHECK(vertices[3 * vertex + component] == Approx(expectedVertices[axis][component]).margin(1.0e-6));
      CHECK(normals[3 * vertex + component] == Approx(expectedNormals[axis][component]).margin(1.0e-6));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("SimplnxCore::FlyingEdges3D 64-bit integer edge interpolation", "[SimplnxCore][FlyingEdges3D][L17]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  struct EdgeCase
  {
    TestType corner0;
    TestType others;
    float64 isoVal;
    TestType isoAsT;
    std::array<std::array<float32, 3>, 3> vertices;
  };
  // The filter receives the Contour Value as float64 and converts it with static_cast<T>.
  // Every isoVal below is exactly representable in float64 AND in T, so the cast is exact
  // (checked with isoAsT). Corner (0,0,0) holds corner0 and the other seven corners hold
  // others, so only the three edges incident on corner (0,0,0) are cut, each with
  // t = (iso - corner0) / (others - corner0). Vertices are (1 + 2t, 2, 3), (1, 2 + 4t, 3) and
  // (1, 2, 3 + 6t). Cases 0/1: the endpoints and the iso value all round to the same float64
  // (2^54), so subtracting after converting to float64 gives 0/0. Cases 2/3: the full type
  // range, where subtracting in T overflows or wraps. Cases 4/5: values next to the type limit
  // that again round to one float64. Even cases ascend, odd cases descend.
  const std::array<std::array<float32, 3>, 3> half = {{{2.0F, 2.0F, 3.0F}, {1.0F, 4.0F, 3.0F}, {1.0F, 2.0F, 6.0F}}};
  const std::array<std::array<float32, 3>, 3> quarter = {{{1.5F, 2.0F, 3.0F}, {1.0F, 3.0F, 3.0F}, {1.0F, 2.0F, 4.5F}}};
  const std::array<std::array<float32, 3>, 3> threeQuarters = {{{2.5F, 2.0F, 3.0F}, {1.0F, 5.0F, 3.0F}, {1.0F, 2.0F, 7.5F}}};
  constexpr TestType k_Min = std::numeric_limits<TestType>::min();
  constexpr TestType k_Max = std::numeric_limits<TestType>::max();
  std::array<EdgeCase, 6> cases = {};
  // 2^54 - 1 < 2^54 < 2^54 + 1: t = 1 / 2 both ways.
  cases[0] = {static_cast<TestType>(18014398509481983ULL), static_cast<TestType>(18014398509481985ULL), 18014398509481984.0, static_cast<TestType>(18014398509481984ULL), half};
  cases[1] = {static_cast<TestType>(18014398509481985ULL), static_cast<TestType>(18014398509481983ULL), 18014398509481984.0, static_cast<TestType>(18014398509481984ULL), half};
  if constexpr(std::is_signed_v<TestType>)
  {
    // iso = 2^62. Ascending: t = (2^62 + 2^63) / (2^64 - 1) = 0.75 + 4.1e-20.
    // Descending: t = (2^62 - (2^63 - 1)) / (-2^63 - (2^63 - 1)) = (2^62 - 1) / (2^64 - 1) = 0.25 - 4.1e-20.
    cases[2] = {k_Min, k_Max, 4611686018427387904.0, static_cast<TestType>(4611686018427387904LL), threeQuarters};
    cases[3] = {k_Max, k_Min, 4611686018427387904.0, static_cast<TestType>(4611686018427387904LL), quarter};
    // iso = -2^63 + 1024, endpoints -2^63 + 1023 and -2^63 + 1025: t = 1 / 2.
    cases[4] = {k_Min + 1023, k_Min + 1025, -9223372036854774784.0, k_Min + 1024, half};
    cases[5] = {k_Min + 1025, k_Min + 1023, -9223372036854774784.0, k_Min + 1024, half};
  }
  else
  {
    // iso = 2^62. Ascending: t = 2^62 / (2^64 - 1) = 0.25 + 1.4e-20.
    // Descending: t = (2^62 - (2^64 - 1)) / (0 - (2^64 - 1)) = (3 * 2^62 - 1) / (2^64 - 1) = 0.75 - 1.4e-20.
    cases[2] = {k_Min, k_Max, 4611686018427387904.0, static_cast<TestType>(4611686018427387904ULL), quarter};
    cases[3] = {k_Max, k_Min, 4611686018427387904.0, static_cast<TestType>(4611686018427387904ULL), threeQuarters};
    // iso = 2^64 - 2048, endpoints 2^64 - 2049 and 2^64 - 2047: t = 1 / 2.
    cases[4] = {k_Max - 2048, k_Max - 2046, 18446744073709549568.0, k_Max - 2047, half};
    cases[5] = {k_Max - 2046, k_Max - 2048, 18446744073709549568.0, k_Max - 2047, half};
  }
  const usize caseIndex = GENERATE(range(usize{0}, usize{6}));
  CAPTURE(caseIndex);
  const EdgeCase& edgeCase = cases[caseIndex];
  REQUIRE(static_cast<TestType>(edgeCase.isoVal) == edgeCase.isoAsT);
  REQUIRE(static_cast<float64>(edgeCase.isoAsT) == edgeCase.isoVal);

  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  auto* image = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 2, 2});
  image->setOrigin({1.0F, 2.0F, 3.0F});
  image->setSpacing({2.0F, 4.0F, 6.0F});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{2, 2, 2}, image->getId());
  REQUIRE(cells != nullptr);
  image->setCellData(*cells);
  auto* values = DataArray<TestType>::Create(dataStructure, "Values", std::make_shared<DataStore<TestType>>(cells->getShape(), ShapeType{1}, 0), cells->getId());
  REQUIRE(values != nullptr);
  values->fill(edgeCase.others);
  (*values)[0] = edgeCase.corner0;

  FlyingEdges3DFilter filter;
  Arguments args;
  args.insertOrAssign(FlyingEdges3DFilter::k_IsoVal_Key, std::make_any<float64>(edgeCase.isoVal));
  args.insertOrAssign(FlyingEdges3DFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FlyingEdges3DFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(DataPath({"Image", "Cell Data", "Values"})));
  args.insertOrAssign(FlyingEdges3DFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({"Contour"})));
  auto result = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath contourPath({"Contour"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(contourPath));
  const auto& contour = dataStructure.getDataRefAs<TriangleGeom>(contourPath);
  REQUIRE(contour.getNumberOfVertices() == 3);
  REQUIRE(contour.getNumberOfFaces() == 1);
  const auto& vertices = contour.getVerticesRef();
  const DataPath normalsPath = contourPath.createChildPath(INodeGeometry0D::k_VertexAttributeMatrixName).createChildPath(ContourTest::k_VertexNormals);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(normalsPath));
  const auto& normals = dataStructure.getDataRefAs<Float32Array>(normalsPath);
  std::array<bool, 3> seen = {};
  for(usize vertex = 0; vertex < 3; ++vertex)
  {
    const float32 x = vertices[3 * vertex];
    const float32 y = vertices[3 * vertex + 1];
    const float32 z = vertices[3 * vertex + 2];
    CAPTURE(vertex, x, y, z);
    REQUIRE(std::isfinite(x));
    REQUIRE(std::isfinite(y));
    REQUIRE(std::isfinite(z));
    for(usize component = 0; component < 3; ++component)
    {
      REQUIRE(std::isfinite(normals[3 * vertex + component]));
    }
    // The nearest wrong answer (a snapped vertex) is 0.5 away; NaN already failed above.
    const usize axis = x > 1.0F ? 0 : (y > 2.0F ? 1 : 2);
    REQUIRE_FALSE(seen[axis]);
    seen[axis] = true;
    CHECK(x == Approx(edgeCase.vertices[axis][0]).epsilon(0.0).margin(1.0e-5));
    CHECK(y == Approx(edgeCase.vertices[axis][1]).epsilon(0.0).margin(1.0e-5));
    CHECK(z == Approx(edgeCase.vertices[axis][2]).epsilon(0.0).margin(1.0e-5));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
