#pragma once

#include "AbstractStringStore.hpp"

#include <string>
#include <vector>

namespace nx::core
{
/**
 * @class StringStore
 * @brief The concrete in-memory string storage backend for StringArray.
 *
 * StringStore owns a `std::vector<std::string>` and provides full
 * read/write access to its elements. This is the "real" store that holds
 * loaded string data, as opposed to EmptyStringStore which is a
 * metadata-only placeholder.
 *
 * @see AbstractStringStore The abstract interface this class implements.
 * @see EmptyStringStore The placeholder counterpart used during preflight
 *      or OOC skeleton construction.
 */
class SIMPLNX_EXPORT StringStore : public AbstractStringStore
{
public:
  /**
   * @brief Constructs a StringStore with the specified tuple shape.
   * @param tupleShape The shape of the tuple dimensions
   */
  explicit StringStore(const ShapeType& tupleShape);

  /**
   * @brief Constructs a StringStore with the specified strings and tuple shape.
   * @param strings Vector of initial string values
   * @param tupleShape The shape of the tuple dimensions
   */
  explicit StringStore(std::vector<std::string> strings, const ShapeType& tupleShape);

  /**
   * @brief Destructor.
   */
  ~StringStore() override;

  /**
   * @brief Creates a deep copy of this StringStore.
   * @return std::unique_ptr<AbstractStringStore> Unique pointer to the deep copy
   */
  std::unique_ptr<AbstractStringStore> deepCopy() const override;

  /**
   * @brief Returns the number of tuples in the ListStore.
   * @return usize
   */
  usize getNumberOfTuples() const override;

  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  const ShapeType& getTupleShape() const override;

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  void resizeTuples(const ShapeType& tupleShape) override;

  /**
   * @brief Returns the total number of strings in the store.
   * @return usize The number of strings
   */
  usize size() const override;

  /**
   * @brief Returns whether the string store is empty.
   * @return bool True if the store has no strings, false otherwise
   */
  bool empty() const override;

  /**
   * @brief Returns false because StringStore always contains real, accessible
   * string data (backed by a std::vector<std::string>).
   *
   * This distinguishes StringStore from EmptyStringStore, which is a
   * metadata-only placeholder. Import/backfill code uses isPlaceholder()
   * to decide which string arrays still need their data loaded from disk.
   *
   * @return false Always returns false for StringStore.
   */
  bool isPlaceholder() const override
  {
    return false;
  }

  /**
   * @brief Array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return reference Reference to the string at the specified index
   */
  reference operator[](usize index) override;

  /**
   * @brief Const array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  const_reference operator[](usize index) const override;

  /**
   * @brief Returns a const reference to the string at the specified index with bounds checking.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  const_reference at(usize index) const override;

  /**
   * @brief Returns the string value at the specified index.
   * @param index The index to retrieve
   * @return const_reference Const reference to the string value
   */
  const_reference getValue(usize index) const override;

  /**
   * @brief Sets the string value at the specified index.
   * @param index The index to set
   * @param value The string value to set
   */
  void setValue(usize index, const value_type& value) override;

  /**
   * @brief Assignment operator to replace all strings with values from a vector.
   * @param values Vector of strings to assign
   * @return AbstractStringStore& Reference to this StringStore
   */
  AbstractStringStore& operator=(const std::vector<std::string>& values) override;

private:
  ShapeType m_TupleShape;
  ShapeType::value_type m_NumTuples;
  std::vector<std::string> m_Data;
};
} // namespace nx::core
