#pragma once

#include "simplnx/Common/BoundingBox.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/Utilities/TooltipGenerator.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
class AbstractTileIndex;
class IGeometry;

/**
 * @class AbstractMontage
 * @brief
 */
class SIMPLNX_EXPORT AbstractMontage : public BaseGroup
{
public:
  using CollectionType = std::vector<IGeometry*>;
  using Iterator = CollectionType::iterator;
  using ConstIterator = CollectionType::const_iterator;
  using BoundsType = void;

  static inline constexpr StringLiteral k_TypeName = "AbstractMontage";

  /**
   * @brief Creates a copy of the target AbstractMontage but does not add it to
   * DataStructure. The caller is responsible for deleting the constructed
   * montage.
   * @param other
   */
  AbstractMontage(const AbstractMontage& other);

  /**
   * @brief Create an AbstractMontage and moves values from the specified
   * AbstractMontage.
   * @param other
   */
  AbstractMontage(AbstractMontage&& other);

  ~AbstractMontage() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns the number of tiles in the montage.
   * @return usize
   */
  usize getTileCount() const;

  /**
   * @brief Returns a collection of geometries in the Montage.
   * @return CollectionType
   */
  CollectionType getGeometries() const;

  /**
   * @brief Returns a collection of the geometry names in the montage.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getGeometryNames() const;

  /**
   * @brief Returns a collection of LinkedPaths to geometries in the montage.
   * @return std::vector<LinkedPath>
   */
  std::vector<LinkedPath> getLinkedPaths() const;

  /**
   * @brief Returns a TooltipGenerator for the montage.
   * @return TooltipGenerator
   */
  virtual TooltipGenerator getTooltipGenerator() const = 0;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index.
   * Returns nullptr if no geometry was found.
   * @param index
   * @return IGeometry*
   */
  virtual IGeometry* getGeometry(const AbstractTileIndex* index) = 0;

  /**
   * @brief Returns a pointer to the geometry at the specified tile index.
   * Returns nullptr if no geometry was found.
   * @param index
   * @return const IGeometry*
   */
  virtual const IGeometry* getGeometry(const AbstractTileIndex* index) const = 0;

  /**
   * @brief Returns the tile index for the specified geometry. This is a pure
   * virtual method to be implemented in derived classes. Returns nullptr if
   * the geometry is not part of the montage.
   * @param geom
   * @return std::shared_ptr<AbstractTileIndex>
   */
  virtual std::shared_ptr<AbstractTileIndex> getTileIndex(IGeometry* geom) const = 0;

  /**
   * @brief Sets the geometry for the target tile index. The implementation is
   * dependent on the derived class.
   * @param index
   * @param geom
   */
  virtual void setGeometry(const AbstractTileIndex* index, IGeometry* geom) = 0;

  /**
   * @brief Returns an iterator to the beginning of the montage.
   * @return iterator
   */
  virtual Iterator begin();

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return iterator
   */
  virtual Iterator end();

  /**
   * @brief Returns an iterator to the beginning of the montage.
   * @return iterator
   */
  virtual ConstIterator begin() const;

  /**
   * @brief Returns an iterator to the end of the montage.
   * @return iterator
   */
  virtual ConstIterator end() const;

protected:
  /**
   * @brief Constructs a new Abstract montage for the target DataStructure
   * using the provided name.
   * @param dataStructure
   * @param name
   */
  AbstractMontage(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a new Abstract montage for the target DataStructure
   * using the provided name.
   * @param dataStructure
   * @param name
   * @param importId
   */
  AbstractMontage(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Checks if the specified DataObject can be added to the montage.
   * Returns true if the object can be added. Returns false otherwise.

   * This is an override of the method in BaseGroup to ensure that only
   * geometries are added directly to a montage.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

  /**
   * @brief Returns a reference of the collection for use in derived classes.
   * @return CollectionType&
   */
  CollectionType& getCollection();

  /**
   * @brief Returns a reference of the collection for use in derived classes.
   * @return const CollectionType&
   */
  const CollectionType& getCollection() const;

private:
  CollectionType m_Collection;
};
} // namespace nx::core
