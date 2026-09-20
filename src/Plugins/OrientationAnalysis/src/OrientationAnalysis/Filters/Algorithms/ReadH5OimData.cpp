#include "ReadH5OimData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <memory>

using namespace nx::core;

ReadH5OimData::ReadH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: IEbsdOemReader<ebsdlib::H5OIMReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

ReadH5OimData::~ReadH5OimData() noexcept = default;

Result<> ReadH5OimData::operator()()
{
  return execute();
}

Result<> ReadH5OimData::copyRawEbsdData(int sliceIndex)
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize tuplesPerScan = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize sliceTupleStart = sliceIndex * tuplesPerScan;

  // Map nonpositive phase IDs to phase one for downstream ensemble indexing.
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
  auto* phasePtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::Ang::PhaseData));

  const auto* phi1Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi1));
  const auto* phiPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi));
  const auto* phi2Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi2));
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::EulerAngles));

  const auto* imageQualPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::ImageQuality));
  auto& imageQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ImageQuality));

  const auto* confIndexPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::ConfidenceIndex));
  auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ConfidenceIndex));

  const auto* semSigPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::SEMSignal));
  auto& semSignal = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::SEMSignal));

  const auto* fitPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Fit));
  auto& fit = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::Fit));

  // EbsdLib owns one resident scan. Use bounded destination pages to avoid
  // per-value disk writes and a second scan-sized allocation.
  constexpr usize kTuplesPerBatch = 65536;
  // The two pages total one MiB, so keep them off Windows' default one-MiB thread stack.
  auto phaseBuffer = std::make_unique<int32[]>(kTuplesPerBatch);
  auto eulerBuffer = std::make_unique<float32[]>(kTuplesPerBatch * 3);
  for(usize tupleOffset = 0; tupleOffset < tuplesPerScan; tupleOffset += kTuplesPerBatch)
  {
    if(m_ShouldCancel)
      return {};
    const usize count = std::min(kTuplesPerBatch, tuplesPerScan - tupleOffset);
    for(usize i = 0; i < count; i++)
    {
      phaseBuffer[i] = std::max(phasePtr[tupleOffset + i], 1);
      eulerBuffer[i * 3] = phi1Ptr[tupleOffset + i];
      eulerBuffer[i * 3 + 1] = phiPtr[tupleOffset + i];
      eulerBuffer[i * 3 + 2] = phi2Ptr[tupleOffset + i];
    }
    Result<> result = phases.getDataStoreRef().copyFromBuffer(sliceTupleStart + tupleOffset, nonstd::span<const int32>(phaseBuffer.get(), count));
    if(result.invalid())
      return result;
    result = eulerAngles.getDataStoreRef().copyFromBuffer((sliceTupleStart + tupleOffset) * 3, nonstd::span<const float32>(eulerBuffer.get(), count * 3));
    if(result.invalid())
      return result;
  }
  // Scalar channels already have the destination layout, so stream them directly.
  const auto copyScalar = [this, sliceTupleStart, tuplesPerScan, kTuplesPerBatch](auto& array, const float32* values) -> Result<> {
    for(usize tupleOffset = 0; tupleOffset < tuplesPerScan; tupleOffset += kTuplesPerBatch)
    {
      if(m_ShouldCancel)
        return {};
      const usize count = std::min(kTuplesPerBatch, tuplesPerScan - tupleOffset);
      auto result = array.getDataStoreRef().copyFromBuffer(sliceTupleStart + tupleOffset, nonstd::span<const float32>(values + tupleOffset, count));
      if(result.invalid())
        return result;
    }
    return {};
  };
  Result<> result = copyScalar(imageQuality, imageQualPtr);
  if(result.invalid())
    return result;
  result = copyScalar(confidenceIndex, confIndexPtr);
  if(result.invalid())
    return result;
  result = copyScalar(semSignal, semSigPtr);
  if(result.invalid())
    return result;
  result = copyScalar(fit, fitPtr);
  if(result.invalid())
    return result;
  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    if(patternDataPtr == nullptr)
    {
      return MakeErrorResult(-34880, "Pattern data was requested but no pattern data was found in the data file");
    }
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::PatternData));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize tupleOffset = 0; tupleOffset < tuplesPerScan; tupleOffset += kTuplesPerBatch)
      {
        if(m_ShouldCancel)
          return {};
        const usize count = std::min(kTuplesPerBatch, tuplesPerScan - tupleOffset);
        auto patternResult = patternData.getDataStoreRef().copyFromBuffer((sliceTupleStart + tupleOffset) * numComponents,
                                                                          nonstd::span<const uint8>(patternDataPtr + tupleOffset * numComponents, count * numComponents));
        if(patternResult.invalid())
          return patternResult;
      }
    }
  }

  return {};
}
