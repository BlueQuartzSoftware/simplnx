#pragma once

#include "AbstractStringStore.hpp"

#include <string>
#include <vector>

namespace nx::core
{

/**
 * @class StringStore
 * @brief Stores materialized StringArray values in memory.
 *
 * StringStore owns a std::vector<std::string>. Import finalization replaces an
 * EmptyStringStore placeholder with this store before string access. Tuple-shape
 * products must fit usize. The value constructor derives the data count from
 * strings. For a materialized nonzero array, the tuple-shape product must equal
 * strings.size(). A metadata-only import can supply no strings only when it
 * immediately replaces the store. resizeTuples() retains leading values and
 * value-initializes new values.
 */
class SIMPLNX_EXPORT StringStore : public AbstractStringStore
{
public:
  explicit StringStore(const ShapeType& tupleShape);

  explicit StringStore(std::vector<std::string> strings, const ShapeType& tupleShape);

  ~StringStore() override;

  std::unique_ptr<AbstractStringStore> deepCopy() const override;

  usize getNumberOfTuples() const override;

  const ShapeType& getTupleShape() const override;

  void resizeTuples(const ShapeType& tupleShape) override;

  usize size() const override;
  bool empty() const override;

  bool isPlaceholder() const override
  {
    return false;
  }

  reference operator[](usize index) override;

  const_reference operator[](usize index) const override;

  const_reference at(usize index) const override;

  const_reference getValue(usize index) const override;

  void setValue(usize index, const value_type& value) override;

  /**
   * @brief Replaces stored values.
   * @param values Source strings.
   * @return This store.
   * @pre values.size() equals getNumberOfTuples().
   *
   * The implementation does not update the stored tuple count.
   */
  AbstractStringStore& operator=(const std::vector<std::string>& values) override;

private:
  ShapeType m_TupleShape;
  ShapeType::value_type m_NumTuples;
  std::vector<std::string> m_Data;
};
} // namespace nx::core
