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

namespace nx::core::HDF5
{
DataIOManager::DataIOManager()
: IDataIOManager()
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
  addFactory<UInt8ArrayIO>();
  addFactory<UInt16ArrayIO>();
  addFactory<UInt32ArrayIO>();
  addFactory<UInt64ArrayIO>();
  addFactory<Int8ArrayIO>();
  addFactory<Int16ArrayIO>();
  addFactory<Int32ArrayIO>();
  addFactory<Int64ArrayIO>();
  addFactory<Float32ArrayIO>();
  addFactory<Float64ArrayIO>();
  addFactory<BoolArrayIO>();
  addFactory<StringArrayIO>();

  addFactory<UInt8NeighborIO>();
  addFactory<UInt16NeighborIO>();
  addFactory<UInt32NeighborIO>();
  addFactory<UInt64NeighborIO>();
  addFactory<Int8NeighborIO>();
  addFactory<Int16NeighborIO>();
  addFactory<Int32NeighborIO>();
  addFactory<Int64NeighborIO>();
  addFactory<Float32NeighborIO>();
  addFactory<Float64NeighborIO>();

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
} // namespace nx::core::HDF5
