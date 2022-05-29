#include "ReadH5Ebsd.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/EbsdMacros.h"
#include "EbsdLib/IO/H5EbsdVolumeInfo.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5CtfVolumeReader.h"
#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/H5AngVolumeReader.h"

namespace RotateSampleRefFrame
{
// Parameter Keys
static inline constexpr complex::StringLiteral k_RotationRepresentation_Key = "rotation_representation";
static inline constexpr complex::StringLiteral k_RotationAngle_Key = "rotation_angle";
static inline constexpr complex::StringLiteral k_RotationAxis_Key = "rotation_axis";
static inline constexpr complex::StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
static inline constexpr complex::StringLiteral k_SelectedCellArrays_Key = "selected_cell_arrays";
static inline constexpr complex::StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";
static inline constexpr complex::StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";

static inline constexpr complex::StringLiteral k_RotatedGeometryName = ".RotatedGeometry";

enum class RotationRepresentation : uint64_t
{
  AxisAngle = 0,
  RotationMatrix = 1
};

} // namespace RotateSampleRefFrame

namespace RenameDataObject
{
static inline constexpr complex::StringLiteral k_DataObject_Key = "data_object";
static inline constexpr complex::StringLiteral k_NewName_Key = "new_name";
} // namespace RenameDataObject

namespace DeleteData
{
static inline constexpr complex::StringLiteral k_DataPath_Key = "removed_data_path";

}

namespace
{

/**
 * @brief loadInfo Reads the values for the phase type, crystal structure
 * and precipitate fractions from the EBSD file.
 * @param m_InputValues
 * @param m_DataStructure
 * @param reader EbsdReader instance pointer
 * @return
 */
template <typename EbsdReaderType, typename EbsdPhase>
complex::Result<> LoadInfo(const complex::ReadH5EbsdInputValues* m_InputValues, complex::DataStructure& m_DataStructure, std::shared_ptr<EbsdReaderType>& reader)
{
  reader->setFileName(m_InputValues->inputFilePath);
  reader->setSliceStart(m_InputValues->startSlice);
  reader->setSliceEnd(m_InputValues->endSlice);

  std::vector<typename EbsdPhase::Pointer> phases = reader->getPhases();
  if(phases.size() == 0)
  {
    return {complex::MakeErrorResult(-50027, fmt::format("Error reading phase information from file '{}'.", m_InputValues->inputFilePath))};
  }

  // Resize the Ensemble Attribute Matrix to be the correct number of phases.
  std::vector<size_t> tDims = {phases.size() + 1};

  complex::DataPath cellEnsembleMatrixPath = m_InputValues->cellEnsembleMatrixPath;

  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::UInt32Array& xtalData = m_DataStructure.getDataRefAs<complex::UInt32Array>(xtalDataPath);
  xtalData.getIDataStore()->reshapeTuples(tDims);

  complex::DataPath latticeDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::LatticeConstants);
  complex::Float32Array& latticData = m_DataStructure.getDataRefAs<complex::Float32Array>(latticeDataPath);
  latticData.getIDataStore()->reshapeTuples(tDims);

