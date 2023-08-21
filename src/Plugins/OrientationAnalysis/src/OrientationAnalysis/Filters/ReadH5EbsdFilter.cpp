#include "ReadH5EbsdFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ReadH5Ebsd.hpp"
#include "OrientationAnalysis/Parameters/H5EbsdReaderParameter.h"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5CtfVolumeReader.h"
#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/H5AngVolumeReader.h"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::name() const
{
  return FilterTraits<ReadH5EbsdFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::className() const
{
  return FilterTraits<ReadH5EbsdFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadH5EbsdFilter::uuid() const
{
  return FilterTraits<ReadH5EbsdFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadH5EbsdFilter::humanName() const
{
  return "Import H5EBSD File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadH5EbsdFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "EDAX", "ANG", "EBSD", "CTF", "Oxford"};
}

//------------------------------------------------------------------------------
Parameters ReadH5EbsdFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<H5EbsdReaderParameter>(k_ReadH5EbsdFilter_Key, "Import H5Ebsd File", "The input .h5ebsd file path", H5EbsdReaderParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Created Image Geometry", "The complete path to the imported Image Geometry", DataPath({"DataContainer"})));
  // params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Created Cell Attribute Matrix",
                                                          "The name of the created cell attribute matrix associated with the imported geometry", "CellData"));
  // params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellEnsembleAttributeMatrixName_Key, "Created Cell Ensemble Attribute Matrix",
                                                          "The name of the created cell ensemble attribute matrix associated with the imported geometry", "CellEnsembleData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadH5EbsdFilter::clone() const
{
  return std::make_unique<ReadH5EbsdFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadH5EbsdFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{

  auto pReadH5EbsdFilterValue = filterArgs.value<H5EbsdReaderParameter::ValueType>(k_ReadH5EbsdFilter_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  DataPath cellAttributeMatrixPath = imageGeomPath.createChildPath(pCellAttributeMatrixNameValue);
  auto pCellEnsembleAttributeMatrixNameValue = imageGeomPath.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  H5EbsdVolumeReader::Pointer reader = H5EbsdVolumeReader::New();
  reader->setFileName(pReadH5EbsdFilterValue.inputFilePath);
  int32_t err = reader->readVolumeInfo();
  if(err < 0)
  {
    // This really should have been hit during the parameter validation... but just in case.
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }

  std::vector<int64_t> dims = {0, 0, 0};
  reader->getDims(dims.at(0), dims.at(1), dims.at(2));

  CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(dims[0]), static_cast<size_t>(dims[1]),
                                                            static_cast<size_t>(pReadH5EbsdFilterValue.endSlice - pReadH5EbsdFilterValue.startSlice + 1)};
  std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  CreateImageGeometryAction::SpacingType spacing = {1.0F, 1.0F, 1.0F};
  reader->getSpacing(spacing.at(0), spacing.at(1), spacing.at(2));

  CreateImageGeometryAction::OriginType origin = {0.0F, 0.0F, 0.0F};

  resultOutputActions.value().appendAction(
      std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(imageGeomDims), std::move(origin), std::move(spacing), pCellAttributeMatrixNameValue));

  // Create the Ensemble AttributeMatrix
  {
    auto createDataGroupAction = std::make_unique<CreateAttributeMatrixAction>(pCellEnsembleAttributeMatrixNameValue, std::vector<size_t>{2});
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  EbsdLib::OEM m_Manufacturer = {EbsdLib::OEM::Unknown};
  std::string manufacturer = reader->getManufacturer();
  if(manufacturer == EbsdLib::Ang::Manufacturer)
  {
    m_Manufacturer = EbsdLib::OEM::EDAX;
  }
  else if(manufacturer == EbsdLib::Ctf::Manufacturer)
  {
    m_Manufacturer = EbsdLib::OEM::Oxford;
  }

  std::vector<std::string> names;
  if(m_Manufacturer == EbsdLib::OEM::EDAX)
  {
    AngFields angFeatures;
    reader = H5AngVolumeReader::New();
    names = angFeatures.getFilterFeatures<std::vector<std::string>>();
  }
  else if(m_Manufacturer == EbsdLib::OEM::Oxford)
  {
    CtfFields cfeatures;
    reader = H5CtfVolumeReader::New();
    names = cfeatures.getFilterFeatures<std::vector<std::string>>();
  }
  else
  {
    return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
  }

  std::set<std::string> m_SelectedArrayNames;
  for(const auto& selectedArrayName : pReadH5EbsdFilterValue.hdf5DataPaths)
  {
    m_SelectedArrayNames.insert(selectedArrayName);
  }
  // std::set<std::string> m_DataArrayNames = reader->getDataArrayNames();

  std::vector<size_t> cDims = {1ULL};
  for(int32_t i = 0; i < names.size(); ++i)
  {
    // First check to see if the name is in our list of names to read.
    if(m_SelectedArrayNames.find(names[i]) == m_SelectedArrayNames.end())
    {
      continue;
    }
    if(reader->getPointerType(names[i]) == EbsdLib::NumericTypes::Type::Int32)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(names[i]);
      auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
    else if(reader->getPointerType(names[i]) == EbsdLib::NumericTypes::Type::Float)
    {
      DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(names[i]);
      auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  }

  // Only read these arrays if the user wants them
  if(m_SelectedArrayNames.find(EbsdLib::CellData::EulerAngles) != m_SelectedArrayNames.end())
  {
    cDims[0] = 3;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Only read the phases if the user wants it.
  if(m_SelectedArrayNames.find(EbsdLib::H5Ebsd::Phases) != m_SelectedArrayNames.end())
  {
    cDims[0] = 1;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(EbsdLib::H5Ebsd::Phases);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Now create the Ensemble arrays for the XTal Structures, Material Names and LatticeConstants
  cDims[0] = 1;
  tupleDims = {2};
  {
    DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::uint32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  //  {
  //    DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::MaterialName);
  //    auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
  //    resultOutputActions.value().appendAction(std::move(action));
  //  }

  cDims[0] = 6;
  {
    DataPath dataArrayPath = pCellEnsembleAttributeMatrixNameValue.createChildPath(EbsdLib::EnsembleData::LatticeConstants);
    auto action = std::make_unique<CreateArrayAction>(complex::DataType::float32, tupleDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadH5EbsdFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReadH5EbsdFilterValue = filterArgs.value<H5EbsdReaderParameter::ValueType>(k_ReadH5EbsdFilter_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pCellEnsembleAttributeMatrixNameValue = pDataContainerNameValue.createChildPath(filterArgs.value<std::string>(k_CellEnsembleAttributeMatrixName_Key));

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  ReadH5EbsdInputValues inputValues;
  inputValues.inputFilePath = pReadH5EbsdFilterValue.inputFilePath;
  inputValues.startSlice = pReadH5EbsdFilterValue.startSlice;
  inputValues.endSlice = pReadH5EbsdFilterValue.endSlice;
  inputValues.eulerRepresentation = pReadH5EbsdFilterValue.eulerRepresentation;
  inputValues.hdf5DataPaths = pReadH5EbsdFilterValue.hdf5DataPaths;
  inputValues.useRecommendedTransform = pReadH5EbsdFilterValue.useRecommendedTransform;
  inputValues.dataContainerPath = pDataContainerNameValue;
  inputValues.cellAttributeMatrixPath = pDataContainerNameValue.createChildPath(pCellAttributeMatrixNameValue);
  inputValues.cellEnsembleMatrixPath = pCellEnsembleAttributeMatrixNameValue;

  ReadH5Ebsd readH5Ebsd(dataStructure, messageHandler, shouldCancel, &inputValues);
  return readH5Ebsd();
}
} // namespace complex
