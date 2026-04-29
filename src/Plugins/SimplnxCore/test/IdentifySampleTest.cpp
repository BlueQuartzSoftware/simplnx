
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
#include <nonstd/span.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;

namespace
{
// =============================================================================
// 200x200x200 large-scale exemplar test (OOC commit) -- shared fixtures
// =============================================================================
const std::string k_ArchiveName = "identify_sample_exemplars.tar.gz";
const std::string k_DataDirName = "identify_sample_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_ExemplarFile = k_DataDir / "identify_sample.dream3d";

constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";

const DataPath k_GeomPath({k_GeomName});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});

constexpr usize k_Dim = 200;

/**
 * @brief Builds an IdentifySample test dataset: a sphere of "good" voxels
 * with interior holes and exterior noise.
 */
void BuildIdentifySampleTestData(DataStructure& ds, usize dimX, usize dimY, usize dimZ, const std::string& geomName = "DataContainer")
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, geomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath maskPath = DataPath({geomName, "CellData", "Mask"});
  auto maskDataStore = DataStoreUtilities::CreateDataStore<uint8>(ds, maskPath, cellShape, {1}, IDataAction::Mode::Execute);
  auto* maskArray = DataArray<uint8>::Create(ds, "Mask", maskDataStore, cellAM->getId());
  auto& maskStore = maskArray->getDataStoreRef();

  const float32 cx = dimX / 2.0f;
  const float32 cy = dimY / 2.0f;
  const float32 cz = dimZ / 2.0f;
  const float32 radius = dimX * 0.4f;

  const usize sliceSize = dimY * dimX;
  std::vector<uint8> sliceBuffer(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const float32 dx = static_cast<float32>(x) - cx;
        const float32 dy = static_cast<float32>(y) - cy;
        const float32 dz = static_cast<float32>(z) - cz;
        const float32 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        bool good = dist < radius;

        // Interior holes (positions relative to geometry size so they work at any dim)
        const float32 h1cx = cx + radius * 0.3f;
        const float32 h1cy = cy + radius * 0.3f;
        const float32 h1cz = cz + radius * 0.3f;
        const float32 h1r = dimX * 0.053f; // ~4 at 75, ~10.6 at 200
        const float32 h2cx = cx - radius * 0.3f;
        const float32 h2cy = cy - radius * 0.3f;
        const float32 h2cz = cz - radius * 0.3f;
        const float32 h2r = dimX * 0.04f; // ~3 at 75, ~8 at 200

        if(good)
        {
          const float32 h1 = std::sqrt((static_cast<float32>(x) - h1cx) * (static_cast<float32>(x) - h1cx) + (static_cast<float32>(y) - h1cy) * (static_cast<float32>(y) - h1cy) +
                                       (static_cast<float32>(z) - h1cz) * (static_cast<float32>(z) - h1cz));
          if(h1 < h1r)
          {
            good = false;
          }
          const float32 h2 = std::sqrt((static_cast<float32>(x) - h2cx) * (static_cast<float32>(x) - h2cx) + (static_cast<float32>(y) - h2cy) * (static_cast<float32>(y) - h2cy) +
                                       (static_cast<float32>(z) - h2cz) * (static_cast<float32>(z) - h2cz));
          if(h2 < h2r)
          {
            good = false;
          }
        }

        // Isolated noise outside the sphere
        if(!good && dist < radius + 5.0f && dist > radius)
        {
          if((x + y + z) % 7 == 0)
          {
            good = true;
          }
        }

        sliceBuffer[y * dimX + x] = good ? 1 : 0;
      }
    }
    maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const uint8>(sliceBuffer.data(), sliceSize));
  }
}

/**
 * @brief Populates IdentifySampleFilter arguments from a test variant name.
 *
 * Name convention: "whole_fill", "sliced_xy_nofill", etc.
 */
void SetupArgs(Arguments& args, const std::string& testName, const DataPath& geomPath, const DataPath& maskPath)
{
  const bool fillHoles = (testName.find("nofill") == std::string::npos);
  const bool sliceBySlice = (testName.find("sliced") != std::string::npos);
  ChoicesParameter::ValueType slicePlane = 0;
  if(testName.find("xz") != std::string::npos)
  {
    slicePlane = 1;
  }
  else if(testName.find("yz") != std::string::npos)
  {
    slicePlane = 2;
  }

  args.insertOrAssign(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
  args.insertOrAssign(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
  args.insertOrAssign(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(slicePlane));
}

// =============================================================================
// Hand-built non-square 2D fixture (Nathan Young, PR #1590).
//
// Builds a 3x4 mask (shaped per the empty axis) with a 4-voxel top-left
// connected component and a 2-voxel bottom-right connected component.
// IdentifySample should keep the larger (4-voxel) region and drop the smaller
// one. A wrong row-stride in the Empty2D dispatches would either merge the
// two components or step outside the buffer.
//
// Mask (T = true = good, F = false = bad), oriented in the two non-empty axes:
//   row 0: T T F
//   row 1: T T F
//   row 2: F F T
//   row 3: F F T
// =============================================================================
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

  const std::array<bool, 12> expected = {
      true,  true,  false, //
      true,  true,  false, //
      false, false, false, //
      false, false, false  //
  };
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

TEST_CASE("SimplnxCore::IdentifySampleFilter: 200x200x200 Exemplar Comparison", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // uint8 1-comp => 200*200*1 = 40,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 40000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("whole_fill", "whole_nofill", "sliced_xy_fill", "sliced_xy_nofill", "sliced_xz_fill", "sliced_xz_nofill", "sliced_yz_fill", "sliced_yz_nofill");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    DataStructure dataStructure;
    BuildIdentifySampleTestData(dataStructure, k_Dim, k_Dim, k_Dim);

    UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(k_MaskPath));

    IdentifySampleFilter filter;
    Arguments args;
    SetupArgs(args, testName, k_GeomPath, k_MaskPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = testName + "_Exemplar";
    const DataPath exemplarMaskPath({exemplarGeomName, std::string(k_CellDataName), "Mask"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarMaskPath));
    CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarMaskPath), dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: Generate Test Data", "[SimplnxCore][IdentifySampleFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/identify_sample", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  DataStructure ds;
  for(const auto& name : {"whole_fill", "whole_nofill", "sliced_xy_fill", "sliced_xy_nofill", "sliced_xz_fill", "sliced_xz_nofill", "sliced_yz_fill", "sliced_yz_nofill"})
  {
    BuildIdentifySampleTestData(ds, k_Dim, k_Dim, k_Dim, name);
  }
  UnitTest::WriteTestDataStructure(ds, outputDir / "input.dream3d");
}

TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();

  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

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

      auto executeResult = filter.execute(dataStructure, args);
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

// -----------------------------------------------------------------------------
// Non-square 2D regression tests (Nathan Young, PR #1590). Exercise each
// EmptyX/Y/Z 2D dispatch with a 3x4 layout so any wrong row-stride in the
// flood-fill would merge the two components or step off the end of the buffer.
// See the comment at CreateNonSquare2DMaskDataStructure for the mask layout
// and expected output.
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 4, 1});
  ::RunIdentifySampleAndCheck(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{3, 1, 4});
  ::RunIdentifySampleAndCheck(dataStructure);
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  DataStructure dataStructure = ::CreateNonSquare2DMaskDataStructure(SizeVec3{1, 3, 4});
  ::RunIdentifySampleAndCheck(dataStructure);
}
