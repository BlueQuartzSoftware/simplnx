/*
# Test Plan

Input Files:
DREAM3D_Data/TestFiles/ASCIIData/FeatureIds.csv (int32, 1 component)
DREAM3D_Data/TestFiles/ASCIIData/Quats.csv (float32, 4 component)
DREAM3D_Data/TestFiles/ASCIIData/Phases.csv (int32, 1 component)

Output DataArrays:
AvgEulerAngles  (float32, 3 component)
AvgQuats  (float32, 4 component)

Comparison Files:
DREAM3D_Data/TestFiles/ASCIIData/AvgEulerAngles.csv
DREAM3D_Data/TestFiles/ASCIIData/AvgQuats.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
of doing that.

*/

#include "OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

void runReadTextDataArrayFilter(const std::string k_InputFileName, nx::core::NumericType k_NumericType, const uint64 k_NumTuples, const uint64 k_NumComponents, const DataPath k_InputFileDataPath,
                                DataStructure& dataStructure)
{
  auto* filterList = Application::Instance()->getFilterList();

  Arguments args;
  args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key,
                      std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/ASCIIData/{}.csv", unit_test::k_TestFilesDir, k_InputFileName))));
  args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(k_NumericType));
  args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(k_NumTuples)}}));
  args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NumComponents));
  args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_InputFileDataPath));

  auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
  REQUIRE(nullptr != filter);

  // Preflight the filter and check result
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

TEST_CASE("OrientationAnalysis::ComputeAvgOrientations", "[OrientationAnalysis][ComputeAvgOrientations]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ASCIIData.tar.gz", "ASCIIData");

  // Instantiate an "Application" instance to load plugins
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const uint64 k_NumTuples = 480000;
  const uint64 k_FeatureNumTuples = 409;

  // Setup constants here that are going to be needed in multiple contexts
  const std::string k_GrainDataStr = "Grain Data";
  const DataPath k_AvgQuatsDataPath({k_GrainDataStr, k_AvgQuats});

  const std::string k_AvgEulers("AvgEulerAngles");
  const DataPath k_AvgEulersDataPath({k_GrainDataStr, k_AvgEulers});

  const std::string k_ExemplarAvgQuats("ExemplarAvgQuats");
  const DataPath k_ExemplarAvgQuatsDataPath({k_ExemplarAvgQuats});

  const std::string k_ExemplarAvgEulers("ExemplarAvgEulers");
  const DataPath k_ExemplarAvgEulersDataPath({k_ExemplarAvgEulers});

  const std::string k_CrystalStuctures("Crystal Structures");
  const DataPath k_CrystalStructureDataPath({k_CrystalStuctures});

  const DataPath k_PhasesDataPath({k_Phases});

  const DataPath k_FeatureIdsDataPath({k_FeatureIds});

  const DataPath k_QuatsDataPath({k_Quats});

  DataStructure dataStructure;

  // Create the Crystal Structures Array (needed later down the pipeline)
  UInt32Array* crystalStructuresPtr = nx::core::UnitTest::CreateTestDataArray<uint32>(dataStructure, k_CrystalStuctures, {2}, {1});
  (*crystalStructuresPtr)[0] = 999; // Unknown Crystal Structure
  (*crystalStructuresPtr)[1] = 1;   // Cubic Laue Class

  // Run the "Import Text" Filter to import the data for the FeatureIds, Phases, Quats and Exemplar AvgQuats and AvgEulers
  runReadTextDataArrayFilter(k_Phases, NumericType::int32, k_NumTuples, 1, k_PhasesDataPath, dataStructure);
  runReadTextDataArrayFilter(k_Quats, NumericType::float32, k_NumTuples, 4, k_QuatsDataPath, dataStructure);
  runReadTextDataArrayFilter(k_FeatureIds, NumericType::int32, k_NumTuples, 1, k_FeatureIdsDataPath, dataStructure);
  runReadTextDataArrayFilter(k_AvgQuats, NumericType::float32, k_FeatureNumTuples, 4, k_ExemplarAvgQuatsDataPath, dataStructure);
  runReadTextDataArrayFilter(k_AvgEulers, NumericType::float32, k_FeatureNumTuples, 3, k_ExemplarAvgEulersDataPath, dataStructure);

  // Create the cell feature attribute matrix where the output arrays will be stored
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsDataPath);
  usize largestFeature = 0;
  for(const int32& featureId : featureIds)
  {
    if(static_cast<usize>(featureId) > largestFeature)
    {
      largestFeature = featureId;
    }
  }
  AttributeMatrix* cellFeatureData = AttributeMatrix::Create(dataStructure, k_Grain_Data, {largestFeature + 1});

  // Run the ComputeAvgOrientationsFilter
  {
    // Instantiate the filter and an Arguments Object
    ComputeAvgOrientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesDataPath));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsDataPath));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructureDataPath));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>({k_GrainDataStr}));

    // These are the output AvgQuats and output AvgEuler paths NOT the Exemplar AvgQuats & AvgEulers
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_AvgQuatsArrayName_Key, std::make_any<std::string>(k_AvgQuats));
    args.insertOrAssign(ComputeAvgOrientationsFilter::k_AvgEulerAnglesArrayName_Key, std::make_any<std::string>(k_AvgEulers));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the data sets
  {
    const auto& avgQuats = dataStructure.getDataRefAs<Float32Array>(k_AvgQuatsDataPath);
    const auto& exemplarAvgQuats = dataStructure.getDataRefAs<Float32Array>(k_ExemplarAvgQuatsDataPath);

    size_t numElements = avgQuats.getSize();
    bool sameValue = true;
    for(size_t i = 0; i < numElements; i++)
    {
      float absDif = std::fabs(avgQuats[i] - exemplarAvgQuats[i]);
      sameValue = (absDif < 0.0001);
      if(!sameValue)
      {
        REQUIRE(absDif < 0.0001);
      }
    }
  }

  {
    const auto& avgEulers = dataStructure.getDataRefAs<Float32Array>(k_AvgEulersDataPath);
    const auto& exemplarAvgEulers = dataStructure.getDataRefAs<Float32Array>(k_ExemplarAvgEulersDataPath);

    size_t numElements = avgEulers.getSize();
    bool sameValue = true;
    for(size_t i = 0; i < numElements; i++)
    {
      float absDif = std::fabs(avgEulers[i] - exemplarAvgEulers[i]);
      sameValue = (absDif < 0.0001);
      if(!sameValue)
      {
        REQUIRE(absDif < 0.0001);
      }
    }
  }
}
