#pragma once

#include "complex/Utilities/Parsing/Zarr/ZarrIDataFactory.hpp"

namespace complex
{
namespace Zarr
{
class COMPLEX_EXPORT QuadGeomFactory : public Zarr::IDataFactory
{
public:
  QuadGeomFactory();

  ~QuadGeomFactory() override;

  /**
   * @brief Returns the name of the DataObject subclass that the factory is designed for.
   * @return std::string
   */
  std::string getDataTypeName() const override;

  /**
   * @brief Creates and adds a DataObject to the provided DataStructure from
   * the target Zarr object.
   * @param dataStructureReader Current DataStructureReader for reading the DataStructure.
   * @param parentReader Wrapper around the parent Zarr group.
   * @param baseReader Wrapper around an Zarr object reader.
   * @param parentId = {} Optional DataObject ID describing which parent object
   * to create the generated DataObject under.
   * @param preflight = false
   * @return error_type
   */
  error_type read(Zarr::DataStructureReader& dataStructureReader, const FileVec::Group& parentReader, const FileVec::BaseCollection& baseReader,
                  const std::optional<complex::DataObject::IdType>& parentId = {}, bool preflight = false) override;
};
} // namespace Zarr
} // namespace complex
