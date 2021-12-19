#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKOtsuMultipleThresholdsImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Default parameter settings
TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImage: default", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKOtsuMultipleThresholdsImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Short.nrrd
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Short.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    int32_t result = complex::ITKTestBase::ReadImage(ds, filePath, inputGeometryPath, inputDataPath);
    REQUIRE(result == 0);
  } // End Scope Section

  // Test the filter itself
  {
    Arguments args;
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    auto pNumberOfThresholds = 1u;
    auto pLabelOffset = 0u;
    auto pNumberOfHistogramBins = 128u;
    auto pValleyEmphasis = false;
    auto pReturnBinMidpoint = false;
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<int32>(pNumberOfThresholds));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_LabelOffset_Key, std::make_any<int32>(pLabelOffset));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<float64>(pNumberOfHistogramBins));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<bool>(pValleyEmphasis));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_OtsuMultipleThresholdsImageFilter_default.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_OtsuMultipleThresholdsImageFilter_default.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "a9c3b0c0971c5cbda12b29db916451c6");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "OtsuMultipleThresholdsImageFilter_default.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Default parameter settings
TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImage: two_on_float", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKOtsuMultipleThresholdsImage filter;
  DataStructure ds;
  // Read the input image: Input/Ramp-Zero-One-Float.nrrd
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/Ramp-Zero-One-Float.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    int32_t result = complex::ITKTestBase::ReadImage(ds, filePath, inputGeometryPath, inputDataPath);
    REQUIRE(result == 0);
  } // End Scope Section

  // Test the filter itself
  {
    Arguments args;
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    auto pNumberOfThresholds = 2;
    auto pReturnBinMidpoint = true;
    auto pLabelOffset = 0u;
    auto pNumberOfHistogramBins = 128u;
    auto pValleyEmphasis = false;
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<int32>(pNumberOfThresholds));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_LabelOffset_Key, std::make_any<int32>(pLabelOffset));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<float64>(pNumberOfHistogramBins));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<bool>(pValleyEmphasis));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_OtsuMultipleThresholdsImageFilter_two_on_float.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_OtsuMultipleThresholdsImageFilter_two_on_float.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "1ab20d3cd9a354b45ac07ec59c0413b3");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "OtsuMultipleThresholdsImageFilter_two_on_float.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Default parameter settings
TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImage: three_on", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKOtsuMultipleThresholdsImage filter;
  DataStructure ds;
  // Read the input image: Input/cthead1.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/cthead1.png";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    int32_t result = complex::ITKTestBase::ReadImage(ds, filePath, inputGeometryPath, inputDataPath);
    REQUIRE(result == 0);
  } // End Scope Section

  // Test the filter itself
  {
    Arguments args;
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    auto pNumberOfThresholds = 3;
    auto pNumberOfHistogramBins = 256;
    auto pReturnBinMidpoint = true;
    auto pLabelOffset = 0u;
    auto pValleyEmphasis = false;
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<int32>(pNumberOfThresholds));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_LabelOffset_Key, std::make_any<int32>(pLabelOffset));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<float64>(pNumberOfHistogramBins));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<bool>(pValleyEmphasis));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_OtsuMultipleThresholdsImageFilter_three_on.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_OtsuMultipleThresholdsImageFilter_three_on.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "b61c3f4e063fcdd24dba76227129ae34");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "OtsuMultipleThresholdsImageFilter_three_on.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Default parameter settings
TEST_CASE("ITKImageProcessing::ITKOtsuMultipleThresholdsImage: valley_emphasis", "[ITKImageProcessing][ITKOtsuMultipleThresholdsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKOtsuMultipleThresholdsImage filter;
  DataStructure ds;
  // Read the input image: Input/cthead1.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/cthead1.png";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    int32_t result = complex::ITKTestBase::ReadImage(ds, filePath, inputGeometryPath, inputDataPath);
    REQUIRE(result == 0);
  } // End Scope Section

  // Test the filter itself
  {
    Arguments args;
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    auto pNumberOfThresholds = 3;
    auto pValleyEmphasis = true;
    auto pReturnBinMidpoint = true;
    auto pLabelOffset = 0u;
    auto pNumberOfHistogramBins = 128u;
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfThresholds_Key, std::make_any<int32>(pNumberOfThresholds));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_LabelOffset_Key, std::make_any<int32>(pLabelOffset));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NumberOfHistogramBins_Key, std::make_any<float64>(pNumberOfHistogramBins));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_ValleyEmphasis_Key, std::make_any<bool>(pValleyEmphasis));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKOtsuMultipleThresholdsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_OtsuMultipleThresholdsImageFilter_valley_emphasis.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath =
        fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_OtsuMultipleThresholdsImageFilter_valley_emphasis.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "fb65e730472c8001185f355fb626ca3f");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "OtsuMultipleThresholdsImageFilter_valley_emphasis.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
