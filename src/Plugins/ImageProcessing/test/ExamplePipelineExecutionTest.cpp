#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <atomic>
#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr std::array<std::string_view, 16> k_ExamplePipelineNames = {
    "05_AM_XCT_Porosity_Segmentation/05_AM_XCT_Porosity_Segmentation.d3dpipeline",
    "06_AM_Powder-Bed_Layer_Inspection/06_AM_Powder-Bed_Layer_Inspection.d3dpipeline",
    "07_AM_Melt-Pool_and_Track_Inspection/07_AM_Melt-Pool_and_Track_Inspection.d3dpipeline",
    "08_Powder_Particle_Watershed_Segmentation/08_Powder_Particle_Watershed_Segmentation.d3dpipeline",
    "09_Microstructure_Watershed_Segmentation/09_Microstructure_Watershed_Segmentation.d3dpipeline",
    "10_Binary_Mask_Repair_and_Skeletonization/10_Binary_Mask_Repair_and_Skeletonization.d3dpipeline",
    "11_Grayscale_Surface-Defect_Morphology/11_Grayscale_Surface-Defect_Morphology.d3dpipeline",
    "12_Pore_Distance_and_Wall-Thickness_Metrology/12_Pore_Distance_and_Wall-Thickness_Metrology.d3dpipeline",
    "13_Denoising_and_Edge_Detection/13_Denoising_and_Edge_Detection.d3dpipeline",
    "14_Projection-Based_Quality_Summaries/14_Projection-Based_Quality_Summaries.d3dpipeline",
    "15_Radiography_Intensity_Calibration/15_Radiography_Intensity_Calibration.d3dpipeline",
    "16_Phase_and_Angle_Field_Transforms/16_Phase_and_Angle_Field_Transforms.d3dpipeline",
    "17_Scientific_Volume_Interoperability/17_Scientific_Volume_Interoperability.d3dpipeline",
    "18_Industrial_XCT_Format_Import/18_Industrial_XCT_Format_Import.d3dpipeline",
    "19_Serial-Section_Stack_Reconstruction/19_Serial-Section_Stack_Reconstruction.d3dpipeline",
    "20_Fiji_Microscopy_Montage_Import/20_Fiji_Microscopy_Montage_Import.d3dpipeline",
};

usize CountOocArrays(const DataStructure& dataStructure)
{
  usize count = 0;
  for(const DataObject::IdType identifier : dataStructure.getAllDataObjectIds())
  {
    const auto* array = dataStructure.getDataAs<IDataArray>(identifier);
    if(array != nullptr && array->getDataFormat() == "HDF5-OOC")
    {
      count++;
    }
  }
  return count;
}
} // namespace

TEST_CASE("ImageProcessing::Real-world examples execute with the selected storage mode", "[ImageProcessing][ExamplePipelines][Execution]")
{
  UnitTest::LoadPlugins();
  const bool hasOocManager = Application::Instance()->getIOCollection().getManager("HDF5-OOC") != nullptr;
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 0);

  const fs::path pluginSourceDir(unit_test::k_SourceDir.str());
  const fs::path pipelineDir = pluginSourceDir / "pipelines";
  const fs::path runtimeDir(unit_test::k_BuildDir.str());

  const fs::path originalWorkingDirectory = fs::current_path();
  auto workingDirectoryGuard = MakeScopeGuard([&originalWorkingDirectory]() noexcept {
    std::error_code restoreError;
    fs::current_path(originalWorkingDirectory, restoreError);
  });
  fs::current_path(runtimeDir);

  for(const std::string_view pipelineName : k_ExamplePipelineNames)
  {
    CAPTURE(pipelineName);
    Result<Pipeline> pipelineResult = Pipeline::FromFile(pipelineDir / pipelineName);
    SIMPLNX_RESULT_REQUIRE_VALID(pipelineResult);

    DataStructure dataStructure;
    const std::atomic_bool shouldCancel = false;
    REQUIRE(pipelineResult.value().execute(dataStructure, shouldCancel));

    const usize oocArrayCount = CountOocArrays(dataStructure);
    if(hasOocManager)
    {
      REQUIRE(oocArrayCount > 0);
    }
    else
    {
      REQUIRE(oocArrayCount == 0);
    }
  }
}
