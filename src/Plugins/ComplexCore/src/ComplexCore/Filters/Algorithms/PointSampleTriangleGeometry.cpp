/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "PointSampleTriangleGeometry.hpp"

#include "TupleTransfer.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"

#include <cassert>
#include <cstring>

using namespace complex;

// -----------------------------------------------------------------------------
PointSampleTriangleGeometry::PointSampleTriangleGeometry(DataStructure& dataStructure, PointSampleTriangleGeometryInputs* inputValues, const IFilter* filter,
                                                         const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_Inputs(inputValues)
, m_Filter(filter)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
PointSampleTriangleGeometry::~PointSampleTriangleGeometry() noexcept = default;

Result<> PointSampleTriangleGeometry::operator()()
{

  DataPath triangleGeometryDataPath = m_Inputs->pTriangleGeometry;
  TriangleGeom& triangle = m_DataStructure.getDataRefAs<TriangleGeom>(triangleGeometryDataPath);
  int64_t numTris = static_cast<int64_t>(triangle.getNumberOfFaces());

  VertexGeom& vertex = m_DataStructure.getDataRefAs<VertexGeom>(m_Inputs->pVertexGeometryPath);
  vertex.resizeVertexList(m_Inputs->pNumberOfSamples);

  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  std::mt19937_64 generator(seed);
  std::uniform_real_distribution<> distribution(0.0f, 1.0f);

  // Create the Triangle Ids vector and fill it from zero to size-1 incrementally
  std::vector<int64_t> triangleIds(numTris);
  std::iota(std::begin(triangleIds), std::end(triangleIds), 0);

  // Initialize the Triangle Weights with the Triangle Area Values
  Float64Array& faceAreas = m_DataStructure.getDataRefAs<Float64Array>(m_Inputs->pTriangleAreasArrayPath);
  std::vector<double> triangleWeights(numTris);
  for(size_t i = 0; i < triangleWeights.size(); i++)
  {
    triangleWeights[i] = faceAreas[i];
  }

  // VS2013 does not implement the iterator contructor for std::discrete_distribution<>, which
  // really is a massively idiotic oversight; hack the equivalent using the unary_op constructor
  std::discrete_distribution<size_t> triangle_distribution(triangleWeights.size(), -0.5, -0.5 + static_cast<double>(triangleWeights.size()),
                                                           [&triangleWeights](double index) { return triangleWeights[static_cast<size_t>(index)]; });

  int64_t progIncrement = m_Inputs->pNumberOfSamples / 100;
  int64_t prog = 1;
  int64_t progressInt = 0;
  int64_t counter = 0;

  Point3Df a(0.0F, 0.0F, 0.0F);
  Point3Df b(0.0F, 0.0F, 0.0F);
  Point3Df c(0.0F, 0.0F, 0.0F);

  // We get the pointer to the Array instead of a reference because it might not have been set because
  // the bool "use_mask" might have been false, but we do NOT want to try to get the array
  // 'on demand' in the loop. That is a BAD idea as is it really slow to do that. (10x slower).
  DataObject* maskArrayDataObject = m_DataStructure.getData(m_Inputs->pMaskArrayPath);
  BoolArray* maskArray = nullptr;
  if(nullptr != maskArrayDataObject && m_Inputs->pUseMask)
  {
    maskArray = m_DataStructure.getDataAs<BoolArray>(m_Inputs->pMaskArrayPath);
  }
  if(maskArray == nullptr && m_Inputs->pUseMask)
  {
    return MakeErrorResult(-502, "Use Mask is true but the MaskArray could not be extracted from the DataStructure. Please ensure the path is correct and that the selected DataArray is of type bool");
  }
  // Get a reference to the Vertex List
  AbstractGeometry::SharedVertexList* vertices = vertex.getVertices();

  // Create a vector of TupleTransferFunctions for each of the Triangle Face to Vertex Data Arrays
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_Inputs->pSelectedDataArrayPaths.size(); i++)
  {
    ::AddTupleTransferInstance(m_DataStructure, m_Inputs->pSelectedDataArrayPaths[i], m_Inputs->pCreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  for(int64_t curVertex = 0; curVertex < m_Inputs->pNumberOfSamples; curVertex++)
  {
    if(m_Filter->isCanceled())
    {
      return {};
    }

    int64_t randomTri = 0;

    if(m_Inputs->pUseMask)
    {
      while(true)
      {
        randomTri = static_cast<int64_t>(triangle_distribution(generator));
        if(randomTri >= numTris || randomTri < 0)
        {
          throw std::out_of_range(fmt::format("Generated Random Triangle Index '{}' was out of range. '0->{}'", randomTri, numTris));
        }
        if((*maskArray)[randomTri])
        {
          break;
        }
      }
    }
    else
    {
      randomTri = static_cast<int64_t>(triangle_distribution(generator));
      if(randomTri >= numTris || randomTri < 0)
      {
        throw std::out_of_range(fmt::format("Generated Random Triangle Index '{}' was out of range. '0->{}'", randomTri, numTris));
      }
    }

    triangle.getVertexCoordsForFace(randomTri, a, b, c);

    float r1 = static_cast<float>(distribution(generator));
    float r2 = static_cast<float>(distribution(generator));

    float prefactorA = 1.0f - sqrtf(r1);
    float prefactorB = sqrtf(r1) * (1 - r2);
    float prefactorC = sqrtf(r1) * r2;

    (*vertices)[curVertex * 3 + 0] = (prefactorA * a[0]) + (prefactorB * b[0]) + (prefactorC * c[0]);
    (*vertices)[curVertex * 3 + 1] = (prefactorA * a[1]) + (prefactorB * b[1]) + (prefactorC * c[1]);
    (*vertices)[curVertex * 3 + 2] = (prefactorA * a[2]) + (prefactorB * b[2]) + (prefactorC * c[2]);

    // Transfer the face data to the vertex data
    for(size_t dataVectorIndex = 0; dataVectorIndex < m_Inputs->pSelectedDataArrayPaths.size(); dataVectorIndex++)
    {
      tupleTransferFunctions[dataVectorIndex]->transfer(randomTri, curVertex);
    }

    if(counter > prog)
    {
      progressInt = static_cast<int64_t>((static_cast<float>(counter) / static_cast<float>(m_Inputs->pNumberOfSamples)) * 100.0f);
      std::string ss = fmt::format("Sampling Triangles || {}% Completed", progressInt);
      m_MessageHandler(IFilter::Message::Type::Info, ss);
      prog = prog + progIncrement;
    }
    counter++;
  }

  return {};
}
