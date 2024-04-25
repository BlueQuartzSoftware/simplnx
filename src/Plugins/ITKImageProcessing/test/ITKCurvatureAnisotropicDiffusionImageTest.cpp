#include <catch2/catch.hpp>

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKCurvatureAnisotropicDiffusionImageFilter.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

TEST_CASE("ITKImageProcessing::ITKCurvatureAnisotropicDiffusionImageFilter(defaults)", "[ITKImageProcessing][ITKCurvatureAnisotropicDiffusionImage][defaults]")
{
  DataStructure dataStructure;
  const ITKCurvatureAnisotropicDiffusionImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_TimeStep_Key, std::make_any<Float64Parameter::ValueType>(0.01));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const fs::path baselineFilePath = fs::path(nx::core::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters_CurvatureAnisotropicDiffusionImageFilter_defaults.nrrd";
  const DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  const DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  const Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_BaselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 0.1);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult)
}

TEST_CASE("ITKImageProcessing::ITKCurvatureAnisotropicDiffusionImageFilter(longer)", "[ITKImageProcessing][ITKCurvatureAnisotropicDiffusionImage][longer]")
{
  DataStructure dataStructure;
  const ITKCurvatureAnisotropicDiffusionImage filter;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Float.nrrd";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  Arguments args;
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_InputImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_OutputImageArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_TimeStep_Key, std::make_any<Float64Parameter::ValueType>(0.01));
  args.insertOrAssign(ITKCurvatureAnisotropicDiffusionImage::k_NumberOfIterations_Key, std::make_any<UInt32Parameter::ValueType>(10));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const fs::path baselineFilePath = fs::path(nx::core::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters_CurvatureAnisotropicDiffusionImageFilter_longer.nrrd";
  const DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
  const DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  const DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
  const Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_BaselineDataPath);
  Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 0.1);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult)
}
