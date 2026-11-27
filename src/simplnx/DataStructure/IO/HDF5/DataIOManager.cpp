#include "DataIOManager.hpp"

#include "simplnx/DataStructure/IO/HDF5/AttributeMatrixIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataGroupIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/EdgeGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/HexahedralGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/ImageGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/NeighborListIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/QuadGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/RectGridGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/ScalarDataIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/StringArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/TetrahedralGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/TriangleGeomIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/VertexGeomIO.hpp"

namespace nx::core::hdf5
{
DataIOManager::DataIOManager()

{
  addCoreFactories();
}

DataIOManager::~DataIOManager() noexcept = default;

std::string DataIOManager::formatName() const
{
  return "HDF5";
}

void DataIOManager::addCoreFactories()
{
  addFactory<UInt8ArrayIoType>();
  addFactory<UInt16ArrayIoType>();
  addFactory<UInt32ArrayIoType>();
  addFactory<UInt64ArrayIoType>();
  addFactory<Int8ArrayIoType>();
  addFactory<Int16ArrayIoType>();
  addFactory<Int32ArrayIoType>();
  addFactory<Int64ArrayIoType>();
  addFactory<Float32ArrayIoType>();
  addFactory<Float64ArrayIoType>();
  addFactory<BoolArrayIoType>();
  addFactory<StringArrayIO>();

  addFactory<UInt8NeighborIoType>();
  addFactory<UInt16NeighborIoType>();
  addFactory<UInt32NeighborIoType>();
  addFactory<UInt64NeighborIoType>();
  addFactory<Int8NeighborIoType>();
  addFactory<Int16NeighborIoType>();
  addFactory<Int32NeighborIoType>();
  addFactory<Int64NeighborIoType>();
  addFactory<Float32NeighborIoType>();
  addFactory<Float64NeighborIoType>();

  addFactory<ScalarDataIO<uint8>>();
  addFactory<ScalarDataIO<uint16>>();
  addFactory<ScalarDataIO<uint32>>();
  addFactory<ScalarDataIO<uint64>>();
  addFactory<ScalarDataIO<int8>>();
  addFactory<ScalarDataIO<int16>>();
  addFactory<ScalarDataIO<int32>>();
  addFactory<ScalarDataIO<int64>>();
  addFactory<ScalarDataIO<float32>>();
  addFactory<ScalarDataIO<float64>>();
  addFactory<ScalarDataIO<bool>>();

  addFactory<AttributeMatrixIO>();
  addFactory<DataGroupIO>();
  addFactory<EdgeGeomIO>();
  addFactory<HexahedralGeomIO>();
  addFactory<ImageGeomIO>();
  addFactory<QuadGeomIO>();
  addFactory<RectGridGeomIO>();
  addFactory<TetrahedralGeomIO>();
  addFactory<TriangleGeomIO>();
  addFactory<VertexGeomIO>();
}
} // namespace nx::core::hdf5
