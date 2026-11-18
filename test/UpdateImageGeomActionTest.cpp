#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
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

  for(const auto* selectedActions : std::array<const OutputActions*, 2>{&actions, &copiedActions})
  {
    DataStructure dataStructure;
    auto* geometry = ImageGeom::Create(dataStructure, "Image");
    REQUIRE(geometry != nullptr);
    geometry->setDimensions({4, 2, 6});
    geometry->setOrigin({10.0f, -20.0f, 30.0f});
    geometry->setSpacing({1.0f, 1.0f, 1.0f});
    const auto result = selectedActions->applyRegular(dataStructure, mode);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    // Half extents are (4, 3, 12), so centering at (-2, 7, -5) requires origin (-6, 4, -17).
    const FloatVec3 expectedOrigin = centerOrigin ? FloatVec3{-6.0f, 4.0f, -17.0f} : requestedOrigin;
    CHECK(geometry->getOrigin() == expectedOrigin);
    CHECK(geometry->getSpacing() == requestedSpacing);
    CHECK(geometry->getDimensions() == SizeVec3(4, 2, 6));
    if(centerOrigin)
    {
      CHECK(geometry->getBoundingBoxf().center() == Point3Df(-2.0f, 7.0f, -5.0f));
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("simplnx::UpdateImageGeomAction: Optional result validation and modes", "[simplnx][UpdateImageGeomAction]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);
  DataStructure ds;
  auto* geom = ImageGeom::Create(ds, "Image");
  REQUIRE(geom != nullptr);
  geom->setDimensions({10, 1, 1});
  geom->setSpacing({row == 4 ? 0.0f : 1.0f, 1, 1});
  geom->setOrigin({0, 0, 0});
  const auto oldSpacing = geom->getSpacing();
  const bool preflight = row == 0 || row == 2;
  const bool check = row != 2 && row != 3;
  const std::optional<FloatVec3> spacing = row == 4 ? std::nullopt : row == 6 ? FloatVec3{0, 1, 1} : row == 7 ? std::optional<FloatVec3>{} : FloatVec3{3e38f, 1, 1};
  const FloatVec3 origin = row == 7 ? FloatVec3{std::numeric_limits<float32>::infinity(), 0, 0} : FloatVec3{0, 0, 0};
  UpdateImageGeomAction action(origin, spacing, DataPath({"Image"}), row < 4 || row == 5, check);
  auto cloned = action.clone();
  auto result = (row == 5 ? cloned.get() : &action)->apply(ds, preflight ? IDataAction::Mode::Preflight : IDataAction::Mode::Execute);
  if(row == 1 || row == 5 || row == 6 || row == 7)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == (row == 6 ? -86028 : row == 7 ? -86029 : -86030));
    const auto& message = result.errors()[0].message;
    CHECK(message.find("'Image'") != std::string::npos);
    CHECK(message.find("X") != std::string::npos);
    CHECK(message.find("Parameter") == std::string::npos);
    if(row == 6)
    {
      CHECK(message.find("assigned spacing 0") != std::string::npos);
      CHECK(message.find("existing spacing 1") != std::string::npos);
      CHECK(message.find("computed result 0") != std::string::npos);
      CHECK(message.find("must be finite and greater than 0") != std::string::npos);
    }
    else if(row == 7)
    {
      CHECK(message.find("assigned origin inf") != std::string::npos);
      CHECK(message.find("existing origin 0") != std::string::npos);
      CHECK(message.find("computed result inf") != std::string::npos);
      CHECK(message.find("must be finite") != std::string::npos);
    }
    else
    {
      CHECK(message.find("centering the origin 0") != std::string::npos);
      CHECK(message.find("spacing 3e+38") != std::string::npos);
      CHECK(message.find("dimension 10") != std::string::npos);
      CHECK(message.find("produces -inf") != std::string::npos);
    }
    CHECK(geom->getSpacing() == oldSpacing);
    CHECK(geom->getOrigin() == FloatVec3{0, 0, 0});
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    if(row != 4)
    {
      CHECK_FALSE(std::isfinite(geom->getOrigin()[0]));
    }
    else
    {
      CHECK(geom->getSpacing()[0] == 0);
    }
  }
}
