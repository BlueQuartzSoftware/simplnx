#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

namespace complex
{
class COMPLEX_EXPORT StringArray : public DataObject
{
public:
  using value_type = std::string;
  using collection_type = std::vector<value_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = typename collection_type::iterator;
  using const_iterator = typename collection_type::const_iterator;

  /**
   * @brief Static function to get the typename
   * @return std::string
   */
  static std::string GetTypeName();

  static StringArray* Create(DataStructure& ds, const std::string_view& name, const std::optional<IdType>& parentId = {});
  static StringArray* CreateWithValues(DataStructure& ds, const std::string_view& name, collection_type strings, const std::optional<IdType>& parentId = {});

  static StringArray* Import(DataStructure& ds, const std::string_view& name, IdType importId, collection_type strings, const std::optional<IdType>& parentId = {});

  StringArray(const StringArray& other);
  StringArray(StringArray&& other) noexcept;

  ~StringArray() noexcept override;

  DataObject::Type getDataObjectType() const override;
  std::string getTypeName() const override;

  DataObject* shallowCopy() override;
  DataObject* deepCopy() override;

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
   * @brief Writes the DataObject to the target HDF5 group.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable = true
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable = true) const override;

protected:
  StringArray(DataStructure& dataStructure, std::string name);
  StringArray(DataStructure& dataStructure, std::string name, IdType importId, collection_type strings);

private:
  collection_type m_Strings;
};
} // namespace complex
