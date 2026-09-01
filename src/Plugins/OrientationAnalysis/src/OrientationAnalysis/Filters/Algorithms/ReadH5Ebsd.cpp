#include "ReadH5Ebsd.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Core/EbsdMacros.h>
#include <EbsdLib/IO/H5EbsdVolumeInfo.h>
#include <EbsdLib/IO/HKL/CtfFields.h>
#include <EbsdLib/IO/HKL/H5CtfVolumeReader.h>
#include <EbsdLib/IO/TSL/AngFields.h>
#include <EbsdLib/IO/TSL/H5AngVolumeReader.h>

using namespace nx::core;

namespace
{
constexpr nx::core::StringLiteral k_RotationRepresentation_Key = "rotation_representation_index";
constexpr nx::core::StringLiteral k_RotationAxisAngle_Key = "rotation_axis_angle";
constexpr nx::core::StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";
constexpr nx::core::StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
constexpr nx::core::StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
constexpr nx::core::StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";

/**
 * @enum RotationRepresentation
 * @brief Selects the representation passed to the sample-frame rotation filter.
 */
enum class RotationRepresentation : uint64
{
  AxisAngle = 0,
  RotationMatrix = 1
};

/**
 * @brief Imports phase metadata into the ensemble arrays.
 * @tparam EbsdReaderType H5Ebsd volume reader type.
 * @tparam EbsdPhase EBSD phase metadata type.
 * @param mInputValues Identifies the file, slice range, and ensemble path.
 * @param mDataStructure Contains the destination arrays.
 * @param reader Reader that supplies phase metadata.
 * @return Error when the file contains no phase metadata.
 * @pre Each positive phase index fits the destination arrays.
 * @pre Each phase supplies six lattice constants.
 */
template <typename EbsdReaderType, typename EbsdPhase>
nx::core::Result<> LoadInfo(const nx::core::ReadH5EbsdInputValues* mInputValues, nx::core::DataStructure& mDataStructure, std::shared_ptr<EbsdReaderType>& reader)
{
  reader->setFileName(mInputValues->inputFilePath);
  reader->setSliceStart(mInputValues->startSlice);
  reader->setSliceEnd(mInputValues->endSlice);

  std::vector<typename EbsdPhase::Pointer> phases = reader->getPhases();
  if(phases.empty())
  {
    return {nx::core::MakeErrorResult(-50027, fmt::format("Error reading phase information from file '{}'.", mInputValues->inputFilePath))};
  }

  // Reserve index zero for invalid phase data.
  ShapeType tDims = {phases.size() + 1};

  nx::core::DataPath cellEnsembleMatrixPath = mInputValues->cellEnsembleMatrixPath;

  nx::core::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(ebsdlib::EnsembleData::CrystalStructures);
  auto& xtalData = mDataStructure.getDataRefAs<nx::core::UInt32Array>(xtalDataPath);
  xtalData.getIDataStore()->resizeTuples(tDims);

  nx::core::DataPath latticeDataPath = cellEnsembleMatrixPath.createChildPath(ebsdlib::EnsembleData::LatticeConstants);
  auto& latticData = mDataStructure.getDataRefAs<nx::core::Float32Array>(latticeDataPath);
  latticData.getIDataStore()->resizeTuples(tDims);

  nx::core::DataPath matNamesDataath = cellEnsembleMatrixPath.createChildPath(ebsdlib::EnsembleData::MaterialName);
  auto& matNameData = mDataStructure.getDataRefAs<nx::core::StringArray>(matNamesDataath);
  matNameData.resizeTuples(tDims);

  // Index zero represents cells without a valid phase.
  xtalData[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  latticData[0] = 0.0f;
  latticData[1] = 0.0f;
  latticData[2] = 0.0f;
  latticData[3] = 0.0f;
  latticData[4] = 0.0f;
  latticData[5] = 0.0f;

  for(usize i = 0; i < phases.size(); i++)
  {
    int32 phaseID = phases[i]->getPhaseIndex();
    xtalData[phaseID] = phases[i]->determineOrientationOpsIndex();
    matNameData[phaseID] = phases[i]->getMaterialName();
    std::vector<float32> latticeConstant = phases[i]->getLatticeConstants();

    latticData[phaseID * 6ULL] = latticeConstant[0];
    latticData[phaseID * 6ULL + 1] = latticeConstant[1];
    latticData[phaseID * 6ULL + 2] = latticeConstant[2];
    latticData[phaseID * 6ULL + 3] = latticeConstant[3];
    latticData[phaseID * 6ULL + 4] = latticeConstant[4];
    latticData[phaseID * 6ULL + 5] = latticeConstant[5];
  }

  return {};
}

/**
 * @brief Copies selected EBSD data arrays from the H5Ebsd reader into the DataStructure.
 *
 * Each selected array uses one bulk destination write. EbsdLib owns the complete
 * source buffer, so this operation supports an out-of-core destination but does
 * not provide bounded source memory.
 *
 * @tparam H5EbsdReaderType H5Ebsd volume reader type.
 * @tparam T Array value type.
 * @param dataStructure Contains the destination arrays.
 * @param ebsdReader Owns parsed source buffers.
 * @param arrayNames Candidate array names of type T.
 * @param selectedArrayNames Names selected for import.
 * @param cellAttributeMatrixPath Parent path for destination arrays.
 * @param totalPoints Volume tuple count.
 * @pre Reader pointers and destination ranges contain totalPoints times each array's component count.
 *
 * The current implementation does not inspect copyFromBuffer() Result values.
 */
template <typename H5EbsdReaderType, typename T>
void CopyData(nx::core::DataStructure& dataStructure, H5EbsdReaderType* ebsdReader, const std::vector<std::string>& arrayNames, std::set<std::string> selectedArrayNames,
              const nx::core::DataPath& cellAttributeMatrixPath, usize totalPoints)
{
  using DataArrayType = nx::core::DataArray<T>;
  for(const auto& arrayName : arrayNames)
  {
    if(selectedArrayNames.find(arrayName) != selectedArrayNames.end())
    {
      T* source = reinterpret_cast<T*>(ebsdReader->getPointerByName(arrayName));
      nx::core::DataPath dataPath = cellAttributeMatrixPath.createChildPath(arrayName);
      auto& destination = dataStructure.getDataRefAs<DataArrayType>(dataPath);
      // Use one destination transfer because EbsdLib already owns the full source array.
      destination.getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(source, totalPoints * destination.getNumberOfComponents()));
    }
  }
}

/**
 * @brief Loads one supported H5Ebsd manufacturer representation.
 * @tparam H5EbsdReaderType H5Ebsd volume reader type.
 * @tparam PhaseType EBSD phase metadata type.
 * @param mInputValues Identifies the file, slice range, arrays, and destinations.
 * @param dataStructure Contains destination objects.
 * @param eulerNames Names the three Euler channels and phase channel.
 * @param mMessageHandler Receives status messages.
 * @param selectedArrayNames Names selected for import.
 * @param dcDims Selected volume dimensions.
 * @param floatArrayNames Candidate float arrays.
 * @param intArrayNames Candidate integer arrays.
 * @return Phase or EbsdLib load errors.
 * @pre Dimension products fit usize and destination arrays match the selected volume.
 * @pre Cell phase IDs index the crystal-structure array when Oxford correction applies.
 *
 * EbsdLib materializes selected source arrays. The current destination bulk
 * writes do not propagate their Result values.
 */
template <typename H5EbsdReaderType, typename PhaseType>
nx::core::Result<> LoadEbsdData(const nx::core::ReadH5EbsdInputValues* mInputValues, nx::core::DataStructure& dataStructure, const std::vector<std::string>& eulerNames,
                                const nx::core::IFilter::MessageHandler& mMessageHandler, std::set<std::string> selectedArrayNames, const std::array<usize, 3>& dcDims,
                                const std::vector<std::string>& floatArrayNames, const std::vector<std::string>& intArrayNames)
{
  int32 err = 0;
  std::shared_ptr<H5EbsdReaderType> ebsdReader = std::dynamic_pointer_cast<H5EbsdReaderType>(H5EbsdReaderType::New());
  if(nullptr == ebsdReader)
  {
    return {nx::core::MakeErrorResult(-50006, fmt::format("Error instantiating H5EbsdVolumeReader for file '{}'.", mInputValues->inputFilePath))};
  }
  ebsdReader->setFileName(mInputValues->inputFilePath);
  nx::core::Result<> result = LoadInfo<H5EbsdReaderType, PhaseType>(mInputValues, dataStructure, ebsdReader);
  if(result.invalid())
  {
    return result;
  }

  std::string manufacturer = ebsdReader->getManufacturer();

  if(selectedArrayNames.find(ebsdlib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[0]);
    selectedArrayNames.insert(eulerNames[1]);
    selectedArrayNames.insert(eulerNames[2]);
  }
  if(selectedArrayNames.find(ebsdlib::CellData::Phases) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[3]);
  }

  mMessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, fmt::format("Reading EBSD Data from file {}", mInputValues->inputFilePath)});
  uint32 mRefFrameZDir = ebsdReader->getStackingOrder();

  ebsdReader->setSliceStart(mInputValues->startSlice);
  ebsdReader->setSliceEnd(mInputValues->endSlice);
  ebsdReader->readAllArrays(false);
  ebsdReader->setArraysToRead(selectedArrayNames);
  err = ebsdReader->loadData(dcDims[0], dcDims[1], dcDims[2], mRefFrameZDir);
  if(err < 0)
  {
    return {nx::core::MakeErrorResult(-50003, fmt::format("Error loading data from H5Ebsd file '{}'. Error from EbsdLib is {}", mInputValues->inputFilePath, err))};
  }

  nx::core::DataPath geometryPath = mInputValues->dataContainerPath;
  nx::core::DataPath cellAttributeMatrixPath = mInputValues->cellAttributeMatrixPath;

  usize totalPoints = dcDims[0] * dcDims[1] * dcDims[2];

  // Oxford correction uses the ensemble crystal structure for each cell phase.
  nx::core::DataPath cellEnsembleMatrixPath = mInputValues->cellEnsembleMatrixPath;
  nx::core::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(ebsdlib::EnsembleData::CrystalStructures);
  auto& xtalData = dataStructure.getDataRefAs<nx::core::UInt32Array>(xtalDataPath);

  auto* phasePtr = reinterpret_cast<int32*>(ebsdReader->getPointerByName(eulerNames[3]));
  nx::core::DataPath phaseDataPath = cellAttributeMatrixPath.createChildPath(ebsdlib::H5Ebsd::Phases);
  nx::core::Int32Array* phaseDataArrayPtr = nullptr;

  if(selectedArrayNames.find(eulerNames[3]) != selectedArrayNames.end())
  {
    phaseDataArrayPtr = dataStructure.getDataAs<nx::core::Int32Array>(phaseDataPath);
    phaseDataArrayPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalPoints));
  }

  if(selectedArrayNames.find(ebsdlib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    auto* euler0 = reinterpret_cast<float32*>(ebsdReader->getPointerByName(eulerNames[0]));
    auto* euler1 = reinterpret_cast<float32*>(ebsdReader->getPointerByName(eulerNames[1]));
    auto* euler2 = reinterpret_cast<float32*>(ebsdReader->getPointerByName(eulerNames[2]));
    nx::core::DataPath eulerDataPath = cellAttributeMatrixPath.createChildPath(ebsdlib::CellData::EulerAngles);
    auto& eulerData = dataStructure.getDataRefAs<nx::core::Float32Array>(eulerDataPath);

    float32 degToRad = 1.0f;
    if(mInputValues->eulerRepresentation != ebsdlib::AngleRepresentation::Radians && mInputValues->useRecommendedTransform)
    {
      degToRad = nx::core::numbers::pi_v<float32> / 180.0F;
    }
    // Interleave the three Euler channels in bounded destination pages. Apply
    // Oxford correction in the same pass to avoid a second output scan.
    constexpr usize k_ChunkTuples = 65536;
    auto eulerBuf = std::make_unique<float32[]>(k_ChunkTuples * 3);

    // Oxford correction caches all cell phases to avoid random out-of-core reads.
    // This cache is volume-sized and makes the correction path unbounded.
    std::unique_ptr<int32[]> phaseCache;
    std::unique_ptr<uint32[]> xtalCache;
    bool applyHexCorrection = (manufacturer == ebsdlib::Ctf::Manufacturer && phaseDataArrayPtr != nullptr);
    if(applyHexCorrection)
    {
      phaseCache = std::make_unique<int32[]>(totalPoints);
      phaseDataArrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(phaseCache.get(), totalPoints));
      usize numXtal = xtalData.getSize();
      xtalCache = std::make_unique<uint32[]>(numXtal);
      xtalData.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(xtalCache.get(), numXtal));
    }

    auto& eulerStore = eulerData.getDataStoreRef();
    for(usize startTup = 0; startTup < totalPoints; startTup += k_ChunkTuples)
    {
      usize count = std::min(k_ChunkTuples, totalPoints - startTup);
      for(usize i = 0; i < count; i++)
      {
        usize srcIdx = startTup + i;
        eulerBuf[i * 3] = euler0[srcIdx] * degToRad;
        eulerBuf[i * 3 + 1] = euler1[srcIdx] * degToRad;
        eulerBuf[i * 3 + 2] = euler2[srcIdx] * degToRad;
        if(applyHexCorrection && xtalCache[phaseCache[srcIdx]] == ebsdlib::CrystalStructure::Hexagonal_High)
        {
          eulerBuf[i * 3 + 2] += (30.0F * degToRad);
        }
      }
      eulerStore.copyFromBuffer(startTup * 3, nonstd::span<const float32>(eulerBuf.get(), count * 3));
    }
  }

  // Copy the remaining selected EbsdLib buffers to their destination arrays.
  ::CopyData<H5EbsdReaderType, float32>(dataStructure, ebsdReader.get(), floatArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);
  ::CopyData<H5EbsdReaderType, int>(dataStructure, ebsdReader.get(), intArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);

  return {};
}

} // namespace

