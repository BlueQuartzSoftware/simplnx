#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"
#include "complex/DataStructure/NeighborList.hpp"

namespace complex
{
template <typename T>
class INeighborListWriter : public IDataFactory
{
public:
  virtual ~INeighborListWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return NeighborList<T>::GetTypeName();
  }

protected:
  INeighborListWriter();
};
} // namespace complex
