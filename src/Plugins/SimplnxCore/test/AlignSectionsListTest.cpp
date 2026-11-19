#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/Filters/CombineAttributeArraysFilter.hpp"
#include "SimplnxCore/Filters/ReadCSVFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
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

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Legacy cumulative file requires zero row", "[SimplnxCore][AlignSectionsListFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool prependZero = GENERATE(false, true);
  CAPTURE(prependZero);

  const fs::path directory = fs::temp_directory_path() / ("AlignSectionsListLegacyShifts_" + Uuid::GenerateV4().str());
  auto cleanup = MakeScopeGuard([&directory]() noexcept {
    std::error_code error;
    fs::remove_all(directory, error);
  });
  REQUIRE(fs::create_directory(directory));
  const auto filePath = directory / "legacy.txt";
  std::ofstream file(filePath);
  REQUIRE(file.is_open());
  if(prependZero)
  {
    file << "3\t4\t0\t0\t0\t0\n";
  }
  // Legacy rows describe sections 2, 1, 0, in that order. Shifts are in cells.
  file << "2\t3\t1\t0\t1\t0\n1\t2\t0\t1\t1\t1\n0\t1\t1\t0\t2\t1\n";
  file.close();
  REQUIRE_FALSE(file.fail());

  DataStructure dataStructure;
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({5, 5, 4});
  geometry->setSpacing({1.0F, 1.0F, 1.0F});
  geometry->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", {4, 5, 5}, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(*cells);
  auto* values = Int32Array::Create(dataStructure, "Position", DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Image", "Cells", "Position"}), {4, 5, 5}, {1}), cells->getId());
  REQUIRE(values != nullptr);
  for(usize z = 0; z < 4; z++)
  {
    for(usize y = 0; y < 5; y++)
    {
      for(usize x = 0; x < 5; x++)
      {
        (*values)[z * 25 + y * 5 + x] = static_cast<int32>(1 + x + 10 * y + 100 * z);
      }
    }
  }

  ReadCSVData csv;
  csv.inputFilePath = filePath.string();
  csv.customHeaders = {"slice", "next", "relX", "relY", "cumX", "cumY"};
  csv.dataTypes = std::vector<CSVType>(6, CSVType::int64);
  csv.skippedArrayMask = std::vector<bool>(6, false);
  csv.delimiters = {'\t'};
  csv.headerMode = ReadCSVData::HeaderMode::CUSTOM;
  csv.startImportRow = 1;
  csv.tupleDims = {prependZero ? usize{4} : usize{3}};
  Arguments readArgs;
  readArgs.insertOrAssign(ReadCSVFileFilter::k_ReadCSVData_Key, std::make_any<ReadCSVData>(csv));
  readArgs.insertOrAssign(ReadCSVFileFilter::k_UseExistingGroup_Key, std::make_any<bool>(false));
  readArgs.insertOrAssign(ReadCSVFileFilter::k_CreatedDataGroup_Key, std::make_any<DataPath>(DataPath({"Shifts"})));
  auto readResult = ReadCSVFileFilter{}.execute(dataStructure, readArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult.result);

  Arguments combineArgs;
  combineArgs.insertOrAssign(CombineAttributeArraysFilter::k_SelectedDataArrayPaths_Key,
                             std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"Shifts", "cumX"}), DataPath({"Shifts", "cumY"})}));
  combineArgs.insertOrAssign(CombineAttributeArraysFilter::k_StackedDataArrayName_Key, std::make_any<std::string>("Cumulative"));
  combineArgs.insertOrAssign(CombineAttributeArraysFilter::k_NormalizeData_Key, std::make_any<bool>(false));
  combineArgs.insertOrAssign(CombineAttributeArraysFilter::k_MoveValues_Key, std::make_any<bool>(false));
  auto combineResult = CombineAttributeArraysFilter{}.execute(dataStructure, combineArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(combineResult.result);

  Arguments alignArgs;
  alignArgs.insertOrAssign(AlignSectionsListFilter::k_InputArrayType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  alignArgs.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  alignArgs.insertOrAssign(AlignSectionsListFilter::k_ShiftsArrayPath_Key, std::make_any<DataPath>(DataPath({"Shifts", "Cumulative"})));
  auto alignResult = scope.executeFilter(AlignSectionsListFilter{}, dataStructure, alignArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(alignResult.result);

  // Hand-computed shifts by section 0, 1, 2, 3. Without the zero row, the first
  // legacy shift is unused, the middle sections use the row below, and section 0 is unchanged.
  const std::array<std::array<int64, 2>, 4> legacyMapping = {{{0, 0}, {2, 1}, {1, 1}, {0, 0}}};
  const std::array<std::array<int64, 2>, 4> correctedMapping = {{{2, 1}, {1, 1}, {1, 0}, {0, 0}}};
  const auto& expectedShifts = prependZero ? correctedMapping : legacyMapping;
  for(usize z = 0; z < 4; z++)
  {
    for(int64 y = 0; y < 5; y++)
    {
      for(int64 x = 0; x < 5; x++)
      {
        const int64 sourceX = x + expectedShifts[z][0];
        const int64 sourceY = y + expectedShifts[z][1];
        const int32 expected = sourceX < 5 && sourceY < 5 ? static_cast<int32>(1 + sourceX + 10 * sourceY + 100 * z) : 0;
        CAPTURE(z, x, y);
        REQUIRE((*values)[z * 25 + y * 5 + x] == expected);
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
