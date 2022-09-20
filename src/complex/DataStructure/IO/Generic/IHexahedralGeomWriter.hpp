#pragma once

#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
template <typename T>
class IHexahedralGeomWriter : public IDataFactory
{
public:
  virtual ~IHexahedralGeomWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  virtual std::string getDataTypeName() const
  {
    return HexahedralGeom::GetTypeName();
  }

protected:
  IHexahedralGeomWriter();
};
} // namespace complex
