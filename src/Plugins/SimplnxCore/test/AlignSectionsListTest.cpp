#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_AlignmentAMPath = constants::k_DataContainerPath.createChildPath(constants::k_AlignmentAMName);

struct CompareArraysFunctor
{
  template <typename T>
  void operator()(const IDataArray& computedArray, const IDataArray& exemplarArray)
  {
    unit_test::CompareDataArrays<T>(computedArray, exemplarArray);
  }
};
} // namespace

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Relative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  auto app = Application::GetOrCreateInstance();
  auto* filterListPtr = app->getFilterList();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));

  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({constants::k_RelativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64ArrayType>(k_AlignmentAMPath.createChildPath(constants::k_RelativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64ArrayType>(k_AlignmentAMPath.createChildPath(constants::k_RelativeShiftsArrayName));
  unit_test::CreateTestDataArray<int64>(dataStructure, constants::k_RelativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64ArrayType>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64ArrayType>(newShiftsPath);
  auto newShiftsCopyResult = copy_from_array::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());
  SIMPLNX_RESULT_REQUIRE_VALID(newShiftsCopyResult);

  // Apply the mask filter before alignment.
  small_in100::ExecuteMultiThresholdObjects(dataStructure, *filterListPtr);

  // Execute the alignment filter.
  {
    // Configure the filter arguments.
    AlignSectionsListFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(path));
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/relative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure, small_in100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Cumulative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  auto* filterListPtr = Application::GetOrCreateInstance()->getFilterList();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::unit_test::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(constants::k_PhasesArrayPath));

  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({constants::k_CumulativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64ArrayType>(k_AlignmentAMPath.createChildPath(constants::k_CumulativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64ArrayType>(k_AlignmentAMPath.createChildPath(constants::k_CumulativeShiftsArrayName));
  unit_test::CreateTestDataArray<int64>(dataStructure, constants::k_CumulativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64ArrayType>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64ArrayType>(newShiftsPath);
  auto newShiftsCopyResult = copy_from_array::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());
  SIMPLNX_RESULT_REQUIRE_VALID(newShiftsCopyResult);

  // Apply the mask filter before alignment.
  small_in100::ExecuteMultiThresholdObjects(dataStructure, *filterListPtr);

  // Execute the alignment filter.
  {
    // Configure the filter arguments.
    AlignSectionsListFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
  REQUIRE(selectedCellArrays.has_value());

  for(const auto& path : selectedCellArrays.value())
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(path));
    REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(path));
    const auto& computedIDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    const auto& exemplarIDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(path);

    ExecuteDataFunction(::CompareArraysFunctor{}, computedIDataArray.getDataType(), computedIDataArray, exemplarIDataArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_list/cumulative_align_sections_list.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure, small_in100::k_TupleCheckIgnoredPaths);
}
