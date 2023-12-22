#include "OrientationUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

namespace nx::core
{
namespace OrientationUtilities
{

std::string CrystalStructureEnumToString(uint32_t crystalStructureType)
{
  if(crystalStructureType > 10)
  {
    return "UnknownCrystalStructure";
  }
  const std::vector<std::string> allLaueNames = LaueOps::GetLaueNames();
  return allLaueNames[crystalStructureType];
}

} // namespace OrientationUtilities
} // namespace nx::core
