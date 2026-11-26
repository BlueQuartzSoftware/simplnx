#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <vector>

namespace nx::core
{
class AttributeMatrix;
class DataPath;
class DataStructure;
class ImageGeom;
struct OutputActions;
class VertexGeom;

namespace ImageGeometryFeatureTest
{
/** @brief Creates a 4 x 4 x 1 image whose cell in column x has Feature Id x + 1. */
DataStructure CreateFourColumnImage();

/** @brief Adds counts, neighbors and names for four adjacent features, plus their ensemble data. */
void AddFourFeatureData(DataStructure& dataStructure, const ImageGeom& imageGeom, int32 cellsPerFeature);

struct FeatureAliasSnapshot
{
  DataObject::IdType featureId;
  ShapeType shape;
  DataObject::ParentCollectionType parents;
  std::vector<int32> counts;
};

/** @brief Links the Feature Attribute Matrix under Misc, links Misc under Outside, and records the source before filtering. */
FeatureAliasSnapshot AddNestedFeatureAlias(DataStructure& ds, const DataPath& sourcePath);
void CheckNestedFeatureCopyAction(const OutputActions& actions, const DataPath& sourcePath, const DataPath& outputPath, bool clear);
void CheckNestedFeatureAlias(const DataStructure& ds, const DataPath& sourcePath, const DataPath& outputPath, bool clear, const FeatureAliasSnapshot& original);

/** @brief Adds a vertex geometry that also uses the feature matrix as its vertex data. */
VertexGeom& ShareFeatureMatrixWithVertexGeometry(DataStructure& ds, const AttributeMatrix& features);

void CreateImageWithGeometryNameInChildren(DataStructure& ds);
void CheckChildNamesInCopyAction(const OutputActions& actions, const DataPath& srcGeomPath, const DataPath& destGeomPath);
void CheckCopiedChildNames(const DataStructure& ds, const DataPath& destGeomPath);
} // namespace ImageGeometryFeatureTest
} // namespace nx::core
