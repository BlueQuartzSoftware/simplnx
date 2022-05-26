#include "CropVertexGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
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
    croppedData.reshapeTuples({croppedPoints.size()});

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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_VertexGeom_Key, "Vertex Geometry to Crop", "DataPath to target VertexGeom", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CroppedGeom_Key, "Cropped Vertex Geometry", "Created VertexGeom path", DataPath{}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinPos_Key, "Min Pos", "Minimum vertex position", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxPos_Key, "Max Pos", "Maximum vertex position", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_TargetArrayPaths_Key, "Vertex Data Arrays to crop", "", std::vector<DataPath>(), complex::GetAllDataTypes()));
  params.insert(std::make_unique<StringParameter>(k_CroppedGroupName_Key, "Cropped Group Name", "", "Cropped Data"));
  return params;
}

IFilter::UniquePointer CropVertexGeometry::clone() const
{
  return std::make_unique<CropVertexGeometry>();
}

IFilter::PreflightResult CropVertexGeometry::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto croppedGroupName = args.value<std::string>(k_CroppedGroupName_Key);

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

  auto action = std::make_unique<CreateVertexGeometryAction>(croppedGeomPath, 0);
  actions.actions.push_back(std::move(action));

  DataPath croppedGroupPath = croppedGeomPath.createChildPath(croppedGroupName);
  auto groupAction = std::make_unique<CreateDataGroupAction>(croppedGroupPath);
  actions.actions.push_back(std::move(groupAction));

  for(auto&& targetArrayPath : targetArrays)
  {
    auto& targetArray = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);

    DataType type = targetArray.getDataType();
    auto tDims = targetArray.getNumberOfTuples();
    auto cDims = targetArray.getNumberOfComponents();
    auto createArrayAction = std::make_unique<CreateArrayAction>(type, std::vector<usize>{tDims}, std::vector<usize>{cDims}, croppedGroupPath.createChildPath(targetArrayPath.getTargetName()));
    actions.actions.push_back(std::move(createArrayAction));
  }

  return {std::move(actions)};
}

Result<> CropVertexGeometry::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel) const
{
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeom_Key);
  auto croppedGeomPath = args.value<DataPath>(k_CroppedGeom_Key);
  auto posMin = args.value<std::vector<float32>>(k_MinPos_Key);
  auto posMax = args.value<std::vector<float32>>(k_MaxPos_Key);
  auto targetArrays = args.value<std::vector<DataPath>>(k_TargetArrayPaths_Key);
  auto croppedGroupName = args.value<std::string>(k_CroppedGroupName_Key);

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
  crop.resizeVertexList(croppedPoints.size());

  DataPath croppedGroupPath = croppedGeomPath.createChildPath(croppedGroupName);

  for(usize i = 0; i < croppedPoints.size(); i++)
  {
    if(shouldCancel)
    {
      return {};
    }
    auto coords = vertices.getCoords(croppedPoints[i]);
    crop.setCoords(i, coords);
  }

  std::vector<usize> tDims = {croppedPoints.size()};

  for(auto&& targetArrayPath : targetArrays)
  {
    DataPath destArrayPath(croppedGroupPath.createChildPath(targetArrayPath.getTargetName()));

    const auto& srcArray = dataStructure.getDataRefAs<IDataArray>(targetArrayPath);
    auto& destArray = dataStructure.getDataRefAs<IDataArray>(destArrayPath);

    ExecuteDataFunction(CopyDataToCroppedGeometryFunctor{}, srcArray.getDataType(), srcArray, destArray, croppedPoints);
  }

  return {};
}
} // namespace complex
