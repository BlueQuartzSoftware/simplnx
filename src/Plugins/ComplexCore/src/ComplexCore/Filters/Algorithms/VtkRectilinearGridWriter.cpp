#include "VtkRectilinearGridWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
VtkRectilinearGridWriter::VtkRectilinearGridWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   VtkRectilinearGridWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VtkRectilinearGridWriter::~VtkRectilinearGridWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VtkRectilinearGridWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VtkRectilinearGridWriter::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 res = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  int err = 0;
  FILE* f = nullptr;
  f = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == f)
  {
    return MakeErrorResult(-2073, fmt::format("Error opening output vtk file '{}'", m_InputValues->OutputFile.string()));
  }

  return {};
}