  // Reshape the Material Names here also.

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  xtalData[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  // materialNames->setValue(0, QString("Invalid Phase"));
  latticData[0] = 0.0f;
  latticData[1] = 0.0f;
  latticData[2] = 0.0f;
  latticData[3] = 0.0f;
  latticData[4] = 0.0f;
  latticData[5] = 0.0f;

  for(size_t i = 0; i < phases.size(); i++)
  {
    int32_t phaseID = phases[i]->getPhaseIndex();
    xtalData[phaseID] = phases[i]->determineLaueGroup();
    // materialNames[phaseID] = phases[i]->getMaterialName();
    std::vector<float> latticeConstant = phases[i]->getLatticeConstants();

    latticData[phaseID * 6ULL] = latticeConstant[0];
    latticData[phaseID * 6ULL + 1] = latticeConstant[1];
    latticData[phaseID * 6ULL + 2] = latticeConstant[2];
    latticData[phaseID * 6ULL + 3] = latticeConstant[3];
    latticData[phaseID * 6ULL + 4] = latticeConstant[4];
    latticData[phaseID * 6ULL + 5] = latticeConstant[5];
  }

  return {};
}

template <typename H5EbsdReaderType, typename T>
void CopyData(complex::DataStructure& dataStructure, H5EbsdReaderType* ebsdReader, const std::vector<std::string>& arrayNames, std::set<std::string> selectedArrayNames,
              complex::DataPath cellAttributeMatrixPath, size_t totalPoints)
{
  using DataArrayType = complex::DataArray<T>;
  for(const auto& arrayName : arrayNames)
  {
    if(selectedArrayNames.find(arrayName) != selectedArrayNames.end())
    {
      T* source = reinterpret_cast<T*>(ebsdReader->getPointerByName(arrayName));
      complex::DataPath dataPath = cellAttributeMatrixPath.createChildPath(arrayName); // get the data from the DataStructure
      DataArrayType& destination = dataStructure.getDataRefAs<DataArrayType>(dataPath);
      for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
      {
        destination[tupleIndex] = source[tupleIndex];
      }
    }
  }
}

/**
 * @brief LoadEbsdData
 * @param m_InputValues
 * @param dataStructure
 * @param eulerNames
 * @param m_MessageHandler
 * @param selectedArrayNames
 * @param dcDims
 * @param floatArrays
 * @param intArrays
 * @return
 */
template <typename H5EbsdReaderType, typename PhaseType>
complex::Result<> LoadEbsdData(const complex::ReadH5EbsdInputValues* m_InputValues, complex::DataStructure& dataStructure, const std::vector<std::string>& eulerNames,
                               const complex::IFilter::MessageHandler& m_MessageHandler, std::set<std::string> selectedArrayNames, const std::array<size_t, 3>& dcDims,
                               const std::vector<std::string>& floatArrayNames, const std::vector<std::string>& intArrayNames)
{
  int32_t err = 0;
  std::shared_ptr<H5EbsdReaderType> ebsdReader = std::dynamic_pointer_cast<H5EbsdReaderType>(H5EbsdReaderType::New());
  if(nullptr == ebsdReader)
  {
    return {complex::MakeErrorResult(-50006, fmt::format("Error instantiating H5EbsdVolumeReader"))};
  }
  ebsdReader->setFileName(m_InputValues->inputFilePath);
  complex::Result<> result = LoadInfo<H5EbsdReaderType, PhaseType>(m_InputValues, dataStructure, ebsdReader);
  if(result.invalid())
  {
    return result;
  }

  std::string manufacturer = ebsdReader->getManufacturer();

  if(selectedArrayNames.find(EbsdLib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[0]);
    selectedArrayNames.insert(eulerNames[1]);
    selectedArrayNames.insert(eulerNames[2]);
  }
  if(selectedArrayNames.find(EbsdLib::CellData::Phases) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[3]);
  }

