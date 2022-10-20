#include "AttributeMatrix.hpp"

#include <exception>
#include <stdexcept>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IArray.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
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

std::shared_ptr<DataObject> AttributeMatrix::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  auto copy = std::shared_ptr<AttributeMatrix>(new AttributeMatrix(dataStruct, copyPath.getTargetName(), getId()));
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
  return "AttributeMatrix";
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

H5::ErrorType AttributeMatrix::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  auto attribute = groupReader.getAttribute(H5Constants::k_TupleDims);
  m_TupleShape = attribute.readAsVector<usize>();

  if(m_TupleShape.empty())
  {
    return -1;
  }

  H5::ErrorType error = BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  return error;
}

H5::ErrorType AttributeMatrix::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = BaseGroup::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto attribute = groupWriter.createAttribute(H5Constants::k_TupleDims);
  error = attribute.writeVector(H5::AttributeWriter::DimsVector{m_TupleShape.size()}, m_TupleShape);
  if(error < 0)
  {
    return error;
  }

  return error;
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
