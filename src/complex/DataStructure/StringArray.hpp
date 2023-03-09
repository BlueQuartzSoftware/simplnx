#pragma once

#include "complex/DataStructure/IArray.hpp"

namespace complex
{
class COMPLEX_EXPORT StringArray : public IArray
{
public:
  using value_type = std::string;
  using collection_type = std::vector<value_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = typename collection_type::iterator;
  using const_iterator = typename collection_type::const_iterator;

  static inline constexpr StringLiteral k_TypeName = "StringArray";

  static StringArray* Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId = {});
  static StringArray* CreateWithValues(DataStructure& dataStructure, const std::string_view& name, collection_type strings, const std::optional<IdType>& parentId = {});

  static StringArray* Import(DataStructure& dataStructure, const std::string_view& name, IdType importId, collection_type strings, const std::optional<IdType>& parentId = {});

  StringArray(const StringArray& other);
  StringArray(StringArray&& other) noexcept;

  ~StringArray() noexcept override;

  DataObject::Type getDataObjectType() const override;
  std::string getTypeName() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override;

  DataObject* shallowCopy() override;
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  size_t size() const;
  const collection_type& values() const;

  reference operator[](usize index);
  const_reference operator[](usize index) const;
  const_reference at(usize index) const;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  StringArray& operator=(const StringArray& rhs);
  StringArray& operator=(StringArray&& rhs) noexcept;

  /**
   * @brief Returns the number of elements.
   * @return usize
   */
  usize getSize() const override;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  ShapeType getTupleShape() const override;

  /**
   * @brief Returns the component shape.
   * @return
   */
  ShapeType getComponentShape() const override;

  /**
   * @brief Returns the number of tuples.
   * @return usize
   */
  usize getNumberOfTuples() const override;

  /**
   * @brief Returns the number of components per tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override;

  /**
   * @brief Resizes the internal array to accomondate
   */
  void reshapeTuples(const std::vector<usize>& tupleShape) override;

protected:
  StringArray(DataStructure& dataStructure, std::string name);
  StringArray(DataStructure& dataStructure, std::string name, collection_type strings);
  StringArray(DataStructure& dataStructure, std::string name, IdType importId, collection_type strings);

private:
  collection_type m_Strings;
};
} // namespace complex
