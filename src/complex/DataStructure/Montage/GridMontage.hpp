#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Montage/AbstractMontage.hpp"
#include "complex/DataStructure/Montage/GridTileIndex.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class GridTileIndex;

/**
 * class GridMontage
 *
 */

class COMPLEX_EXPORT GridMontage : virtual public AbstractMontage
{
public:
  friend class DataStructure;

  using Iterator = void;
  using DimensionsType = SizeVec3;
  using TileIdType = GridTileIndex;

  /**
   * @brief
   * @param other
   */
  GridMontage(const GridMontage& other);

  /**
   * @brief
   * @param other
   */
  GridMontage(GridMontage&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~GridMontage();

  /**
   * @brief Returns a shallow copy of the current DataObject.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of hte current DataObject.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Returns the number of rows in the montage.
   * @return size_t
   */
  size_t getRowCount() const;

  /**
   * @brief Returns the number of columns in the montage.
   * @return size_t
   */
  size_t getColumnCount() const;

  /**
   * @brief Returns the depth of the montage in number of items.
   * @return size_t
   */
  size_t getDepth() const;

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
  void resizeTileDims(size_t row, size_t col, size_t depth);

  /**
   * @brief Returns the GridTileIndex for the specified tile position. Returns
   * an invalid GridTileIndex if the tile position is out of bounds.
   * @param row
   * @param col
   * @param depth
   * @return GridTileIndex
   */
  GridTileIndex getTileIndex(size_t row, size_t col, size_t depth) const;

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
  std::optional<SizeVec3> getTilePosOfGeometry(const AbstractGeometry* geom) const;

  /**
   * @brief Returns the tile index for the target geometry. Returns nullptr if the geometry
   * is not part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  std::shared_ptr<AbstractTileIndex> getTileIndex(AbstractGeometry* geom) const override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return AbstractGeometry*
   */
  AbstractGeometry* getGeometry(const AbstractTileIndex* index) override;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return const AbstractGeometry*
   */
  const AbstractGeometry* getGeometry(const AbstractTileIndex* index) const override;

  /**
   * @brief Sets the geometry at the position specified by the provided
   * AbstractTileIndex. Does nothing if the index does not reference a valid
   * position within the montage.
   * @param index
   * @param geom
   */
  void setGeometry(const AbstractTileIndex* index, AbstractGeometry* geom) override;

  /**
   * @brief Sets the geometry at the specified 3D tile position. Does nothing
   * if the tile position is not within the current dimensions.
   * @param position
   * @param geom
   */
  void setGeometry(const SizeVec3& position, AbstractGeometry* geom);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const AbstractGeometry*
   */
  AbstractGeometry* getGeometry(const SizeVec3& position);

  /**
   * @brief Returns the geometry at the specified position.
   * Returns nullptr if no geometry could be found.
   * @return const AbstractGeometry*
   */
  const AbstractGeometry* getGeometry(const SizeVec3& position) const;

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
   * @param ds
   * @param name
   */
  GridMontage(DataStructure* ds, const std::string& name);

  /**
   * @brief Returns the appropriate linear offset from the provided
   * GridTileIndex.
   * @param tileId
   * @return size_t
   */
  size_t getOffsetFromTileId(const TileIdType& tileId) const;

  /**
   * @brief Returns the appropriate linear offset for the specified 3D tile
   * index. This is done by calling getOffsetFromTilePos using the current
   * dimensions.
   * @param tilePos
   * @return size_t
   */
  size_t getOffsetFromTilePos(const SizeVec3& tilePos) const;

  /**
   * @brief Returns the appropriate linear offset for the specified 3D tile
   * index and dimensions.
   * @param tilePos
   * @param dims
   * @return size_t
   */
  static size_t getOffsetFromTilePos(const SizeVec3& tilePos, const DimensionsType& dims);

  /**
   * @brief Returns the 3D tile position from the specified linear index.
   * @param offset
   * @return SizeVec3
   */
  SizeVec3 getTilePosFromOffset(size_t offset) const;

private:
  size_t m_RowCount = 0;
  size_t m_ColumnCount = 0;
  size_t m_DepthCount = 0;
};
} // namespace complex