ReadH5Ebsd::ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(mesgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

ReadH5Ebsd::~ReadH5Ebsd() noexcept = default;

Result<> ReadH5Ebsd::operator()()
{
  // Read volume metadata before EbsdLib allocates selected array buffers.
  ebsdlib::H5EbsdVolumeInfo::Pointer volumeInfoReader = ebsdlib::H5EbsdVolumeInfo::New();
  volumeInfoReader->setFileName(m_InputValues->inputFilePath);
  int err = volumeInfoReader->readVolumeInfo();
  if(err < 0)
  {
    return MakeErrorResult(-50000, fmt::format("Could not read H5EbsdVolumeInfo from file '{}", m_InputValues->inputFilePath));
  }
  std::array<int64, 3> dims = {0, 0, 0};
  std::array<float32, 3> res = {0.0f, 0.0f, 0.0f};
  volumeInfoReader->getDimsAndResolution(dims[0], dims[1], dims[2], res[0], res[1], res[2]);

  std::array<usize, 3> dcDims = {static_cast<usize>(dims[0]), static_cast<usize>(dims[1]), static_cast<usize>(dims[2])};

  // Restrict Z to the inclusive slice range selected by the caller.
  dcDims[2] = m_InputValues->endSlice - m_InputValues->startSlice + 1;

  std::string manufacturer = volumeInfoReader->getManufacturer();

  std::array<float32, 3> sampleTransAxis = volumeInfoReader->getSampleTransformationAxis();
  float32 sampleTransAngle = volumeInfoReader->getSampleTransformationAngle();

  std::array<float32, 3> eulerTransAxis = volumeInfoReader->getEulerTransformationAxis();
  float32 eulerTransAngle = volumeInfoReader->getEulerTransformationAngle();

  // Release the metadata reader before volume data allocation.
  volumeInfoReader = ebsdlib::H5EbsdVolumeInfo::NullPointer();

  std::set<std::string> mSelectedArrayNames;
  for(const auto& selectedArrayName : m_InputValues->hdf5DataPaths)
  {
    mSelectedArrayNames.insert(selectedArrayName);
  }

  if(manufacturer == ebsdlib::Ang::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {ebsdlib::Ang::Phi1, ebsdlib::Ang::Phi, ebsdlib::Ang::Phi2, ebsdlib::Ang::PhaseData};
    std::vector<std::string> floatArrays = {ebsdlib::Ang::ImageQuality, ebsdlib::Ang::ConfidenceIndex, ebsdlib::Ang::SEMSignal, ebsdlib::Ang::Fit, ebsdlib::Ang::XPosition, ebsdlib::Ang::YPosition};
    std::vector<std::string> intArrays = {};
    Result<> result =
        LoadEbsdData<ebsdlib::H5AngVolumeReader, ebsdlib::AngPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, mSelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else if(manufacturer == ebsdlib::Ctf::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {ebsdlib::Ctf::Euler1, ebsdlib::Ctf::Euler2, ebsdlib::Ctf::Euler3, ebsdlib::Ctf::Phase};
    std::vector<std::string> floatArrays = {ebsdlib::Ctf::MAD, ebsdlib::Ctf::X, ebsdlib::Ctf::Y};
    std::vector<std::string> intArrays = {ebsdlib::Ctf::Bands, ebsdlib::Ctf::Error, ebsdlib::Ctf::BC, ebsdlib::Ctf::BS};
    Result<> result =
        LoadEbsdData<ebsdlib::H5CtfVolumeReader, ebsdlib::CtfPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, mSelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else
  {
    return MakeErrorResult(-50001, fmt::format("Could not determine or match a supported manufacturer from the data file. Supported manufacturer codes are: '{}' and '{}'", ebsdlib::Ctf::Manufacturer,
                                               ebsdlib::Ang::Manufacturer));
  }
  if(m_InputValues->useRecommendedTransform)
  {

    nx::core::DataPath eulerDataPath = m_InputValues->cellAttributeMatrixPath.createChildPath(ebsdlib::CellData::EulerAngles);

    if(eulerTransAngle > 0 && m_DataStructure.containsData(eulerDataPath))
    {
      RotateEulerRefFrameFilter rotEuler;
      Arguments args;
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key,
                          std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{eulerTransAxis[0], eulerTransAxis[1], eulerTransAxis[2], eulerTransAngle}));

      args.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerDataPath));
      // Validate the stored Euler transform before it changes the imported array.
      auto preflightResult = rotEuler.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        Result<> result;
        for(const auto& error : preflightResult.outputActions.errors())
        {
          result.errors().push_back(error);
        }
        return result;
      }

      auto executeResult = rotEuler.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
        return MakeErrorResult(-50011, fmt::format("Error executing {}", rotEuler.humanName()));
      }
    }

    if(sampleTransAngle > 0)
    {
      const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
      auto* filterList = Application::Instance()->getFilterList();

      const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("d2451dc1-a5a1-4ac2-a64d-7991669dcffc");
      const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_SimplnxCorePluginId);

      auto filter = filterList->createFilter(k_RotateSampleRefFrameFilterHandle);
      if(nullptr == filter)
      {
        return MakeErrorResult(-50010, fmt::format("Error creating RotateSampleRefFrame filter. Ensure the SimplnxCore plugin is loaded."));
      }
      Arguments args;

      args.insertOrAssign(::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_InputValues->dataContainerPath));
      args.insertOrAssign(::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

      args.insertOrAssign(::k_RotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(::RotationRepresentation::AxisAngle)));
      args.insertOrAssign(::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>({sampleTransAxis[0], sampleTransAxis[1], sampleTransAxis[2], sampleTransAngle}));
      args.insertOrAssign(::k_RotateSliceBySlice_Key, std::make_any<bool>(true));

      // Validate the plugin-provided sample transform before it changes geometry.
      m_MessageHandler(nx::core::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Preflighting {}...", filter->humanName())});
      nx::core::IFilter::PreflightResult preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        Result<> result;
        for(const auto& error : preflightResult.outputActions.errors())
        {
          result.errors().push_back(error);
        }
        return result;
      }

      m_MessageHandler(nx::core::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Executing {}", filter->humanName())});
      auto executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
        return {{nonstd::make_unexpected(executeResult.result.errors())}};
      }
    }
  }

  return {};
}
