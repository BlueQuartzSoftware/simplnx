/**

# Test Plan #

Read the following data from DREAM3D_Data:

Input Data:
DREAM3D_Data/TestFiles/ASCII_Data/EulerAngles.csv
Float32, 3 component. 480000 tuples

use 30 Degrees about the <1, 1, 1> axis

Run the filter which does the rotation *in place*.

Read the reference data set:

DREAM3D_Data/TestFiles/ASCII_Data/EulersRotated.csv
Float32, 3 component. 480000 tuples

Compare the 2 data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
of doing that.

*/

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "complex_plugins/Utilities/TestUtilities.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::RotateEulerRefFrame", "[OrientationAnalysis]")
{
  // Instantiate an "Application" instance to load plugins
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  const uint64 k_NumComponents = 3;
  const uint64 k_NumTuples = 480000;
  const complex::NumericType k_NumericType = complex::NumericType::float32;

  // Constant strings and DataPaths to be used later
  const DataPath k_EulerAnglesDataPath({k_EulerAngles});

  const std::string k_EulersRotated("EulersRotated");
  const DataPath k_EulersRotatedDataPath({k_EulersRotated});

  std::string inputFile = fmt::format("{}/TestFiles/ASCII_Data/EulerAngles.csv", unit_test::k_DREAM3DDataDir.view());
  std::string comparisonDataFile = fmt::format("{}/TestFiles/ASCII_Data/EulersRotated.csv", unit_test::k_DREAM3DDataDir.view());

  // These are the argument keys for the Import Text filter. We cannot use the ones from the
  // header file as that would bring in a dependency on the ComplexCorePlugin
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
    args.insertOrAssign(k_NTuplesKey, std::make_any<uint64>(k_NumTuples));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_EulerAnglesDataPath));

    auto filter = filterList->createFilter(k_ImportTextFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    Arguments args;
    args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(fs::path(comparisonDataFile)));
    args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
    args.insertOrAssign(k_NTuplesKey, std::make_any<uint64>(k_NumTuples));
    args.insertOrAssign(k_NCompKey, std::make_any<uint64>(k_NumComponents));
    args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_EulersRotatedDataPath));

    auto filter = filterList->createFilter(k_ImportTextFilterHandle);
    REQUIRE(nullptr != filter);

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Run the RotateEulerRefFrameFilter
  {
    RotateEulerRefFrameFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAngle_Key, std::make_any<float32>(30));
    args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(RotateEulerRefFrameFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the 2 data sets
  {
    const auto& eulerAngles = dataStructure.getDataRefAs<Float32Array>(k_EulerAnglesDataPath);
    const auto& eulersRotated = dataStructure.getDataRefAs<Float32Array>(k_EulersRotatedDataPath);

    size_t numElements = eulerAngles.getSize();
    bool sameValue = true;
    for(size_t i = 0; i < numElements; i++)
    {
      float absDif = std::fabs(eulerAngles[i] - eulersRotated[i]);
      sameValue = (absDif < 0.0001);
      if(!sameValue)
      {
        REQUIRE(absDif < 0.0001);
      }
    }
  }
}
