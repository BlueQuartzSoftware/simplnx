#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberBounds.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateAMScanPaths.hpp"
#include "SimplnxCore/Filters/CreateAMScanPathsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <cmath>
#include <limits>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const nx::core::DataPath k_ExemplarEdgeGeometryPath = DataPath({"Exemplar Slice Geometry"});
const nx::core::DataPath k_ExemplarScanVectorsPath = DataPath({"Exemplar Scan Paths Geometry"});
const nx::core::DataPath k_RegionIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Part Number"});
const nx::core::DataPath k_SliceIdsPath = DataPath({"Exemplar Slice Geometry", "Edge Data", "Slice Ids"});

const nx::core::DataPath k_ComputedScanVectorsPath = DataPath({"Output Scan Vectors"});

const DataObjectNameParameter::ValueType k_EdgeData("Edge Data");
// const DataObjectNameParameter::ValueType k_VertexData("VertexData");
const DataObjectNameParameter::ValueType k_RegionIdsName("RegionIds");
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Valid Filter Execution", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  UnitTest::LoadPlugins();

  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_0_SurfaceMesh_Test_Files_v2.tar.gz", "7_0_SurfaceMesh_Test_Files");
  auto baseDataFilePath = fs::path(fmt::format("{}/7_0_SurfaceMesh_Test_Files/7_0_SurfaceMesh_Test_Files.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateAMScanPathsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, std::make_any<float32>(0.14f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_StripeWidth_Key, std::make_any<float32>(7.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, std::make_any<float32>(67.0f));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, std::make_any<DataPath>(k_ExemplarEdgeGeometryPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, std::make_any<DataPath>(k_SliceIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, std::make_any<DataPath>(k_RegionIdsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, std::make_any<DataPath>(k_ComputedScanVectorsPath));
  args.insertOrAssign(CreateAMScanPathsFilter::k_VertexAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_VertexData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_EdgeData));
  args.insertOrAssign(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_RegionIdsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result)

// Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  std::cout << "Writing File: " << fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir) << "\n";
  WriteTestDataStructure(dataStructure, fmt::format("{}/create_am_scan_paths_test.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  // Compare the exemplar and the computed outputs
  {
    auto exemplarGeom = dataStructure.getDataAs<IGeometry>(k_ExemplarScanVectorsPath);
    auto computedGeom = dataStructure.getDataAs<IGeometry>(k_ComputedScanVectorsPath);
    REQUIRE(UnitTest::CompareIGeometry(exemplarGeom, computedGeom));
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath(k_SliceIdsPath.getTargetName());
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_SliceIdsPath.getTargetName());
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  {
    DataPath exemplarDataArray = k_ExemplarScanVectorsPath.createChildPath("EdgeData").createChildPath("RegionIds");
    DataPath computedDataArray = k_ComputedScanVectorsPath.createChildPath(k_EdgeData).createChildPath(k_RegionIdsName);
    UnitTest::CompareArrays<int32>(dataStructure, exemplarDataArray, computedDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateAMScanPathsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateAMScanPathsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateAMScanPathsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<float32>(CreateAMScanPathsFilter::k_StripeWidth_Key) == Approx(2.5f));
      CHECK(args.value<float32>(CreateAMScanPathsFilter::k_HatchSpacing_Key) == Approx(2.5f));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "SliceIds"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "RegionIds"}));
      CHECK(args.value<DataPath>(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key) == DataPath({"HatchDataContainer"}));
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_VertexAttributeMatrixName_Key) == "VertexData");
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_HatchAttributeMatrixName_Key) == "HatchData");
      CHECK(args.value<std::string>(CreateAMScanPathsFilter::k_RegionIdsArrayName_Key) == "RegionIds");
    }
  }
}

