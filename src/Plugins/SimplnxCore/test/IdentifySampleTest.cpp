
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
const DataPath k_ExemplarArrayPath = Constants::k_DataContainerPath.createChildPath(Constants::k_CellData).createChildPath("Mask Exemplar");

// -----------------------------------------------------------------------------
// Hand-built non-square 2D fixture. Builds a 3x4 mask (shaped per the empty
// axis) with a 4-voxel top-left connected component and a 2-voxel bottom-right
// connected component. IdentifySample should keep the larger (4-voxel) region
// and drop the smaller one. A wrong row-stride in the Empty2D dispatches would
// either merge the two components or step outside the buffer.
//
// Mask (T = true = good, F = false = bad), oriented in the two non-empty axes:
//   row 0: T T F
//   row 1: T T F
//   row 2: F F T
//   row 3: F F T
// -----------------------------------------------------------------------------
const DataPath k_NonSquareImagePath = DataPath({"Image"});
const DataPath k_NonSquareMaskPath = k_NonSquareImagePath.createChildPath("CellData").createChildPath("Mask");

DataStructure CreateNonSquare2DMaskDataStructure(const SizeVec3& dims)
{
  DataStructure dataStructure = {};
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_NonSquareImagePath.getTargetName());
  imageGeom->setSpacing(FloatVec3{std::array<float32, 3>{1.0f, 1.0f, 1.0f}});
  imageGeom->setOrigin(FloatVec3{std::array<float32, 3>{0.0f, 0.0f, 0.0f}});
  imageGeom->setDimensions(dims);

  const ShapeType imageShape{dims[0], dims[1], dims[2]};
  AttributeMatrix* cellData = AttributeMatrix::Create(dataStructure, "CellData", imageShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  BoolArray* mask = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, "Mask", cellData->getShape(), ShapeType{1}, cellData->getId());

  // The mask pattern in logical (row, col) form:
  //   row 0: T T F
  //   row 1: T T F
  //   row 2: F F T
  //   row 3: F F T
  // Each dimensionality state maps (row, col) -> linear index using row = first
  // non-empty axis and col = second non-empty axis. Since all three layouts
  // share the shape {3 cols, 4 rows} in row-major order, the linear-index
  // arithmetic is identical across them.
  const std::array<bool, 12> values = {
      true,  true,  false, // row 0
      true,  true,  false, // row 1
      false, false, true,  // row 2
      false, false, true   // row 3
  };
  REQUIRE(mask->getNumberOfTuples() == values.size());
  for(usize i = 0; i < values.size(); i++)
  {
    mask->setValue(i, values[i]);
  }
  return dataStructure;
}

void RunIdentifySampleAndCheck(DataStructure& dataStructure)
{
  IdentifySampleFilter filter;
  Arguments args;
  args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_NonSquareImagePath));
  args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_NonSquareMaskPath));
  args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(false));
  args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Expected output: the 4-voxel top-left component (indices 0, 1, 3, 4)
  // survives; the 2-voxel bottom-right component (indices 8, 11) is cleared.
  const std::array<bool, 12> expected = {
      true,  true,  false, //
      true,  true,  false, //
      false, false, false, //
      false, false, false  //
  };
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(k_NonSquareMaskPath);
  REQUIRE(mask.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO("linear index " << i);
    REQUIRE(mask[i] == expected[i]);
  }
}
} // namespace
TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();

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
      std::cout << inputFilePath.string() << std::endl;

      DataStructure dataStructure = LoadDataStructure(inputFilePath);
      IdentifySampleFilter filter;
      Arguments args;
      args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
      args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
      args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
      args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
      args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(sliceBySlicePlane));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
      WriteTestDataStructure(dataStructure, fmt::format("{}/identify_sample_output_{}_{}_{}.dream3d", unit_test::k_BinaryTestOutputDir, fillHoles, sliceBySlice, sliceBySlicePlane));
#endif

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

// -----------------------------------------------------------------------------
// Non-square 2D regression tests. Exercise each EmptyX/Y/Z 2D dispatch with a
// 3x4 layout so any wrong row-stride in the flood-fill would merge the two
// components or step off the end of the buffer. See the comment at
// CreateNonSquare2DMaskDataStructure for the mask layout and expected output.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 4, 1});
  ::RunIdentifySampleAndCheck(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 1, 4});
  ::RunIdentifySampleAndCheck(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{1, 3, 4});
  ::RunIdentifySampleAndCheck(dataStructure);
}
