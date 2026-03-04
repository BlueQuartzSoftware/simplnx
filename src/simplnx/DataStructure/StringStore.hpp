#pragma once

#include "IStringStore.hpp"

#include <string>
#include <vector>

namespace nx::core
{
class StringStore : public IStringStore
{
public:
  explicit StringStore(const ShapeType& tupleShape);
  explicit StringStore(std::vector<std::string> strings, const ShapeType& tupleShape);
  ~StringStore();

  std::unique_ptr<IStringStore> deepCopy() const override;

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

  usize size() const override;
  bool empty() const override;

  reference operator[](usize index) override;
  const_reference operator[](usize index) const override;
  const_reference at(usize index) const override;

  const_reference getValue(usize index) const override;
  void setValue(usize index, const value_type& value) override;

  IStringStore& operator=(const std::vector<std::string>& values) override;

private:
  ShapeType m_TupleShape;
  ShapeType::value_type m_NumTuples;
  std::vector<std::string> m_Data;
};
} // namespace nx::core
