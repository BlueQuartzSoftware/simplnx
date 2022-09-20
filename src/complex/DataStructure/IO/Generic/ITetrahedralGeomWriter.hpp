#pragma once

#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

namespace complex
{
class ITetrahedralGeomWriter : public IDataFactory
{
public:
  virtual ~ITetrahedralGeomWriter() noexcept;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override
  {
    return TetrahedralGeom::GetTypeName();
  }

protected:
  ITetrahedralGeomWriter();
};
} // namespace complex
