#include "OrientationAnalysis/Filters/ComputeFZQuaternionsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
DataStructure CreateDataStructure()
{
  DataStructure dataStructure;

  DataGroup* group = nx::core::DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
  DataGroup* scanData = nx::core::DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_SmallIn100ImageGeom, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  nx::core::SizeVec3 imageGeomDims = {100, 100, 2};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  std::vector<size_t> compDims = {4};
  std::vector<size_t> tupleDims = {100, 100, 2};

  std::string filePath = nx::core::unit_test::k_DataDir.str();

  std::string fileName = "/quats.raw";
  nx::core::ImportFromBinaryFile<float>(filePath + fileName, k_Quats, dataStructure, tupleDims, compDims, scanData->getId());

  fileName = "/fz_quats.raw";
  nx::core::ImportFromBinaryFile<float>(filePath + fileName, "FZ_QUATS_EXEMPLAR", dataStructure, tupleDims, compDims, scanData->getId());

  Int32Array* phases_data = nx::core::UnitTest::CreateTestDataArray<int32>(dataStructure, k_Phases, tupleDims, {1}, scanData->getId());
  phases_data->fill(1);

  // Add in another group that is just information about the grid data.
  DataGroup* phaseGroup = nx::core::DataGroup::Create(dataStructure, k_PhaseData, group->getId());
  UInt32Array* laueClass = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_LaueClass, {2}, {1}, phaseGroup->getId());
  (*laueClass)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*laueClass)[1] = ebsdlib::CrystalStructure::Cubic_High;

  return dataStructure;
}

void MessageHandlerFunction(const IFilter::Message& message)
{
}

} // namespace

TEST_CASE("OrientationAnalysis::ComputeFZQuaternions", "[OrientationAnalysis][ComputeFZQuaternions]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFZQuaternionsFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  DataPath scanDataPath = DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData});
  // Create default Parameters for the filter.

  args.insertOrAssign(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Quats)));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key, std::make_any<std::string>(k_FZQuats));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_PhaseData, k_LaueClass})));

  args.insertOrAssign(ComputeFZQuaternionsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare Results
  auto generatedFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath(k_FZQuats));
  auto exemplarFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath("FZ_QUATS_EXEMPLAR"));
  UnitTest::CompareArrays<float32>(dataStructure, scanDataPath.createChildPath(k_FZQuats), scanDataPath.createChildPath("FZ_QUATS_EXEMPLAR"));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFZQuaternionsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFZQuaternionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFZQuaternionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFZQuaternionsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeFZQuaternionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeFZQuaternionsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key) == "TestArray");
    }
  }
}
