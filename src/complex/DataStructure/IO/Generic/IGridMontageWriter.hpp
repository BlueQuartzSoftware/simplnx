#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

namespace complex
{
template <typename T>
class IGridMontageWriter : public IDataFactory
{
public:
  virtual ~IGridMontageWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const
  {
    return GridMontage::GetTypeName();
  }

protected:
  IGridMontageWriter();
};
} // namespace complex
