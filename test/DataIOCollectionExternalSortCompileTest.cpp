#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"

namespace nx::core
{
void DataIOCollectionExternalSortCompileUse()
{
  DataIOCollection collection;
  ExternalSortConfig config;
  auto result = collection.createExternalSort(config);
  (void)result;
}
} // namespace nx::core
