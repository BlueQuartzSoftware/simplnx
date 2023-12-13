#include "GeometryHelpers.hpp"

using namespace complex;
namespace complex::GeometryHelpers::Description
{

std::string GenerateGeometryInfo(const complex::SizeVec3& dims, const complex::FloatVec3& spacing, const complex::FloatVec3& origin, IGeometry::LengthUnit units)
{
  std::stringstream description;

  std::string unitStr;
  if(units != IGeometry::LengthUnit::Unknown && units != IGeometry::LengthUnit::Unspecified)
  {
    unitStr = IGeometry::LengthUnitToString(units);
  }

  std::array<std::string, 3> label = {"X", "Y", "Z"};

  for(size_t i = 0; i < 3; i++)
  {
    description << label[i] << " Bounds: " << origin[i] << " to " << (origin[i] + (static_cast<float>(dims[i]) * spacing[i])) << " (Delta: " << (dims[i] * spacing[i]) << ") " << unitStr
                << "     Extent " << 0 << "-" << dims[i] - 1 << " (dimension: " << dims[i] << ") Voxels\n";
  }

  return description.str();
}

} // namespace complex::GeometryHelpers::Description
