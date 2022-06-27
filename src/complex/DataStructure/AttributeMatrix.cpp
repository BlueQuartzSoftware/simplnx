#include "AttributeMatrix.hpp"

#include <exception>
#include <stdexcept>

#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

AttributeMatrix::AttributeMatrix(DataStructure& ds, std::string name)
: BaseGroup(ds, std::move(name))
{
}

AttributeMatrix::AttributeMatrix(DataStructure& ds, std::string name, IdType importId)
: BaseGroup(ds, std::move(name), importId)
{
}

AttributeMatrix::AttributeMatrix(const AttributeMatrix& other)
: BaseGroup(other)
{
}

AttributeMatrix::AttributeMatrix(AttributeMatrix&& other) noexcept
: BaseGroup(std::move(other))
{
}

AttributeMatrix::~AttributeMatrix() noexcept = default;

DataObject::Type AttributeMatrix::getDataObjectType() const
{
  return Type::AttributeMatrix;
}

AttributeMatrix* AttributeMatrix::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

AttributeMatrix* AttributeMatrix::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

DataObject* AttributeMatrix::deepCopy()
{
  return new AttributeMatrix(*this);
}

DataObject* AttributeMatrix::shallowCopy()
{
  return new AttributeMatrix(*this);
}

std::string AttributeMatrix::getTypeName() const
{
  return "AttributeMatrix";
}

bool AttributeMatrix::canInsert(const DataObject* obj) const
{
  if(!BaseGroup::canInsert(obj))
  {
    return false;
  }

  Type type = obj->getDataObjectType();
  if(type != Type::DataArray)
  {
    return false;
  }

  const auto& dataArray = dynamic_cast<const IDataArray&>(*obj);

  IDataStore::ShapeType arrayTupleShape = dataArray.getIDataStoreRef().getTupleShape();

  auto totalTuples = std::accumulate(m_TupleShape.begin(), m_TupleShape.end(), 1ULL, std::multiplies<>());
  auto incomingTupleCount = std::accumulate(arrayTupleShape.begin(), arrayTupleShape.end(), 1ULL, std::multiplies<>());

  return (totalTuples == incomingTupleCount);
}

H5::ErrorType AttributeMatrix::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType AttributeMatrix::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  return BaseGroup::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
}

const AttributeMatrix::ShapeType& AttributeMatrix::getShape() const
{
  return m_TupleShape;
}

void AttributeMatrix::setShape(ShapeType shape)
{
  m_TupleShape = std::move(shape);
}
