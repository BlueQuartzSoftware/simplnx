#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <memory>
#include <vector>

namespace nx::core
{
class DataStructure;
class IDataStoreFormatResolver;

namespace HDF5
{
class FileIO;
}

namespace DREAM3D::detail
{
/**
 * @brief Imports metadata using one local policy throughout NX or legacy traversal.
 * @param fileReader Supplies the open file and its version.
 * @param resolver Supplies policy before any
 * metadata factory runs.
 * @return Metadata and original disk-import diagnostics.
 */
SIMPLNX_EXPORT Result<DataStructure> ImportMetadata(const HDF5::FileIO& fileReader, std::shared_ptr<const IDataStoreFormatResolver> resolver);

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
