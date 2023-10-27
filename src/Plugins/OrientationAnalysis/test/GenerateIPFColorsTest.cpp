/*
# Test Plan

Input Files:
DREAM3D_Data/TestFiles/ASCIIData/EulerAngles.csv
DREAM3D_Data/TestFiles/ASCIIData/Phases.csv

Output DataArrays:
IPFColors (3 component UInt8 Array)

Comparison Files:
DREAM3D_Data/TestFiles/ASCIIData/IPFColor.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. The values should be exactly the same.

*/
#include "OrientationAnalysis/Filters/GenerateIPFColorsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

namespace complex::Constants
{
inline constexpr StringLiteral k_ImageDataContainer("ImageDataContainer");
inline constexpr StringLiteral k_OutputIPFColors("IPF Colors_Test_Output");
} // namespace complex::Constants

TEST_CASE("OrientationAnalysis::GenerateIPFColors", "[OrientationAnalysis][GenerateIPFColors]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "so3_cubic_high_ipf_001.tar.gz",
                                                             "so3_cubic_high_ipf_001.dream3d");

  DataStructure dataStructure;
  {

    // This test file was produced by SIMPL/DREAM3D. our results should match theirs
    auto exemplarFilePath = fs::path(fmt::format("{}/so3_cubic_high_ipf_001.dream3d", unit_test::k_TestFilesDir));
    REQUIRE(fs::exists(exemplarFilePath));
    auto result = DREAM3D::ImportDataStructureFromFile(exemplarFilePath);
    REQUIRE(result.valid());
    dataStructure = result.value();
  }

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    GenerateIPFColorsFilter filter;
    Arguments args;

    DataPath cellEulerAnglesPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_EulerAngles});
    DataPath cellPhasesArrayPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Phases});
    DataPath goodVoxelsPath({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Mask});
    DataPath crystalStructuresArrayPath({Constants::k_ImageDataContainer, Constants::k_CellEnsembleData, Constants::k_CrystalStructures});
    DataPath cellIPFColorsArrayName({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_OutputIPFColors});

    // Create default Parameters for the filter.
    args.insertOrAssign(GenerateIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F}));
    args.insertOrAssign(GenerateIPFColorsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(GenerateIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(cellEulerAnglesPath));
    args.insertOrAssign(GenerateIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesArrayPath));
    args.insertOrAssign(GenerateIPFColorsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(goodVoxelsPath));
    args.insertOrAssign(GenerateIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresArrayPath));
    args.insertOrAssign(GenerateIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<std::string>(Constants::k_OutputIPFColors));

    REQUIRE(dataStructure.getData(goodVoxelsPath) != nullptr);
    REQUIRE(dataStructure.getData(cellEulerAnglesPath) != nullptr);
    REQUIRE(dataStructure.getData(cellPhasesArrayPath) != nullptr);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    {
      // Write out the DataStructure for later viewing/debugging
      Result<complex::HDF5::FileWriter> result = complex::HDF5::FileWriter::CreateFile(fmt::format("{}/GenerateIPFColors_Test.dream3d", unit_test::k_BinaryTestOutputDir));
      complex::HDF5::FileWriter fileWriter = std::move(result.value());
      auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
      COMPLEX_RESULT_REQUIRE_VALID(resultH5);
    }

    DataPath ipfColors({Constants::k_ImageDataContainer, Constants::k_CellData, Constants::k_Ipf_Colors});

    UInt8Array& exemplar = dataStructure.getDataRefAs<UInt8Array>(ipfColors);
    UInt8Array& output = dataStructure.getDataRefAs<UInt8Array>(cellIPFColorsArrayName);

    size_t totalElements = exemplar.getSize();
    bool valid = true;
    for(size_t i = 0; i < totalElements; i++)
    {
      if(exemplar[i] != output[i])
      {
        valid = false;
        break;
      }
    }
    REQUIRE(valid == true);
  }
}
