#include "ConvertOrientationsToVertexGeometry.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertOrientations.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <iostream>

using namespace nx::core;

// -----------------------------------------------------------------------------
ConvertOrientationsToVertexGeometry::ConvertOrientationsToVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                         ConvertOrientationsToVertexGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Result<> ConvertOrientationsToVertexGeometry::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  DataStructure tmpDs;

  auto inputArrayF32 = m_DataStructure.getDataAs<Float32Array>(m_InputValues->InputOrientationArrayPath);
  if(inputArrayF32 != nullptr)
  {
    auto tmpArray = Float32Array::CreateWithStore<Float32DataStore>(tmpDs, inputArrayF32->getName(), inputArrayF32->getTupleShape(), inputArrayF32->getComponentShape());
    auto result = CopyFromArray::CopyData(*inputArrayF32, *tmpArray, 0, 0, inputArrayF32->getNumberOfTuples());
    if(result.invalid())
    {
      return result;
    }
  }
  else
  {
    auto inputArrayF64 = m_DataStructure.getDataAs<Float64Array>(m_InputValues->InputOrientationArrayPath);
    auto tmpArray = Float32Array::CreateWithStore<Float32DataStore>(tmpDs, inputArrayF64->getName(), inputArrayF64->getTupleShape(), inputArrayF64->getComponentShape());
    std::copy(inputArrayF64->begin(), inputArrayF64->begin() + inputArrayF64->getSize(), tmpArray->begin());
  }

  DataPath quatsArrayPath;
  if(m_InputValues->InputOrientationType == ebsdlib::orientations::Type::Quaternion)
  {
    quatsArrayPath = DataPath({m_InputValues->InputOrientationArrayPath.getTargetName()});
  }
  else
  {
    auto inputArray = tmpDs.getDataRefAs<Float32Array>(DataPath({m_InputValues->InputOrientationArrayPath.getTargetName()}));
    const std::string quatsArrayName = "Quats_Array";
    quatsArrayPath = DataPath({quatsArrayName});
    Float32Array::CreateWithStore<Float32DataStore>(tmpDs, quatsArrayName, inputArray.getTupleShape(), {4});

    ConvertOrientationsInputValues inputValues;
    inputValues.InputType = m_InputValues->InputOrientationType;
    inputValues.OutputType = ebsdlib::orientations::Type::Quaternion;
    inputValues.InputOrientationArrayPath = DataPath({m_InputValues->InputOrientationArrayPath.getTargetName()});
    inputValues.OutputOrientationArrayName = quatsArrayName;
    Result<> result = ConvertOrientations(tmpDs, m_MessageHandler, m_ShouldCancel, &inputValues)();
    if(result.invalid())
    {
      return result;
    }
  }

  auto quatsArray = tmpDs.getDataRefAs<Float32Array>(quatsArrayPath);
  auto* crystalStructuresArray = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto* phasesArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& outputVertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->OutputVertexGeometryPath);
  Float32Array& vertices = outputVertexGeom.getVerticesRef();
  std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
  for(usize i = 0; i < quatsArray.getNumberOfTuples(); i++)
  {
    ebsdlib::QuatD quat(quatsArray[i * 4 + 0], quatsArray[i * 4 + 1], quatsArray[i * 4 + 2], quatsArray[i * 4 + 3]);
    if(m_InputValues->ConvertToFundamentalZone)
    {
      int32 currentPhaseId = phasesArray->getValue(i);
      uint32 laueClass = crystalStructuresArray->getValue(currentPhaseId);
      quat = (laueClass < ops.size()) ? ops[laueClass]->getFZQuat(quat) : ebsdlib::QuatD(0, 0, 0, 1);
    }

    ebsdlib::StereographicDType st = ebsdlib::QuaternionDType(quat.getPositiveOrientation()).toStereographic();
    vertices.setComponent(i, 0, static_cast<float32>(st[0]));
    vertices.setComponent(i, 1, static_cast<float32>(st[1]));
    vertices.setComponent(i, 2, static_cast<float32>(st[2]));
  }

  return {};
}
