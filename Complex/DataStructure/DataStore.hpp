#pragma once

namespace Complex
{
template <typename T>
class DataStore
{
public:
  using Iterator = void;
  using ConstIterator = void;

  /**
   * @brief
   * @param tupleSize
   * @param tupleCount
   */
  DataStore(size_t tupleSize, size_t tupleCount);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStore(const DataStore& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataStore(DataStore&& other) noexcept;

  virtual ~DataStore();

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  virtual size_t getTupleCount() const = 0;

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  virtual size_t getTupleSize() const = 0;

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return size_t
   */
  size_t size() const
  {
    return getTupleCount() * getTupleSize();
  }

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param index
   * @return T
   */
  virtual T getValue(size_t index) const = 0;

  /**
   * @brief Sets the value stored at the specified index.
   * @param index
   * @param value
   */
  virtual void setValue(size_t index, T value) = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This cannot be used to edit the value found at the specified index.
   * @param  index
   * @return T
   */
  virtual T operator[](size_t index) const = 0;

  /**
   * @brief Returns the value found at the specified index of the DataStore.
   * This can be used to edit the value found at the specified index.
   * @param  index
   * @return T&
   */
  virtual T& operator[](size_t index) = 0;

  /**
   * @brief Returns an Iterator to the begining of the DataStore.
   * @return Iterator
   */
  virtual Iterator begin();

  /**
   * @brief Returns an Iterator to the end of the DataArray.
   * @return Iterator
   */
  virtual Iterator end();

  /**
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  virtual ConstIterator begin() const;

  /**
   * @brief Returns an ConstIterator to the end of the DataArray.
   * @return ConstIterator
   */
  virtual ConstIterator end() const;
};
} // namespace Complex
