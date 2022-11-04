#pragma once

#include "complex/DataStructure/DataObject.hpp"

namespace complex
{
class COMPLEX_EXPORT IArray : public DataObject
{
public:
  using ShapeType = std::vector<usize>;

  static inline constexpr StringLiteral k_TypeName = "IArray";

  ~IArray() override = default;

  /**
   * @brief Returns the number of elements.
   * @return usize
   */
  virtual usize getSize() const = 0;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  virtual ShapeType getTupleShape() const = 0;

  /**
   * @brief Returns the component shape.
   * @return
   */
  virtual ShapeType getComponentShape() const = 0;

  /**
   * @brief Returns the number of tuples.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;

  /**
   * @brief Returns the number of components per tuple.
   * @return usize
   */
  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Resizes the internal array to accomondate
   */
  virtual void reshapeTuples(const std::vector<usize>& tupleShape) = 0;

protected:
  IArray() = delete;

  IArray(DataStructure& dataStructure, std::string name)
  : DataObject(dataStructure, std::move(name))
  {
  }

  IArray(DataStructure& dataStructure, std::string name, IdType importId)
  : DataObject(dataStructure, std::move(name), importId)
  {
  }

  IArray(const IArray&) = default;
  IArray(IArray&&) = default;

  IArray& operator=(const IArray&) = delete;
  IArray& operator=(IArray&&) noexcept = delete;
};
} // namespace complex
