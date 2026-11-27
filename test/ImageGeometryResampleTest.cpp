#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("simplnx::PreflightImageGeometryResample: the planned geometry keeps the source length unit", "[simplnx][PreflightImageGeometryResample]")
{
  const auto units = GENERATE(IGeometry::LengthUnit::Millimeter, IGeometry::LengthUnit::Meter, IGeometry::LengthUnit::Unspecified);
  CAPTURE(units);

  DataStructure dataStructure;
  const DataPath srcGeomPath({"Input Geometry"});
  const CreateImageGeometryAction inputGeometryAction(srcGeomPath, {2, 2, 2}, {1.0F, -2.0F, 3.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data", units);
  auto inputGeometryResult = inputGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(inputGeometryResult);

  ResampleImageGeomInputValues inputValues;
  inputValues.resampling_mode = 0;
  inputValues.spacing = {0.5F, 0.5F, 0.5F};
  inputValues.selected_image_geometry_path = srcGeomPath;
  inputValues.remove_original_image_geom = false;
  inputValues.created_image_geometry_path = DataPath({"Resampled Geometry"});
  inputValues.renumber_features = false;

  const auto preflightResult = PreflightImageGeometryResample(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const CreateImageGeometryAction* plannedGeometryPtr = nullptr;
  for(const auto& action : preflightResult.output_actions.value().actions)
  {
    if(const auto* candidatePtr = dynamic_cast<const CreateImageGeometryAction*>(action.get()); candidatePtr != nullptr)
    {
      plannedGeometryPtr = candidatePtr;
      break;
    }
  }
  REQUIRE(plannedGeometryPtr != nullptr);
  CHECK(plannedGeometryPtr->units() == units);
  CHECK(plannedGeometryPtr->path() == inputValues.created_image_geometry_path);
}
