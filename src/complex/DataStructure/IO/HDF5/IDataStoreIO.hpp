#pragma once

#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"

namespace complex
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
typename complex::IDataStore::ShapeType COMPLEX_EXPORT ReadTupleShape(const H5::DatasetReader& datasetReader);

/**
 * @brief Attempts to read the DataStore component shape from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param datasetReader
 * @return Result<>
 */
typename complex::IDataStore::ShapeType COMPLEX_EXPORT ReadComponentShape(const H5::DatasetReader& datasetReader);
} // namespace IDataStoreIO
} // namespace HDF5
} // namespace complex
