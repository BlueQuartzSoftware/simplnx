#include "CropVertexGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{
namespace
{
template <typename T>
void copyDataToCroppedGeometry(IDataArray::Pointer inDataPtr, IDataArray::Pointer outDataPtr, std::vector<int64>& croppedPoints)
{
  typename DataArray<T>::Pointer inputDataPtr = std::dynamic_pointer_cast<DataArray<T>>(inDataPtr);
  T* inputData = static_cast<T*>(inputDataPtr->getPointer(0));
  typename DataArray<T>::Pointer croppedDataPtr = std::dynamic_pointer_cast<DataArray<T>>(outDataPtr);
  T* croppedData = static_cast<T*>(croppedDataPtr->getPointer(0));

  size_t nComps = inDataPtr->getNumberOfComponents();
  size_t tmpIndex = 0;
  size_t ptrIndex = 0;

  for(std::vector<int64>::size_type i = 0; i < croppedPoints.size(); i++)
  {
    for(size_t d = 0; d < nComps; d++)
    {
      tmpIndex = nComps * i + d;
      ptrIndex = nComps * croppedPoints[i] + d;
      croppedData[tmpIndex] = inputData[ptrIndex];
    }
  }
}
} // namespace

std::string CropVertexGeometry::name() const
{
  return FilterTraits<CropVertexGeometry>::name;
}

std::string CropVertexGeometry::className() const
{
  return FilterTraits<CropVertexGeometry>::className;
}

Uuid CropVertexGeometry::uuid() const
{
  return FilterTraits<CropVertexGeometry>::uuid;
}

std::string CropVertexGeometry::humanName() const
{
  return "Crop Geometry (Vertex)";
}

Parameters CropVertexGeometry::parameters() const
{
  Parameters params;
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_VertexGeom_Key, "Vertex Geometry to Crop", "DataPath to target VertexGeom", DataPath{}, std::set<DataObject::Type>{DataObject::Type::VertexGeom}));
  params.insert(std::make_unique<DataPathCreationParameter>(k_CroppedGeom_Key, "Cropped Vertex Geometry", "DataPath to create the cropped VertexGeom", DataPath{}));
  params.insert(std::make_unique<Int32Parameter>(k_MinX_Key, "X Min", "Minimum X position", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MinY_Key, "Y Min", "Minimum Y position", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MinZ_Key, "Z Min", "Minimum Z position", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxX_Key, "X Max", "Maximum X position", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxY_Key, "Y Max", "Maximum Y position", 0));
  params.insert(std::make_unique<Int32Parameter>(k_MaxZ_Key, "Z Max", "Maximum Z position", 0));
  return params;
}

IFilter::UniquePointer CropVertexGeometry::clone() const
{
  return std::make_unique<CropVertexGeometry>();
}

