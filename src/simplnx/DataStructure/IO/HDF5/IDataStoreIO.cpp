#include "IDataStoreIO.hpp"

#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/Common/Types.hpp"

#include <vector>

using namespace nx::core;

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadTupleShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  std::vector<usize> tupleShape;
  datasetReader.readAttribute(IOConstants::k_TupleShapeTag, tupleShape);
  return tupleShape;
}

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadComponentShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  std::vector<usize> compShape;
  datasetReader.readAttribute(IOConstants::k_ComponentShapeTag, compShape);
  return compShape;
}
