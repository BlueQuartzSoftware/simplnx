#pragma once

#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT IDataStore
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
} // namespace complex
