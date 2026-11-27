#include "IDataStoreIO.hpp"

#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/Common/Types.hpp"

#include <vector>

using namespace nx::core;

Result<ShapeType> nx::core::hdf5::i_data_store_io::ReadTupleShape(const nx::core::hdf5::DatasetIO& datasetReader)
{
  return datasetReader.readVectorAttribute<usize>(io_constants::k_TupleShapeTag);
}

Result<ShapeType> nx::core::hdf5::i_data_store_io::ReadComponentShape(const nx::core::hdf5::DatasetIO& datasetReader)
{
  return datasetReader.readVectorAttribute<usize>(io_constants::k_ComponentShapeTag);
}
