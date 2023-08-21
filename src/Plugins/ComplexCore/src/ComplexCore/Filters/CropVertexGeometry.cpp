#include "CropVertexGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

namespace complex
{
namespace
{
struct CopyDataToCroppedGeometryFunctor
{
  template <typename T>
  void operator()(const IDataArray& inDataRef, IDataArray& outDataRef, const std::vector<int64>& croppedPoints)
  {
    const DataArray<T>& inputDataArray = dynamic_cast<const DataArray<T>&>(inDataRef);
    const auto& inputData = inputDataArray.getDataStoreRef();
    DataArray<T>& croppedDataArray = dynamic_cast<DataArray<T>&>(outDataRef);
    auto& croppedData = croppedDataArray.getDataStoreRef();

    usize nComps = inDataRef.getNumberOfComponents();

    for(std::vector<int64>::size_type i = 0; i < croppedPoints.size(); i++)
    {
      for(usize d = 0; d < nComps; d++)
      {
        usize tmpIndex = nComps * i + d;
        usize ptrIndex = nComps * croppedPoints[i] + d;
        croppedData[tmpIndex] = inputData[ptrIndex];
      }
    }
  }
};
} // namespace

//------------------------------------------------------------------------------
std::string CropVertexGeometry::name() const
{
  return FilterTraits<CropVertexGeometry>::name;
}

//------------------------------------------------------------------------------
std::string CropVertexGeometry::className() const
{
  return FilterTraits<CropVertexGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CropVertexGeometry::uuid() const
{
  return FilterTraits<CropVertexGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CropVertexGeometry::humanName() const
{
  return "Crop Geometry (Vertex)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CropVertexGeometry::defaultTags() const
{
  return {className(), "Crop", "Vertex Geometry", "Geometry", "Memory Management", "Cut"};
}

//------------------------------------------------------------------------------
Parameters CropVertexGeometry::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeom_Key, "Vertex Geometry to Crop", "DataPath to target VertexGeom", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CroppedGeom_Key, "Cropped Vertex Geometry", "Created VertexGeom path", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataName_Key, "Vertex Data Name", "Name of the vertex data AttributeMatrix", INodeGeometry0D::k_VertexDataName));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinPos_Key, "Min Pos", "Minimum vertex position", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxPos_Key, "Max Pos", "Maximum vertex position", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_TargetArrayPaths_Key, "Vertex Data Arrays to crop", "The complete path to all the vertex data arrays to crop", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CropVertexGeometry::clone() const
{
  return std::make_unique<CropVertexGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CropVertexGeometry::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto vertexDataName = args.value<std::string>(k_VertexDataName_Key);

  auto xMin = posMin[0];
  auto yMin = posMin[1];
  auto zMin = posMin[2];
  auto xMax = posMax[0];
  auto yMax = posMax[1];
  auto zMax = posMax[2];

  const auto& vertexGeom = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);

  OutputActions actions;

  {
    std::vector<Error> errors;
    bool xDimError = (xMax < xMin);
    bool yDimError = (yMax < yMin);
    bool zDimError = (zMax < zMin);
    if(xDimError)
    {
      std::string ss = fmt::format("X Max ({}) less than X Min ({})", xMax, xMin);
      errors.push_back(Error{-5550, std::move(ss)});
    }
    if(yDimError)
    {
      std::string ss = fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin);
      errors.push_back(Error{-5550, std::move(ss)});
    }
    if(zDimError)
    {
      std::string ss = fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin);
      errors.push_back(Error{-5550, std::move(ss)});
    }
    if(xDimError || yDimError || zDimError)
    {
      return PreflightResult{{nonstd::make_unexpected(errors)}};
    }
  }

  auto* vertexAM = vertexGeom.getVertexAttributeMatrix();
  if(vertexAM == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-5551, "Could not find vertex data AttributeMatrix in selected Vertex Geometry"), {}};
  }
  auto tupleShape = vertexAM->getShape();
  usize numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  auto action = std::make_unique<CreateVertexGeometryAction>(croppedGeomPath, numTuples, vertexDataName, CreateVertexGeometryAction::k_SharedVertexListName);
  DataPath croppedVertexDataPath = action->getVertexDataPath();
  actions.appendAction(std::move(action));

  for(auto&& targetArrayPath : targetArrays)
  {
    auto& targetArray = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);

    DataType type = targetArray.getDataType();
    auto tDims = targetArray.getNumberOfTuples();
    auto cDims = targetArray.getNumberOfComponents();
    auto createArrayAction = std::make_unique<CreateArrayAction>(type, std::vector<usize>{tDims}, std::vector<usize>{cDims}, croppedVertexDataPath.createChildPath(targetArrayPath.getTargetName()));
    actions.appendAction(std::move(createArrayAction));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CropVertexGeometry::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto vertexDataName = args.value<std::string>(k_VertexDataName_Key);

  auto xMin = posMin[0];
  auto yMin = posMin[1];
  auto zMin = posMin[2];
  auto xMax = posMax[0];
  auto yMax = posMax[1];
  auto zMax = posMax[2];

  auto& vertices = dataStructure.getDataRefAs<VertexGeom>(vertexGeomPath);
  int64 numVerts = vertices.getNumberOfVertices();
  auto* verticesPtr = vertices.getVertices();
  auto& allVerts = verticesPtr->getDataStoreRef();
  std::vector<int64> croppedPoints;
  croppedPoints.reserve(numVerts);

  for(int64 i = 0; i < numVerts; i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(allVerts[3 * i + 0] >= xMin && allVerts[3 * i + 0] <= xMax && allVerts[3 * i + 1] >= yMin && allVerts[3 * i + 1] <= yMax && allVerts[3 * i + 2] >= zMin && allVerts[3 * i + 2] <= zMax)
    {
      croppedPoints.push_back(i);
    }
  }

  croppedPoints.shrink_to_fit();

  auto& crop = dataStructure.getDataRefAs<VertexGeom>(croppedGeomPath);
  usize numTuples = croppedPoints.size();
  crop.resizeVertexList(numTuples);
  std::vector<usize> tDims = {numTuples};

  DataPath croppedVertexDataPath = croppedGeomPath.createChildPath(vertexDataName);
  auto& vertedDataAttMatrix = dataStructure.getDataRefAs<AttributeMatrix>(croppedVertexDataPath);
  vertedDataAttMatrix.resizeTuples(tDims);

  for(usize i = 0; i < numTuples; i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto coords = vertices.getVertexCoordinate(croppedPoints[i]);
    crop.setVertexCoordinate(i, coords);
  }

  for(auto&& targetArrayPath : targetArrays)
  {
    DataPath destArrayPath(croppedVertexDataPath.createChildPath(targetArrayPath.getTargetName()));

    const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);
    auto& destArray = dataStructure.getDataRefAs<IDataArray>(destArrayPath);

    ExecuteDataFunction(CopyDataToCroppedGeometryFunctor{}, srcArray.getDataType(), srcArray, destArray, croppedPoints);
  }

  return {};
}
} // namespace complex
