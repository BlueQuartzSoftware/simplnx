#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
template <typename T>
class IDataArrayReader : public IDataFactory
{
public:
  virtual ~IDataArrayReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const
  {
    return DataArray<T>::GetTypeName();
  }

protected:
  IDataArrayReader();
};
} // namespace complex
