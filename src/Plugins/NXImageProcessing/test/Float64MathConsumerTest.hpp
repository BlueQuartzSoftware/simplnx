#pragma once

#include "simplnx/DataStructure/IDataStore.hpp"

namespace nx::core::UnitTest // NOLINT(readability-identifier-naming) Reopens the shared test namespace.
{
/**
 * @brief Checks math outputs through thresholding and component labeling.
 * @param expectedStore Expected backing for each array. The caller configures the storage preference before calling.
 */
void CheckFloat64MathConsumers(IDataStore::StoreType expectedStore);
} // namespace nx::core::UnitTest
