#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_AlignmentAMPath = Constants::k_DataContainerPath.createChildPath(Constants::k_AlignmentAMName);

struct CompareArraysFunctor
{
  template <typename T>
  void operator()(const IDataArray& computedArray, const IDataArray& exemplarArray)
  {
    UnitTest::CompareDataArrays<T>(computedArray, exemplarArray);
  }
};
} // namespace

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Relative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();

  auto app = Application::GetOrCreateInstance();
  auto* filterList = app->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_RelativeShiftsArrayName});
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_RelativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Align Sections List Filter
  {
    // Instantiate the filter and an Arguments Object
    AlignSectionsListFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/relative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Cumulative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();
  auto* filterList = Application::GetOrCreateInstance()->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_CumulativeShiftsArrayName});
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_CumulativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Align Sections List Filter
  {
    // Instantiate the filter and an Arguments Object
    AlignSectionsListFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/cumulative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: SIMPL Backwards Compatibility", "[SimplnxCore][AlignSectionsListFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "AlignSectionsListFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "AlignSectionsListFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<AlignSectionsListFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