IFilter::PreflightResult CropVertexGeometry::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto xMin = args.value<int32>(k_MinX_Key);
  auto xMax = args.value<int32>(k_MaxX_Key);
  auto yMin = args.value<int32>(k_MinY_Key);
  auto yMax = args.value<int32>(k_MaxY_Key);
  auto zMin = args.value<int32>(k_MinZ_Key);
  auto zMax = args.value<int32>(k_MaxZ_Key);

  const auto* vertexGeom = data.geDataAs<VertexGeom>(vertexGeomPath);

  if(xMax < xMin)
  {
    std::string ss = fmt::format("X Max ({}) less than X Min ({})", xMax, xMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(yMax < yMin)
  {
    std::string ss = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }
  if(zMax < zMin)
  {
    std::string ss = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
    return {MakeErrorResult<OutputActions>(-5550, ss)};
  }

  auto action = std::make_unique<CreateVertexGeometryAction>(croppedGeomPath);
  actions.actions.push_back(std::move(action));

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getDataContainerName());
  m_AttrMatList = m->getAttributeMatrixNames();
  std::vector<size_t> tDims(1, 0);
  QList<std::string> tempDataArrayList;
  DataArrayPath tempPath;
  AttributeMatrix::Type tempAttrMatType = AttributeMatrix::Type::Vertex;

  if(getErrorCode() < 0)
  {
    return;
  }

  for(auto&& attr_mat : m_AttrMatList)
  {
    AttributeMatrix::Pointer tmpAttrMat = m->getPrereqAttributeMatrix(this, attr_mat, -301);
    if(getErrorCode() >= 0)
    {
      tempAttrMatType = tmpAttrMat->getType();
      if(tempAttrMatType != AttributeMatrix::Type::Vertex)
      {
        AttributeMatrix::Pointer attrMat = tmpAttrMat->deepCopy(getInPreflight());
        dc->addOrReplaceAttributeMatrix(attrMat);
      }
      else
      {
        dc->createNonPrereqAttributeMatrix(this, tmpAttrMat->getName(), tDims, AttributeMatrix::Type::Vertex);
        tempDataArrayList = tmpAttrMat->getAttributeArrayNames();
        for(auto&& data_array : tempDataArrayList)
        {
          tempPath.update(getCroppedDataContainerName().getDataContainerName(), tmpAttrMat->getName(), data_array);
          IDataArray::Pointer tmpDataArray = tmpAttrMat->getPrereqIDataArray(this, data_array, -90002);
          if(getErrorCode() >= 0)
          {
            std::vector<size_t> cDims = tmpDataArray->getComponentDimensions();
            TemplateHelpers::CreateNonPrereqArrayFromArrayType()(this, tempPath, cDims, tmpDataArray);
          }
        }
      }
    }
  }

  //
  auto action = std::make_unique<CropVertexGeometryAction>(dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CropVertexGeometry::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto xMin = args.value<int32>(k_MinX_Key);
  auto xMax = args.value<int32>(k_MaxX_Key);
  auto yMin = args.value<int32>(k_MinY_Key);
  auto yMax = args.value<int32>(k_MaxY_Key);
  auto zMin = args.value<int32>(k_MinZ_Key);
  auto zMax = args.value<int32>(k_MaxZ_Key);

  auto* vertices = data.getDataAs<VertexGeom>(vertexGeomPath);
  int64 numVerts = vertices->getNumberOfVertices();
  auto* verticesPtr = vertices->getVertices();
  auto& allVerts = verticesPtr->getDataStoreRef();
  std::vector<int64> croppedPoints;
  croppedPoints.reserve(numVerts);

  for(int64 i = 0; i < numVerts; i++)
  {
    //if(getCancel())
    //{
    //  return;
    //}
    if(allVerts[3 * i + 0] >= xMin && allVerts[3 * i + 0] <= xMax && allVerts[3 * i + 1] >= yMin && allVerts[3 * i + 1] <= yMax && allVerts[3 * i + 2] >= zMin &&
       allVerts[3 * i + 2] <= zMax)
    {
      croppedPoints.push_back(i);
    }
  }

  croppedPoints.shrink_to_fit();

  auto* crop = data.getDataAs<VertexGeom>(croppedGeomPath);
  crop->resizeVertexList(croppedPoints.size());
  float coords[3] = {0.0f, 0.0f, 0.0f};

  for(usize i = 0; i < croppedPoints.size(); i++)
  {
    //if(getCancel())
    //{
    //  return;
    //}
    auto coords = vertices->getCoords(croppedPoints[i]);
    crop->setCoords(i, coords);
  }

  std::vector<size_t> tDims(1, croppedPoints.size());

  for(auto&& attr_mat : m_AttrMatList)
  {
    AttributeMatrix::Pointer tmpAttrMat = dc->getPrereqAttributeMatrix(this, attr_mat, -301);
    if(getErrorCode() >= 0)
    {
      AttributeMatrix::Type tempAttrMatType = tmpAttrMat->getType();
      if(tempAttrMatType == AttributeMatrix::Type::Vertex)
      {
        tmpAttrMat->resizeAttributeArrays(tDims);
        QList<QString> srcDataArrays = tmpAttrMat->getAttributeArrayNames();
        AttributeMatrix::Pointer srcAttrMat = m->getAttributeMatrix(tmpAttrMat->getName());
        assert(srcAttrMat);

        for(auto&& data_array : srcDataArrays)
        {
          IDataArray::Pointer src = srcAttrMat->getAttributeArray(data_array);
          IDataArray::Pointer dest = tmpAttrMat->getAttributeArray(data_array);

          assert(src);
          assert(dest);
          assert(src->getNumberOfComponents() == dest->getNumberOfComponents());

          EXECUTE_FUNCTION_TEMPLATE(this, copyDataToCroppedGeometry, src, src, dest, croppedPoints)
        }
      }
    }
  }

  return {};
}
} // namespace complex
