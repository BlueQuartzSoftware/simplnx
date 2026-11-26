#include "ImageGeometryFeatureTestUtils.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>

namespace nx::core::ImageGeometryFeatureTest
{
void AddFourFeatureData(DataStructure& dataStructure, const ImageGeom& imageGeom, int32 cellsPerFeature)
{
  auto* featureData = AttributeMatrix::Create(dataStructure, "Cell Feature Data", ShapeType{5}, imageGeom.getId());
  REQUIRE(featureData != nullptr);
  auto* numElements = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumElements", {5}, {1}, featureData->getId());
  REQUIRE(numElements != nullptr);
  numElements->fill(cellsPerFeature);
  (*numElements)[0] = 0;
  auto* neighbors = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{5}, featureData->getId());
  REQUIRE(neighbors != nullptr);
  neighbors->setLists({{}, {2}, {1, 3}, {2, 4}, {3}});
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Names", ShapeType{5}, {"Background", "One", "Two", "Three", "Four"}, featureData->getId()) != nullptr);

  auto* ensembleData = AttributeMatrix::Create(dataStructure, "Cell Ensemble Data", ShapeType{2}, imageGeom.getId());
  REQUIRE(ensembleData != nullptr);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleData->getId());
  REQUIRE(crystalStructures != nullptr);
  (*crystalStructures)[0] = 999;
  (*crystalStructures)[1] = 1;
}

DataStructure CreateFourColumnImage()
{
  DataStructure dataStructure;
  dataStructure.setFormatResolver(std::make_shared<InMemoryFormatResolver>());
  auto* imageGeom = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({4, 4, 1});
  REQUIRE(imageGeom->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imageGeom->setOrigin({0.0F, 0.0F, 0.0F}).valid());

  auto* cellData = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 4, 4}, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);
  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", cellData->getShape(), {1}, cellData->getId());
  REQUIRE(featureIds != nullptr);
  for(usize y = 0; y < 4; ++y)
  {
    for(usize x = 0; x < 4; ++x)
    {
      (*featureIds)[y * 4 + x] = static_cast<int32>(x + 1);
    }
  }

  AddFourFeatureData(dataStructure, *imageGeom, 4);
  return dataStructure;
}

FeatureAliasSnapshot AddNestedFeatureAlias(DataStructure& ds, const DataPath& sourcePath)
{
  const auto featurePath = sourcePath.createChildPath("Cell Feature Data");
  const auto* features = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(features != nullptr);
  const auto featureId = features->getId();
  const auto shape = features->getShape();
  const auto countsPath = featurePath.createChildPath("NumElements");
  const auto* counts = ds.getDataAs<Int32Array>(countsPath);
  REQUIRE(counts != nullptr);
  std::vector<int32> originalCounts(counts->getSize());
  for(usize i = 0; i < originalCounts.size(); ++i)
  {
    originalCounts[i] = (*counts)[i];
  }
  const auto* source = ds.getData(sourcePath);
  REQUIRE(source != nullptr);
  auto* nested = DataGroup::Create(ds, "Misc", source->getId());
  REQUIRE(nested != nullptr);
  auto* keep = UnitTest::CreateTestDataArray<int32>(ds, "Keep", {1}, {1}, nested->getId());
  REQUIRE(keep != nullptr);
  (*keep)[0] = 123;
  REQUIRE(ds.setAdditionalParent(featureId, nested->getId()));
  auto* outside = DataGroup::Create(ds, "Outside");
  REQUIRE(outside != nullptr);
  REQUIRE(ds.setAdditionalParent(nested->getId(), outside->getId()));
  return {featureId, shape, features->getParentIds(), std::move(originalCounts)};
}

void CheckNestedFeatureCopyAction(const OutputActions& actions, const DataPath& sourcePath, const DataPath& outputPath, bool clear)
{
  bool foundNestedCopy = false;
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Cell Feature Data");
  for(const auto& action : actions.actions)
  {
    const auto* copy = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(copy != nullptr && copy->path() == sourcePath.createChildPath("Misc"))
    {
      foundNestedCopy = true;
      bool declaresFeature = false;
      for(const auto& created : copy->getAllCreatedPaths())
      {
        if(created.rebase(nestedOutputPath, DataPath{}).has_value())
        {
          declaresFeature = true;
        }
      }
      CHECK(declaresFeature == !clear);
    }
  }
  REQUIRE(foundNestedCopy);
}

