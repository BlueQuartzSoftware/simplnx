#include "StringArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/StringStore.hpp"

#include <numeric>
#include <stdexcept>

namespace nx::core
{
StringArray* StringArray::Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId)
{
  return CreateWithValues(dataStructure, name, {0}, {}, parentId);
}

StringArray* StringArray::CreateWithValues(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, collection_type strings, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(dataStructure, name.data()));
  data->m_Strings = std::make_shared<StringStore>(strings, tupleShape);
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

StringArray* StringArray::Import(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, IdType importId, collection_type strings,
                                 const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(dataStructure, name.data(), tupleShape, importId, std::move(strings)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

StringArray::StringArray(DataStructure& dataStructure, std::string name)
: IArray(dataStructure, std::move(name))
{
}

StringArray::StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, collection_type strings)
: IArray(dataStructure, std::move(name))
{
  m_Strings = std::make_shared<StringStore>(strings, tupleShape);
}

StringArray::StringArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type>& store)
: IArray(dataStructure, std::move(name))
, m_Strings(store)
{
}

StringArray::StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, IdType importId, collection_type strings)
: IArray(dataStructure, std::move(name), importId)
{
  m_Strings = std::make_shared<StringStore>(strings, tupleShape);
}

StringArray::StringArray(const StringArray& other)
: IArray(other)
, m_Strings(other.m_Strings)
{
}

/*
 * C++ initializes base subobjects before members, regardless of mem-initializer order.
 * This constructor therefore moves IArray before m_Strings.
 * Do not use other after the initializer list because both subobjects are moved.
 * The order is required even when clang-tidy reports the second move.
 */
StringArray::StringArray(StringArray&& other) noexcept
: IArray(std::move(other))
, m_Strings(std::move(other.m_Strings))
{
}

StringArray::~StringArray() noexcept = default;

DataObject::Type StringArray::getDataObjectType() const
{
  return DataObject::Type::StringArray;
}
std::string StringArray::getTypeName() const
{
  return k_TypeName;
}

IArray::ArrayType StringArray::getArrayType() const
{
  return ArrayType::StringArray;
}

DataObject* StringArray::shallowCopy()
{
  return new StringArray(*this);
}

std::shared_ptr<DataObject> StringArray::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  if(dataStruct.containsData(copyPath))
  {
    return nullptr;
  }
  // The non-import constructor generates an identifier for the copied object.
  const auto copy = std::shared_ptr<StringArray>(new StringArray(dataStruct, copyPath.getTargetName(), m_Strings));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    return copy;
  }
  return nullptr;
}

size_t StringArray::size() const
{
  return m_Strings->size();
}

StringArray::collection_type StringArray::values() const
{
  return {begin(), end()};
}

StringArray::reference StringArray::operator[](usize index)
{
  return m_Strings->operator[](index);
}

StringArray::const_reference StringArray::operator[](usize index) const
{
  return m_Strings->operator[](index);
}

void StringArray::setValue(usize index, const std::string& value)
{
  m_Strings->setValue(index, value);
}

StringArray::const_reference StringArray::at(usize index) const
{
  return m_Strings->at(index);
}

std::string StringArray::toString(usize tupleIndex, usize compIndex, const std::string& format) const
{
  const usize valueIndex = tupleIndex * getNumberOfComponents() + compIndex;
  return at(valueIndex);
}

bool StringArray::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value)
{
  setValue(tupleIndex, value);
  return true;
}

StringArray::iterator StringArray::begin()
{
  return m_Strings->begin();
}

StringArray::iterator StringArray::end()
{
  return m_Strings->end();
}

StringArray::const_iterator StringArray::begin() const
{
  return m_Strings->cbegin();
}

StringArray::const_iterator StringArray::end() const
{
  return m_Strings->cend();
}
StringArray::const_iterator StringArray::cbegin() const
{
  return m_Strings->cbegin();
}

StringArray::const_iterator StringArray::cend() const
{
  return m_Strings->cend();
}

StringArray& StringArray::operator=(const StringArray& rhs)
{
  DataObject::operator=(rhs);
  m_Strings = rhs.m_Strings;
  return *this;
}

StringArray& StringArray::operator=(StringArray&& rhs) noexcept
{
  DataObject::operator=(rhs);
  m_Strings = std::move(rhs.m_Strings);
  return *this;
}

usize StringArray::getSize() const
{
  return size();
}

bool StringArray::empty() const
{
  return m_Strings->empty();
}

ShapeType StringArray::getTupleShape() const
{
  return m_Strings->getTupleShape();
}

ShapeType StringArray::getComponentShape() const
{
  return {1};
}

usize StringArray::getNumberOfTuples() const
{
  return size();
}

usize StringArray::getNumberOfComponents() const
{
  return 1;
}

void StringArray::resizeTuples(const ShapeType& tupleShape)
{
  m_Strings->resizeTuples(tupleShape);
}

void StringArray::swapTuples(usize index0, usize index1)
{
  if(index0 == index1)
  {
    return;
  }
  auto value = (*m_Strings)[index0];
  (*m_Strings)[index0] = (*m_Strings)[index1];
  (*m_Strings)[index1] = value;
}

void StringArray::setStore(const std::shared_ptr<AbstractStringStore>& newStore)
{
  m_Strings = newStore;
}

bool StringArray::isPlaceholder() const
{
  // A null store has no data and is treated as a placeholder; otherwise defer to the store.
  return m_Strings == nullptr || m_Strings->isPlaceholder();
}
} // namespace nx::core
