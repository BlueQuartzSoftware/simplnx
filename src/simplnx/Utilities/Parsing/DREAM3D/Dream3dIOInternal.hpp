#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <vector>

namespace nx::core
{
class DataStructure;

namespace HDF5
{
class FileIO;
}

namespace DREAM3D::detail
{
/**
 * @brief Materializes exact imported leaf paths from one open DREAM3D file.
 * @param preparedStructure Owns placeholders whose final plans are already recorded.
 * @param fileReader Supplies current or legacy stored values.
 * @param importedLeafPaths Identifies only the imported leaves from this file.
 * @return Finalizer and eager-load warnings or the first contextual error.
 */
SIMPLNX_EXPORT Result<> MaterializeImportedPaths(DataStructure& preparedStructure, const HDF5::FileIO& fileReader, const std::vector<DataPath>& importedLeafPaths);
} // namespace DREAM3D::detail
} // namespace nx::core
