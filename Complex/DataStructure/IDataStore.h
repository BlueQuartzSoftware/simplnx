#pragma once

namespace SIMPL
{
class IDataStore
{
  /**
   * @brief
   * @return size_t
   */
  virtual size_t size() const = 0;

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
};
}
