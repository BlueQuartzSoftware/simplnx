#include "WriteNodesAndElementsFileFilter.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteNodesAndElementsFile.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteNodesAndElementsFileFilter::name() const
{
  return FilterTraits<WriteNodesAndElementsFileFilter>::name;
}

//------------------------------------------------------------------------------
std::string WriteNodesAndElementsFileFilter::className() const
{
  return FilterTraits<WriteNodesAndElementsFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteNodesAndElementsFileFilter::uuid() const
{
  return FilterTraits<WriteNodesAndElementsFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteNodesAndElementsFileFilter::humanName() const
{
  return "Write Nodes And Elements File(s)";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteNodesAndElementsFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Nodes", "Elements", "Cells", "Vertices", "Geometry"};
}

//------------------------------------------------------------------------------
Parameters WriteNodesAndElementsFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedGeometry, "Geometry To Write", "The Geometry that will be written to the output file(s).", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex, IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad,
                                                                                                      IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteNodeFile, "Write Node File", "Whether or not to write the node information out to a file.", true));
  params.insert(std::make_unique<BoolParameter>(k_NumberNodes, "Number Nodes", "Whether or not to number each node in the node information output file.", true));
  params.insert(std::make_unique<BoolParameter>(k_IncludeNodeFileHeader, "Include Node File Header", "Whether or not to include the node file header in the node output file.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteElementFile, "Write Element/Cell File", "Whether or not to write the element/cell information out to a file.", true));
  params.insert(std::make_unique<BoolParameter>(k_NumberElements, "Number Elements/Cells", "Whether or not to number each element/cell in the element information output file.", true));
  params.insert(
      std::make_unique<BoolParameter>(k_IncludeElementFileHeader, "Include Element/Cell File Header", "Whether or not to include the element/cell file header in the element/cell output file.", true));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_NodeFilePath, "Output Node File Path", "The node information will be written to this file path.", "Untitled.node",
                                                          FileSystemPathParameter::ExtensionsType{".node", ".txt"}, FileSystemPathParameter::PathType::OutputFile, true));
  params.insert(std::make_unique<FileSystemPathParameter>(k_ElementFilePath, "Output Element/Cell File Path", "The element/cell information will be written to this file path.", "Untitled.ele",
                                                          FileSystemPathParameter::ExtensionsType{".ele", ".txt"}, FileSystemPathParameter::PathType::OutputFile, true));

  params.linkParameters(k_WriteNodeFile, k_NumberNodes, true);
  params.linkParameters(k_WriteNodeFile, k_IncludeNodeFileHeader, true);
  params.linkParameters(k_WriteNodeFile, k_NodeFilePath, true);
  params.linkParameters(k_WriteElementFile, k_NumberElements, true);
  params.linkParameters(k_WriteElementFile, k_IncludeElementFileHeader, true);
  params.linkParameters(k_WriteElementFile, k_ElementFilePath, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteNodesAndElementsFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WriteNodesAndElementsFileFilter::clone() const
{
  return std::make_unique<WriteNodesAndElementsFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteNodesAndElementsFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  DataPath selectedGeometryPath = args.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometry);
  bool writeNodeFile = args.value<BoolParameter::ValueType>(k_WriteNodeFile);
  bool numberNodes = args.value<BoolParameter::ValueType>(k_NumberNodes);
  fs::path nodeFilePath = args.value<FileSystemPathParameter::ValueType>(k_NodeFilePath);
  bool writeElementFile = args.value<BoolParameter::ValueType>(k_WriteElementFile);
  bool numberElements = args.value<BoolParameter::ValueType>(k_NumberElements);
  fs::path elementFilePath = args.value<FileSystemPathParameter::ValueType>(k_ElementFilePath);

  if(!writeNodeFile && !writeElementFile)
  {
    return {MakeErrorResult<OutputActions>(to_underlying(WriteNodesAndElementsFile::ErrorCodes::NoFileWriterChosen),
                                           "Neither 'Write Node File' nor 'Write Element/Cell File' have been chosen.  Please choose at least one of these options.")};
  }

  auto& selectedGeometry = dataStructure.getDataRefAs<IGeometry>(selectedGeometryPath);
  if(selectedGeometry.getGeomType() == IGeometry::Type::Vertex && writeElementFile)
  {
    return {MakeErrorResult<OutputActions>(
        to_underlying(WriteNodesAndElementsFile::ErrorCodes::VertexGeomHasNoElements),
        "The selected geometry is a vertex geometry, so an element file cannot be written.  Please turn off 'Write Element/Cell File' or select a different geometry with a type other than Vertex.")};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> WriteNodesAndElementsFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  WriteNodesAndElementsFileInputValues inputValues;

  inputValues.SelectedGeometryPath = args.value<GeometrySelectionParameter::ValueType>(k_SelectedGeometry);
  inputValues.WriteNodeFile = args.value<BoolParameter::ValueType>(k_WriteNodeFile);
  inputValues.NumberNodes = args.value<BoolParameter::ValueType>(k_NumberNodes);
  inputValues.IncludeNodeFileHeader = args.value<BoolParameter::ValueType>(k_IncludeNodeFileHeader);
  inputValues.NodeFilePath = args.value<FileSystemPathParameter::ValueType>(k_NodeFilePath);
  inputValues.WriteElementFile = args.value<BoolParameter::ValueType>(k_WriteElementFile);
  inputValues.NumberElements = args.value<BoolParameter::ValueType>(k_NumberElements);
  inputValues.IncludeElementFileHeader = args.value<BoolParameter::ValueType>(k_IncludeElementFileHeader);
  inputValues.ElementFilePath = args.value<FileSystemPathParameter::ValueType>(k_ElementFilePath);

  return WriteNodesAndElementsFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
