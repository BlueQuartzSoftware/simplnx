#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKGrayscaleMorphologicalOpeningImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKGrayscaleMorphologicalOpeningImageFilter(GrayscaleMorphologicalOpening)", "[ITKImageProcessing][ITKGrayscaleMorphologicalOpeningImage][GrayscaleMorphologicalOpening]")
{
  DataStructure ds;
  ITKGrayscaleMorphologicalOpeningImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkBall));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "867de5ed8cf49c4657e1545bd57f2c23");
}

TEST_CASE("ITKGrayscaleMorphologicalOpeningImageFilter(GrayscaleMorphologicalOpeningVectorRadius1)",
          "[ITKImageProcessing][ITKGrayscaleMorphologicalOpeningImage][GrayscaleMorphologicalOpeningVectorRadius1]")
{
  DataStructure ds;
  ITKGrayscaleMorphologicalOpeningImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{20, 5, 2}));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkCross));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "5651a92320cfd9f01be4463131a4e573");
}

TEST_CASE("ITKGrayscaleMorphologicalOpeningImageFilter(GrayscaleMorphologicalOpeningVectorRadius2)",
          "[ITKImageProcessing][ITKGrayscaleMorphologicalOpeningImage][GrayscaleMorphologicalOpeningVectorRadius2]")
{
  DataStructure ds;
  ITKGrayscaleMorphologicalOpeningImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{20, 5}));
  args.insertOrAssign(ITKGrayscaleMorphologicalOpeningImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkBox));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "0a5ac0dbca31e1b92eb6d48e990582a7");
}
