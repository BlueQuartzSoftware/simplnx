#pragma once

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class ITriangleGeomWriter : public IDataFactory
{
public:
  virtual ~ITriangleGeomWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return TriangleGeom::GetTypeName();
  }

protected:
  ITriangleGeomWriter();
};
} // namespace complex