  // Initialize all the arrays with some default values
  m_MessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, fmt::format("Reading EBSD Data from file {}", m_InputValues->inputFilePath)});
  uint32_t m_RefFrameZDir = ebsdReader->getStackingOrder();

  ebsdReader->setSliceStart(m_InputValues->startSlice);
  ebsdReader->setSliceEnd(m_InputValues->endSlice);
  ebsdReader->readAllArrays(false);
  ebsdReader->setArraysToRead(selectedArrayNames);
  err = ebsdReader->loadData(dcDims[0], dcDims[1], dcDims[2], m_RefFrameZDir);
  if(err < 0)
  {
    return {complex::MakeErrorResult(-50003, fmt::format("Error loading data from H5Ebsd file '{}'", m_InputValues->inputFilePath))};
  }

  complex::DataPath geometryPath = m_InputValues->dataContainerPath;
  complex::DataPath cellAttributeMatrixPath = m_InputValues->cellAttributeMatrixPath;

  size_t totalPoints = dcDims[0] * dcDims[1] * dcDims[2];

  // Get the Crystal Structure data which should have already been read from the file and copied to the array
  complex::DataPath cellEnsembleMatrixPath = m_InputValues->cellEnsembleMatrixPath;
  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::UInt32Array& xtalData = dataStructure.getDataRefAs<complex::UInt32Array>(xtalDataPath);

  // Copy the Phase Values from the EBSDReader to the DataStructure
  auto* phasePtr = reinterpret_cast<int32_t*>(ebsdReader->getPointerByName(eulerNames[3]));           // get the phase data from the EbsdReader
  complex::DataPath phaseDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::H5Ebsd::Phases); // get the phase data from the DataStructure
  auto& phaseData = dataStructure.getDataRefAs<complex::Int32Array>(phaseDataPath);
  for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
  {
    phaseData[tupleIndex] = phasePtr[tupleIndex];
  }

  if(selectedArrayNames.find(EbsdLib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    //  radianconversion = M_PI / 180.0;
    auto* euler0 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[0]));
    auto* euler1 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[1]));
    auto* euler2 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[2]));
    //  std::vector<size_t> cDims = {3};
    complex::DataPath eulerDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
    auto& eulerData = dataStructure.getDataRefAs<complex::Float32Array>(eulerDataPath);

    float degToRad = 1.0f;
    if(m_InputValues->eulerRepresentation != EbsdLib::AngleRepresentation::Radians && m_InputValues->useRecommendedTransform)
    {
      degToRad = complex::numbers::pi_v<float> / 180.0F;
    }
    for(size_t elementIndex = 0; elementIndex < totalPoints; elementIndex++)
    {
      eulerData[3 * elementIndex] = euler0[elementIndex] * degToRad;
      eulerData[3 * elementIndex + 1] = euler1[elementIndex] * degToRad;
      eulerData[3 * elementIndex + 2] = euler2[elementIndex] * degToRad;
    }
    // THIS IS ONLY TO BRING OXFORD DATA INTO THE SAME HEX REFERENCE AS EDAX HEX REFERENCE
    if(manufacturer == EbsdLib::Ctf::Manufacturer)
    {
      for(size_t elementIndex = 0; elementIndex < totalPoints; elementIndex++)
      {
        if(xtalData[phaseData[elementIndex]] == EbsdLib::CrystalStructure::Hexagonal_High)
        {
          eulerData[3 * elementIndex + 2] = eulerData[3 * elementIndex + 2] + (30.0F * degToRad);
        }
      }
    }
  }

  // Copy the EBSD Data from its temp location into the final DataStructure location.
  ::CopyData<H5EbsdReaderType, float>(dataStructure, ebsdReader.get(), floatArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);
  ::CopyData<H5EbsdReaderType, int>(dataStructure, ebsdReader.get(), intArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);

  return {};
}

} // namespace

using namespace complex;

