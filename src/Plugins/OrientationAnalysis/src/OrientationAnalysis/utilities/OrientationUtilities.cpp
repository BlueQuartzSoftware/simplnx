#include "OrientationUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/Euler.hpp>

namespace nx::core::orientation_utilities
{

std::array<float64, 3> PrincipalAxisFrameToEuler(Matrix3dRType frame)
{
  // A row sign change preserves each axis line and makes an improper frame right-handed.
  if(frame.determinant() < 0.0)
  {
    frame.row(2) *= -1.0;
  }
  const auto euler = ebsdlib::OrientationMatrixDType(frame.data()).toEuler();
  return {euler[0], euler[1], euler[2]};
}

std::string CrystalStructureEnumToString(uint32_t crystalStructureType)
{
  if(crystalStructureType > 10)
  {
    return "UnknownCrystalStructure";
  }
  const std::vector<std::string> allLaueNames = ebsdlib::LaueOps::GetLaueNames();
  return allLaueNames[crystalStructureType];
}

} // namespace nx::core::orientation_utilities
