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
  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Test reading 2D & 3D image data
  fs::path exemplaryFilePath = fs::path(unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest/ExemplarySmallIN100.dream3d";
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

  ITKMhaFileReader filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplaryFilePath);
  Arguments args;

  std::string geomName = "ImageGeom";
  std::string amName = "CellData";
  std::string arrName = "ImageArray";
  std::string tMatrixName = "TransformationMatrix";
  DataPath geomPath{{geomName}};
  DataPath arrayPath{{geomName, amName, arrName}};
  DataPath tMatrixPath{{geomName, tMatrixName}};

  std::string exemplaryAMName = "Cell Data";
  std::string exemplaryArrName = "ImageData";
  std::string exemplaryTMatrixName = "TransformationMatrix";
  DataPath exemplaryGeomPath{{exemplaryGeomName}};
  DataPath exemplaryArrayPath{{exemplaryGeomName, exemplaryAMName, exemplaryArrName}};
  DataPath exemplaryTMatrixPath{{exemplaryGeomName, exemplaryTMatrixName}};

  args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
  args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, geomPath);
  args.insertOrAssign(ITKImageReader::k_CellDataName_Key, amName);
  args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, arrayPath);
  args.insertOrAssign(ITKMhaFileReader::k_ApplyImageTransformation, true);
  args.insertOrAssign(ITKMhaFileReader::k_SaveImageTransformationAsArray, true);
  args.insertOrAssign(ITKMhaFileReader::k_TransformationMatrixDataArrayPath_Key, tMatrixPath);

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(geomPath);
  REQUIRE(imageGeom != nullptr);

  const auto* exemplaryImageGeom = dataStructure.getDataAs<ImageGeom>(exemplaryGeomPath);
  REQUIRE(exemplaryImageGeom != nullptr);

  REQUIRE(imageGeom->getDimensions() == exemplaryImageGeom->getDimensions());
  REQUIRE(imageGeom->getOrigin() == exemplaryImageGeom->getOrigin());
  REQUIRE(imageGeom->getSpacing() == exemplaryImageGeom->getSpacing());

  const auto* dataArray = dataStructure.getDataAs<Float32Array>(arrayPath);
  REQUIRE(dataArray != nullptr);

  const auto* exemplaryDataArray = dataStructure.getDataAs<Float32Array>(exemplaryArrayPath);
  REQUIRE(exemplaryDataArray != nullptr);

  REQUIRE(dataArray->getTupleShape() == exemplaryDataArray->getTupleShape());
  REQUIRE(dataArray->getComponentShape() == exemplaryDataArray->getComponentShape());
  REQUIRE(std::equal(dataArray->begin(), dataArray->end(), exemplaryDataArray->begin(), exemplaryDataArray->end()));

  const auto* tMatrix = dataStructure.getDataAs<Float32Array>(tMatrixPath);
  REQUIRE(tMatrix != nullptr);

  const auto* exemplaryTMatrix = dataStructure.getDataAs<Float32Array>(exemplaryTMatrixPath);
  REQUIRE(exemplaryTMatrix != nullptr);

  REQUIRE(tMatrix->getTupleShape() == exemplaryTMatrix->getTupleShape());
  REQUIRE(tMatrix->getComponentShape() == exemplaryTMatrix->getComponentShape());
  REQUIRE(std::equal(tMatrix->begin(), tMatrix->end(), exemplaryTMatrix->begin(), exemplaryTMatrix->end()));
}
