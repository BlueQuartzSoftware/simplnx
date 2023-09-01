#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class COMPLEX_EXPORT CreateGridMontageAction : public IDataCreationAction
{
public:
  using DimensionType = std::vector<usize>;
  using OriginType = std::vector<float32>;
  using SpacingType = std::vector<float32>;

  CreateGridMontageAction() = delete;
  /**
   * @brief
   * @param path
   * @param dims The dimensions of the newly created Image Geometry: ORDERED: X, Y, Z.
   * @param origin
   * @param spacing
   * @param cellAttributeMatrixName
   */
  CreateGridMontageAction(const DataPath& path, const DimensionType& dims, const OriginType& origin, const SpacingType& spacing);

  ~CreateGridMontageAction() noexcept = default;

  CreateGridMontageAction(const CreateGridMontageAction&) = delete;
  CreateGridMontageAction(CreateGridMontageAction&&) noexcept = delete;
  CreateGridMontageAction& operator=(const CreateGridMontageAction&) = delete;
  CreateGridMontageAction& operator=(CreateGridMontageAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return DataPath
   */
  DataPath path() const;

  /**
   * @brief Returns the dimensions of the ImageGeometry to be created.
   * @return
   */
  const DimensionType& dims() const;

  /**
   * @brief Returns the origin of the ImageGeometry to be created.
   * @return
   */
  const OriginType& origin() const;

  /**
   * @brief Returns the spacing of the ImageGeometry to be created.
   * @return
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
} // namespace complex
