
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <memory>
#include <optional>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
/*
 * The non-square fixture detects an incorrect row stride in each Empty2D dispatch.
 * Its 3 by 4 mask has one four-voxel component and one two-voxel component.
 * The filter must retain only the larger component.
 * Rows 0 and 1 are `T T F`. Rows 2 and 3 are `F F T`.
 */
const DataPath k_NonSquareImagePath = DataPath({"Image"});
const DataPath k_NonSquareMaskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");

/**
 * @brief Builds a non-square two-dimensional mask for row-stride tests.
 * @param dims Image dimensions with exactly one axis of size 1.
 * @param useConfiguredStore True to create the mask with the configured store factory.
 * @return A DataStructure with the 3 by 4 component mask.
 */
DataStructure CreateNonSquare2DMaskDataStructure(const SizeVec3& dims, bool useConfiguredStore = false)
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_NonSquareImagePath.getTargetName());
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 1.0f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(dims);

  const ShapeType imageShape{dims[0], dims[1], dims[2]};
  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, "CellData", imageShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  std::shared_ptr<AbstractDataStore<bool>> maskStore;
  if(useConfiguredStore)
  {
    maskStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_NonSquareMaskPath, cellData->getShape(), ShapeType{1}, IDataAction::Mode::Execute);
  }
  else
  {
    maskStore = std::make_shared<BoolDataStore>(cellData->getShape(), ShapeType{1}, std::optional<bool>{});
  }
  BoolArray* mask = BoolArray::Create(dataStructure, "Mask", maskStore, cellData->getId());

  const std::array<bool, 12> values = {true, true, false, true, true, false, false, false, true, false, false, true};
  REQUIRE(mask->getNumberOfTuples() == values.size());
  for(usize i = 0; i < values.size(); i++)
  {
    mask->setValue(i, values[i]);
  }
  return dataStructure;
}

/**
 * @brief Creates IdentifySample arguments for the non-square mask fixture.
 * @return Configured whole-volume arguments without hole filling.
 */
Arguments CreateNonSquareArguments()
{
  Arguments args;
  args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_NonSquareImagePath));
  args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_NonSquareMaskPath));
  args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  return args;
}

/**
 * @brief Executes IdentifySample and verifies that only the larger component remains.
 * @param dataStructure Contains the non-square mask to update.
 */
void RunIdentifySampleAndCheck(DataStructure& dataStructure)
{
  IdentifySampleFilter filter;
  Arguments args = CreateNonSquareArguments();

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::array<bool, 12> expected = {true, true, false, true, true, false, false, false, false, false, false, false};
  const DataPath maskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(maskPath);
  REQUIRE(mask.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO("linear index " << i);
    REQUIRE(mask[i] == expected[i]);
  }
}

const DataPath k_ExemplarArrayPath = Constants::k_DataContainerPath.createChildPath(Constants::k_CellData).createChildPath("Mask Exemplar");
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "identify_sample_v2.tar.gz", "identify_sample_v2");
  using TestArgType = std::tuple<std::string, std::string, std::string>;
  /* clang-format off */
  std::vector<TestArgType> allTestParams = {
    {"sliced", "xy", "fill"},
    {"sliced", "xy", "nofill"},
    {"sliced", "xz", "fill"},
    {"sliced", "xz", "nofill"},
    {"sliced", "yz", "fill"},
    {"sliced", "yz", "nofill"},

    {"whole", "xy", "fill"},
    {"whole", "xy", "nofill"},
    {"whole", "xz", "fill"},
    {"whole", "xz", "nofill"},
    {"whole", "yz", "fill"},
    {"whole", "yz", "nofill"},
  };
  /* clang-format on */
  for(const auto& testParam : allTestParams)
  {
    std::string slice_by_slice = std::get<0>(testParam);
    bool sliceBySlice = slice_by_slice == "sliced";

    std::string slice_plane = std::get<1>(testParam);

    ChoicesParameter::ValueType sliceBySlicePlane = 0;
    if(slice_plane == "xz")
      sliceBySlicePlane = 1;
    else if(slice_plane == "yz")
      sliceBySlicePlane = 2;

    std::string fill_holes = std::get<2>(testParam);
    bool fillHoles = fill_holes == "fill";

    SECTION(fmt::format("{}_{}_{}", slice_by_slice, slice_plane, fill_holes))
    {
      fs::path inputFilePath = fs::path(fmt::format("{}/identify_sample_v2/{}_{}_{}.dream3d", unit_test::k_TestFilesDir, slice_by_slice, slice_plane, fill_holes));

      DataStructure dataStructure = LoadDataStructure(inputFilePath);
      IdentifySampleFilter filter;
      Arguments args;
      args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
      args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
      args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
      args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
      args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(sliceBySlicePlane));

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      const IDataArray& computedArray = dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath);
      const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(k_ExemplarArrayPath);
      CompareDataArrays<uint8>(computedArray, exemplarArray);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][IdentifySampleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "IdentifySampleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "IdentifySampleFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<IdentifySampleFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(IdentifySampleFilter::k_FillHoles_Key) == true);
      CHECK(args.value<DataPath>(IdentifySampleFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(IdentifySampleFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

// These cases rotate the 3 by 4 fixture through each Empty2D dispatch.
// An incorrect flood-fill stride merges the components or accesses outside the mask.
TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 4, 1});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 1, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{1, 3, 4});
  scope.execute([&] { ::RunIdentifySampleAndCheck(dataStructure); });
}
