#pragma once

#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IRectGeomReader : public IDataFactory
{
public:
  virtual ~IRectGeomReader() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return RectGridGeom::GetTypeName();
  }

protected:
  IRectGeomReader();
};
} // namespace complex
