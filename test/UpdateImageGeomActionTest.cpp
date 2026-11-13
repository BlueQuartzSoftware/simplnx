#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <memory>

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
