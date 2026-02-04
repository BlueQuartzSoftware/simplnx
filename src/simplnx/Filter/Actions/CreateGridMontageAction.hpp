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
class SIMPLNX_EXPORT CreateGridMontageAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<usize>;
  using OriginType = std::vector<float32>;
  using SpacingType = std::vector<float32>;

  CreateGridMontageAction() = delete;

  /**
   * @brief Constructs a CreateGridMontageAction.
   * @param path The path where the GridMontage will be created
   * @param dims The dimensions of the montage grid: ORDERED: X, Y, Z
   * @param origin The origin of the montage
   * @param spacing The spacing of the montage
   */
  CreateGridMontageAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing);

  ~CreateGridMontageAction() noexcept override = default;

  CreateGridMontageAction(const CreateGridMontageAction&) = delete;
  CreateGridMontageAction(CreateGridMontageAction&&) noexcept = delete;
  CreateGridMontageAction& operator=(const CreateGridMontageAction&) = delete;
  CreateGridMontageAction& operator=(CreateGridMontageAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
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
   * @brief Returns the dimensions of the GridMontage to be created.
   * @return const DimensionType& The montage dimensions
   */
  const DimensionType& dims() const;

  /**
   * @brief Returns the origin of the GridMontage to be created.
   * @return const OriginType& The montage origin
   */
  const OriginType& origin() const;

  /**
   * @brief Returns the spacing of the GridMontage to be created.
   * @return const SpacingType& The montage spacing
   */
  const SpacingType& spacing() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  DimensionType m_Dims;
  OriginType m_Origin;
  SpacingType m_Spacing;
};
} // namespace nx::core
