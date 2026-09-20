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

  /**
   * @brief Copies string storage into the required destination format.
   * @param destinationFormat Resolved destination format; empty and the canonical in-memory name select memory.
   * @return Independent values with the same shape, or an independent placeholder without values.
   * @throws std::runtime_error If the selected factory or copy fails, or an OOC build rejects an unavailable format.
   * @throws std::bad_alloc If an allocation fails.
   *
   * In-core builds use memory for unavailable formats. OOC builds reject unavailable formats. Factory failures never fall back.
   * Generic transfer holds one string plus backend buffers.
   * @warning Explicit in-memory selection requires a complete resident destination and can exhaust available RAM.
   * This store obeys destinationFormat and does not resolve storage policy itself.
   * Resolve the destination policy before this call. Use the destination DataStructure for automatic policy selection.
   */
  std::unique_ptr<AbstractStringStore> deepCopy(const std::string& destinationFormat) const override;

  usize getNumberOfTuples() const override;

  const ShapeType& getTupleShape() const override;

  /**
   * @brief Changes the tuple shape and retains values in the shared prefix.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   * @return Valid on success. Allocation failure returns error -6035 and preserves the prior store.
   *
   * The Result contract prevents an allocation failure from escaping across the store boundary.
   */
  [[nodiscard]] Result<> resizeTuples(const ShapeType& tupleShape) override;

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
