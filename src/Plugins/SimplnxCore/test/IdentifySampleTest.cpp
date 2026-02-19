
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_ExemplarArrayPath = Constants::k_DataContainerPath.createChildPath(Constants::k_CellData).createChildPath("Mask Exemplar");
}
TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "identify_sample.tar.gz", "identify_sample", true, true);
  using TestArgType = std::tuple<bool, bool, int>;
  /* clang-format off */
  std::vector<TestArgType> allTestParams = {
    {false, false, 0},
    {false, true, 0},
    {false, true, 1},
    {false, true, 2},

    {true, false, 0},
    {true, true, 0},
    {true, true, 1},
    {true, true, 2},
  };
  /* clang-format on */
  for(const auto& testParam : allTestParams)
  {
    bool fillHoles = std::get<0>(testParam);
    bool sliceBySlice = std::get<1>(testParam);
    int sliceBySlicePlane = std::get<2>(testParam);
    SECTION(fmt::format("FillHole:{} SliceBySlice:{} SlicePlane:{}", fillHoles, sliceBySlice, sliceBySlicePlane))
    {
      fs::path inputFilePath = fs::path(fmt::format("{}/identify_sample/exemplar_{}_{}_{}.dream3d", unit_test::k_TestFilesDir, fillHoles, sliceBySlice, sliceBySlicePlane));
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
