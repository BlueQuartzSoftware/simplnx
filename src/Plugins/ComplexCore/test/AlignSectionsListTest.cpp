#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AlignSectionsListFilter.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

TEST_CASE("ComplexCore::AlignSectionsListFilter: Valid filter execution", "[ComplexCore][AlignSectionsListFilter]")
{
  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  const std::string kDataInputArchive = "6_6_align_sections_misorientation.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_align_sections_misorientation";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  const std::string kDataInputArchive1 = "Small_IN100_dream3d.tar.gz";
  const std::string kExpectedOutputTopLevel1 = "Small_IN100.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive1, kExpectedOutputTopLevel1,
                                                              complex::unit_test::k_BinaryTestOutputDir);

  const std::string kDataInputArchive2 = "align_sections.tar.gz";
  const std::string kExpectedOutputTopLevel2 = "align_sections_misorientation.txt";
  const complex::UnitTest::TestFileSentinel testDataSentinel2(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive2, kExpectedOutputTopLevel2,
                                                              complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Align Sections List Filter
  {
    // Instantiate the filter and an Arguments Object
    AlignSectionsListFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(AlignSectionsListFilter::k_InputFile_Key,
                        std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/align_sections_misorientation.txt", unit_test::k_TestFilesDir))));
    args.insertOrAssign(AlignSectionsListFilter::k_DREAM3DAlignmentFile_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(complex::Constants::k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);
}

TEST_CASE("ComplexCore::AlignSectionsListFilter: Invalid filter execution", "[ComplexCore][AlignSectionsListFilter]")
{
  const std::string kDataInputArchive1 = "Small_IN100_dream3d.tar.gz";
  const std::string kExpectedOutputTopLevel1 = "Small_IN100.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive1, kExpectedOutputTopLevel1,
                                                              complex::unit_test::k_BinaryTestOutputDir);

  const std::string kDataInputArchive = "export_files_test.tar.gz";
  const std::string kExpectedOutputTopLevel = "export_files_test";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  // Instantiate the filter and an Arguments Object
  AlignSectionsListFilter filter;
  Arguments args;

  args.insertOrAssign(AlignSectionsListFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(
                                                                    fs::path(fmt::format("{}/export_files_test/write_ascii_data_exemplars/float32/0_0_exemplar_0.txt", unit_test::k_TestFilesDir))));
  args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(complex::Constants::k_DataContainerPath));

  SECTION("Invalid DREAM3D Alignment file format")
  {
    args.insertOrAssign(AlignSectionsListFilter::k_DREAM3DAlignmentFile_Key, std::make_any<bool>(true));
  }
  SECTION("Invalid user Alignment file format")
  {
    args.insertOrAssign(AlignSectionsListFilter::k_DREAM3DAlignmentFile_Key, std::make_any<bool>(false));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}
