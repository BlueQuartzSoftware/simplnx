#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;

namespace fs = std::filesystem;

namespace CreateImageGeometryUnitTest
{

const fs::path k_DataDir = "test/data";
const fs::path k_TestFile = "CreateImageGeometry_Test.dream3d";

} // namespace CreateImageGeometryUnitTest

TEST_CASE("SimplnxCore::CreateImageGeometryFilter", "[SimplnxCore]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataStructure = nx::core::UnitTest::CreateAllPrimitiveTypes(imageDims);
  //  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Complexk_LevelZero);
  //  DataGroup* levelOneGroup = DataGroup::Create(dataStructure, Complexk_LevelOne, levelZeroGroup->getId());
  const std::string k_ImageGeometryName("[Image Geometry]");
  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne, k_ImageGeometryName});
  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(CreateImageGeometryFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(selectedDataGroupPath));
  args.insertOrAssign(CreateImageGeometryFilter::k_Dimensions_Key, std::make_any<std::vector<uint64_t>>(inputDims));
  args.insertOrAssign(CreateImageGeometryFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageOrigin));
  args.insertOrAssign(CreateImageGeometryFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageSpacing));

  CreateImageGeometryFilter filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Execute the filter and check the result
  executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid() == false);

#if 0
  fs::path filePath = SIMPLNX_BUILD_DIR / CreateImageGeometryUnitTest::k_DataDir / CreateImageGeometryUnitTest::k_TestFile;

  H5::FileWriter::ResultType result = H5::FileWriter::CreateFile(filePath);
  REQUIRE(result.valid());

  H5::FileWriter& fileWriter = *(result.value());
  REQUIRE(fileWriter.isValid());

  herr_t err;
  err = dataStructure.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::CreateImageGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateImageGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateImageGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateImageGeometryFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateImageGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(CreateImageGeometryFilter::k_GeometryDataPath_Key) == DataPath({"DataContainer"}));
      // Complex type (UInt64Vec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
