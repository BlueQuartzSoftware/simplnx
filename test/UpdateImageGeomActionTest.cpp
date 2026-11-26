#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>

using namespace nx::core;

TEST_CASE("simplnx::UpdateImageGeomAction: copied actions preserve centering", "[simplnx][UpdateImageGeomAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool centerOrigin = GENERATE(true, false);
  CAPTURE(mode, centerOrigin);
  const DataPath geometryPath({"Image"});
  const FloatVec3 requestedOrigin{-2.0f, 7.0f, -5.0f};
  const FloatVec3 requestedSpacing{2.0f, 3.0f, 4.0f};
  OutputActions actions;
  actions.appendAction(std::make_unique<UpdateImageGeomAction>(requestedOrigin, requestedSpacing, geometryPath, centerOrigin));
  const OutputActions copiedActions = actions;

  for(const auto* selectedActionsPtr : std::array<const OutputActions*, 2>{&actions, &copiedActions})
  {
    DataStructure dataStructure;
    auto* geometryPtr = ImageGeom::Create(dataStructure, "Image");
    REQUIRE(geometryPtr != nullptr);
    geometryPtr->setDimensions({4, 2, 6});
    REQUIRE(geometryPtr->setOrigin({10.0f, -20.0f, 30.0f}).valid());
    REQUIRE(geometryPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
    const auto result = selectedActionsPtr->applyRegular(dataStructure, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    // Half extents are (4, 3, 12), so centering at (-2, 7, -5) requires origin (-6, 4, -17).
    const FloatVec3 expectedOrigin = centerOrigin ? FloatVec3{-6.0f, 4.0f, -17.0f} : requestedOrigin;
    CHECK(geometryPtr->getOrigin() == expectedOrigin);
    CHECK(geometryPtr->getSpacing() == requestedSpacing);
    CHECK(geometryPtr->getDimensions() == SizeVec3(4, 2, 6));
    if(centerOrigin)
    {
      CHECK(geometryPtr->getBoundingBoxf().center() == Point3Df(-2.0f, 7.0f, -5.0f));
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("simplnx::UpdateImageGeomAction: Invalid metadata is rejected before either setter", "[simplnx][UpdateImageGeomAction]")
{
  const auto mode = GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute);
  const bool useClone = GENERATE(false, true);
  CAPTURE(mode, useClone);
  DataStructure ds;
  const DataPath geometryPath({"Parent", "Image"});
  auto* parentPtr = DataGroup::Create(ds, "Parent");
  REQUIRE(parentPtr != nullptr);
  auto* geomPtr = ImageGeom::Create(ds, "Image", parentPtr->getId());
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({10, 1, 1});
  const FloatVec3 oldSpacing{1, 1, 1};
  const FloatVec3 oldOrigin{0, 0, 0};
  REQUIRE(geomPtr->setSpacing(oldSpacing).valid());
  REQUIRE(geomPtr->setOrigin(oldOrigin).valid());

  std::optional<FloatVec3> spacing;
  FloatVec3 origin{5, 0, 0};
  bool centerOrigin = false;
  int32 expectedCode = -1557;
  std::string expectedMessage;
  SECTION("Assigned zero spacing")
  {
    spacing = FloatVec3{0, 1, 1};
    expectedCode = -1556;
    expectedMessage = "Spacing on the X axis must be finite and greater than zero. Received 0.";
  }
  SECTION("Assigned infinite origin")
  {
    spacing = FloatVec3{2, 1, 1};
    origin[0] = std::numeric_limits<float32>::infinity();
    expectedMessage = "Origin on the X axis must be finite. Received inf.";
  }
  SECTION("Centered origin overflows")
  {
    spacing = FloatVec3{3e38f, 1, 1};
    centerOrigin = true;
    expectedMessage = "Origin on the X axis must be finite. Received -inf.";
  }

  const UpdateImageGeomAction action(origin, spacing, geometryPath, centerOrigin);
  const auto clone = action.clone();
  const IDataAction* selectedActionPtr = useClone ? clone.get() : &action;
  const auto result = selectedActionPtr->apply(ds, mode);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == expectedCode);
  CHECK(result.errors().front().message.find("'Parent/Image'") != std::string::npos);
  CHECK(result.errors().front().message.find(expectedMessage) != std::string::npos);
  CHECK(geomPtr->getSpacing() == oldSpacing);
  CHECK(geomPtr->getOrigin() == oldOrigin);
}
