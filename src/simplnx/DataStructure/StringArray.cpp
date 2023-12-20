#include "StringArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"

#include "fmt/format.h"

#include <numeric>
#include <stdexcept>

namespace nx::core
{
StringArray* StringArray::Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId)
{
  return CreateWithValues(dataStructure, name, {}, parentId);
}

StringArray* StringArray::CreateWithValues(DataStructure& dataStructure, const std::string_view& name, collection_type strings, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(dataStructure, name.data()));
  data->m_Strings = std::move(strings);
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

StringArray* StringArray::Import(DataStructure& dataStructure, const std::string_view& name, IdType importId, collection_type strings, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(dataStructure, name.data(), importId, std::move(strings)));
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

StringArray::StringArray(DataStructure& dataStructure, std::string name, collection_type strings)
: IArray(dataStructure, std::move(name))
, m_Strings(std::move(strings))
{
}

StringArray::StringArray(DataStructure& dataStructure, std::string name, IdType importId, collection_type strings)
: IArray(dataStructure, std::move(name), importId)
, m_Strings(std::move(strings))
{
}

StringArray::StringArray(const StringArray& other)
: IArray(other)
, m_Strings(other.m_Strings)
{
}

StringArray::StringArray(StringArray&& other) noexcept
: IArray(other)
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
  // Don't construct with identifier since it will get created when inserting into data structure
  const auto copy = std::shared_ptr<StringArray>(new StringArray(dataStruct, copyPath.getTargetName(), m_Strings));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    return copy;
  }
  return nullptr;
}

size_t StringArray::size() const
{
  return m_Strings.size();
}

const StringArray::collection_type& StringArray::values() const
{
  return m_Strings;
}

StringArray::reference StringArray::operator[](usize index)
{
  return m_Strings[index];
}

StringArray::const_reference StringArray::operator[](usize index) const
{
  return m_Strings[index];
}

StringArray::const_reference StringArray::at(usize index) const
{
  if(index >= size())
  {
    throw std::out_of_range(fmt::format("Attempting to access string at index {} out of {}", index, size()));
  }
  return m_Strings[index];
}

StringArray::iterator StringArray::begin()
{
  return m_Strings.begin();
}

StringArray::iterator StringArray::end()
{
  return m_Strings.end();
}

StringArray::const_iterator StringArray::begin() const
{
  return m_Strings.begin();
}

StringArray::const_iterator StringArray::end() const
{
  return m_Strings.end();
}
StringArray::const_iterator StringArray::cbegin() const
{
  return m_Strings.begin();
}

StringArray::const_iterator StringArray::cend() const
{
  return m_Strings.end();
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
  return m_Strings.empty();
}

IArray::ShapeType StringArray::getTupleShape() const
{
  return {size()};
}

IArray::ShapeType StringArray::getComponentShape() const
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

void StringArray::resizeTuples(const std::vector<usize>& tupleShape)
{
  auto numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  if(numTuples != size())
  {
    m_Strings.resize(numTuples);
  }
}
} // namespace nx::core
