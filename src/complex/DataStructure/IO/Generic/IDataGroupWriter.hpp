#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IDataGroupFactory : public IDataFactory
{
public:
  virtual ~IDataGroupFactory() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

protected:
  IDataGroupFactory();
};
} // namespace complex
