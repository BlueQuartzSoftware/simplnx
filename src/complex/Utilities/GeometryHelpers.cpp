#include "GeometryHelpers.hpp"

using namespace complex;
namespace complex::GeometryHelpers::Description
{

std::string GenerateGeometryInfo(const complex::SizeVec3& dims, const complex::FloatVec3& spacing, const complex::FloatVec3& origin)
{
  std::stringstream description;

  description << "X Range: " << origin[0] << " to " << (origin[0] + (static_cast<float>(dims[0]) * spacing[0])) << " (Delta: " << (dims[0] * spacing[0]) << ") " << 0 << "-" << dims[0] - 1
              << " Voxels\n";
  description << "Y Range: " << origin[1] << " to " << (origin[1] + (static_cast<float>(dims[1]) * spacing[1])) << " (Delta: " << (dims[1] * spacing[1]) << ") " << 0 << "-" << dims[1] - 1
              << " Voxels\n";
  description << "Z Range: " << origin[2] << " to " << (origin[2] + (static_cast<float>(dims[2]) * spacing[2])) << " (Delta: " << (dims[2] * spacing[2]) << ") " << 0 << "-" << dims[2] - 1
              << " Voxels\n";
  return description.str();
}

} // namespace complex::GeometryHelpers::Description
