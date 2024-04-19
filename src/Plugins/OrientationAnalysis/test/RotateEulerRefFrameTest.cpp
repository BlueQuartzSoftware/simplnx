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

  // Make sure we can load the "Import Text Filter" filter from the plugin
  auto* filterList = Application::Instance()->getFilterList();
  // Make sure we can instantiate the Import Text Filter

  DataStructure dataStructure;

  // Run the "Import Text" Filter to import the data for the EulerAngles and EulersRotated
  {
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesDataPath));

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
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(comparisonDataFile)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_NumTuples));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_EulersRotatedDataPath));

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