namespace
{
void CreateSamplingEdges(DataStructure& ds, bool squares = false)
{
  auto result = CreateEdgeGeometryAction(DataPath({"Edges"}), squares ? 8 : 1, squares ? 8 : 2, "Vertex Data", "Edge Data", "Vertices", "Edges").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"})));
  auto& geom = ds.getDataRefAs<EdgeGeom>(DataPath({"Edges"}));
  auto& vertices = geom.getVerticesRef();
  auto& edges = geom.getEdgesRef();
  if(squares)
  {
    const std::array<float32, 24> coords = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};
    auto write = vertices.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
    SIMPLNX_RESULT_REQUIRE_VALID(write);
    for(usize i = 0; i < 8; ++i)
    {
      edges[2 * i] = i;
      edges[2 * i + 1] = (i / 4) * 4 + (i + 1) % 4;
    }
    for(const auto& name : {"Layers", "Regions"})
    {
      auto array = CreateArrayAction(DataType::int32, {8}, {1}, DataPath({"Edges", "Edge Data", name})).apply(ds, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(array);
      REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Edges", "Edge Data", name})));
      auto& values = ds.getDataRefAs<Int32Array>(DataPath({"Edges", "Edge Data", name}));
      for(usize i = 0; i < 8; ++i)
      {
        values[i] = std::string(name) == "Layers" ? static_cast<int32>(i / 4) : 0;
      }
    }
  }
  else
  {
    const std::array<float32, 6> coords = {0, 0, 0, 2, 0, 0};
    auto write = vertices.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
    SIMPLNX_RESULT_REQUIRE_VALID(write);
    edges[0] = 0;
    edges[1] = 1;
  }
}

Arguments MakeSamplingArgs(const CreateAMScanPathsFilter& filter)
{
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, DataPath({"Edges"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Layers"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Regions"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, DataPath({"Hatches"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, 0.5F);
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, 0.0F);
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Spatial parameter domains", "[SimplnxCore][CreateAMScanPathsFilter][SpatialParameterDomains]")
{
  DataStructure ds;
  CreateSamplingEdges(ds, true);
  CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  int32 expectedCode;
  std::string parameter;
  std::string actual;
  SECTION("Hatch Spacing")
  {
    auto value = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
    CAPTURE(value);
    actual = fmt::format("{}", value);
    args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, value);
    expectedCode = std::isfinite(value) ? k_NumberBelowLowerBound : k_NumberNotFinite;
    parameter = "Hatch Spacing";
  }
  SECTION("Hatch Rotation Angle")
  {
    auto value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
    CAPTURE(value);
    actual = fmt::format("{}", value);
    args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, value);
    expectedCode = k_NumberNotFinite;
    parameter = "Hatch Rotation Angle (Degrees)";
  }

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  CHECK(result.outputActions.errors()[0].message.find(parameter) != std::string::npos);
  CHECK(result.outputActions.errors()[0].message.find(actual) != std::string::npos);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Finite hatch controls", "[SimplnxCore][CreateAMScanPathsFilter][SpatialSamplingControls]")
{
  DataStructure ds;
  CreateSamplingEdges(ds, true);
  CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  const auto angle = GENERATE(0.0F, -90.0F);
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, angle);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(DataPath({"Hatches"})));
  const auto& output = ds.getDataRefAs<EdgeGeom>(DataPath({"Hatches"}));
  REQUIRE(output.getNumberOfEdges() > 0);
  const auto& vertices = output.getVerticesRef();
  const auto& edges = output.getEdgesRef();
  for(usize edge = 0; edge < output.getNumberOfEdges(); ++edge)
  {
    for(usize end = 0; end < 2; ++end)
    {
      const auto vertex = edges[2 * edge + end];
      const auto x = vertices[3 * vertex];
      const auto y = vertices[3 * vertex + 1];
      const auto z = vertices[3 * vertex + 2];
      CHECK(x >= -1e-6F);
      CHECK(x <= 1 + 1e-6F);
      CHECK(y >= -1e-6F);
      CHECK(y <= 1 + 1e-6F);
      CHECK((z == 0 || z == 1));
      CHECK((std::abs(x) < 1e-6F || std::abs(x - 1) < 1e-6F || std::abs(y) < 1e-6F || std::abs(y - 1) < 1e-6F));
    }
    if(angle == 0)
    {
      CHECK(vertices[3 * edges[2 * edge] + 1] == Approx(vertices[3 * edges[2 * edge + 1] + 1]));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Tiny hatch spacing stops execution", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  constexpr float32 k_MinY = 1000.0F;
  const auto row = GENERATE(0, 1, 2);
  const float32 spacing = row == 2 ? 1e-4F : 1e-5F;
  const float32 height = row == 0 ? 1.0F : row == 1 ? 16.0F : 200.0F;
  CHECK((k_MinY + spacing == k_MinY) == (row < 2));

  DataStructure ds;
  CreateSamplingEdges(ds, true);
  const DataPath inputPath({"Edges"});
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(inputPath));
  auto& vertices = ds.getDataRefAs<EdgeGeom>(inputPath).getVerticesRef();
  for(usize vertex = 0; vertex < vertices.getNumberOfTuples(); ++vertex)
  {
    const usize yIndex = 3 * vertex + 1;
    vertices[yIndex] = k_MinY + height * vertices[yIndex];
  }

  CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, spacing);
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  CHECK(result.result.errors()[0].code == (row == 0 ? -86227 : -86210));
  CHECK(result.result.errors()[0].message.find("Increase Hatch Spacing") != std::string::npos);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Rounded hatch count is within the line limit", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  constexpr float32 k_HatchSpacing = 9e-5F;
  constexpr float32 k_MinY = 1024.0F;
  constexpr float32 k_MaxY = 1129.0F;
  // All coordinates are in the same float32 exponent range. The original
  // recurrence advances one float32 step, even though the requested step is smaller.
  constexpr float64 k_RoundedStep = 1.0 / 8192.0;
  REQUIRE((k_MaxY - k_MinY) / k_RoundedStep < 1'000'000);
  REQUIRE((k_MaxY - k_MinY) / k_HatchSpacing > 1'000'000);

  DataStructure ds;
  CreateSamplingEdges(ds);
  const DataPath inputPath({"Edges"});
  for(const auto& name : {"Layers", "Regions"})
  {
    auto action = CreateArrayAction(DataType::int32, {1}, {1}, inputPath.createChildPath("Edge Data").createChildPath(name)).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(action);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(inputPath));
  auto& vertices = ds.getDataRefAs<EdgeGeom>(inputPath).getVerticesRef();
  vertices[1] = k_MinY;
  vertices[4] = k_MaxY;

  CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, k_HatchSpacing);
  const auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* hatches = ds.getDataAs<EdgeGeom>(DataPath({"Hatches"}));
  REQUIRE(hatches != nullptr);
  // One isolated edge gives one intersection. It gives no hatch segment.
  CHECK(hatches->getNumberOfEdges() == 0);
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Hatch position narrowing", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  const bool isStart = GENERATE(false, true);
  CAPTURE(isStart);
  const float64 max = std::numeric_limits<float32>::max();
  const float32 inside = std::nextafter(std::numeric_limits<float32>::max(), 0.0F);
  const int32 code = isStart ? -86249 : -86250;
  const DataPath inputPath({"Edges"});
  const auto check = [&](float64 y) { return CreateAMScanPathsDetail::CheckedHatchY(y, isStart, inputPath); };
  for(const auto position : {static_cast<float64>(inside), -static_cast<float64>(inside)})
  {
    const auto result = check(position);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.value() == (position < 0 ? -inside : inside));
  }
  for(const float64 position : {std::nextafter(max, std::numeric_limits<float64>::infinity()), std::nextafter(-max, -std::numeric_limits<float64>::infinity()),
                                std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::quiet_NaN()})
  {
    const auto result = check(position);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == code);
    CHECK(result.errors()[0].message.find(isStart ? "first hatch line" : "hatch line position") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Intersection X ordering laws", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  constexpr auto k_Nan = std::numeric_limits<float32>::quiet_NaN();
  constexpr auto k_Inf = std::numeric_limits<float32>::infinity();
  const std::array<float32, 8> ordered = {-k_Inf, -3, -0.0F, 0.0F, 2, k_Inf, k_Nan, -k_Nan};
  const std::array<int, 8> rank = {0, 1, 2, 2, 3, 4, 5, 5};
  const auto less = CreateAMScanPathsDetail::IntersectionXLess;
  for(usize a = 0; a < ordered.size(); ++a)
  {
    CHECK_FALSE(less(ordered[a], ordered[a]));
    for(usize b = 0; b < ordered.size(); ++b)
    {
      CHECK(less(ordered[a], ordered[b]) == (rank[a] < rank[b]));
      if(less(ordered[a], ordered[b]))
      {
        CHECK_FALSE(less(ordered[b], ordered[a]));
        for(const auto c : ordered)
        {
          if(less(ordered[b], c))
          {
            CHECK(less(ordered[a], c));
          }
        }
      }
    }
  }
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Mixed intersection X coordinates", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  DataStructure ds;
  const DataPath inputPath({"Edges"});
  const auto geometry = CreateEdgeGeometryAction(inputPath, 4, 8, "Vertex Data", "Edge Data", "Vertices", "Edges").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(geometry);
  for(const auto name : {"Layers", "Regions"})
  {
    const auto path = inputPath.createChildPath("Edge Data").createChildPath(name);
    const auto array = CreateArrayAction(DataType::int32, {4}, {1}, path).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(array);
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(path));
    ds.getDataRefAs<Int32Array>(path).fill(0);
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(inputPath));
  auto& geom = ds.getDataRefAs<EdgeGeom>(inputPath);
  // The only hatch is at Y=0. The first edge gives 0 * infinity in X.
  const std::array<float32, 24> coords = {-2e38F, -2e38F, 0, 2e38F, 2e38F, 0, 3, -1, 0, 3, 1, 0, 1, -1, 0, 1, 1, 0, 2, -1, 0, 2, 1, 0};
  const auto write = geom.getVerticesRef().getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(coords));
  SIMPLNX_RESULT_REQUIRE_VALID(write);
  for(usize i = 0; i < 8; ++i)
  {
    geom.getEdgesRef()[i] = i;
  }

  const CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, 3e38F);
  const auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto* hatches = ds.getDataAs<EdgeGeom>(DataPath({"Hatches"}));
  REQUIRE(hatches != nullptr);
  REQUIRE(hatches->getNumberOfEdges() == 2);
  REQUIRE(hatches->getNumberOfVertices() == 4);
  const auto& points = hatches->getVerticesRef();
  CHECK(points[0] == 1);
  CHECK(points[3] == 2);
  CHECK(points[6] == 3);
  CHECK(std::isnan(points[9]));
}

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: First hatch in the coordinate range", "[SimplnxCore][CreateAMScanPathsFilter]")
{
  const bool overflow = GENERATE(false, true);
  CAPTURE(overflow);
  DataStructure ds;
  CreateSamplingEdges(ds, true);
  const DataPath inputPath({"Edges"});
  REQUIRE_NOTHROW(ds.getDataRefAs<EdgeGeom>(inputPath));
  auto& geom = ds.getDataRefAs<EdgeGeom>(inputPath);
  const float32 limit = std::numeric_limits<float32>::max();
  const float32 nearLimit = std::nextafter(limit, 0.0F);
  for(usize vertex = 4; vertex < 8; ++vertex)
  {
    geom.setVertexCoordinate(vertex, {overflow ? limit : static_cast<float32>(vertex), overflow ? -limit : -nearLimit, 1});
  }

  const CreateAMScanPathsFilter filter;
  auto args = MakeSamplingArgs(filter);
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, nearLimit);
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, overflow ? 45.0F : 0.0F);
  const auto result = filter.execute(ds, args);
  if(overflow)
  {
    // The second slice rotates by 45 degrees. Its Y coordinate is below the float32 range.
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    CHECK(result.result.errors()[0].code == -86249);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  }
}
