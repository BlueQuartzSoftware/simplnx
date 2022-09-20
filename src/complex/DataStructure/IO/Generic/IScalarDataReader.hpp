#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/DataStructure/ScalarData.hpp"

namespace complex
{
template <typename T>
class IScalarDataReader : public IDataFactory
{
public:
  virtual ~IScalarDataReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return ScalarData<T>::GetTypeName();
  }

protected:
  IScalarDataReader();
};
} // namespace complex
