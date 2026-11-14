#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/CreateAMScanPathsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

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

#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include <cmath>
#include <limits>

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
} // namespace

TEST_CASE("SimplnxCore::CreateAMScanPathsFilter: Spatial parameter domains", "[SimplnxCore][CreateAMScanPathsFilter][SpatialParameterDomains]")
{

  DataStructure ds;
  CreateSamplingEdges(ds, true);
  CreateAMScanPathsFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, DataPath({"Edges"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Layers"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Regions"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, DataPath({"Hatches"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, 0.5F);
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, 0.0F);
  int32 expectedCode;
  std::string parameter;
  std::string actual;
  SECTION("Hatch Spacing")
  {
    auto value = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
    CAPTURE(value);
    actual = fmt::format("{}", value);
    args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, value);
    expectedCode = -86100;
    parameter = "Hatch Spacing";
  }
  SECTION("Hatch Rotation Angle")
  {
    auto value = GENERATE(std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
    CAPTURE(value);
    actual = fmt::format("{}", value);
    args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, value);
    expectedCode = -86101;
    parameter = "Hatch Rotation Angle";
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
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceDataContainerPath_Key, DataPath({"Edges"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADSliceIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Layers"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_CADRegionIdsArrayPath_Key, DataPath({"Edges", "Edge Data", "Regions"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchDataContainerPath_Key, DataPath({"Hatches"}));
  args.insertOrAssign(CreateAMScanPathsFilter::k_HatchSpacing_Key, 0.5F);
  args.insertOrAssign(CreateAMScanPathsFilter::k_RotationAngle, 0.0F);
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
