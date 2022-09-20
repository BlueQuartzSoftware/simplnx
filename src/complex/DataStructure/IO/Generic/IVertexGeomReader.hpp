#pragma once

#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IVertexGeomReader : public IDataFactory
{
public:
  virtual ~IVertexGeomReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return VertexGeom::GetTypeName();
  }

protected:
  IVertexGeomReader();
};
} // namespace complex
