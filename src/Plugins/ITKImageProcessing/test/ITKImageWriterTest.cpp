#include <catch2/catch.hpp>

#include "ITKImageProcessing/Filters/ITKImageWriter.hpp"
#include "ITKImageProcessing/ITKImageProcessingPlugin.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

#include "complex/Core/Application.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ITKImageProcessing::ITKImageWriter: Write PNG", "[ITKImageProcessing][ITKImageWriter]")
{
  Application app;

  ITKImageWriter filter;
  Arguments args;

  fs::path dream3dFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/data/PngTest.dream3d";

  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(dream3dFilePath);
  COMPLEX_RESULT_REQUIRE_VALID(dataStructureResult);

  DataStructure dataStructure = std::move(dataStructureResult.value());

  DataPath arrayPath{{"ImageGeom", "ImageArray"}};
  DataPath imagePath = arrayPath.getParent();

  fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "PngTestOutput.png";

  args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(imagePath));
  args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(arrayPath));
  args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_XYPlane));

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
}
