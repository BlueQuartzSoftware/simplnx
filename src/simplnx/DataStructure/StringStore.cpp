#include "StringStore.hpp"

#include <numeric>

namespace nx::core
{
StringStore::StringStore(const ShapeType& tupleShape)
: AbstractStringStore()
, m_TupleShape(tupleShape.cbegin(), tupleShape.cend())
, m_NumTuples(std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>()))
, m_Data(m_NumTuples)
{
}

StringStore::StringStore(std::vector<std::string> strings, const ShapeType& tupleShape)
: AbstractStringStore()
, m_TupleShape(tupleShape.cbegin(), tupleShape.cend())
, m_NumTuples(strings.size())
, m_Data(std::move(strings))
{
}

StringStore::~StringStore() = default;

usize StringStore::getNumberOfTuples() const
{
  return m_NumTuples;
}

const StringStore::ShapeType& StringStore::getTupleShape() const
{
  return m_TupleShape;
}

void StringStore::resizeTuples(const ShapeType& tupleShape)
{
  m_TupleShape = tupleShape;
  m_NumTuples = std::accumulate(m_TupleShape.cbegin(), m_TupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  m_Data.resize(m_NumTuples);
}

usize StringStore::size() const
{
  return m_NumTuples;
}

bool StringStore::empty() const
{
  return m_NumTuples == 0;
}

StringStore::reference StringStore::operator[](usize index)
{
  return m_Data[index];
}
StringStore::const_reference StringStore::operator[](usize index) const
{
  return m_Data[index];
}
StringStore::const_reference StringStore::at(usize index) const
{
  return getValue(index);
}

StringStore::const_reference StringStore::getValue(usize index) const
{
  return m_Data.at(index);
}

void StringStore::setValue(usize index, const value_type& value)
{
  m_Data.at(index) = value;
}

std::unique_ptr<AbstractStringStore> StringStore::deepCopy() const
{
  return std::make_unique<StringStore>(m_Data, m_TupleShape);
}

AbstractStringStore& StringStore::operator=(const std::vector<std::string>& values)
{
  m_Data = values;
  m_TupleShape = ShapeType{values.size()};
  return *this;
}
} // namespace nx::core
