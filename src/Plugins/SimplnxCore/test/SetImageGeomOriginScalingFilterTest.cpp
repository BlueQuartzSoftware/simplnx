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

  DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool kChangeOrigin = false;
  bool kChangeResolution = false;
  std::vector<float32> kOrigin{0, 0, 0};
  std::vector<float32> kSpacing{1, 1, 1};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(kChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(kChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(kOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(kSpacing));

  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: Valid Execution", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool kChangeOrigin = true;
  bool kChangeResolution = true;
  std::vector<float32> kOrigin{7, 6, 5};
  std::vector<float32> kSpacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(kChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(kChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(kOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(kSpacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  REQUIRE(imageGeom.getOrigin() == FloatVec3{7, 6, 5});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: 0,0,0 Central Origin", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool kChangeOrigin = true;
  bool kChangeResolution = true;
  std::vector<float32> kOrigin{0.0, 0.0, 0.0};
  std::vector<float32> kSpacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(kChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(kChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(kOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(kSpacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  REQUIRE(imageGeom.getBoundingBoxf().center() == Point3Df{0.0f, 0.0f, 0.0f});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: Custom Central Origin", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  UnitTest::LoadPlugins();

  DataPath kImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  bool kChangeOrigin = true;
  bool kChangeResolution = true;
  std::vector<float32> kOrigin{7.0, 6.0, 5.0};
  std::vector<float32> kSpacing{2, 2, 2};

  SetImageGeomOriginScalingFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(kImageGeomPath));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, std::make_any<bool>(kChangeOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, std::make_any<bool>(true));
  args.insert(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, std::make_any<bool>(kChangeResolution));
  args.insert(SetImageGeomOriginScalingFilter::k_Origin_Key, std::make_any<std::vector<float32>>(kOrigin));
  args.insert(SetImageGeomOriginScalingFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(kSpacing));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(kImageGeomPath);
  REQUIRE(imageGeom.getBoundingBoxf().center() == Point3Df{7.0, 6.0, 5.0});
  REQUIRE(imageGeom.getSpacing() == FloatVec3{2, 2, 2});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScalingFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SetImageGeomOriginScalingFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SetImageGeomOriginScalingFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SetImageGeomOriginScalingFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<SetImageGeomOriginScalingFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  auto* geometryPtr = ImageGeom::Create(dataStructure, geometryPath.getTargetName());
  REQUIRE(geometryPtr != nullptr);
  geometryPtr->setDimensions({2, 2, 2});
  REQUIRE(geometryPtr->setOrigin({10.0f, 20.0f, 30.0f}).valid());
  REQUIRE(geometryPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());

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
  CHECK(geometryPtr->getOrigin() == expectedOrigin);
  const auto center = geometryPtr->getBoundingBoxf().center();
  for(usize axis = 0; axis < 3; axis++)
  {
    CHECK(center[axis] == requestedCenter[axis]);
  }
  CHECK(geometryPtr->getSpacing() == (changeSpacing ? FloatVec3{2.0f, 3.0f, 4.0f} : FloatVec3{1.0f, 1.0f, 1.0f}));
}

namespace
{
ImageGeom* MakeValidationImage(DataStructure& dataStructure, const SizeVec3& dims = {2, 2, 2}, const FloatVec3& spacing = {1, 1, 1}, const FloatVec3& origin = {0, 0, 0})
{
  auto* geomPtr = ImageGeom::Create(dataStructure, "ValidationImage");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions(dims);
  REQUIRE(geomPtr->setSpacing(spacing).valid());
  REQUIRE(geomPtr->setOrigin(origin).valid());
  auto* cellsPtr = AttributeMatrix::Create(dataStructure, "Cells", {dims[2], dims[1], dims[0]}, geomPtr->getId());
  REQUIRE(cellsPtr != nullptr);
  geomPtr->setCellData(*cellsPtr);
  return geomPtr;
}
} // namespace

TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Parameter domain regression", "[SimplnxCore][SetImageGeomOriginScalingFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1);
  CAPTURE(row);
  DataStructure dataStructure;
  MakeValidationImage(dataStructure);
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 0);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{0, 1, 1});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, VectorFloat32Parameter::ValueType{std::numeric_limits<float32>::quiet_NaN(), 0, 0});
  auto result = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  CHECK(result.outputActions.errors()[0].code == (row == 0 ? -86011 : -86012));
}
TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Centered result regression", "[SimplnxCore][SetImageGeomOriginScalingFilter][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1);
  CAPTURE(row);
  DataStructure dataStructure;
  auto* geomPtr = MakeValidationImage(dataStructure, row == 0 ? SizeVec3{10, 4, 2} : SizeVec3{10, 1, 1}, row == 0 ? FloatVec3{1, 0.5f, 2} : FloatVec3{1, 1, 1});
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, row == 0 ? VectorFloat32Parameter::ValueType{-10, 5, -2} : VectorFloat32Parameter::ValueType{0, 0, 0});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, VectorFloat32Parameter::ValueType{3e38f, 1, 1});
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  if(row == 1)
  {
    REQUIRE_FALSE(pre.outputActions.warnings().empty());
    CHECK(pre.outputActions.warnings()[0].code == -86013);
    CHECK(pre.outputActions.warnings()[0].message.find("Put Input Origin at the Center of Geometry") != std::string::npos);
    CHECK(pre.outputActions.warnings()[0].message.find("X origin 0, spacing 3e+38, dimension 10 produce centered result -inf") != std::string::npos);
  }
  // Use a copy to verify the action in preflight mode without changing the execution fixture.
  DataStructure preflightDs = dataStructure;
  const auto preflightActionResult = pre.outputActions.value().applyRegular(preflightDs, IDataAction::Mode::Preflight);
  const auto& preflightGeom = preflightDs.getDataRefAs<ImageGeom>(DataPath({"ValidationImage"}));
  auto result = filter.execute(dataStructure, args);
  if(row == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightActionResult);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    CHECK(preflightGeom.getOrigin() == FloatVec3{-15, 4, -4});
    CHECK(geomPtr->getOrigin() == FloatVec3{-15, 4, -4});
    CHECK(geomPtr->getBoundingBoxf().center() == Point3Df{-10, 5, -2});
  }
  else
  {
    for(const auto* failurePtr : std::array<const Result<>*, 2>{&preflightActionResult, &result.result})
    {
      SIMPLNX_RESULT_REQUIRE_INVALID(*failurePtr);
      REQUIRE(failurePtr->errors().size() == 1);
      CHECK(failurePtr->errors().front().code == -1557);
      const auto& message = failurePtr->errors().front().message;
      CHECK(message.find("'ValidationImage'") != std::string::npos);
      CHECK(message.find("Origin on the X axis must be finite. Received -inf.") != std::string::npos);
    }
    CHECK(preflightGeom.getSpacing() == FloatVec3{1, 1, 1});
    CHECK(preflightGeom.getOrigin() == FloatVec3{0, 0, 0});
    CHECK(geomPtr->getSpacing() == FloatVec3{1, 1, 1});
    CHECK(geomPtr->getOrigin() == FloatVec3{0, 0, 0});
  }
}

