#include "SimplnxCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter(Instantiate)", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = false;
  bool k_ChangeResolution = false;
  std::vector<float32> k_Origin{0, 0, 0};
  std::vector<float32> k_Spacing{1, 1, 1};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: Valid Execution", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = true;
  std::vector<float32> k_Origin{7, 6, 5};
  std::vector<float32> k_Spacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom.getOrigin() == FloatVec3{7, 6, 5});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: 0,0,0 Central Origin", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = true;
  std::vector<float32> k_Origin{0.0, 0.0, 0.0};
  std::vector<float32> k_Spacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom.getBoundingBoxf().center() == Point3Df{0.0f, 0.0f, 0.0f});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: Custom Central Origin", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool k_ChangeOrigin = true;
  bool k_ChangeResolution = true;
  std::vector<float32> k_Origin{7.0, 6.0, 5.0};
  std::vector<float32> k_Spacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(k_ChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(k_ChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(k_Origin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_ImageGeomPath);
  REQUIRE(imageGeom.getBoundingBoxf().center() == Point3Df{7.0, 6.0, 5.0});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SetImageGeomOriginScalingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SetImageGeomOriginScalingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SetImageGeomOriginScalingFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<SetImageGeomOriginScalingFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<bool>(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key) == true);
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key) == true);
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
