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
    "(05) AM XCT Porosity Segmentation/(05) AM XCT Porosity Segmentation.d3dpipeline",
    "(06) AM Powder-Bed Layer Inspection/(06) AM Powder-Bed Layer Inspection.d3dpipeline",
    "(07) AM Melt-Pool and Track Inspection/(07) AM Melt-Pool and Track Inspection.d3dpipeline",
    "(08) Powder Particle Watershed Segmentation/(08) Powder Particle Watershed Segmentation.d3dpipeline",
    "(09) Microstructure Watershed Segmentation/(09) Microstructure Watershed Segmentation.d3dpipeline",
    "(10) Binary Mask Repair and Skeletonization/(10) Binary Mask Repair and Skeletonization.d3dpipeline",
    "(11) Grayscale Surface-Defect Morphology/(11) Grayscale Surface-Defect Morphology.d3dpipeline",
    "(12) Pore Distance and Wall-Thickness Metrology/(12) Pore Distance and Wall-Thickness Metrology.d3dpipeline",
    "(13) Denoising and Edge Detection/(13) Denoising and Edge Detection.d3dpipeline",
    "(14) Projection-Based Quality Summaries/(14) Projection-Based Quality Summaries.d3dpipeline",
    "(15) Radiography Intensity Calibration/(15) Radiography Intensity Calibration.d3dpipeline",
    "(16) Phase and Angle Field Transforms/(16) Phase and Angle Field Transforms.d3dpipeline",
    "(17) Scientific Volume Interoperability/(17) Scientific Volume Interoperability.d3dpipeline",
    "(18) Industrial XCT Format Import/(18) Industrial XCT Format Import.d3dpipeline",
    "(19) Serial-Section Stack Reconstruction/(19) Serial-Section Stack Reconstruction.d3dpipeline",
    "(20) Fiji Microscopy Montage Import/(20) Fiji Microscopy Montage Import.d3dpipeline",
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
