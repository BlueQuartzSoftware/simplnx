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
  inputValues.ResamplingMode = 0;
  inputValues.Spacing = {0.5F, 0.5F, 0.5F};
  inputValues.SelectedImageGeometryPath = srcGeomPath;
  inputValues.RemoveOriginalImageGeom = false;
  inputValues.CreatedImageGeometryPath = DataPath({"Resampled Geometry"});
  inputValues.RenumberFeatures = false;

  const auto preflightResult = PreflightImageGeometryResample(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const CreateImageGeometryAction* plannedGeometryPtr = nullptr;
  for(const auto& action : preflightResult.outputActions.value().actions)
  {
    if(const auto* candidatePtr = dynamic_cast<const CreateImageGeometryAction*>(action.get()); candidatePtr != nullptr)
    {
      plannedGeometryPtr = candidatePtr;
      break;
    }
  }
  REQUIRE(plannedGeometryPtr != nullptr);
  CHECK(plannedGeometryPtr->units() == units);
  CHECK(plannedGeometryPtr->path() == inputValues.CreatedImageGeometryPath);
}
