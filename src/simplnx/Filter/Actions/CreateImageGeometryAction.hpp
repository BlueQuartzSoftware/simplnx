#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class SIMPLNX_EXPORT CreateImageGeometryAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  CreateImageGeometryAction() = delete;

  /**
   * @brief Constructs a CreateImageGeometryAction.
   * @param path The path where the ImageGeometry will be created
   * @param dims The dimensions of the newly created ImageGeometry: ORDERED: X, Y, Z
   * @param origin The origin of the ImageGeometry
   * @param spacing The spacing of the ImageGeometry
   * @param cellAttributeMatrixName The name of the cell AttributeMatrix to be created
   * @param units The length units for the geometry
   */
  CreateImageGeometryAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing, const std::string& cellAttributeMatrixName,
                            IGeometry::LengthUnit units = IGeometry::LengthUnit::Micrometer);

  ~CreateImageGeometryAction() noexcept override;

  CreateImageGeometryAction(const CreateImageGeometryAction&) = delete;
  CreateImageGeometryAction(CreateImageGeometryAction&&) noexcept = delete;
  CreateImageGeometryAction& operator=(const CreateImageGeometryAction&) = delete;
  CreateImageGeometryAction& operator=(CreateImageGeometryAction&&) noexcept = delete;

  /**
   * @brief Validates image metadata in preflight and execute before creating the geometry.
   * A metadata error leaves the data structure unchanged. A later creation error can leave a partial result.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Success or a metadata error. Error -5707 indicates fewer than three origin or spacing components.
   * Error -1557 indicates a nonfinite origin. Error -1556 indicates nonfinite or nonpositive spacing.
   * Path and creation errors use -5701 through -5706.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns the dimensions of the ImageGeometry to be created.
   * @return const DimensionType& The geometry dimensions
   */
  const DimensionType& dims() const;

  /**
   * @brief Returns the origin of the ImageGeometry to be created.
   * @return const OriginType& The geometry origin
   */
  const OriginType& origin() const;

  /**
   * @brief Returns the spacing of the ImageGeometry to be created.
   * @return const SpacingType& The geometry spacing
   */
  const SpacingType& spacing() const;

  /**
   * @brief Returns the cell attribute matrix name of the ImageGeometry to be created.
   * @return std::string The cell attribute matrix name
   */
  std::string cellAttributeMatrixName() const;

  /**
   * @brief Returns the units of the ImageGeometry to be created.
   * @return IGeometry::LengthUnit The length units
   */
  IGeometry::LengthUnit units() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  DimensionType m_Dims;
  OriginType m_Origin;
  SpacingType m_Spacing;
  IGeometry::LengthUnit m_Units;
  std::string m_CellDataName;
};
} // namespace nx::core
