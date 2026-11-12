#include "simplnx/Core/Preferences.hpp"

#include <iostream>

int main()
{
  nx::core::Preferences preferences;
  std::cout << preferences.defaultValueAs<nx::core::uint64>(nx::core::Preferences::k_LargeDataSize_Key) << '\n';
  std::cout << preferences.defaultValueAs<nx::core::uint64>(nx::core::Preferences::k_LargeDataStructureSize_Key) << '\n';
  return 0;
}
