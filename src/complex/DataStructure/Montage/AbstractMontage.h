#pragma once

#include "SIMPL/DataStructure/BaseGroup.h"
#include "SIMPL/DataStructure/LinkedPath.h"
#include "SIMPL/Utilities/TooltipGenerator.h"

namespace SIMPL
{
class AbstractTileIndex;
class AbstractGeometry;

/**
 * class AbstractMontage
 *
 */

class AbstractMontage : public BaseGroup
{
public:
  using Iterator = void;
  using ConstIterator = void;
  using CollectionType = std::vector<AbstractGeometry*>;
  using BoundsType = void;

  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractMontage(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param other
   */
  AbstractMontage(const AbstractMontage& other);

  /**
   * @brief
   * @param other
   * @return
   */
  AbstractMontage(AbstractMontage&& other) noexcept;

  /**
   * @brief
   */
  virtual ~AbstractMontage();

  /**
   * @brief
   * @return bool
   */
  virtual bool isValid() const = 0;

  /**
   * @brief Returns the number of tiles in the montage.
   * @return size_t
   */
  virtual size_t getTileCount() const = 0;

  /**
   * @brief Returns a collection of geometries in the Montage.
   * @return CollectionType
   */
  virtual CollectionType getGeometries() const = 0;

  /**
   * @brief Returns a collection of the geometry names in the montage.
   * @return std::vector<std::string>
   */
  virtual std::vector<std::string> getGeometryNames() const = 0;

  /**
   * @brief Returns a collection of LinkedPaths to geometries in the montage.
   * @return std::vector<LinkedPath>
   */
  virtual std::vector<LinkedPath> getLinkedPaths() const = 0;

  /**
   * @brief Returns a TooltipGenerator for the montage.
   * @return SIMPL::TooltipGenerator
   */
  virtual SIMPL::TooltipGenerator getTooltipGenerator() const = 0;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index. Returns nullptr
   * if no geometry was found.
   * @param index
   * @return AbstractGeometry*
   * @param  index
   */
  virtual AbstractGeometry* getGeometry(AbstractTileIndex* index) const = 0;

  /**
   * @brief Returns the tile index for the specified geometry. This is a pure virtual method
   * to be implemented in derived classes. Returns nullptr if the geometry is not
   * part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  virtual std::shared_ptr<AbstractTileIndex> getTileIndex(AbstractGeometry* geom) const = 0;

  /**
   * @brief
   * @param index
   * @param geom
   */
  virtual void setGeometry(AbstractTileIndex* index, AbstractGeometry* geom) = 0;

  /**
   * @brief Creates and returns a deep copy of the montage.
   * @return AbstractMontage*
   */
  virtual AbstractMontage* deepCopy() const = 0;

  /**
   * @brief Returns an iterator to the begining of the montage.
   * @return iterator
   */
  virtual Iterator begin() = 0;

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return iterator
   */
  virtual Iterator end() = 0;

protected:
  /**
   * @brief Checks if the specified DataObject can be added to the montage. Returns true if
   * the object can be added. Returns false otherwise.

   * This is an override of the
   * method in BaseGroup to ensure that only geometries are added directly to a
   * montage.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

private:
};
} // namespace SIMPL
