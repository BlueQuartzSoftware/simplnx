#pragma once

#include "IDataStore.h"

namespace SIMPL
{
template <typename T>
class DataStore : public IDataStore
{
public:
  using Iterator = void;
  using ConstIterator = void;

  DataStore(size_t tupleSize, size_t tupleCount);
  DataStore(const DataStore& other);
  DataStore(DataStore&& other) noexcept;
  virtual ~DataStore();

  /**
   * @brief 
   * @return size_t
  */
  size_t size() const override;

  /**
   * @brief Returns the number of tuples in the DataStore.
   * @return size_t
   */
  size_t getTupleCount() const override;

  /**
   * @brief Returns the tuple size.
   * @return size_t
   */
  size_t getTupleSize() const override;

  /**
   * @brief Returns the value found at the specified index of the DataStore. This cannot be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T operator[](size_t index) const;

  /**
   * @brief Returns the value found at the specified index of the DataStore. This can be
   * used to edit the value found at the specified index.
   * @param  index
   */
  T& operator[](size_t index);

  /**
   * @brief Returns an Iterator to the begining of the DataStore.
   * @return ConstIterator
   */
  Iterator begin();

  /**
   * @brief Returns an Iterator to the end of the DataArray.
   * @return ConstIterator
   */
  Iterator end();

  /**
   * @brief Returns an ConstIterator to the begining of the DataStore.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an ConstIterator to the end of the DataArray.
   * @return ConstIterator
   */
  ConstIterator end() const;
};
} // namespace SIMPL