// -----------------------------------------------------------------------------
ReadH5Ebsd::ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadH5Ebsd::~ReadH5Ebsd() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5Ebsd::operator()()
{
  using FloatVec3Type = std::array<float, 3>;

  // Get the Size and Spacing of the Volume
  H5EbsdVolumeInfo::Pointer volumeInfoReader = H5EbsdVolumeInfo::New();
  volumeInfoReader->setFileName(m_InputValues->inputFilePath);
  int err = volumeInfoReader->readVolumeInfo();
  if(err < 0)
  {
    return {MakeErrorResult(-50000, fmt::format("Could not read H5EbsdVolumeInfo from file '{}", m_InputValues->inputFilePath))};
  }
  std::array<int64_t, 3> dims = {0, 0, 0};
  std::array<float, 3> res = {0.0f, 0.0f, 0.0f};
  volumeInfoReader->getDimsAndResolution(dims[0], dims[1], dims[2], res[0], res[1], res[2]);

  std::array<size_t, 3> dcDims = {static_cast<size_t>(dims[0]), static_cast<size_t>(dims[1]), static_cast<size_t>(dims[2])};

  // The dimensions/spacing/origin of the ImageGeometry should have already been set during preflight.
  complex::ImageGeom& imageGeom = m_DataStructure.getDataRefAs<complex::ImageGeom>(m_InputValues->dataContainerPath);

  // Now Calculate our "subvolume" of slices, ie, those start and end values that the user selected from the GUI
  dcDims[2] = m_InputValues->endSlice - m_InputValues->startSlice + 1;

  std::string manufacturer = volumeInfoReader->getManufacturer();
  uint32_t m_RefFrameZDir = volumeInfoReader->getStackingOrder();

  std::array<float, 3> sampleTransAxis = volumeInfoReader->getSampleTransformationAxis();
  float sampleTransAngle = volumeInfoReader->getSampleTransformationAngle();

  std::array<float, 3> eulerTransAxis = volumeInfoReader->getEulerTransformationAxis();
  float eulerTransAngle = volumeInfoReader->getEulerTransformationAngle();

  // This will effectively close the reader
  volumeInfoReader = H5EbsdVolumeInfo::NullPointer();

  // Now create the specific reader that we need.
  H5EbsdVolumeReader::Pointer ebsdReader;

  std::set<std::string> m_SelectedArrayNames;
  for(const auto& selectedArrayName : m_InputValues->hdf5DataPaths)
  {
    m_SelectedArrayNames.insert(selectedArrayName);
  }

  if(manufacturer == EbsdLib::Ang::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ang::Phi1, EbsdLib::Ang::Phi, EbsdLib::Ang::Phi2, EbsdLib::Ang::PhaseData};
    std::vector<std::string> floatArrays = {EbsdLib::Ang::ImageQuality, EbsdLib::Ang::ConfidenceIndex, EbsdLib::Ang::SEMSignal, EbsdLib::Ang::Fit, EbsdLib::Ang::XPosition, EbsdLib::Ang::YPosition};
    std::vector<std::string> intArrays = {};
    Result<> result = LoadEbsdData<H5AngVolumeReader, AngPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, m_SelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else if(manufacturer == EbsdLib::Ctf::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ang::Phi1, EbsdLib::Ang::Phi, EbsdLib::Ang::Phi2, EbsdLib::Ctf::Phase};
    std::vector<std::string> floatArrays = {EbsdLib::Ctf::MAD, EbsdLib::Ctf::X, EbsdLib::Ctf::Y};
    std::vector<std::string> intArrays = {EbsdLib::Ctf::Bands, EbsdLib::Ctf::Error, EbsdLib::Ctf::BC, EbsdLib::Ctf::BS};
    Result<> result = LoadEbsdData<H5CtfVolumeReader, CtfPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, m_SelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else
  {
    return {MakeErrorResult(-50001, fmt::format("Could not determine or match a supported manufacturer from the data file. Supported manufacturer codes are: '{}' and '{}'", EbsdLib::Ctf::Manufacturer,
                                                EbsdLib::Ang::Manufacturer))};
  }

  // Sanity Check the Error Condition or the state of the EBSD Reader Object.
  if(m_InputValues->useRecommendedTransform)
  {

    if(eulerTransAngle > 0)
    {
      RotateEulerRefFrameFilter rot_Euler;
      Arguments args;
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAngle_Key, std::make_any<Float32Parameter::ValueType>(eulerTransAngle));
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxis_Key,
                          std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{eulerTransAxis[0], eulerTransAxis[1], eulerTransAxis[2]}));

      complex::DataPath eulerDataPath = m_InputValues->cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
      args.insertOrAssign(RotateEulerRefFrameFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerDataPath));
      // Preflight the filter and check result
      auto preflightResult = rot_Euler.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      auto executeResult = rot_Euler.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
    }

    if(sampleTransAngle > 0)
    {
      const Uuid k_CorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
      auto* filterList = Application::Instance()->getFilterList();

      /*************************************************************************
       * First rename the entire Generated Image Geometry to a temp name
       ************************************************************************/
      const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6");
      const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_CorePluginId);
      auto filter = filterList->createFilter(k_RenameDataObjectFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating RenameDataObject filter"))};
      }
      Arguments args;
      args.insertOrAssign(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(m_InputValues->dataContainerPath));
      args.insertOrAssign(RenameDataObject::k_NewName_Key, std::make_any<std::string>(RotateSampleRefFrame::k_RotatedGeometryName));
      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Preflighting {}...", filter->humanName())});
      complex::IFilter::PreflightResult preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
        return {MakeErrorResult(-50010, fmt::format("Error preflighting {}", filter->humanName()))};
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Executing {}...", filter->humanName())});
      auto executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }

      /*************************************************************************
       * Rotate Sample Ref Frame
       ************************************************************************/
      const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("5efdf395-33fb-4dc0-986e-0dc0ae990f6a");
      const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_CorePluginId);

      filter = filterList->createFilter(k_RotateSampleRefFrameFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating RotateSampleRefFrame filter"))};
      }
      args = Arguments();

      args.insertOrAssign(RotateSampleRefFrame::k_RotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::AxisAngle)));
      args.insertOrAssign(RotateSampleRefFrame::k_SelectedImageGeometry_Key, std::make_any<DataPath>(DataPath({RotateSampleRefFrame::k_RotatedGeometryName})));
      args.insertOrAssign(RotateSampleRefFrame::k_CreatedImageGeometry_Key, std::make_any<DataPath>(m_InputValues->dataContainerPath));

      std::vector<DataPath> rotatedDataPaths;
      for(const auto& path : m_InputValues->hdf5DataPaths)
      {
        std::string aPath = m_InputValues->cellAttributeMatrixPath.createChildPath(path).toString();
        aPath = complex::StringUtilities::replace(aPath, m_InputValues->dataContainerPath.toString(), RotateSampleRefFrame::k_RotatedGeometryName);
        rotatedDataPaths.push_back(*DataPath::FromString(aPath));
      }
      args.insertOrAssign(RotateSampleRefFrame::k_SelectedCellArrays_Key, std::make_any<std::vector<DataPath>>(rotatedDataPaths));
      args.insertOrAssign(RotateSampleRefFrame::k_RotationAngle_Key, std::make_any<Float32Parameter::ValueType>(sampleTransAngle));
      args.insertOrAssign(RotateSampleRefFrame::k_RotationAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>({sampleTransAxis[0], sampleTransAxis[1], sampleTransAxis[2]}));
      args.insertOrAssign(RotateSampleRefFrame::k_RotateSliceBySlice_Key, std::make_any<bool>(true));

      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Preflighting {}...", filter->humanName())});
      preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
        return {MakeErrorResult(-50010, fmt::format("Error preflighting {}", filter->humanName()))};
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Executing {}", filter->humanName())});
      executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }

      /*************************************************************************
       * Delete the original Geometry
       ************************************************************************/
      DataPath nonRotatedDataGroup = DataPath({RotateSampleRefFrame::k_RotatedGeometryName});
      // First set the final DataContainer Object as a parent of the Ensemble Data otherwise
      // that will get deleted.
      DataObject* dataContainerObject = m_DataStructure.getData(m_InputValues->dataContainerPath);
      DataObject* ensembleObject = m_DataStructure.getData(nonRotatedDataGroup.createChildPath(m_InputValues->cellEnsembleMatrixPath.getTargetName()));
      m_DataStructure.setAdditionalParent(ensembleObject->getId(), dataContainerObject->getId());

      const Uuid k_DeleteDataFilterId = *Uuid::FromString("bf286740-e987-49fe-a7c8-6e566e3a0606");
      const FilterHandle k_DeleteDataFilterHandle(k_DeleteDataFilterId, k_CorePluginId);
      filter = filterList->createFilter(k_DeleteDataFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating 'Delete Data Object' filter"))};
      }
      args = Arguments();
      args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(nonRotatedDataGroup));

      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Preflighting {}...", filter->humanName())});
      preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
        return {MakeErrorResult(-50010, fmt::format("Error preflighting {}", filter->humanName()))};
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Executing {}", filter->humanName())});
      executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }
    }
  }

  return {};
}
