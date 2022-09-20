#pragma once

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class IImageGeomWriter : public IDataFactory
{
public:
  virtual ~IImageGeomWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return ImageGeom::GetTypeName();
  }

protected:
  IImageGeomWriter();
};
} // namespace complex
