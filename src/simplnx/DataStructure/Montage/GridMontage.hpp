#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/Montage/AbstractMontage.hpp"
#include "simplnx/DataStructure/Montage/GridTileIndex.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class GridTileIndex;

/**
 * @class GridMontage
 * @brief The GridMontage class is a type of montage where geometries are laid
 * out in a three dimensional grid.
 */
class SIMPLNX_EXPORT GridMontage : virtual public AbstractMontage
{
public:
  using Iterator = void;
  using DimensionsType = SizeVec3;
  using TileIdType = GridTileIndex;

  static inline constexpr StringLiteral k_TypeName = "GridMontage";

  /**
   * @brief Attempts to create a new GridMontage and insert it into the
   * DataStructure. If the parentId is provided, then the created montage will
   * be nested under the target DataObject. Otherwise, the montage will be
   * placed directly under the DataStructure.
   *
   * If the created montage cannot be placed under the target parent, then this
   * method returns nullptr. Otherwise, this method returns a pointer to the
   * created montage.
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return GridMontage*
   */
  static GridMontage* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Attempts to create a new GridMontage and insert it into the
   * DataStructure. If the parentId is provided, then the created montage will
   * be nested under the target DataObject. Otherwise, the montage will be
   * placed directly under the DataStructure.
   *
   * If the created montage cannot be placed under the target parent, then this
   * method returns nullptr. Otherwise, this method returns a pointer to the
   * created montage.
   *
   * Unlike Create, Import allows the DataObject ID to be set for use in
   * importing data.
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return GridMontage*
   */
  static GridMontage* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Creates a copy of the specified GridMontage but does not add it to
   * the DataStructure. It is up to the caller to delete the created montage.
   * @param other
   */
  GridMontage(const GridMontage& other);

  /**
   * @brief Creates a new GridMontage and moves values from the specified
   * montage.
   * @param other
   */
  GridMontage(GridMontage&& other);

  ~GridMontage() override;

  /**
   * @brief Returns the typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Returns a shallow copy of the current DataObject but does not add
   * it to the DataStructure. It is up to the caller to delete the returned
   * value.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of the current DataObject but does not add it
   * to the DataStructure. It is up to the caller to delete the returned value.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the number of rows in the montage.
   * @return usize
   */
  usize getRowCount() const;

  /**
   * @brief Returns the number of columns in the montage.
   * @return usize
   */
  usize getColumnCount() const;

  /**
   * @brief Returns the number of items deep are in the montage.
   * @return usize
   */
  usize getDepth() const;

  /**
   * @brief Returns the 3D tile dimensions as a SizeVec3.
   * @return SizeVec3
   */
  SizeVec3 getGridSize() const;

  /**
   * @brief Resizes the montage using the specified values.
   * @param row
   * @param col
   * @param depth
   */
  void resizeTileDims(usize row, usize col, usize depth);

  /**
   * @brief Returns the GridTileIndex for the specified tile position. Returns
   * an invalid GridTileIndex if the tile position is out of bounds.
   * @param row
   * @param col
   * @param depth
   * @return GridTileIndex
   */
  GridTileIndex getTileIndex(usize row, usize col, usize depth = 0) const;

  /**
   * @brief Returns a GridTileIndex pointing at the specified 3D tile index.
   * Returns an invalid index if the position is outside of the current
   * dimensions.
   * @param pos
   * @return GridTileIndex
   */
  GridTileIndex getTileIndex(const SizeVec3& pos) const;

  /**
   * @brief Returns the 3D tile position of the specified geometry if it could
   * be found within the montage. Returns an empty object if the geometry could
   * not be found.
   * @param geom
   * @return std::optional<SizeVec3>
   */
  std::optional<SizeVec3> getTilePosOfGeometry(const IGeometry* geom) const;

  /**
   * @brief Returns the tile index for the target geometry. Returns nullptr if the geometry
   * is not part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  std::shared_ptr<AbstractTileIndex> getTileIndex(IGeometry* geom) const override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return IGeometry*
   */
  IGeometry* getGeometry(const AbstractTileIndex* index) override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return const IGeometry*
   */
  const IGeometry* getGeometry(const AbstractTileIndex* index) const override;

  /**
   * @brief Sets the geometry at the position specified by the provided
   * AbstractTileIndex. Does nothing if the index does not reference a valid
   * position within the montage.
   * @param index
   * @param geom
   */
  void setGeometry(const AbstractTileIndex* index, IGeometry* geom) override;

  /**
   * @brief Sets the geometry at the specified 3D tile position. Does nothing
   * if the tile position is not within the current dimensions.
   * @param position
   * @param geom
   */
  void setGeometry(const SizeVec3& position, IGeometry* geom);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const IGeometry*
   */
  IGeometry* getGeometry(const SizeVec3& position);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const IGeometry*
   */
  const IGeometry* getGeometry(const SizeVec3& position) const;

  /**
   * @brief Returns a TooltipGenerator for generating the appropriate HTML tooltips.
   * @return TooltipGenerator
   */
  TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief Returns the current dimensions as a SizeVec3.
   * @return DimensionsType
   */
  DimensionsType getDimensions() const;

  /**
   * @brief Returns the geometry bounds in 3D.
   * @return BoundsType
   */
  BoundsType getBounds() const;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  GridMontage(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  GridMontage(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Returns the appropriate linear offset from the provided
   * GridTileIndex.
   * @param tileId
   * @return usize
   */
  usize getOffsetFromTileId(const TileIdType& tileId) const;

  /**
   * @brief Returns the appropriate linear offset for the specified 3D tile
   * index. This is done by calling getOffsetFromTilePos using the current
   * dimensions.
   * @param tilePos
   * @return usize
   */
  usize getOffsetFromTilePos(const SizeVec3& tilePos) const;

  /**
   * @brief Returns the appropriate linear offset for the specified 3D tile
   * index and dimensions.
   * @param tilePos
   * @param dims
   * @return usize
   */
  static usize getOffsetFromTilePos(const SizeVec3& tilePos, const DimensionsType& dims);

  /**
   * @brief Returns the 3D tile position from the specified linear index.
   * @param offset
   * @return SizeVec3
   */
  SizeVec3 getTilePosFromOffset(usize offset) const;

private:
  usize m_RowCount = 0;
  usize m_ColumnCount = 0;
  usize m_DepthCount = 0;
};
} // namespace nx::core
