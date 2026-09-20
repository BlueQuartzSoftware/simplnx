#include "IDataStoreIO.hpp"

#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/Common/Types.hpp"

#include <vector>

using namespace nx::core;

Result<ShapeType> nx::core::HDF5::IDataStoreIO::ReadTupleShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  return datasetReader.readVectorAttribute<usize>(IOConstants::k_TupleShapeTag);
}

Result<ShapeType> nx::core::HDF5::IDataStoreIO::ReadComponentShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  return datasetReader.readVectorAttribute<usize>(IOConstants::k_ComponentShapeTag);
}
