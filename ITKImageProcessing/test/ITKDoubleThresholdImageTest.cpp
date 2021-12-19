#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKDoubleThresholdImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Default parameter settings
TEST_CASE("ITKImageProcessing::ITKDoubleThresholdImage: DoubleThreshold1", "[ITKImageProcessing][ITKDoubleThresholdImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKDoubleThresholdImage filter;
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
    auto pThreshold1 = 0.0;
    auto pThreshold2 = 1.0;
    auto pThreshold3 = 254.0;
    auto pThreshold4 = 255.0;
    auto pInsideValue = 1u;
    auto pOutsideValue = 0u;
    auto pFullyConnected = false;
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold1_Key, std::make_any<float64>(pThreshold1));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold2_Key, std::make_any<float64>(pThreshold2));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold3_Key, std::make_any<float64>(pThreshold3));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold4_Key, std::make_any<float64>(pThreshold4));
    args.insertOrAssign(ITKDoubleThresholdImage::k_InsideValue_Key, std::make_any<int32>(pInsideValue));
    args.insertOrAssign(ITKDoubleThresholdImage::k_OutsideValue_Key, std::make_any<int32>(pOutsideValue));
    args.insertOrAssign(ITKDoubleThresholdImage::k_FullyConnected_Key, std::make_any<bool>(pFullyConnected));
    args.insertOrAssign(ITKDoubleThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKDoubleThresholdImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKDoubleThresholdImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_DoubleThresholdImageFilter_DoubleThreshold1.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_DoubleThresholdImageFilter_DoubleThreshold1.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "dbd0ea7d6f16bb93e9c688cb0f1bfd85");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "DoubleThresholdImageFilter_DoubleThreshold1.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// double threshold 2D
TEST_CASE("ITKImageProcessing::ITKDoubleThresholdImage: DoubleThreshold2", "[ITKImageProcessing][ITKDoubleThresholdImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKDoubleThresholdImage filter;
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
    auto pThreshold1 = 0;
    auto pThreshold2 = 0;
    auto pThreshold3 = 3000;
    auto pThreshold4 = 2700;
    auto pInsideValue = 1u;
    auto pOutsideValue = 0u;
    auto pFullyConnected = false;
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold1_Key, std::make_any<float64>(pThreshold1));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold2_Key, std::make_any<float64>(pThreshold2));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold3_Key, std::make_any<float64>(pThreshold3));
    args.insertOrAssign(ITKDoubleThresholdImage::k_Threshold4_Key, std::make_any<float64>(pThreshold4));
    args.insertOrAssign(ITKDoubleThresholdImage::k_InsideValue_Key, std::make_any<int32>(pInsideValue));
    args.insertOrAssign(ITKDoubleThresholdImage::k_OutsideValue_Key, std::make_any<int32>(pOutsideValue));
    args.insertOrAssign(ITKDoubleThresholdImage::k_FullyConnected_Key, std::make_any<bool>(pFullyConnected));
    args.insertOrAssign(ITKDoubleThresholdImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKDoubleThresholdImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKDoubleThresholdImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_DoubleThresholdImageFilter_DoubleThreshold2.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_DoubleThresholdImageFilter_DoubleThreshold2.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "2c8fc2345ccfa980ef42aef5910efaa3");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "DoubleThresholdImageFilter_DoubleThreshold2.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
