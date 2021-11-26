/**
 * Time Tracking:
 * THURS: 11:00 - 12:00
 * THURS: 13:45 - 16:45
 */

#include "StlFileReader.hpp"

#include "complex/Common/ComplexRange.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateTriangleGeometryAction.hpp"

#include "ComplexCore/Filters/Actions/StlFileReaderAction.hpp"
#include "ComplexCore/utils/StlUtilities.hpp"


#include <cstdio>
#include <tuple>
#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{

/**
 * @brief The FindUniqueIdsImpl class implements a threaded algorithm that determines the set of
 * unique vertices in the triangle geometry
 */
class FindUniqueIdsImpl
{
public:
  FindUniqueIdsImpl(AbstractGeometry::SharedVertexList& vertex,
                    const std::vector<std::vector<size_t>>& nodesInBin,
                    complex::Int64Array& uniqueIds)
  : m_Vertex(vertex)
  , m_NodesInBin(nodesInBin)
  , m_UniqueIds(uniqueIds)
  {
  }

  // -----------------------------------------------------------------------------
  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      for(size_t j = 0; j < m_NodesInBin[i].size(); j++)
      {
        size_t node1 = m_NodesInBin[i][j];
        if(m_UniqueIds[node1] == static_cast<int64_t>(node1))
        {
          for(size_t k = j + 1; k < m_NodesInBin[i].size(); k++)
          {
            size_t node2 = m_NodesInBin[i][k];
            if(m_Vertex[node1 * 3] == m_Vertex[node2 * 3] && m_Vertex[node1 * 3 + 1] == m_Vertex[node2 * 3 + 1] && m_Vertex[node1 * 3 + 2] == m_Vertex[node2 * 3 + 2])
            {
              m_UniqueIds[node2] = node1;
            }
          }
        }
      }
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const ComplexRange& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const AbstractGeometry::SharedVertexList& m_Vertex;
  const std::vector<std::vector<size_t>>& m_NodesInBin;
  complex::Int64Array& m_UniqueIds;
};

std::array<float, 6> CreateMinMaxCoords()
{
  return {std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
          -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max()};
}

}

namespace complex
{

//------------------------------------------------------------------------------
std::string StlFileReader::name() const
{
  return FilterTraits<StlFileReader>::name.str();
}

//------------------------------------------------------------------------------
std::string StlFileReader::className() const
{
  return FilterTraits<StlFileReader>::className;
}

//------------------------------------------------------------------------------
Uuid StlFileReader::uuid() const
{
  return FilterTraits<StlFileReader>::uuid;
}

//------------------------------------------------------------------------------
std::string StlFileReader::humanName() const
{
  return "Import STL File";
}

//------------------------------------------------------------------------------
std::vector<std::string> StlFileReader::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters StlFileReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_StlFilePath_Key, "STL File", "Input STL File", fs::path("*.stl"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ParentDataGroupPath_Key, "Parent DataGroup", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<StringParameter>(k_GeometryName_Key, "Geometry Name [Data Group]", "", std::string("[Triangle Geometry]")));
  params.insert(std::make_unique<StringParameter>(k_FaceDataGroupName_Key, "Triangle Face Data [DataGroup]", "", std::string("Triangle Face Data")));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FaceNormalsArrayName_Key, "Face Normals [Data Array]", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer StlFileReader::clone() const
{
  return std::make_unique<StlFileReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult StlFileReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
/** ======================================================
 *  Before we even get here each of the filter parameters will have validated the
 *  input that the user has provided to the best of it's ability. This means
 *  that the input file parameter will have already figured out if the file is
 *  available on the file system, the parent data group is available.
 */

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pParentDataGroupPath = filterArgs.value<DataPath>(k_ParentDataGroupPath_Key);
  auto pFaceGeometryName = filterArgs.value<std::string>(k_GeometryName_Key);
  auto pFaceDataGroupName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
  // to the DataStructure. If none are available then create a new custom Action subclass.
  // If your filter does not make any structural modifications to the DataStructure then
  // you can skip this code.
  //auto stlFileReaderAction = std::make_unique<StlFileReaderAction>(pStlFilePathValue);

  int32_t stlFileType = StlUtilities::DetermineStlFileType(pStlFilePathValue);
  if(stlFileType < 0)
  {
    Error result = {StlConstants::k_UnsupportedFileType, fmt::format("The Input STL File is ASCII which is not currently supported. Please convert it to a binary STL file using another program.", pStlFilePathValue.string())};
    resultOutputActions.errors().push_back(result);
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }
  if(stlFileType > 0)
  {
    Error result = {StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the STL file.", pStlFilePathValue.string())};
    resultOutputActions.errors().push_back(result);
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }

  // Now get the number of Triangles according to the STL Header
  int32_t numTriangles = StlUtilities::NumFacesFromHeader(pStlFilePathValue);
  if(numTriangles < 0)
  {
    Error result = {StlConstants::k_ErrorOpeningFile, fmt::format("Error reading the STL file.", pStlFilePathValue.string())};
    resultOutputActions.errors().push_back(result);
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }
  if(numTriangles == 0)
  {
    numTriangles = 1; // This can happen in a LOT of STL files. Just means the writer didn't go back and update the header.
  }

  std::cout << "num triangles: " << numTriangles << std::endl;
  DataPath geometryDataPath = pParentDataGroupPath.createChildPath(pFaceGeometryName);

  // Assign the outputAction to the Result<OutputActions>::actions vector via a push_back
  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point, we are going to scope each section so that we don't accidentally introduce bugs

  // Create the Triangle Geometry action and store it
  {
    auto createTriangleGeometryAction = std::make_unique<CreateTriangleGeometryAction>(geometryDataPath, numTriangles, 1);
    resultOutputActions.value().actions.push_back(std::move(createTriangleGeometryAction));
  }
  //Create Triangle FaceData (for the Normals) action and store it
  {
    DataPath faceGroupDataPath = geometryDataPath.createChildPath(pFaceDataGroupName);
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(faceGroupDataPath);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // Create the face Normals DataArray action and store it
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::float32,
                                                                 std::vector<usize>{static_cast<usize>(numTriangles)},
                                                                 3, pFaceNormalsArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods. (None to store for this filter... yet)

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> StlFileReader::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pStlFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_StlFilePath_Key);
  auto pParentDataGroupPath = filterArgs.value<DataPath>(k_ParentDataGroupPath_Key);
  auto pFaceGeometryName = filterArgs.value<std::string>(k_GeometryName_Key);
  auto pFaceDataGroupName = filterArgs.value<std::string>(k_FaceDataGroupName_Key);
  auto pFaceNormalsArrayNameValue = filterArgs.value<DataPath>(k_FaceNormalsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
