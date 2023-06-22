#include "LosAlamosFFTWriter.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <fstream>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
using ull = unsigned long long int;
}

// -----------------------------------------------------------------------------
LosAlamosFFTWriter::LosAlamosFFTWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LosAlamosFFTWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LosAlamosFFTWriter::~LosAlamosFFTWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LosAlamosFFTWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LosAlamosFFTWriter::operator()()
{
  /**
   * Header print function was unimplemented in original filter so it was omitted here and condensed to just the writeFile() function
   */

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  std::ofstream file = std::ofstream(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!file.is_open())
  {
    return MakeErrorResult(-73450, fmt::format("Error creating and opening output file at path: {}", m_InputValues->OutputFile.string()));
  }

  SizeVec3 dims = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath)->getDimensions();

  auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  float phi1 = 0.0f, phi = 0.0f, phi2 = 0.0f;

  for(usize z = 0; z < dims[2]; ++z)
  {
    for(usize y = 0; y < dims[1]; ++y)
    {
      for(usize x = 0; x < dims[0]; ++x)
      {
        usize index = (z * dims[0] * dims[1]) + (dims[0] * y) + x;
        phi1 = cellEulerAngles[index * 3] * 180.0 * Constants::k_1OverPiD;
        phi = cellEulerAngles[index * 3 + 1] * 180.0 * Constants::k_1OverPiD;
        phi2 = cellEulerAngles[index * 3 + 2] * 180.0 * Constants::k_1OverPiD;
        file << fmt::format("{:.3f} {:.3f} {:.3f} {} {} {} {} {}\n", phi1, phi, phi2, static_cast<::ull>(x + 1), static_cast<::ull>(y + 1), static_cast<::ull>(z + 1), featureIds[index],
                            cellPhases[index]);
      }
    }
  }

  file.flush();
  file.close();

  return {};
}
