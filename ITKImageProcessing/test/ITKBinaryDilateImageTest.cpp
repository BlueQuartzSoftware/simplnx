#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKBinaryDilateImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKBinaryDilateImageFilter(BinaryDilate)", "[ITKImageProcessing][ITKBinaryDilateImage][BinaryDilate]")
{
  DataStructure ds;
  ITKBinaryDilateImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKBinaryDilateImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{1, 1, 1}));
  args.insertOrAssign(ITKBinaryDilateImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkBall));
  args.insertOrAssign(ITKBinaryDilateImage::k_ForegroundValue_Key, std::make_any<Float64Parameter::ValueType>(255.0));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "9eef659f21dab5eb49e0f715a5d9a21b");
}

TEST_CASE("ITKBinaryDilateImageFilter(BinaryDilateVectorRadius)", "[ITKImageProcessing][ITKBinaryDilateImage][BinaryDilateVectorRadius]")
{
  DataStructure ds;
  ITKBinaryDilateImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath outputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/STAPLE1.png";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKBinaryDilateImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKBinaryDilateImage::k_KernelRadius_Key, std::make_any<VectorParameter<uint32>::ValueType>(std::vector<uint32>{20, 1}));
  args.insertOrAssign(ITKBinaryDilateImage::k_KernelType_Key, std::make_any<ChoicesParameter::ValueType>(itk::simple::sitkBox));
  args.insertOrAssign(ITKBinaryDilateImage::k_ForegroundValue_Key, std::make_any<Float64Parameter::ValueType>(255));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "99108c735fe9727bca09ca28a42827d3");
}
