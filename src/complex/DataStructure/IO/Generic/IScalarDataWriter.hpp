#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/DataStructure/ScalarData.hpp"

namespace complex
{
template <typename T>
class IScalarDataWriter : public IDataFactory
{
public:
  virtual ~IScalarDataWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return ScalarData<T>::GetTypeName();
  }

protected:
  IScalarDataWriter();
};
} // namespace complex
