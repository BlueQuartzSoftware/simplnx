#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Filters/ITKMhaFileReader.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKImageProcessing::ITKMhaFileReader: Read 2D & 3D Image Data", "[ITKImageProcessing][ITKMhaFileReader]")
{
  // Load plugins (this is needed because ITKMhaFileReader needs access to the ComplexCore plugin)
  const std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Test reading 2D & 3D image data
  const fs::path exemplaryFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest/ExemplarySmallIN100.dream3d";
  fs::path filePath;
  std::string exemplaryGeomName;
  SECTION("Test 2D Image Data")
  {
    filePath = fs::path(unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest/SmallIN100_073.mha";
    exemplaryGeomName = "ExemplarySmallIN100_073";
  }
  SECTION("Test 3D Image Data")
  {
    filePath = fs::path(unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest/SmallIN100.mha";
    exemplaryGeomName = "ExemplarySmallIN100";
  }

  const ITKMhaFileReader filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplaryFilePath);
  Arguments args;

  const std::string geomName = "ImageGeom";
  const std::string amName = "CellData";
  const std::string arrName = "ImageArray";
  const std::string tMatrixName = "TransformationMatrix";
  const DataPath geomPath{{geomName}};
  const DataPath arrayPath{{geomName, amName, arrName}};
  const DataPath tMatrixPath{{geomName, tMatrixName}};

  const std::string exemplaryAMName = "Cell Data";
  const std::string exemplaryArrName = "ImageData";
  const std::string exemplaryTMatrixName = "TransformationMatrix";
  const DataPath exemplaryGeomPath{{exemplaryGeomName}};
  const DataPath exemplaryArrayPath{{exemplaryGeomName, exemplaryAMName, exemplaryArrName}};
  const DataPath exemplaryTMatrixPath{{exemplaryGeomName, exemplaryTMatrixName}};

  args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
  args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, geomPath);
  args.insertOrAssign(ITKImageReader::k_CellDataName_Key, amName);
  args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, arrayPath);
  args.insertOrAssign(ITKMhaFileReader::k_ApplyImageTransformation, true);
  args.insertOrAssign(ITKMhaFileReader::k_SaveImageTransformationAsArray, true);
  args.insertOrAssign(ITKMhaFileReader::k_TransformationMatrixDataArrayPathKey, tMatrixPath);

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(geomPath);
  REQUIRE(imageGeomPtr != nullptr);

  const auto* exemplaryImageGeomPtr = dataStructure.getDataAs<ImageGeom>(exemplaryGeomPath);
  REQUIRE(exemplaryImageGeomPtr != nullptr);

  REQUIRE(imageGeomPtr->getDimensions() == exemplaryImageGeomPtr->getDimensions());
  REQUIRE(imageGeomPtr->getOrigin() == exemplaryImageGeomPtr->getOrigin());
  REQUIRE(imageGeomPtr->getSpacing() == exemplaryImageGeomPtr->getSpacing());

  const auto* dataArrayPtr = dataStructure.getDataAs<Float32Array>(arrayPath);
  REQUIRE(dataArrayPtr != nullptr);

  const auto* exemplaryDataArrayPtr = dataStructure.getDataAs<Float32Array>(exemplaryArrayPath);
  REQUIRE(exemplaryDataArrayPtr != nullptr);

  REQUIRE(dataArrayPtr->getTupleShape() == exemplaryDataArrayPtr->getTupleShape());
  REQUIRE(dataArrayPtr->getComponentShape() == exemplaryDataArrayPtr->getComponentShape());
  REQUIRE(std::equal(dataArrayPtr->begin(), dataArrayPtr->end(), exemplaryDataArrayPtr->begin(), exemplaryDataArrayPtr->end()));

  const auto* tMatrixPtr = dataStructure.getDataAs<Float32Array>(tMatrixPath);
  REQUIRE(tMatrixPtr != nullptr);

  const auto* exemplaryTMatrixPtr = dataStructure.getDataAs<Float32Array>(exemplaryTMatrixPath);
  REQUIRE(exemplaryTMatrixPtr != nullptr);

  REQUIRE(tMatrixPtr->getTupleShape() == exemplaryTMatrixPtr->getTupleShape());
  REQUIRE(tMatrixPtr->getComponentShape() == exemplaryTMatrixPtr->getComponentShape());
  REQUIRE(std::equal(tMatrixPtr->begin(), tMatrixPtr->end(), exemplaryTMatrixPtr->begin(), exemplaryTMatrixPtr->end()));
}
