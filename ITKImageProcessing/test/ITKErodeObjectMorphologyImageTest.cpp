#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKErodeObjectMorphologyImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Erode a float image
TEST_CASE("ITKImageProcessing::ITKErodeObjectMorphologyImage: float", "[ITKImageProcessing][ITKErodeObjectMorphologyImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKErodeObjectMorphologyImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Slice-Float.nrrd
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Slice-Float.nrrd";
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
    auto pKernelRadius = 1;
    auto pKernelType = itk::simple::sitkBall;
    auto pObjectValue = 1;
    auto pBackgroundValue = 0;
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(pKernelType));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_ObjectValue_Key, std::make_any<float64>(pObjectValue));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_BackgroundValue_Key, std::make_any<float64>(pBackgroundValue));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_KernelRadius_Key, std::make_any<VectorFloat32Parameter::ValueType>(pKernelRadius));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ErodeObjectMorphologyImageFilter_float.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ErodeObjectMorphologyImageFilter_float.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    error = complex::ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);
    REQUIRE(error == 0);
    Result<> result = complex::ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.01);
    if(result.invalid())
    {
      for(const auto& err : result.errors())
      {
        std::cout << err.code << ": " << err.message << std::endl;
      }
    }
    REQUIRE(result.valid() == true);
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ErodeObjectMorphologyImageFilter_float.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
// Erode a short image
TEST_CASE("ITKImageProcessing::ITKErodeObjectMorphologyImage: short", "[ITKImageProcessing][ITKErodeObjectMorphologyImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKErodeObjectMorphologyImage filter;
  DataStructure ds;
  // Read the input image: Input/RA-Slice-Short.nrrd
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/RA-Slice-Short.nrrd";
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
    auto pKernelRadius = 1;
    auto pKernelType = itk::simple::sitkBall;
    auto pObjectValue = 1;
    auto pBackgroundValue = 0;
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(pKernelType));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_ObjectValue_Key, std::make_any<float64>(pObjectValue));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_BackgroundValue_Key, std::make_any<float64>(pBackgroundValue));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_KernelRadius_Key, std::make_any<VectorFloat32Parameter::ValueType>(pKernelRadius));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKErodeObjectMorphologyImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_ErodeObjectMorphologyImageFilter_short.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_ErodeObjectMorphologyImageFilter_short.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    error = complex::ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);
    REQUIRE(error == 0);
    Result<> result = complex::ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.01);
    if(result.invalid())
    {
      for(const auto& err : result.errors())
      {
        std::cout << err.code << ": " << err.message << std::endl;
      }
    }
    REQUIRE(result.valid() == true);
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "ErodeObjectMorphologyImageFilter_short.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
