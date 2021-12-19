#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKPatchBasedDenoisingImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

// Simply run with default settings
TEST_CASE("ITKImageProcessing::ITKPatchBasedDenoisingImage: default", "[ITKImageProcessing][ITKPatchBasedDenoisingImage]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKPatchBasedDenoisingImage filter;
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
    auto pKernelBandwidthSigma = 400.0;
    auto pPatchRadius = 4u;
    auto pNumberOfIterations = 1u;
    auto pNumberOfSamplePatches = 200u;
    auto pSampleVariance = 400.0;
    auto pNoiseModel = itk::simple::PatchBasedDenoisingImageFilter::NOMODEL;
    auto pNoiseSigma = 0.0;
    auto pNoiseModelFidelityWeight = 0.0;
    auto pAlwaysTreatComponentsAsEuclidean = false;
    auto pKernelBandwidthEstimation = false;
    auto pKernelBandwidthMultiplicationFactor = 1.0;
    auto pKernelBandwidthUpdateFrequency = 3u;
    auto pKernelBandwidthFractionPixelsForEstimation = 0.2;
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NoiseModel_Key, std::make_any<ChoicesParameter::ValueType>(pNoiseModel));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_KernelBandwidthSigma_Key, std::make_any<float64>(pKernelBandwidthSigma));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_PatchRadius_Key, std::make_any<float64>(pPatchRadius));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NumberOfIterations_Key, std::make_any<float64>(pNumberOfIterations));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NumberOfSamplePatches_Key, std::make_any<float64>(pNumberOfSamplePatches));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_SampleVariance_Key, std::make_any<float64>(pSampleVariance));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NoiseSigma_Key, std::make_any<float64>(pNoiseSigma));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NoiseModelFidelityWeight_Key, std::make_any<float64>(pNoiseModelFidelityWeight));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_AlwaysTreatComponentsAsEuclidean_Key, std::make_any<bool>(pAlwaysTreatComponentsAsEuclidean));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_KernelBandwidthEstimation_Key, std::make_any<bool>(pKernelBandwidthEstimation));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_KernelBandwidthMultiplicationFactor_Key, std::make_any<float64>(pKernelBandwidthMultiplicationFactor));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_KernelBandwidthUpdateFrequency_Key, std::make_any<float64>(pKernelBandwidthUpdateFrequency));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_KernelBandwidthFractionPixelsForEstimation_Key, std::make_any<float64>(pKernelBandwidthFractionPixelsForEstimation));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_SelectedCellArrayPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKPatchBasedDenoisingImage::k_NewCellArrayName_Key, std::make_any<DataPath>(outputDataPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.valid());
    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    REQUIRE(executeResult.result.valid());
  } // End Scope Section

  // Write the output data to a file, read and compare to baseline image
  {
    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / "test/BasicFilters_PatchBasedDenoisingImageFilter_default.nrrd";
    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});
    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);
    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);
    REQUIRE(error == 0);
    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / "JSONFilters/Baseline/BasicFilters_PatchBasedDenoisingImageFilter_default.nrrd";
    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});
    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);
    error = complex::ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);
    REQUIRE(error == 0);
    Result<> result = complex::ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 0.9);
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
    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / "test" / "PatchBasedDenoisingImageFilter_default.h5";
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    REQUIRE(result.valid() == true);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err == 0);
  }
#endif
}
