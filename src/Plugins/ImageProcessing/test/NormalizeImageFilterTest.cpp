#include "ImageProcessing/Filters/NormalizeImageFilter.hpp"
#include "ItkGoldenTestUtils.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>

using namespace nx::core;

TEST_CASE("ImageProcessing::C02 NormalizeImage component contract", "[ImageProcessing][NormalizeImageFilter][ComponentContract][C02]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const usize components = GENERATE(usize{1}, usize{3});
  CAPTURE(components);

  DataStructure dataStructure;
  auto* imagePtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imagePtr != nullptr);
  imagePtr->setDimensions({3, 1, 1});
  const ShapeType tupleShape{1, 1, 3};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "Cell Data", tupleShape, imagePtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imagePtr->setCellData(*cellDataPtr);

  const DataPath inputPath({"Image", "Cell Data", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(dataStructure, inputPath, tupleShape, ShapeType{components});
  REQUIRE(store != nullptr);
  auto* inputPtr = Float32Array::Create(dataStructure, "Input", store, cellDataPtr->getId());
  REQUIRE(inputPtr != nullptr);
  REQUIRE(imagePtr->getNumberOfCells() == 3);
  REQUIRE(inputPtr->getNumberOfTuples() == 3);
  REQUIRE(inputPtr->getNumberOfComponents() == components);

  const NormalizeImageFilter filter;
  Arguments args;
  args.insertOrAssign(NormalizeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(NormalizeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(NormalizeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflight = filter.preflight(dataStructure, args);
  if(components == 3)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
    REQUIRE(preflight.outputActions.errors().size() == 1);
    REQUIRE(preflight.outputActions.errors()[0].code == FilterParameter::Constants::k_Validate_TupleShapeValue);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    (*inputPtr)[0] = 1.0F;
    (*inputPtr)[1] = 2.0F;
    (*inputPtr)[2] = 3.0F;
    const auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const DataPath outputPath({"Image", "Cell Data", "Output"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(outputPath));
    const auto& output = dataStructure.getDataRefAs<Float64Array>(outputPath);
    REQUIRE(output.getSize() == 3);
    // Mean is 2 and sample standard deviation is 1.
    REQUIRE(output[0] == Approx(-1.0).margin(1.0e-12));
    REQUIRE(output[1] == Approx(0.0).margin(1.0e-12));
    REQUIRE(output[2] == Approx(1.0).margin(1.0e-12));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKNormalizeImageTest.cpp(defaults): read
// Ramp-Up-Short.nrrd (int16) through OUR reader, run OUR NormalizeImageFilter (AlwaysFloat64 output, no params),
// then compare our float64 output to ITK's committed baseline .nrrd (type: double, SAME reader) at the ITK test's
// tolerance 0.0001 via ip_golden::CompareImages. The ITK `vector` case (VM1111Shrink-RGB.png) is SKIPPED as an
// RGB/vector input (plan Sec.5; it is also disabled in the ITK source).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::NormalizeImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][NormalizeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Up-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(NormalizeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(NormalizeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(NormalizeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  NormalizeImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_NormalizeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
