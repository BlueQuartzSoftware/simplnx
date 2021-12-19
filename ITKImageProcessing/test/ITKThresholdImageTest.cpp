#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKThresholdImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Default
TEST_CASE("ITKImageProcessing::ITKThresholdImage: Default", "[ITKImageProcessing][ITKThresholdImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdImage filter;
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
    auto pLower = 0.0;
    auto pUpper = 1.0;
    auto pOutsideValue = 0.0;
    args.insertOrAssign(ITKThresholdImage::k_Lower_Key, std::make_any<float64>(pLower));
    args.insertOrAssign(ITKThresholdImage::k_Upper_Key, std::make_any<float64>(pUpper));
    args.insertOrAssign(ITKThresholdImage::k_OutsideValue_Key, std::make_any<float64>(pOutsideValue));
    args.insertOrAssign(ITKThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdImageFilter_Default.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdImageFilter_Default.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "59071590099d21dd439896592338bf95");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdImageFilter_Default.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Threshold settings 1
TEST_CASE("ITKImageProcessing::ITKThresholdImage: Threshold1", "[ITKImageProcessing][ITKThresholdImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Slice-Short.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Slice-Short.png";
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
    auto pLower = 25000;
    auto pUpper = 65535;
    auto pOutsideValue = 0.0;
    args.insertOrAssign(ITKThresholdImage::k_Lower_Key, std::make_any<float64>(pLower));
    args.insertOrAssign(ITKThresholdImage::k_Upper_Key, std::make_any<float64>(pUpper));
    args.insertOrAssign(ITKThresholdImage::k_OutsideValue_Key, std::make_any<float64>(pOutsideValue));
    args.insertOrAssign(ITKThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdImageFilter_Threshold1.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdImageFilter_Threshold1.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "f70a31938657e0163b82521af4f8e3b0");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdImageFilter_Threshold1.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Threshold settings 2
TEST_CASE("ITKImageProcessing::ITKThresholdImage: Threshold2", "[ITKImageProcessing][ITKThresholdImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Slice-Short.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Slice-Short.png";
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
    auto pLower = 0;
    auto pUpper = 25000;
    auto pOutsideValue = 25000;
    args.insertOrAssign(ITKThresholdImage::k_Lower_Key, std::make_any<float64>(pLower));
    args.insertOrAssign(ITKThresholdImage::k_Upper_Key, std::make_any<float64>(pUpper));
    args.insertOrAssign(ITKThresholdImage::k_OutsideValue_Key, std::make_any<float64>(pOutsideValue));
    args.insertOrAssign(ITKThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdImageFilter_Threshold2.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdImageFilter_Threshold2.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "898743e4ec2e75c0169d025fb29b7ead");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdImageFilter_Threshold2.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
