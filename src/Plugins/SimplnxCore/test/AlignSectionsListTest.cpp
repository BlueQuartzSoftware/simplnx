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

#include <array>
#include <cmath>
#include <filesystem>

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
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  auto app = Application::GetOrCreateInstance();
  auto* filterList = app->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_RelativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_RelativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_RelativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  auto newShiftsCopyResult = CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());
  SIMPLNX_RESULT_REQUIRE_VALID(newShiftsCopyResult);

  // Apply the mask filter before alignment.
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Execute the alignment filter.
  {
    // Configure the filter arguments.
    AlignSectionsListFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Cumulative Shifts execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  auto* filterList = Application::GetOrCreateInstance()->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(Constants::k_PhasesArrayPath));

  // Load the exemplar output.
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/output_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  const DataPath newShiftsPath = DataPath({Constants::k_CumulativeShiftsArrayName});
  REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName)));
  auto& exemplarShifts = exemplarDataStructure.getDataRefAs<Int64Array>(k_AlignmentAMPath.createChildPath(Constants::k_CumulativeShiftsArrayName));
  UnitTest::CreateTestDataArray<int64>(dataStructure, Constants::k_CumulativeShiftsArrayName, exemplarShifts.getTupleShape(), exemplarShifts.getComponentShape());

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(newShiftsPath));
  auto& newShifts = dataStructure.getDataRefAs<Int64Array>(newShiftsPath);
  auto newShiftsCopyResult = CopyFromArray::CopyData(exemplarShifts, newShifts, 0ULL, 0ULL, exemplarShifts.getNumberOfTuples());
  SIMPLNX_RESULT_REQUIRE_VALID(newShiftsCopyResult);

  // Apply the mask filter before alignment.
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Execute the alignment filter.
  {
    // Configure the filter arguments.
    AlignSectionsListFilter filter;
    Arguments args;

    args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL));

    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));
    args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(newShiftsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(nx::core::Constants::k_DataContainerPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(dataStructure, nx::core::Constants::k_DataContainerPath.createChildPath(cellAttributeMatrix->getName()));
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Nonzero top row", "[SimplnxCore][AlignSectionsListFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto inputType = GENERATE(0ULL, 1ULL);
  const int64 sign = GENERATE(-1, 1);
  CAPTURE(scenario, inputType, sign);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geom != nullptr);
  geom->setDimensions({3, 3, 4});
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", ShapeType{4, 3, 3}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  auto* mask = UInt8Array::Create(dataStructure, "Mask", std::make_shared<DataStore<uint8>>(ShapeType{4, 3, 3}, ShapeType{1}, 0), cells->getId());
  REQUIRE(mask != nullptr);
  auto* shifts = Int64Array::Create(dataStructure, "Shifts", std::make_shared<DataStore<int64>>(ShapeType{4}, ShapeType{2}, 0));
  REQUIRE(shifts != nullptr);
  // Descending cumulative offsets are sign * {1,0,-1,0} on both axes.
  // Relative offsets are their differences, with row 0 as the initial offset.
  // Each source mark is at (1 + offset, 1 + offset), so all outputs must be (1,1).
  const std::array<int64, 4> cumulative = {1, 0, -1, 0};
  const std::array<int64, 4> relative = {1, -1, -1, 1};
  for(usize row = 0; row < 4; row++)
  {
    const usize coordinate = static_cast<usize>(1 + (sign * cumulative[row]));
    (*mask)[((3 - row) * 9) + (coordinate * 3) + coordinate] = 1;
    (*shifts)[row * 2] = sign * (inputType == 0 ? relative[row] : cumulative[row]);
    (*shifts)[(row * 2) + 1] = (*shifts)[row * 2];
  }
  AlignSectionsListFilter filter;
  Arguments args;
  args.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(inputType));
  args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(DataPath({"Shifts"})));
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  for(usize z = 0; z < 4; z++)
  {
    for(usize xy = 0; xy < 9; xy++)
    {
      CAPTURE(z, xy);
      CHECK((*mask)[(z * 9) + xy] == (xy == 4 ? 1 : 0));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
