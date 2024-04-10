#include "SimplnxCore/Filters/AlignSectionsListFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Valid filter execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = app->getFilterList();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_align_sections_misorientation.tar.gz",
                                                              "6_6_align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  const std::string kDataInputArchive2 = "align_sections.tar.gz";
  const std::string kExpectedOutputTopLevel2 = "align_sections_misorientation.txt";
  const nx::core::UnitTest::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, kDataInputArchive2, kExpectedOutputTopLevel2);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
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
    args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);
}

TEST_CASE("SimplnxCore::AlignSectionsListFilter: Invalid filter execution", "[SimplnxCore][AlignSectionsListFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "export_files_test.tar.gz", "export_files_test");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  // Instantiate the filter and an Arguments Object
  AlignSectionsListFilter filter;
  Arguments args;

  args.insertOrAssign(AlignSectionsListFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(
                                                                    fs::path(fmt::format("{}/export_files_test/write_ascii_data_exemplars/float32/0_0_exemplar_0.txt", unit_test::k_TestFilesDir))));
  args.insertOrAssign(AlignSectionsListFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(nx::core::Constants::k_DataContainerPath));

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
}
