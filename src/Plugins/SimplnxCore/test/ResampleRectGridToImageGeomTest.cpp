#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/ResampleRectGridToImageGeomFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_RectGridGeomPath({"RectGridGeom"});
const DataPath k_RectGridCellDataPath = k_RectGridGeomPath.createChildPath(k_CellData);
const DataPath k_RectGridCellFeatureArrayPath = k_RectGridGeomPath.createChildPath(k_CellFeatureData).createChildPath(k_AvgQuats);

const DataPath k_ImageGeomComputed({"ImageGeom [Computed]"});

const std::string k_NeighborListArrayName = "NeighborList";
const std::string k_StringArrayName = "StringArray";
} // namespace

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: Valid Filter Execution", "[SimplnxCore][ResampleRectGridToImageGeomFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz", "6_6_resample_rect_grid_to_image_geom.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create a neighbor list and string array for testing selected arrays with all IArray types
  {
    const auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(k_RectGridCellDataPath);
    const usize numTuples = cellDataAM.getNumberOfTuples();

    std::vector<std::string> stringArrayValues(numTuples);
    auto* neighborList = NeighborList<float32>::Create(dataStructure, k_NeighborListArrayName, cellDataAM.getShape(), cellDataAM.getId());
    for(usize i = 0; i < numTuples; ++i)
    {
      const float32 factor = static_cast<float32>(i) * 0.001;
      std::vector<float32> list = {factor * 117, factor * 875, factor * 1035, factor * 3905, factor * 4214};
      neighborList->setList(i, std::make_shared<std::vector<float32>>(list));
      stringArrayValues[i] = "String_" + StringUtilities::number(factor);
    }

    const auto* stringArray = StringArray::CreateWithValues(dataStructure, k_StringArrayName, cellDataAM.getShape(), stringArrayValues, cellDataAM.getId());
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                          k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds),
                          k_RectGridCellDataPath.createChildPath(k_NeighborListArrayName), k_RectGridCellDataPath.createChildPath(k_StringArrayName)}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));

  std::string exemplarImageGeomName;

  SECTION("Resample Smaller")
  {
    exemplarImageGeomName = "ImageGeom [Smaller]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Resample Bigger")
  {
    exemplarImageGeomName = "ImageGeom [Bigger]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 20)));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_ImageGeomComputed.createChildPath(k_CellData), exemplarImageGeomName);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: InValid Filter Execution")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz", "6_6_resample_rect_grid_to_image_geom.dream3d");
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));

  SECTION("Missing Selected Arrays")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Mismatching Selected Array Tuples")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(
                            MultiArraySelectionParameter::ValueType{k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles),
                                                                    k_RectGridCellDataPath.createChildPath(k_FeatureIds), k_RectGridCellFeatureArrayPath}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Invalid Cell Level Selected Array")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_RectGridCellFeatureArrayPath}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Invalid Dimension Value")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                            k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds)}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{5, 5, -1}));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ResampleRectGridToImageGeomFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ResampleRectGridToImageGeomFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ResampleRectGridToImageGeomFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key) == DataPath({"DataContainer"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key) == DataPath({"ImageGeometry"}));
      CHECK(args.value<std::string>(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key) == "CellData");
    }
  }
}

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"

namespace
{
void CreateCountRectGrid(DataStructure& ds)
{
  auto result = CreateRectGridGeometryAction(DataPath({"Rect"}), 2, 2, 2, "Cell Data", "X", "Y", "Z").apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<RectGridGeom>(DataPath({"Rect"})));
  auto& geom = ds.getDataRefAs<RectGridGeom>(DataPath({"Rect"}));
  geom.getXBounds()->setValue(0, 0);
  geom.getXBounds()->setValue(1, 2);
  geom.getYBounds()->setValue(0, 0);
  geom.getYBounds()->setValue(1, 3);
  geom.getZBounds()->setValue(0, 0);
  geom.getZBounds()->setValue(1, 4);
  result = CreateArrayAction(DataType::int32, {1, 1, 1}, {1}, DataPath({"Rect", "Cell Data", "Marker"})).apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Rect", "Cell Data", "Marker"})));
  ds.getDataRefAs<Int32Array>(DataPath({"Rect", "Cell Data", "Marker"}))[0] = 7;
}
Arguments CountRectGridArguments()
{
  ResampleRectGridToImageGeomFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, DataPath({"Rect"}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, DataPath({"Resampled"}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrixName_Key, std::string("Cell Data"));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key, std::vector<DataPath>{DataPath({"Rect", "Cell Data", "Marker"})});
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: Requested dimension domains", "[SimplnxCore][ResampleRectGridToImageGeomFilter][CountParameterDomains]")
{

  DataStructure ds;
  CreateCountRectGrid(ds);
  ResampleRectGridToImageGeomFilter filter;
  auto args = CountRectGridArguments();
  int32 expectedCode = 0;
  std::string component;
  SECTION("Zero component")
  {
    const auto axis = GENERATE(0, 1, 2);
    CAPTURE(axis);
    component = std::string(1, "XYZ"[axis]);
    std::vector<int32> dims = {1, 1, 1};
    dims[axis] = 0;
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, dims);
    expectedCode = -86156 - axis;
  }
  SECTION("Product overflow")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::vector<int32>(3, 1 << 22));
    expectedCode = -86174;
  }

  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().size() == 1);
  CHECK(result.outputActions.errors()[0].code == expectedCode);
  const auto& diagnostic = result.outputActions.errors()[0].message;
  CHECK(diagnostic.find("Dimensions") != std::string::npos);
  if(!component.empty())
  {
    CHECK(diagnostic.find(component) != std::string::npos);
    CHECK(diagnostic.find("value 0") != std::string::npos);
  }
  else
  {
    CHECK(diagnostic.find("product") != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::ResampleRectGridToImageGeomFilter: Rectilinear count boundaries", "[SimplnxCore][GeometryCountControls]")
{

  DataStructure ds;
  CreateCountRectGrid(ds);
  ResampleRectGridToImageGeomFilter filter;
  auto args = CountRectGridArguments();
  SECTION("Negative components")
  {
    const auto axis = GENERATE(0, 1, 2);
    CAPTURE(axis);
    std::vector<int32> dims = {1, 1, 1};
    dims[axis] = -1;
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, dims);
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    CHECK(result.outputActions.errors()[0].code == -7362 - axis);
  }
  SECTION("Active vector length")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::vector<int32>{1, 1});
    auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    CHECK(result.outputActions.errors()[0].code == -1);
    CHECK(filter.parameters().at(ResampleRectGridToImageGeomFilter::k_Dimensions_Key)->humanName() == "Dimensions (Voxels)");
    CHECK(result.outputActions.errors()[0].message == "VectorParameter requires size 3. Received size 2.");
  }
  SECTION("Ordinary sampling")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::vector<int32>{1, 1, 1});
    auto result = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"})));
    const auto& geom = ds.getDataRefAs<ImageGeom>(DataPath({"Resampled"}));
    CHECK(geom.getDimensions() == SizeVec3{1, 1, 1});
    CHECK(geom.getSpacing() == FloatVec3{2, 3, 4});
    CHECK(geom.getOrigin() == FloatVec3{0, 0, 0});
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"})));
    CHECK(ds.getDataRefAs<Int32Array>(DataPath({"Resampled", "Cell Data", "Marker"}))[0] == 7);
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
}
