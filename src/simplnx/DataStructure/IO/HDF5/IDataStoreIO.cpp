#include "IDataStoreIO.hpp"

#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/Common/Types.hpp"

#include <vector>

using namespace nx::core;

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadTupleShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  std::vector<usize> tupleShape;
  auto tupleShapeResult = datasetReader.readVectorAttribute<usize>(IOConstants::k_TupleShapeTag);
  if (tupleShapeResult.valid())
  {
    tupleShape = std::move(tupleShapeResult.value());
  }
  return tupleShape;
}

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadComponentShape(const nx::core::HDF5::DatasetIO& datasetReader)
{
  std::vector<usize> compShape;
  auto compShapeResult = datasetReader.readVectorAttribute<usize>(IOConstants::k_ComponentShapeTag);
  if(compShapeResult.valid())
  {
    compShape = std::move(compShapeResult.value());
  }
  return compShape;
}
