#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKBinaryMorphologicalOpeningImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Test binary morphological opening
TEST_CASE("ITKImageProcessing::ITKBinaryMorphologicalOpeningImage: BinaryMorphologicalOpening", "[ITKImageProcessing][ITKBinaryMorphologicalOpeningImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKBinaryMorphologicalOpeningImage filter;
  DataStructure ds;
  // Read the input image: Input/STAPLE1.png
  {
    Arguments args;
    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters" / "Input/STAPLE1.png";
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
    auto pForegroundValue = 255;
    auto pBackgroundValue = 0.0;
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(pKernelType));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_BackgroundValue_Key, std::make_any<float64>(pBackgroundValue));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_ForegroundValue_Key, std::make_any<float64>(pForegroundValue));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_KernelRadius_Key, std::make_any<VectorFloat32Parameter::ValueType>(pKernelRadius));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKBinaryMorphologicalOpeningImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_BinaryMorphologicalOpeningImageFilter_BinaryMorphologicalOpening.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath =
        fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_BinaryMorphologicalOpeningImageFilter_BinaryMorphologicalOpening.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    // Compare md5 hash of final image
    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
    REQUIRE(md5Hash == "bc97d7cbf9d3779070373f7a28b932a2");
  }
#if 0
  {
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "BinaryMorphologicalOpeningImageFilter_BinaryMorphologicalOpening.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
