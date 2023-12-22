#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

TEST_CASE("OrientationAnalysis::RotateEulerRefFrame", "[OrientationAnalysis]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ASCIIData.tar.gz", "ASCIIData");

  // Instantiate an "Application" instance to load plugins
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const uint64 k_NumComponents = 3;
  const static DynamicTableInfo::TableDataType k_NumTuples = {{static_cast<double>(480000)}};
  const nx::core::NumericType k_NumericType = nx::core::NumericType::float32;

  // Constant strings and DataPaths to be used later
  const DataPath k_EulerAnglesDataPath({Constants::k_EulerAngles});

  const std::string k_EulersRotated("EulersRotated");
  const DataPath k_EulersRotatedDataPath({k_EulersRotated});

  std::string inputFile = fmt::format("{}/ASCIIData/EulerAngles.csv", unit_test::k_TestFilesDir.view());
  std::string comparisonDataFile = fmt::format("{}/ASCIIData/EulersRotated.csv", unit_test::k_TestFilesDir.view());

  // These are the argument keys for the Import Text filter. We cannot use the ones from the
  // header file as that would bring in a dependency on the SimplnxCorePlugin
  static constexpr StringLiteral k_InputFileKey = "input_file";
  static constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
  static constexpr StringLiteral k_NTuplesKey = "n_tuples";
  static constexpr StringLiteral k_NCompKey = "n_comp";
  static constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
  static constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
  static constexpr StringLiteral k_DataArrayKey = "output_data_array";

  // Make sure we can load the "Import Text Filter" filter from the plugin
  auto* filterList = Application::Instance()->getFilterList();
  // Make sure we can instantiate the Import Text Filter

  DataStructure dataStructure;

  // Run the "Import Text" Filter to import the data for the EulerAngles and EulersRotated
  {
    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_EulerAnglesDataPath));

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(comparisonDataFile)));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_EulersRotatedDataPath));

    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Run the RotateEulerRefFrameFilter
  {
    RotateEulerRefFrameFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F, 30.0F}));
    args.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the 2 data sets
  {
    const auto& eulerAngles = dataStructure.getDataRefAs<Float32Array>(k_EulerAnglesDataPath);
    const auto& eulersRotated = dataStructure.getDataRefAs<Float32Array>(k_EulersRotatedDataPath);

    size_t numElements = eulerAngles.getSize();
    for(size_t i = 0; i < numElements; i++)
    {
      float absDif = std::fabs(eulerAngles[i] - eulersRotated[i]);
      bool sameValue = (absDif < 0.0001);
      if(!sameValue)
      {
        REQUIRE(absDif < 0.0001);
      }
    }
  }
}
