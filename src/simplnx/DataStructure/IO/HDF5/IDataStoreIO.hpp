#pragma once

#include "simplnx/DataStructure/IDataStore.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

namespace nx::core
{
namespace HDF5
{
namespace IDataStoreIO
{
/**
 * @brief Attempts to read the DataStore tuple shape from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param datasetReader
 * @return Result<>
 */
typename IDataStore::ShapeType SIMPLNX_EXPORT ReadTupleShape(const nx::core::HDF5::DatasetIO& datasetReader);

/**
 * @brief Attempts to read the DataStore component shape from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param datasetReader
 * @return Result<>
 */
typename IDataStore::ShapeType SIMPLNX_EXPORT ReadComponentShape(const nx::core::HDF5::DatasetIO& datasetReader);
} // namespace IDataStoreIO
} // namespace HDF5
} // namespace nx::core