TEST_CASE("SimplnxCore::SetImageGeomOriginScaling: Inactive and retained metadata", "[SimplnxCore][SetImageGeomOriginScalingFilter]")
{
  const auto row = GENERATE(0, 1, 2);
  DataStructure dataStructure;
  const float32 nan = std::numeric_limits<float32>::quiet_NaN();
  const FloatVec3 oldSpacing{0.25f, 1, 1};
  const FloatVec3 oldOrigin{-4, 2, 3};
  auto* geomPtr = MakeValidationImage(dataStructure, {2, 2, 2}, oldSpacing, oldOrigin);
  SetImageGeomOriginScalingFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_SelectedImageGeometryPath_Key, DataPath({"ValidationImage"}));
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeOrigin_Key, row == 0);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_ChangeSpacing_Key, row == 1);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Origin_Key, row == 0 ? VectorFloat32Parameter::ValueType{1, 2, 3} :
                                                                     row == 1 ? VectorFloat32Parameter::ValueType{nan, 0, 0} :
                                                                                VectorFloat32Parameter::ValueType{});
  args.insertOrAssign(SetImageGeomOriginScalingFilter::k_Spacing_Key, row == 1 ? VectorFloat32Parameter::ValueType{1, 1, 1} :
                                                                      row == 2 ? VectorFloat32Parameter::ValueType{} :
                                                                                 VectorFloat32Parameter::ValueType{0, 0, 0});
  auto pre = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  CHECK(geomPtr->getSpacing() == (row == 1 ? FloatVec3{1, 1, 1} : oldSpacing));
  CHECK(geomPtr->getOrigin() == (row == 0 ? FloatVec3{0.75f, 1, 2} : oldOrigin));
}
