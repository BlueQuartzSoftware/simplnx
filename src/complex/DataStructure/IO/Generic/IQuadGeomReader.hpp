#pragma once

#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IQuadGeomReader : public IDataFactory
{
public:
  virtual ~IQuadGeomReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return QuadGeom::GetTypeName();
  }

protected:
  IQuadGeomReader();
};
} // namespace complex
