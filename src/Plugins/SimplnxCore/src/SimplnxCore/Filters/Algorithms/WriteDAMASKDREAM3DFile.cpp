#include "WriteDAMASKDREAM3DFile.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_DataContainerName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";
constexpr StringLiteral k_CellFeatureDataName = "CellFeatureData";
constexpr StringLiteral k_CellEnsembleDataName = "CellEnsembleData";
constexpr StringLiteral k_EulerAnglesName = "EulerAngles";
constexpr StringLiteral k_PhasesName = "Phases";
constexpr StringLiteral k_FeatureIdsName = "FeatureIds";
constexpr StringLiteral k_PhaseNameName = "PhaseName";

struct AddSharedArray
{
  template <typename T>
  Result<> operator()(const DataStructure& sourceDataStructure, DataStructure& outputDataStructure, const DataPath& sourcePath, std::string_view outputName, DataObject::IdType parentId) const
  {
    const auto& sourceArray = sourceDataStructure.getDataRefAs<DataArray<T>>(sourcePath);
    auto sourceStore = sourceArray.getDataStorePtr().lock();
    if(sourceStore == nullptr)
    {
      return MakeErrorResult(-12120, fmt::format("The selected array '{}' has no readable data store.", sourcePath.toString()));
    }

    if(DataArray<T>::Create(outputDataStructure, std::string(outputName), std::move(sourceStore), parentId) == nullptr)
    {
      return MakeErrorResult(-12121, fmt::format("Could not map selected array '{}' to canonical output name '{}'.", sourcePath.toString(), outputName));
    }
    return {};
  }
};

Result<> AddCanonicalArray(const DataStructure& sourceDataStructure, DataStructure& outputDataStructure, const DataPath& sourcePath, std::string_view outputName, DataObject::IdType parentId)
{
  const auto& sourceArray = sourceDataStructure.getDataRefAs<IDataArray>(sourcePath);
  return ExecuteDataFunctionNoBool(AddSharedArray{}, sourceArray.getDataType(), sourceDataStructure, outputDataStructure, sourcePath, outputName, parentId);
}
} // namespace

WriteDAMASKDREAM3DFile::WriteDAMASKDREAM3DFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                               WriteDAMASKDREAM3DFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

WriteDAMASKDREAM3DFile::~WriteDAMASKDREAM3DFile() noexcept = default;

Result<> WriteDAMASKDREAM3DFile::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  const auto& sourceGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  DataStructure outputDataStructure;
  auto* outputGeom = ImageGeom::Create(outputDataStructure, k_DataContainerName.str());
  if(outputGeom == nullptr)
  {
    return MakeErrorResult(-12122, "Could not create the canonical output Image Geometry 'DataContainer'.");
  }

  outputGeom->setDimensions(sourceGeom.getDimensions());
  const FloatVec3 sourceSpacing = sourceGeom.getSpacing();
  const FloatVec3 sourceOrigin = sourceGeom.getOrigin();
  const auto scale = static_cast<float32>(m_InputValues->ScaleToMeters);
  outputGeom->setSpacing({sourceSpacing[0] * scale, sourceSpacing[1] * scale, sourceSpacing[2] * scale});
  outputGeom->setOrigin({sourceOrigin[0] * scale, sourceOrigin[1] * scale, sourceOrigin[2] * scale});
  outputGeom->setUnits(IGeometry::LengthUnit::Meter);

  const SizeVec3 dimensions = sourceGeom.getDimensions();
  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* cellData = AttributeMatrix::Create(outputDataStructure, k_CellDataName.str(), cellShape, outputGeom->getId());
  if(cellData == nullptr)
  {
    return MakeErrorResult(-12123, "Could not create the canonical output Attribute Matrix 'DataContainer/CellData'.");
  }
  outputGeom->setCellData(*cellData);

  Result<> result;
  if(m_InputValues->Representation == 0)
  {
    result = AddCanonicalArray(m_DataStructure, outputDataStructure, m_InputValues->CellEulerAnglesArrayPath, k_EulerAnglesName, cellData->getId());
    if(result.invalid())
    {
      return result;
    }
    result = AddCanonicalArray(m_DataStructure, outputDataStructure, m_InputValues->CellPhasesArrayPath, k_PhasesName, cellData->getId());
    if(result.invalid())
    {
      return result;
    }
  }
  else
  {
    result = AddCanonicalArray(m_DataStructure, outputDataStructure, m_InputValues->FeatureIdsArrayPath, k_FeatureIdsName, cellData->getId());
    if(result.invalid())
    {
      return result;
    }

    const auto& featureEulerAngles = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureEulerAnglesArrayPath);
    auto* featureData = AttributeMatrix::Create(outputDataStructure, k_CellFeatureDataName.str(), featureEulerAngles.getTupleShape(), outputGeom->getId());
    if(featureData == nullptr)
    {
      return MakeErrorResult(-12124, "Could not create the canonical output Attribute Matrix 'DataContainer/CellFeatureData'.");
    }
    result = AddCanonicalArray(m_DataStructure, outputDataStructure, m_InputValues->FeatureEulerAnglesArrayPath, k_EulerAnglesName, featureData->getId());
    if(result.invalid())
    {
      return result;
    }
    result = AddCanonicalArray(m_DataStructure, outputDataStructure, m_InputValues->FeaturePhasesArrayPath, k_PhasesName, featureData->getId());
    if(result.invalid())
    {
      return result;
    }
  }

  if(m_InputValues->WritePhaseNames)
  {
    const auto& phaseNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->PhaseNamesArrayPath);
    auto* ensembleData = AttributeMatrix::Create(outputDataStructure, k_CellEnsembleDataName.str(), phaseNames.getTupleShape(), outputGeom->getId());
    if(ensembleData == nullptr)
    {
      return MakeErrorResult(-12125, "Could not create the canonical output Attribute Matrix 'DataContainer/CellEnsembleData'.");
    }
    if(StringArray::CreateWithValues(outputDataStructure, k_PhaseNameName, phaseNames.getTupleShape(), phaseNames.values(), ensembleData->getId()) == nullptr)
    {
      return MakeErrorResult(
          -12126, fmt::format("Could not map selected phase-name array '{}' to canonical output path 'DataContainer/CellEnsembleData/PhaseName'.", m_InputValues->PhaseNamesArrayPath.toString()));
    }
  }

  auto atomicFileResult = AtomicFile::Create(m_InputValues->OutputFile);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing canonical DAMASK DREAM3D file '{}'.", m_InputValues->OutputFile.string()));
  HDF5::DataStructureWriter::WriteOptions writeOptions;
  writeOptions.compressionLevel = m_InputValues->UseCompression ? m_InputValues->CompressionLevel : 0;
  result = DREAM3D::WriteFile(atomicFile.tempFilePath(), outputDataStructure, Pipeline{}, false, writeOptions);
  if(result.invalid() || m_ShouldCancel)
  {
    return result;
  }
  return atomicFile.commit();
}