void CheckNestedFeatureAlias(const DataStructure& ds, const DataPath& sourcePath, const DataPath& outputPath, bool clear, const FeatureAliasSnapshot& original)
{
  const auto featurePath = sourcePath.createChildPath("Cell Feature Data");
  const auto nestedOutputPath = outputPath.createChildPath("Misc").createChildPath("Cell Feature Data");
  const auto* keep = ds.getDataAs<Int32Array>(outputPath.createChildPath("Misc").createChildPath("Keep"));
  REQUIRE(keep != nullptr);
  REQUIRE(keep->getSize() > 0);
  CHECK((*keep)[0] == 123);

  const auto* outputFeatures = ds.getDataAs<AttributeMatrix>(outputPath.createChildPath("Cell Feature Data"));
  REQUIRE(outputFeatures != nullptr);
  CHECK(outputFeatures->getSize() == (clear ? 0 : 3));
  CHECK((ds.getData(nestedOutputPath) == nullptr) == clear);

  const auto* source = ds.getDataAs<AttributeMatrix>(featurePath);
  REQUIRE(source != nullptr);
  CHECK(source->getId() == original.featureId);
  CHECK(source->getParentIds() == original.parents);
  CHECK(source->getShape() == original.shape);
  CHECK(source->getSize() == 3);
  const auto* outsideAlias = ds.getData(DataPath({"Outside", "Misc", "Cell Feature Data"}));
  REQUIRE(outsideAlias != nullptr);
  CHECK(outsideAlias->getId() == original.featureId);

  const auto* counts = ds.getDataAs<Int32Array>(featurePath.createChildPath("NumElements"));
  REQUIRE(counts != nullptr);
  REQUIRE(counts->getSize() == original.counts.size());
  for(usize i = 0; i < original.counts.size(); ++i)
  {
    CHECK((*counts)[i] == original.counts[i]);
  }
  if(!clear)
  {
    const auto* copiedCounts = ds.getDataAs<Int32Array>(nestedOutputPath.createChildPath("NumElements"));
    REQUIRE(copiedCounts != nullptr);
    REQUIRE(copiedCounts->getSize() == original.counts.size());
    for(usize i = 0; i < original.counts.size(); ++i)
    {
      CHECK((*copiedCounts)[i] == original.counts[i]);
    }
  }
}

VertexGeom& ShareFeatureMatrixWithVertexGeometry(DataStructure& ds, const AttributeMatrix& features)
{
  auto* other = VertexGeom::Create(ds, "OtherGeometry");
  REQUIRE(other != nullptr);
  auto* vertices = UnitTest::CreateTestDataArray<float32>(ds, "Vertices", {features.getNumberOfTuples()}, {3}, other->getId());
  REQUIRE(vertices != nullptr);
  other->setVertices(*vertices);
  REQUIRE(ds.setAdditionalParent(features.getId(), other->getId()));
  other->setVertexAttributeMatrix(features);
  return *other;
}

void CreateImageWithGeometryNameInChildren(DataStructure& ds)
{
  auto* geomPtr = ImageGeom::Create(ds, "Geom");
  REQUIRE(geomPtr != nullptr);
  geomPtr->setDimensions({2, 2, 2});
  REQUIRE(geomPtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  REQUIRE(geomPtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  auto* elementAmPtr = AttributeMatrix::Create(ds, "Cell Data", {2, 2, 2}, geomPtr->getId());
  REQUIRE(elementAmPtr != nullptr);
  geomPtr->setCellData(*elementAmPtr);
  auto* dataPtr = Int32Array::Create(ds, "Data", DataStoreUtilities::CreateDataStore<int32>(ds, elementAmPtr->getDataPaths().front().createChildPath("Data"), elementAmPtr->getShape(), {1}),
                                     elementAmPtr->getId());
  REQUIRE(dataPtr != nullptr);
  dataPtr->fill(7);
  auto* featureAmPtr = AttributeMatrix::Create(ds, "Geom Feature Data", {2}, geomPtr->getId());
  REQUIRE(featureAmPtr != nullptr);
  auto* valuesPtr =
      Int32Array::Create(ds, "Geom Values", DataStoreUtilities::CreateDataStore<int32>(ds, featureAmPtr->getDataPaths().front().createChildPath("Geom Values"), {2}, {1}), featureAmPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(42);
}

void CheckChildNamesInCopyAction(const OutputActions& actions, const DataPath& srcGeomPath, const DataPath& destGeomPath)
{
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");
  const CopyDataObjectAction* copyActionPtr = nullptr;
  for(const auto& action : actions.actions)
  {
    const auto* candidatePtr = dynamic_cast<const CopyDataObjectAction*>(action.get());
    if(candidatePtr != nullptr && candidatePtr->path() == srcGeomPath.createChildPath("Geom Feature Data"))
    {
      copyActionPtr = candidatePtr;
      break;
    }
  }
  REQUIRE(copyActionPtr != nullptr);
  const auto createdPaths = copyActionPtr->getAllCreatedPaths();
  REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), copiedValuesPath) != createdPaths.end());
  REQUIRE(std::find(createdPaths.begin(), createdPaths.end(), renamedAmPath.createChildPath("Geom Out Values")) == createdPaths.end());
}

void CheckCopiedChildNames(const DataStructure& ds, const DataPath& destGeomPath)
{
  const DataPath copiedAmPath = destGeomPath.createChildPath("Geom Feature Data");
  const DataPath copiedValuesPath = copiedAmPath.createChildPath("Geom Values");
  const DataPath renamedAmPath = destGeomPath.createChildPath("Geom Out Feature Data");
  REQUIRE(ds.getDataAs<AttributeMatrix>(copiedAmPath) != nullptr);
  REQUIRE(ds.getDataAs<Int32Array>(copiedValuesPath) != nullptr);
  REQUIRE_FALSE(ds.containsData(renamedAmPath));
  REQUIRE(ds.getDataAs<Int32Array>(destGeomPath.createChildPath("Cell Data").createChildPath("Data")) != nullptr);
}
} // namespace nx::core::ImageGeometryFeatureTest
