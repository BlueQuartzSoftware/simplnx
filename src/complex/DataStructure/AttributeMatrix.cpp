#include "AttributeMatrix.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IArray.hpp"

#include <exception>
#include <stdexcept>

using namespace complex;

AttributeMatrix::AttributeMatrix(DataStructure& dataStructure, std::string name)
: BaseGroup(dataStructure, std::move(name))
{
}

AttributeMatrix::AttributeMatrix(DataStructure& dataStructure, std::string name, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
{
}

AttributeMatrix::AttributeMatrix(const AttributeMatrix&) = default;

AttributeMatrix::AttributeMatrix(AttributeMatrix&&) = default;

AttributeMatrix::~AttributeMatrix() noexcept = default;

DataObject::Type AttributeMatrix::getDataObjectType() const
{
  return Type::AttributeMatrix;
}

BaseGroup::GroupType AttributeMatrix::getGroupType() const
{
  return GroupType::AttributeMatrix;
}

AttributeMatrix* AttributeMatrix::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

AttributeMatrix* AttributeMatrix::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::shared_ptr<DataObject> AttributeMatrix::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStruct, copyPath.getTargetName()));
  copy->setShape(m_TupleShape);
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);
    return copy;
  }
  return nullptr;
}

DataObject* AttributeMatrix::shallowCopy()
{
  return new AttributeMatrix(*this);
}

std::string AttributeMatrix::getTypeName() const
{
  return k_TypeName;
}

bool AttributeMatrix::canInsert(const DataObject* obj) const
{
  if(!BaseGroup::canInsert(obj))
  {
    return false;
  }

  const auto* arrayObject = dynamic_cast<const IArray*>(obj);

  if(arrayObject == nullptr)
  {
    return false;
  }

  IArray::ShapeType arrayTupleShape = arrayObject->getTupleShape();

  usize totalTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  usize incomingTupleCount = std::accumulate(arrayTupleShape.cbegin(), arrayTupleShape.cend(), static_cast<usize>(1), std::multiplies<>());

  return (totalTuples == incomingTupleCount);
}

const AttributeMatrix::ShapeType& AttributeMatrix::getShape() const
{
  return m_TupleShape;
}

void AttributeMatrix::setShape(ShapeType shape)
{
  m_TupleShape = std::move(shape);
}

usize AttributeMatrix::getNumTuples() const
{
  return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
}
