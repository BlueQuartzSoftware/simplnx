#include "ReconstructionFilterTestUtils.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"

namespace recon_test
{
DataPath BuildThreeComponentImage(DataStructure& ds)
{
  const DataPath geometryPath({"Image Geometry"});
  const CreateImageGeometryAction image(geometryPath, {6, 6, 1}, {0, 0, 0}, {1, 1, 1}, "CellData", IGeometry::LengthUnit::Meter);
  const auto imageResult = image.apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(imageResult);
  const auto inputPath = geometryPath.createChildPath("CellData").createChildPath("Vec");
  const CreateArrayAction array(DataType::int32, {1, 6, 6}, {3}, inputPath);
  const auto arrayResult = array.apply(ds, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(arrayResult);
  return inputPath;
}
} // namespace recon_test
