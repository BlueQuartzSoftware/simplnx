#pragma once

#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IVertexGeomWriter : public IDataFactory
{
public:
  virtual ~IVertexGeomWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return VertexGeom::GetTypeName();
  }

protected:
  IVertexGeomWriter();
};
} // namespace complex
