#include "OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 * Identify Sample
 * Align Sections Feature Centroid
 *
 * Read DREAM3D File (read the exemplar 'align_sections_feature_centroid.dream3d' file from
 * [Optional] Write out dream3d file
 *
 *
 * Compare the shifts file 'align_sections_feature_centroid.txt' to what was written
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

TEST_CASE("OrientationAnalysis::NeighborOrientationCorrelationFilter: Small IN100 Pipeline", "[OrientationAnalysis][NeighborOrientationCorrelationFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "neighbor_orientation_correlation.tar.gz",
                                                             "neighbor_orientation_correlation.dream3d");

  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "Small_IN100_dream3d.tar.gz", "Small_IN100.dream3d");

  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/neighbor_orientation_correlation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteAlignSectionsMisorientation(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir)));

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Align Sections Feature Centroid Filter
  SmallIn100::ExecuteAlignSectionsFeatureCentroid(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir)));

  // Bad Data Neighbor Orientation Check Filter
  SmallIn100::ExecuteBadDataNeighborOrientationCheck(dataStructure, *filterList);

  // Neighbor Orientation Correlation Filter
  {
    auto filter = filterList->createFilter(k_NeighborOrientationCorrelationFilterHandle);
    REQUIRE(nullptr != filter);

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MinConfidence_Key, std::make_any<float32>(0.2f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_Level_Key, std::make_any<int32>(2));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_ConfidenceIndexArrayPath_Key, std::make_any<DataPath>(k_ConfidenceIndexArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(NeighborOrientationCorrelationFilter::k_IgnoredDataArrayPaths_Key, std::make_any<std::vector<DataPath>>());

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      // Check to see if there is something to compare against in the exemplar file.
      if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
      {
        continue;
      }

      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(), type, exemplarType)
                  << std::endl;
        continue;
      }

      switch(type)
      {
      case DataType::boolean: {
        UnitTest::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        UnitTest::CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        UnitTest::CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        UnitTest::CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        UnitTest::CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        UnitTest::CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        UnitTest::CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        UnitTest::CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        UnitTest::CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        UnitTest::CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/neighbor_orientation_correlation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
}
