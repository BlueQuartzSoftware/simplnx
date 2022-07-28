#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ITKImageProcessing/Common/sitkCommon.hpp"
#include "ITKImageProcessing/Filters/ITKAdaptiveHistogramEqualizationImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

namespace fs = std::filesystem;

using namespace complex;

struct make_shared_enabler : public complex::Application
{
};

namespace ITKImageProcessingUnitTest
{

const std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
bool s_PluginsLoaded = false;
const FilterList* s_FilterList = nullptr;

void InitApplicationAndPlugins()
{
  if(!s_PluginsLoaded)
  {
    ITKImageProcessingUnitTest::app->loadPlugins(unit_test::k_BuildDir.view(), true);
    s_FilterList = Application::Instance()->getFilterList();
    s_PluginsLoaded = true;
  }
}

DataPath ConvertColorToGrayScale(DataStructure& dataStructure, DataPath inputGeometryPath, DataPath inputDataPath)
{
  const Uuid k_CorePluginId = *Uuid::FromString("65a0a3fc-8c93-5405-8ac6-182e7f313a69");

  const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("eb5a89c4-4e71-59b1-9719-d10a652d961e");
  const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_CorePluginId);

  // Parameter Keys
  constexpr StringLiteral k_ConversionAlgorithm_Key = "ConversionAlgorithm";
  constexpr StringLiteral k_ColorWeights_Key = "ColorWeights";
  constexpr StringLiteral k_ColorChannel_Key = "ColorChannel";
  constexpr StringLiteral k_InputDataArrayVector_Key = "InputDataArrayVector";
  constexpr StringLiteral k_CreateNewAttributeMatrix_Key = "CreateNewAttributeMatrix";
  constexpr StringLiteral k_OutputAttributeMatrixName_Key = "OutputAttributeMatrixName";
  constexpr StringLiteral k_OutputArrayPrefix_Key = "OutputArrayPrefix";

  auto filter = s_FilterList->createFilter(k_ColorToGrayScaleFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  args.insertOrAssign(k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign(k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign(k_ColorChannel_Key, std::make_any<Int32Parameter::ValueType>(0));
  args.insertOrAssign(k_InputDataArrayVector_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputDataPath}));
  args.insertOrAssign(k_CreateNewAttributeMatrix_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_OutputAttributeMatrixName_Key, std::make_any<DataGroupCreationParameter::ValueType>(DataPath({"Output Data Path"})));
  args.insertOrAssign(k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>("GrayScale_"));

  auto preflightResult = filter->preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter->execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  return inputGeometryPath.createChildPath(fmt::format("GrayScale_{}", ITKTestBase::k_InputDataPath));
}

} // namespace ITKImageProcessingUnitTest

TEST_CASE("ITKAdaptiveHistogramEqualizationImageFilter(defaults)", "[ITKImageProcessing][ITKAdaptiveHistogramEqualizationImage][defaults]")
{
  ITKImageProcessingUnitTest::InitApplicationAndPlugins();

  DataStructure dataStructure;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  // Read Input Image
  {
    fs::path inputFilePath = fs::path(unit_test::k_DataDir.view()) / "JSONFilters" / "Input/sf4.png";
    REQUIRE(fs::exists(inputFilePath));
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);
  }

  // convert color image to grayscale image
  inputDataPath = ITKImageProcessingUnitTest::ConvertColorToGrayScale(dataStructure, inputGeometryPath, inputDataPath);

  // Run the ITK Filter that is being tested.
  {

    ITKAdaptiveHistogramEqualizationImage filter;

    Arguments args;
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Alpha_Key, std::make_any<Float32Parameter::ValueType>(0.5));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Beta_Key, std::make_any<Float32Parameter::ValueType>(0.5));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Radius_Key, std::make_any<VectorFloat32Parameter::ValueType>({10.0F, 19.0F, 10.0F}));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    fs::path baselineFilePath = fs::path(complex::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest.png";
    DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
    DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
    DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
    Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
    Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 2e-3);
    //  COMPLEX_RESULT_REQUIRE_VALID(compareResult);
  }
}

TEST_CASE("ITKAdaptiveHistogramEqualizationImageFilter(histo)", "[ITKImageProcessing][ITKAdaptiveHistogramEqualizationImage][histo]")
{
  ITKImageProcessingUnitTest::InitApplicationAndPlugins();

  DataStructure dataStructure;

  DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  DataPath inputDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_InputDataPath);
  DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
  DataPath outputDataPath = cellDataPath.createChildPath(ITKTestBase::k_OutputDataPath);

  { // Start Image Comparison Scope
    fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/sf4.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);
    COMPLEX_RESULT_REQUIRE_VALID(imageReadResult);
  } // End Image Comparison Scope

  // convert color image to grayscale image
  inputDataPath = ITKImageProcessingUnitTest::ConvertColorToGrayScale(dataStructure, inputGeometryPath, inputDataPath);

  // Run the ITK Filter that is being tested.
  {
    ITKAdaptiveHistogramEqualizationImage filter;

    Arguments args;
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_OutputImageDataPath_Key, std::make_any<DataPath>(outputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Radius_Key, std::make_any<VectorFloat32Parameter::ValueType>({10.0F, 10.0F, 10.0F}));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Alpha_Key, std::make_any<Float32Parameter::ValueType>(1.0));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Beta_Key, std::make_any<Float32Parameter::ValueType>(0.25));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    fs::path baselineFilePath = fs::path(complex::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest2.png";
    DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
    DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);
    DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
    Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);
    Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, 1e-5);
    COMPLEX_RESULT_REQUIRE_VALID(compareResult);
  }

  {
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/align_sections_feature_centroid.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(result.value());

    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}
