#include "SimplnxCore/Filters/SetImageGeomOriginScalingFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>

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

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: Signed Central Origin", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  const bool changeSpacing = GENERATE(false, true);
  const bool mixedSigns = GENERATE(false, true);
  DataStructure dataStructure;
  const DataPath geometryPath({"Image Geometry"});
  auto* geometry = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(geometry != nullptr);
  geometry->setDimensions({2, 2, 2});
  geometry->setOrigin({10.0f, 20.0f, 30.0f});
  geometry->setSpacing({1.0f, 1.0f, 1.0f});

  const std::vector<float32> requestedCenter = mixedSigns ? std::vector<float32>{-5.0f, 3.0f, -1.0f} : std::vector<float32>{-5.0f, -3.0f, -1.0f};
  // Two cells give half-extents (1,1,1), or (2,3,4) with the new spacing.
  const FloatVec3 expectedOrigin =
      changeSpacing ? (mixedSigns ? FloatVec3{-7.0f, 0.0f, -5.0f} : FloatVec3{-7.0f, -6.0f, -5.0f}) : (mixedSigns ? FloatVec3{-6.0f, 2.0f, -2.0f} : FloatVec3{-6.0f, -4.0f, -2.0f});
  Arguments args;
  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(changeSpacing));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(requestedCenter));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{2.0f, 3.0f, 4.0f}));

  CAPTURE(changeSpacing, mixedSigns);
  SetImageGeomOriginScalingFilter filter;
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CHECK(geometry->getOrigin() == expectedOrigin);
  const auto center = geometry->getBoundingBoxf().center();
  for(usize axis = 0; axis < 3; axis++)
  {
    CHECK(center[axis] == requestedCenter[axis]);
  }
  CHECK(geometry->getSpacing() == (changeSpacing ? FloatVec3{2.0f, 3.0f, 4.0f} : FloatVec3{1.0f, 1.0f, 1.0f}));
}

namespace
{
ImageGeom* MakeValidationImage(DataStructure& ds, const SizeVec3& dims = {2, 2, 2}, const FloatVec3& spacing = {1, 1, 1}, const FloatVec3& origin = {0, 0, 0})
{
  auto* geom = ImageGeom::Create(ds, "ValidationImage");
  REQUIRE(geom != nullptr);
  geom->setDimensions(dims);
  geom->setSpacing(spacing);
  geom->setOrigin(origin);
  auto* cells = AttributeMatrix::Create(ds, "Cells", {dims[2], dims[1], dims[0]}, geom->getId());
  REQUIRE(cells != nullptr);
  geom->setCellData(*cells);
  return geom;
}
} // namespace

TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Parameter domain regression", "[SimplnxCore][SetImageGeomOriginScalingFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1);
  CAPTURE(row);
  DataStructure ds;
  MakeValidationImage(ds);
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 0);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{0, 1, 1});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{std::numeric_limits<float32>::quiet_NaN(), 0, 0});
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  CHECK(result.outputActions.errors()[0].code == (row == 0 ? -86011 : -86012));
}
TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Centered result regression", "[SimplnxCore][SetImageGeomOriginScalingFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1);
  CAPTURE(row);
  DataStructure ds;
  auto* geom = MakeValidationImage(ds, row == 0 ? SizeVec3{10, 4, 2} : SizeVec3{10, 1, 1}, row == 0 ? FloatVec3{1, 0.5f, 2} : FloatVec3{1, 1, 1});
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, row == 0 ? VectorFloat32Parameter::ValueType{-10, 5, -2} : VectorFloat32Parameter::ValueType{0, 0, 0});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{3e38f, 1, 1});
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  if(row == 1)
  {
    REQUIRE_FALSE(pre.outputActions.warnings().empty());
    CHECK(pre.outputActions.warnings()[0].code == -86013);
    CHECK(pre.outputActions.warnings()[0].message.find("Put Input Origin at the Center of Geometry") != std::string::npos);
    CHECK(pre.outputActions.warnings()[0].message.find("X") != std::string::npos);
  }
  auto result = filter.execute(ds, args);
  if(row == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    CHECK(geom->getOrigin() == FloatVec3{-15, 4, -4});
    CHECK(geom->getBoundingBoxf().center() == Point3Df{-10, 5, -2});
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors()[0].code == -86030);
    const auto& message = result.result.errors()[0].message;
    CHECK(message.find("'ValidationImage'") != std::string::npos);
    CHECK(message.find("X") != std::string::npos);
    CHECK(message.find("centering the origin 0") != std::string::npos);
    CHECK(message.find("spacing 3e+38") != std::string::npos);
    CHECK(message.find("dimension 10") != std::string::npos);
    CHECK(message.find("produces -inf") != std::string::npos);
    CHECK(geom->getSpacing() == FloatVec3{1, 1, 1});
    CHECK(geom->getOrigin() == FloatVec3{0, 0, 0});
  }
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Inactive and retained metadata", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  const auto row = GENERATE(0, 1, 2);
  DataStructure ds;
  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  auto* geom = MakeValidationImage(ds, {2, 2, 2}, {row == 0 ? 0.0f : 1.0f, 1, 1}, {row == 1 ? nan : 0, 0, 0});
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, row == 2 ? VectorFloat32Parameter::ValueType{} : VectorFloat32Parameter::ValueType{nan, 0, 0});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, row == 1 ? VectorFloat32Parameter::ValueType{1, 1, 1} :
                                                                      row == 2 ? VectorFloat32Parameter::ValueType{} :
                                                                                 VectorFloat32Parameter::ValueType{0, 0, 0});
  auto pre = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CHECK(geom->getSpacing() == (row == 0 ? FloatVec3{0, 1, 1} : FloatVec3{1, 1, 1}));
  if(row == 1)
  {
    CHECK(std::isnan(geom->getOrigin()[0]));
  }
  else
  {
    CHECK(geom->getOrigin() == FloatVec3{0, 0, 0});
  }
}
