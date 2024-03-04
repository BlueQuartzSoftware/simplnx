#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include "ITKImageProcessing/Filters/ITKAdaptiveHistogramEqualizationImage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"
#include "ITKTestBase.hpp"

namespace fs = std::filesystem;

using namespace nx::core;

namespace ITKImageProcessingUnitTest
{
bool s_PluginsLoaded = false;
FilterList* s_FilterList = nullptr;

void InitApplicationAndPlugins()
{
  if(!s_PluginsLoaded)
  {
    Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
    s_FilterList = Application::Instance()->getFilterList();
    s_PluginsLoaded = true;
  }
}

DataPath ConvertColorToGrayScale(DataStructure& dataStructure, const DataPath& inputGeometryPath, DataPath inputDataPath)
{
  const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
  const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_SimplnxCorePluginId);

  // Parameter Keys
  constexpr StringLiteral k_ConversionAlgorithm_Key = "conversion_algorithm";
  constexpr StringLiteral k_ColorWeights_Key = "color_weights";
  constexpr StringLiteral k_ColorChannel_Key = "color_channel";
  constexpr StringLiteral k_InputDataArrayVector_Key = "input_data_array_vector";
  constexpr StringLiteral k_OutputArrayPrefix_Key = "output_array_prefix";

  auto filter = s_FilterList->createFilter(k_ColorToGrayScaleFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  args.insertOrAssign(k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign(k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign(k_ColorChannel_Key, std::make_any<Int32Parameter::ValueType>(0));
  args.insertOrAssign(k_InputDataArrayVector_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputDataPath}));
  args.insertOrAssign(k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>("GrayScale_"));

  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  return inputGeometryPath.createChildPath(fmt::format("GrayScale_{}", ITKTestBase::k_InputDataName));
}

} // namespace ITKImageProcessingUnitTest

TEST_CASE("ITKImageProcessing::ITKAdaptiveHistogramEqualizationImageFilter(defaults)", "[ITKImageProcessing][ITKAdaptiveHistogramEqualizationImage][defaults]")
{
  ITKImageProcessingUnitTest::InitApplicationAndPlugins();

  DataStructure dataStructure;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  // Read Input Image
  {
    const fs::path inputFilePath = fs::path(unit_test::k_DataDir.view()) / "JSONFilters" / "Input/sf4.png";
    REQUIRE(fs::exists(inputFilePath));
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  }

  // convert color image to grayscale image
  inputDataPath = ITKImageProcessingUnitTest::ConvertColorToGrayScale(dataStructure, cellDataPath, inputDataPath);

  // Run the ITK Filter that is being tested.
  {
    const ITKAdaptiveHistogramEqualizationImage filter;

    Arguments args;
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Alpha_Key, std::make_any<Float32Parameter::ValueType>(0.5f));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Beta_Key, std::make_any<Float32Parameter::ValueType>(0.5f));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(VectorUInt32Parameter::ValueType{10, 19, 10}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const fs::path baselineFilePath = fs::path(nx::core::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest.png";
    const DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
    const DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
    const DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
    const Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_BaselineDataPath);
    const Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 2e-3);
    //  SIMPLNX_RESULT_REQUIRE_VALID(compareResult)
  }
}

TEST_CASE("ITKImageProcessing::ITKAdaptiveHistogramEqualizationImageFilter(histo)", "[ITKImageProcessing][ITKAdaptiveHistogramEqualizationImage][histo]")
{
  ITKImageProcessingUnitTest::InitApplicationAndPlugins();

  DataStructure dataStructure;

  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});
  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
  DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataName);
  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;

  { // Start Image Comparison Scope
    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / "JSONFilters" / "Input/sf4.png";
    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_InputDataName);
    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)
  } // End Image Comparison Scope

  // convert color image to grayscale image
  inputDataPath = ITKImageProcessingUnitTest::ConvertColorToGrayScale(dataStructure, cellDataPath, inputDataPath);

  // Run the ITK Filter that is being tested.
  {
    ITKAdaptiveHistogramEqualizationImage filter;

    Arguments args;
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Alpha_Key, std::make_any<Float32Parameter::ValueType>(1.0f));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Beta_Key, std::make_any<Float32Parameter::ValueType>(0.25f));
    args.insertOrAssign(ITKAdaptiveHistogramEqualizationImage::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(VectorUInt32Parameter::ValueType{10, 10, 10}));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const fs::path baselineFilePath = fs::path(nx::core::unit_test::k_DataDir.view()) / "JSONFilters/Baseline/BasicFilters/ITKAdaptiveHistogramEqualizationFilterTest2.png";
    const DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});
    const DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataName);
    const DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);
    const Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, ITKTestBase::k_ImageCellDataName, ITKTestBase::k_BaselineDataPath);
    const Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName), 1e-5);
    SIMPLNX_RESULT_REQUIRE_VALID(compareResult)
  }

  {
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(fmt::format("{}/itk_adaptive_align_histograms.dream3d", unit_test::k_BinaryTestOutputDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5)
  }
}
