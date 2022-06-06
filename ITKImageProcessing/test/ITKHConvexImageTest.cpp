#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKHConvexImage.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKHConvexImageFilter(HConvex)", "[ITKImageProcessing][ITKHConvexImage][HConvex]")
{
  DataStructure ds;
  ITKHConvexImage filter;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/RA-Short.nrrd";
  Result<> imageReadResult = ITKTestBase::ReadImage(ds, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
  COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);

  Arguments args;
  args.insertOrAssign(ITKHConvexImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
  args.insertOrAssign(ITKHConvexImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
  args.insertOrAssign(ITKHConvexImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
  args.insertOrAssign(ITKHConvexImage::k_Height_Key, std::make_any<Float64Parameter::ValueType>(10000));

  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  std::string md5Hash = ITKTestBase::ComputeMd5Hash(ds, outputDataPath);
  REQUIRE(md5Hash == "f3a7b95a51710d51b3b73e0eb77eb1eb");
}
