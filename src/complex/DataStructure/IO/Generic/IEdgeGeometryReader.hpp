#pragma once

#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
template <typename T>
class IEdgeGeomReader : public IDataFactory
{
public:
  virtual ~IEdgeGeomReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const
  {
    return EdgeGeom<T>::GetTypeName();
  }

protected:
  IEdgeGeomReader();
};
} // namespace complex
