#include "WriteLAMMPSFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
using ull = unsigned long long int;
}

// -----------------------------------------------------------------------------
WriteLAMMPSFile::WriteLAMMPSFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteLAMMPSFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteLAMMPSFile::~WriteLAMMPSFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteLAMMPSFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteLAMMPSFile::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  std::ofstream file = std::ofstream(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!file.is_open())
  {
    return MakeErrorResult(-77450, fmt::format("Error creating and opening output file at path: {}", m_InputValues->OutputFile.string()));
  }

  const auto& vertexGeom = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->VertexGeomPath);
  const AbstractDataStore<VertexGeom::SharedVertexList::value_type>& verts = vertexGeom->getVertices()->getDataStoreRef();

  const Int32AbstractDataStore& atomLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->AtomLabelsPath)->getDataStoreRef();


  m_MessageHandler.sendInfoMessage("Finding Max Atom Label...");
  int32 atomTypes = 0;
  for(usize i = 0; i < atomLabels.getNumberOfTuples(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    int32 atomLabel = atomLabels.getValue(i);
    if(atomLabel > atomTypes)
    {
      atomTypes = atomLabel;
    }
  }

  float xMin = 1000000000.0;
  float xMax = 0.0;
  float yMin = 1000000000.0;
  float yMax = 0.0;
  float zMin = 1000000000.0;
  float zMax = 0.0;
  int dummy = 0;

  m_MessageHandler.sendInfoMessage("Finding Min/Max Vertices...");
  for(usize i = 0; i < verts.getNumberOfTuples(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    std::array<float32, 3> pos = {verts.getValue((i * 3) + 0), verts.getValue((i * 3) + 1), verts.getValue((i * 3) + 2)};
    if(pos[0] < xMin)
    {
      xMin = pos[0];
    }
    if(pos[0] > xMax)
    {
      xMax = pos[0];
    }
    if(pos[1] < yMin)
    {
      yMin = pos[1];
    }
    if(pos[1] > yMax)
    {
      yMax = pos[1];
    }
    if(pos[2] < zMin)
    {
      zMin = pos[2];
    }
    if(pos[2] > zMax)
    {
      zMax = pos[2];
    }
  }

  m_MessageHandler.sendInfoMessage("Writing File Metadata...");
  file << "LAMMPS data file\n";
  file << "\n";
  file << fmt::format("{} atoms\n", static_cast<long long int>(verts.getNumberOfTuples()));
  file << "\n";
  file << fmt::format("{} atom types\n", static_cast<long long int>(atomTypes));
  file << "\n";
  file << fmt::format("{:f} {:f} xlo xhi\n", xMin, xMax);
  file << fmt::format("{:f} {:f} ylo yhi\n", yMin, yMax);
  file << fmt::format("{:f} {:f} zlo zhi\n", zMin, zMax);
  file << "\n";
  file << "Atoms\n";
  file << "\n";

  m_MessageHandler.sendInfoMessage("Exporting Data...");
  ThrottledMessageHandler throttledMessenger(m_MessageHandler);
  // Write the Atom positions (Vertices)
  usize numVerts = verts.getNumberOfTuples();
  usize increment = numVerts / 1000;
  for(usize i = 0; i < numVerts; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(i % increment == 0)
    {
      throttledMessenger.updatePercent("Exporting Data", i, numVerts);
    }
    // Write the positions to the output file
    file << fmt::format("{} {:d} {:f} {:f} {:f} {:d} {:d} {:d}\n", i + 1LL, atomLabels.getValue(i), verts.getValue((i * 3) + 0), verts.getValue((i * 3) + 1), verts.getValue((i * 3) + 2), dummy, dummy,
                        dummy);
  }

  // newline at end of file
  file << "\n";

  file.flush();
  file.close();

  return {};
}
