#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKBinaryProjectionImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// run with some defaults
TEST_CASE("ITKImageProcessing::ITKBinaryProjectionImage: defaults", "[ITKImageProcessing][ITKBinaryProjectionImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKBinaryProjectionImage filter;
  DataStructure ds;
  // Read the input image: Input/2th_cthead1.mha
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/2th_cthead1.mha";
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
    auto pProjectionDimension = 0u;
    auto pForegroundValue = 1.0;
    auto pBackgroundValue = 0.0;
    args.insertOrAssign(ITKBinaryProjectionImage::k_ProjectionDimension_Key, std::make_any<float64>(pProjectionDimension));
    args.insertOrAssign(ITKBinaryProjectionImage::k_ForegroundValue_Key, std::make_any<float64>(pForegroundValue));
    args.insertOrAssign(ITKBinaryProjectionImage::k_BackgroundValue_Key, std::make_any<float64>(pBackgroundValue));
    args.insertOrAssign(ITKBinaryProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKBinaryProjectionImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKBinaryProjectionImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_BinaryProjectionImageFilter_defaults.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_BinaryProjectionImageFilter_defaults.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "3fc3603b27bf51df592190227d6cd6ed");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "BinaryProjectionImageFilter_defaults.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// project in the 2rd dimension
TEST_CASE("ITKImageProcessing::ITKBinaryProjectionImage: another_dimension", "[ITKImageProcessing][ITKBinaryProjectionImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKBinaryProjectionImage filter;
  DataStructure ds;
  // Read the input image: Input/WhiteDots.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/WhiteDots.png";
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
    auto pProjectionDimension = 1;
    auto pForegroundValue = 255;
    auto pBackgroundValue = 0.0;
    args.insertOrAssign(ITKBinaryProjectionImage::k_ProjectionDimension_Key, std::make_any<float64>(pProjectionDimension));
    args.insertOrAssign(ITKBinaryProjectionImage::k_ForegroundValue_Key, std::make_any<float64>(pForegroundValue));
    args.insertOrAssign(ITKBinaryProjectionImage::k_BackgroundValue_Key, std::make_any<float64>(pBackgroundValue));
    args.insertOrAssign(ITKBinaryProjectionImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKBinaryProjectionImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKBinaryProjectionImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_BinaryProjectionImageFilter_another_dimension.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_BinaryProjectionImageFilter_another_dimension.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "827f263ef9fb63d05499d14fcef32f60");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "BinaryProjectionImageFilter_another_dimension.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
