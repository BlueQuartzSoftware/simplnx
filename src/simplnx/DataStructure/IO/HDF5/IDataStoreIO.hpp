#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

namespace nx::core
{
namespace HDF5
{
namespace IDataStoreIO
{
/**
 * @brief Reads the DataStore tuple shape from HDF5.
 * @param datasetReader Source HDF5 dataset.
 * @return Exact stored shape or an attribute-read error.
 */
Result<ShapeType> SIMPLNX_EXPORT ReadTupleShape(const nx::core::HDF5::DatasetIO& datasetReader);

/**
 * @brief Reads the DataStore component shape from HDF5.
 * @param datasetReader Source HDF5 dataset.
 * @return Exact stored shape or an attribute-read error.
 */
Result<ShapeType> SIMPLNX_EXPORT ReadComponentShape(const nx::core::HDF5::DatasetIO& datasetReader);
} // namespace IDataStoreIO
} // namespace HDF5
} // namespace nx::core
