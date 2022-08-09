#include "ZarrDataFactoryManager.hpp"

#include "complex/DataStructure/Factory/Zarr/DataArrayFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/DataGroupFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/EdgeGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/GridMontageFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/HexahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/ImageGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/NeighborListFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/QuadGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/RectGridGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/ScalarDataFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/TetrahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/TriangleGeomFactory.hpp"
#include "complex/DataStructure/Factory/Zarr/VertexGeomFactory.hpp"
#include "complex/Utilities/Parsing/Zarr/Zarr.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrIDataFactory.hpp"

using namespace complex;
using namespace complex::Zarr;

Zarr::DataFactoryManager::DataFactoryManager()
{
  addCoreFactories();
}

Zarr::DataFactoryManager::~DataFactoryManager() = default;

void Zarr::DataFactoryManager::addCoreFactories()
{
  addFactory(new UInt8ArrayFactory());
  addFactory(new UInt16ArrayFactory());
  addFactory(new UInt32ArrayFactory());
  addFactory(new UInt64ArrayFactory());
  addFactory(new Int8ArrayFactory());
  addFactory(new Int16ArrayFactory());
  addFactory(new Int32ArrayFactory());
  addFactory(new Int64ArrayFactory());
  addFactory(new USizeArrayFactory());
  addFactory(new Float32ArrayFactory());
  addFactory(new Float64ArrayFactory());
  addFactory(new BoolArrayFactory());

  addFactory(new NeighborListFactory());

  addFactory(new DataGroupFactory());
  addFactory(new EdgeGeomFactory());
  addFactory(new GridMontageFactory());
  addFactory(new HexahedralGeomFactory());
  addFactory(new ImageGeomFactory());
  addFactory(new QuadGeomFactory());
  addFactory(new RectGridGeomFactory());
  addFactory(new ScalarDataFactory());
  addFactory(new TetrahedralGeomFactory());
  addFactory(new TriangleGeomFactory());
  addFactory(new VertexGeomFactory());
}

void Zarr::DataFactoryManager::addFactory(IDataFactory* factory)
{
  if(factory == nullptr)
  {
    return;
  }

  m_Factories[factory->getDataTypeName()] = std::shared_ptr<IDataFactory>(factory);
}

std::vector<Zarr::IDataFactory*> Zarr::DataFactoryManager::getFactories() const
{
  std::vector<Zarr::IDataFactory*> factories;
  for(const auto& iter : m_Factories)
  {
    factories.push_back(iter.second.get());
  }
  return factories;
}

Zarr::IDataFactory* Zarr::DataFactoryManager::getFactory(const std::string& typeName) const
{
  if(m_Factories.find(typeName) == m_Factories.end())
  {
    throw std::runtime_error("Could not find Zarr::IDataFactory for " + typeName);
  }
  return m_Factories.at(typeName).get();
}
