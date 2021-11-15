#pragma once

#include "complex/DataStructure/DataObject.hpp"

namespace complex
{
class COMPLEX_EXPORT IDataArray : public DataObject
{
public:
  virtual ~IDataArray() = default;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return "DataArray";
  }

  /**
   * @brief Returns the number of elements in the DataArray.
   * @return usize
   */
  virtual usize getSize() const = 0;

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return usize
   */
  virtual size_t getNumberOfTuples() const = 0;

  /**
   * @brief Returns the tuple getSize.
   * @return usize
   */
  virtual size_t getNumberOfComponents() const = 0;

protected:
  IDataArray(DataStructure& dataStructure, const std::string& name)
  : DataObject(dataStructure, name)
  {
  }
  IDataArray(DataStructure& dataStructure, const std::string& name, IdType importId)
  : DataObject(dataStructure, name, importId)
  {
  }
};
} // namespace complex
