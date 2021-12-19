#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKThresholdMaximumConnectedComponentsImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// 2D
TEST_CASE("ITKImageProcessing::ITKThresholdMaximumConnectedComponentsImage: default", "[ITKImageProcessing][ITKThresholdMaximumConnectedComponentsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdMaximumConnectedComponentsImage filter;
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
    auto pMinimumObjectSizeInPixels = 0u;
    auto pUpperBoundary = std::numeric_limits<double>::max();
    auto pInsideValue = 1u;
    auto pOutsideValue = 0u;
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_MinimumObjectSizeInPixels_Key, std::make_any<float64>(pMinimumObjectSizeInPixels));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_UpperBoundary_Key, std::make_any<float64>(pUpperBoundary));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_InsideValue_Key, std::make_any<int32>(pInsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_OutsideValue_Key, std::make_any<int32>(pOutsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_default.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath =
        fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_default.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "c84b75c78c33844251a1095d9cbcffb9");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdMaximumConnectedComponentsImageFilter_default.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// 2D
TEST_CASE("ITKImageProcessing::ITKThresholdMaximumConnectedComponentsImage: parameters", "[ITKImageProcessing][ITKThresholdMaximumConnectedComponentsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdMaximumConnectedComponentsImage filter;
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
    auto pMinimumObjectSizeInPixels = 40;
    auto pUpperBoundary = 150;
    auto pInsideValue = 1u;
    auto pOutsideValue = 0u;
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_MinimumObjectSizeInPixels_Key, std::make_any<float64>(pMinimumObjectSizeInPixels));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_UpperBoundary_Key, std::make_any<float64>(pUpperBoundary));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_InsideValue_Key, std::make_any<int32>(pInsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_OutsideValue_Key, std::make_any<int32>(pOutsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_parameters.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath =
        fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_parameters.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "27c6cf8494fcc4e414f1c420e7a9ca6f");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdMaximumConnectedComponentsImageFilter_parameters.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// 3D-float
TEST_CASE("ITKImageProcessing::ITKThresholdMaximumConnectedComponentsImage: float", "[ITKImageProcessing][ITKThresholdMaximumConnectedComponentsImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKThresholdMaximumConnectedComponentsImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Float.nrrd
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Float.nrrd";
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
    auto pMinimumObjectSizeInPixels = 0u;
    auto pUpperBoundary = std::numeric_limits<double>::max();
    auto pInsideValue = 1u;
    auto pOutsideValue = 0u;
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_MinimumObjectSizeInPixels_Key, std::make_any<float64>(pMinimumObjectSizeInPixels));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_UpperBoundary_Key, std::make_any<float64>(pUpperBoundary));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_InsideValue_Key, std::make_any<int32>(pInsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_OutsideValue_Key, std::make_any<int32>(pOutsideValue));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKThresholdMaximumConnectedComponentsImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_float.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath =
        fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ThresholdMaximumConnectedComponentsImageFilter_float.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "e475b27bd0dd66ede330c4eab93c17e9");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ThresholdMaximumConnectedComponentsImageFilter_float.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
