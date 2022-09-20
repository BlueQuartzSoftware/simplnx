#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

namespace complex
{
template <typename T>
class IGridMontageReader : public IDataFactory
{
public:
  virtual ~IGridMontageReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const
  {
    return GridMontage::GetTypeName();
  }

protected:
  IGridMontageReader();
};
} // namespace complex
