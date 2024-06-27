#pragma once

#include "simplnx/Common/IteratorUtility.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

#define NOMINMAX

#include <xtensor/xarray.hpp>
#include <xtensor/xchunked_array.hpp>
#include <xtensor/xstrides.hpp>

#include <nonstd/span.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <mutex>
#include <vector>

namespace nx::core
{
/**
 * @class AbstractDataStore
 * @brief The AbstractDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined but the specifics relating to how data is stored are implemented
 * in subclasses.
 * @tparam T
 */
template <typename T>
class AbstractDataStore : public IDataStore
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using ShapeType = typename IDataStore::ShapeType;
  using index_type = uint64;
  using XArrayType = typename xt::xarray<T>;
  using Iterator = typename XArrayType::iterator;
  using ConstIterator = typename XArrayType::const_iterator;
  using XArrayShapeType = typename xt::xarray<T>::shape_type;

  ~AbstractDataStore() override = default;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return value_type
   */
  virtual value_type getValue(usize index) const = 0;

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  virtual void setValue(usize index, value_type value) = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference operator[](usize index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return const_reference
   */
  virtual const_reference at(usize index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return T&
   */
  virtual reference operator[](usize index) = 0;

  virtual XArrayType& xarray() = 0;

  virtual const XArrayType& xarray() const = 0;

  /**
   * @brief Returns an Iterator to the begining of the DataStore.
   * @return Iterator
   */
  Iterator begin()
  {
    return xarray().begin();
  }

  /**
   * @brief Returns an Iterator to the end of the DataArray.
   * @return Iterator
   */
  Iterator end()
  {
    return xarray().end();
  }

  /**
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return xarray().cbegin();
  }

  /**
   * @brief Returns an ConstIterator to the end of the DataArray.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return xarray().cend();
  }

  /**
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  ConstIterator cbegin() const
  {
    return begin();
  }

  /**
   * @brief Returns an ConstIterator to the end of the DataStore.
   * @return ConstIterator
   */
  ConstIterator cend() const
  {
    return end();
  }

  /**
   * @brief Fills the AbstractDataStore with the specified value.
   * @param value
   */
  virtual void fill(value_type value)
  {
    std::lock_guard<std::mutex> guard(m_Mutex);

    xarray().fill(value);
  }

  virtual bool copy(const AbstractDataStore& other)
  {
    std::lock_guard<std::mutex> guard(m_Mutex);

    if(getSize() != other.getSize())
    {
      return false;
    }
    xarray() = other.xarray();
    return true;
  }

  /**
   * @brief Returns the DataStore's DataType as an enum
   * @return DataType
   */
  DataType getDataType() const override
  {
    return GetDataType<T>();
  }

  /**
   * @brief Returns the size of the stored type of the data store.
   * @return usize
   */
  usize getTypeSize() const override
  {
    return sizeof(T);
  }

  /**
   * @brief Returns the data format used for storing the array data.
   * @return data format as string
   */
  std::string getDataFormat() const override
  {
    return "";
  }

  /**
   * @brief copyData This method copies the number of tuples specified by the
   * totalSrcTuples value starting from the source tuple offset value in <b>sourceArray</b>
   * into the current array starting at the target destination tuple offset value.
   *
   * For example if the DataStore has 10 tuples, the source DataArray has 10 tuples,
   *  the destTupleOffset = 5, the srcTupleOffset = 5, and the totalSrcTuples = 3,
   *  then tuples 5, 6, and 7 will be copied from the source into tuples 5, 6, and 7
   * of the destination array. In psuedo code it would be the following:
   * @code
   *  destArray[5] = sourceArray[5];
   *  destArray[6] = sourceArray[6];
   *  destArray[7] = sourceArray[7];
   *  .....
   * @endcode
   * @param destTupleOffset
   * @param source
   * @param srcTupleOffset
   * @param totalSrcTuples
   * @return
   */
  Result<> copyFrom(usize destTupleOffset, const AbstractDataStore& source, usize srcTupleOffset, usize totalSrcTuples)
  {
    std::lock_guard<std::mutex> guard(m_Mutex);

    if(destTupleOffset >= getNumberOfTuples())
    {
      return MakeErrorResult(-14600, fmt::format("The destination tuple offset ({}) is out of range of the number of available tuples in the data store ({}). Please ensure the destination tuple "
                                                 "offset is less than the number of available tuples.",
                                                 destTupleOffset, getNumberOfTuples()));
    }

    usize sourceNumComponents = source.getNumberOfComponents();
    usize numComponents = getNumberOfComponents();

    if(sourceNumComponents != numComponents)
    {
      return MakeErrorResult(-14601, fmt::format("The number of components in the source data store ({}) does not match the number of components in the destination data store ({}). Please verify "
                                                 "that source and destination data stores have the same number of components.",
                                                 sourceNumComponents, numComponents));
    }

    if((totalSrcTuples * sourceNumComponents + destTupleOffset * numComponents) > getSize())
    {
      return MakeErrorResult(-14602,
                             fmt::format("The total size of tuples to be copied ({}) plus the offset in the destination data store ({}) exceeds the available size of the destination data store ({}).",
                                         totalSrcTuples * sourceNumComponents, destTupleOffset * numComponents, getSize()));
    }

    auto srcBegin = source.begin() + (srcTupleOffset * sourceNumComponents);
    auto srcEnd = srcBegin + (totalSrcTuples * sourceNumComponents);
    auto dstBegin = begin() + (destTupleOffset * numComponents);
    std::copy(srcBegin, srcEnd, dstBegin);
    return {};
  }

  /**
   * @brief Sets all the components of tuple i to value.
   * @param tupleIndex
   * @param value
   */
  void fillTuple(index_type tupleIndex, T value)
  {
    usize numComponents = getNumberOfComponents();
    index_type offset = tupleIndex * numComponents;
    for(usize i = 0; i < numComponents; i++)
    {
      setValue(offset + i, value);
    }
  }

  /**
   * @brief Sets all component values for a tuple using a pointer array of values.
   * The provided pointer is expected to contain at least the same number of values
   * as the number of components.
   *
   * If the tuple index is out of bounds or the provided pointer is null, this method throws a runtime_error.
   * @param tupleIndex
   * @param values
   * @throw std::runtime_error
   */
  void setTuple(index_type tupleIndex, const value_type* values)
  {
    if(values == nullptr)
    {
      throw std::runtime_error("Provided values pointer cannot be null");
    }

    nonstd::span<const value_type> valueSpan(values, values + getNumberOfComponents());
    setTuple(tupleIndex, valueSpan);
  }

  /**
   * @brief Sets all component values for a tuple using a span of values.
   *
   * If the tuple index is out of bounds or the provided span does not match
   * the number of components, this method throws a runtime_error.
   * @param tupleIndex
   * @param values
   * @throw std::runtime_error
   */
  void setTuple(index_type tupleIndex, nonstd::span<const value_type> values)
  {
    if(values.size() != getNumberOfComponents())
    {
      auto ss = fmt::format("Span size ({}) does not match the number of components ({})", values.size(), getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    index_type numComponents = getNumberOfComponents();
    index_type offset = tupleIndex * numComponents;
    usize count = values.size();
    for(usize i = 0; i < count; i++)
    {
      setValue(offset + i, values[i]);
    }
  }

  /**
   * @brief Sets the component value using a given tuple and component index.
   *
   * This method does nothing if the tuple or component indices are out of bounds
   * @param tupleIndex
   * @param componentIndex
   * @param value
   */
  void setComponent(index_type tupleIndex, index_type componentIndex, value_type value)
  {
    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    if(componentIndex >= getNumberOfComponents())
    {
      auto ss = fmt::format("Component index ({}) is greater than or equal to the number of components ({})", componentIndex, getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    index_type index = tupleIndex * getNumberOfComponents() + componentIndex;
    setValue(index, value);
  }

  /**
   * @brief Returns the component value at the specified tuple and component index.
   *
   * This method returns the default T value if either index is out of bounds.
   * @param tupleIndex
   * @param componentIndex
   * @return value_type
   */
  value_type getComponentValue(index_type tupleIndex, index_type componentIndex) const
  {
    if(tupleIndex >= getNumberOfTuples())
    {
      auto ss = fmt::format("Tuple index ({}) is greater than or equal to the number of tuples ({})", tupleIndex, getNumberOfTuples());
      throw std::runtime_error(ss);
    }

    if(componentIndex >= getNumberOfComponents())
    {
      auto ss = fmt::format("Component index ({}) is greater than or equal to the number of components ({})", componentIndex, getNumberOfComponents());
      throw std::runtime_error(ss);
    }

    index_type index = tupleIndex * getNumberOfComponents() + componentIndex;
    return getValue(index);
  }

  std::optional<ShapeType> getChunkShape() const override
  {
    return {};
  }

  /**
   * @brief Returns the data for a particular data chunk. Returns an empty span if the data is not chunked.
   * @param chunkPosition
   * @return chunk data as span
   */
  virtual std::vector<T> getChunkValues(const ShapeType& chunkPosition) const
  {
    return {};
  }

  /**
   * @brief Flushes the data store to its respective target.
   * In-memory DataStores are not affected.
   */
  virtual void flush() const
  {
  }

  virtual uint64 memoryUsage() const
  {
    return sizeof(T) * getSize();
  }

protected:
  /**
   * @brief Default constructor
   */
  AbstractDataStore() = default;
  AbstractDataStore(const AbstractDataStore& other)
  : IDataStore(other)
  {
  }

  AbstractDataStore(AbstractDataStore&& other)
  : IDataStore(std::move(other))
  {
  }

  mutable std::mutex m_Mutex;
};

using UInt8AbstractDataStore = AbstractDataStore<uint8>;
using UInt16AbstractDataStore = AbstractDataStore<uint16>;
using UInt32AbstractDataStore = AbstractDataStore<uint32>;
using UInt64AbstractDataStore = AbstractDataStore<uint64>;

using Int8AbstractDataStore = AbstractDataStore<int8>;
using Int16AbstractDataStore = AbstractDataStore<int16>;
using Int32AbstractDataStore = AbstractDataStore<int32>;
using Int64AbstractDataStore = AbstractDataStore<int64>;

using BoolAbstractDataStore = AbstractDataStore<bool>;

using Float32AbstractDataStore = AbstractDataStore<float32>;
using Float64AbstractDataStore = AbstractDataStore<float64>;
} // namespace nx::core
