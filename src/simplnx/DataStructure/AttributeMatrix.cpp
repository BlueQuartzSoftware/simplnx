#include "AttributeMatrix.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IArray.hpp"

#include <exception>

using namespace nx::core;

AttributeMatrix::AttributeMatrix(DataStructure& dataStructure, std::string name, ShapeType tupleShape)
: BaseGroup(dataStructure, std::move(name))
, m_TupleShape(std::move(tupleShape))
{
}

AttributeMatrix::AttributeMatrix(DataStructure& dataStructure, std::string name, ShapeType tupleShape, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
, m_TupleShape(std::move(tupleShape))
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

AttributeMatrix* AttributeMatrix::Create(DataStructure& dataStructure, std::string name, ShapeType tupleShape, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStructure, std::move(name), std::move(tupleShape)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

AttributeMatrix* AttributeMatrix::Import(DataStructure& dataStructure, std::string name, ShapeType tupleShape, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStructure, std::move(name), std::move(tupleShape), importId));
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
  auto copy = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStruct, copyPath.getTargetName(), m_TupleShape));
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

  const auto* arrayObjectPtr = dynamic_cast<const IArray*>(obj);

  if(arrayObjectPtr == nullptr)
  {
    return false;
  }

  const IArray::ShapeType arrayTupleShape = arrayObjectPtr->getTupleShape();

  const usize totalTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  const usize incomingTupleCount = std::accumulate(arrayTupleShape.cbegin(), arrayTupleShape.cend(), static_cast<usize>(1), std::multiplies<>());

  return (totalTuples == incomingTupleCount);
}

const AttributeMatrix::ShapeType& AttributeMatrix::getShape() const
{
  return m_TupleShape;
}

usize AttributeMatrix::getNumTuples() const
{
  return std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
}

void AttributeMatrix::resizeTuples(ShapeType tupleShape)
{
  m_TupleShape = std::move(tupleShape);
  auto childArrays = findAllChildrenOfType<IArray>();
  for(const auto& array : childArrays)
  {
    array->resizeTuples(m_TupleShape);
  }
}

Result<> AttributeMatrix::validate() const
{
  Result<> result;
  auto childArrays = findAllChildrenOfType<IArray>();
  usize numTuples = getNumTuples();
  for(const auto& array : childArrays)
  {
    if(array->getNumberOfTuples() != numTuples)
    {
      result = MergeResults(result, MakeErrorResult(-4701, fmt::format("AttributeMatrix '{}' has {} tuples but the contained IArray objet '{}' has {} total tuples.", getName(), numTuples,
                                                                       array->getName(), array->getNumberOfTuples())));
    }
  }
  return result;
}
