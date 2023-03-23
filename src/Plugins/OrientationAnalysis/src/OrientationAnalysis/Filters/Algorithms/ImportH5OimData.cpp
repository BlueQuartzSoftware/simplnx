#include "ImportH5OimData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportH5OimData::ImportH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportH5OimDataInputValues* oimInputValues,
                                 ImportH5DataInputValues* inputValues)
: ImportH5Data<H5OIMReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
, m_OimInputValues(oimInputValues)
{
}

// -----------------------------------------------------------------------------
ImportH5OimData::~ImportH5OimData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ImportH5OimData::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ImportH5OimData::copyRawEbsdData(int index)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  // Adjust the values of the 'phase' data to correct for invalid values
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::Phases));
  auto* phasePtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::Ang::PhaseData));
  for(size_t i = 0; i < totalPoints; i++)
  {
    if(phasePtr[i] < 1)
    {
      phasePtr[i] = 1;
    }
    phases[offset + i] = phasePtr[i];
  }

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi1));
    const auto* f2 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi));
    const auto* f3 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi2));
    auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::EulerAngles));
    for(size_t i = 0; i < totalPoints; i++)
    {
      eulerAngles[offset + 3 * i] = f1[i];
      eulerAngles[offset + 3 * i + 1] = f2[i];
      eulerAngles[offset + 3 * i + 2] = f3[i];
    }
  }
  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::ImageQuality));
    auto& imageQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ImageQuality));
    for(size_t i = 0; i < totalPoints; i++)
    {
      imageQuality[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::ConfidenceIndex));
    auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ConfidenceIndex));
    for(size_t i = 0; i < totalPoints; i++)
    {
      confidenceIndex[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::SEMSignal));
    auto& semSignal = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::SEMSignal));
    for(size_t i = 0; i < totalPoints; i++)
    {
      semSignal[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Fit));
    auto& fit = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::Fit));
    for(size_t i = 0; i < totalPoints; i++)
    {
      fit[offset + i] = f1[i];
    }
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::PatternData));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize i = 0; i < totalPoints; i++)
      {
        for(usize j = 0; j < numComponents; ++j)
        {
          patternData[offset + numComponents * i + j] = patternDataPtr[numComponents * i + j];
        }
      }
    }
  }

  return {};
}