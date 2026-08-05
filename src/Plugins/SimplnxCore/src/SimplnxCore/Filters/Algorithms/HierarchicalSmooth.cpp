// Hierarchical Smoothing Algorithm
// Original algorithm by Siddharth Maddali (2016-2018)
// Ported to simplnx filter framework

#include "HierarchicalSmooth.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/Sparse>

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// ============================================================================
// Type aliases matching the original HierarchicalSmooth types
// ============================================================================
using trimesh = Eigen::Array<int, Eigen::Dynamic, 3>;
using meshnode = Eigen::Matrix<double, 3, Eigen::Dynamic>;
using facelabel = Eigen::Array<int, Eigen::Dynamic, 2>;
using nodetype = Eigen::Array<int, Eigen::Dynamic, 1>;
using is_smoothed = Eigen::Array<bool, Eigen::Dynamic, 1>;
using matindex = Eigen::Matrix<int, Eigen::Dynamic, 1>;
using EdgePair = std::pair<int, int>;
using EdgeList = std::vector<EdgePair>;
using SpMat = Eigen::SparseMatrix<double>;
using Triplet = Eigen::Triplet<double>;
using Smoother = Eigen::ConjugateGradient<SpMat, Eigen::Upper | Eigen::Lower>;

// Use std::map for the edge/boundary dictionaries to ensure deterministic iteration
// order across platforms and STL implementations (libc++ vs libstdc++). The algorithm
// is hierarchical and its boundary/free-boundary traversal order is result-affecting,
// so a hash-ordered container would produce non-reproducible cross-platform output.
template <typename T>
using OrderedEdgeDict = std::map<EdgePair, T>;

struct EdgeCount
{
  EdgePair origPair;
  int count;
  EdgeCount(int x, int y)
  : origPair(std::make_pair(x, y))
  , count(1)
  {
  }
};

// ============================================================================
// Slice helpers
// ============================================================================

// Slice rows from a dense matrix: out = mat(rowIdx, :)
Eigen::MatrixXd sliceDenseRows(const Eigen::MatrixXd& mat, const matindex& rowIdx)
{
  int numRows = static_cast<int>(rowIdx.size());
  Eigen::MatrixXd result(numRows, mat.cols());
  for(int i = 0; i < numRows; i++)
  {
    result.row(i) = mat.row(rowIdx(i));
  }
  return result;
}

// Merge rows from source into target at specified row indices
void mergeDenseRows(const Eigen::MatrixXd& source, Eigen::MatrixXd& target, const matindex& locations)
{
  for(int i = 0; i < static_cast<int>(source.rows()); i++)
  {
    target.row(locations(i)) = source.row(i);
  }
}

// Slice rows and columns from a sparse matrix: out = mat(rowIdx, colIdx)
SpMat sliceSparse(const SpMat& mat, const matindex& rowIdx, const matindex& colIdx)
{
  int numRows = static_cast<int>(rowIdx.size());
  int numCols = static_cast<int>(colIdx.size());
  SpMat result(numRows, numCols);

  std::unordered_map<int, int> rowMap;
  for(int i = 0; i < numRows; i++)
  {
    rowMap[rowIdx(i)] = i;
  }
  std::unordered_map<int, int> colMap;
  for(int i = 0; i < numCols; i++)
  {
    colMap[colIdx(i)] = i;
  }

  std::vector<Triplet> triplets;
  triplets.reserve(mat.nonZeros());
  for(int k = 0; k < mat.outerSize(); ++k)
  {
    auto colFound = colMap.find(k);
    if(colFound == colMap.end())
    {
      continue;
    }
    for(SpMat::InnerIterator it(mat, k); it; ++it)
    {
      auto rowFound = rowMap.find(static_cast<int>(it.row()));
      if(rowFound != rowMap.end())
      {
        triplets.push_back(Triplet(rowFound->second, colFound->second, it.value()));
      }
    }
  }
  result.setFromTriplets(triplets.begin(), triplets.end());
  result.makeCompressed();
  return result;
}

// Slice columns from meshnode (3xN matrix): out = mat(:, colIdx)
meshnode sliceMeshnodeCols(const meshnode& mat, const matindex& colIdx)
{
  int numCols = static_cast<int>(colIdx.size());
  meshnode result(3, numCols);
  for(int i = 0; i < numCols; i++)
  {
    result.col(i) = mat.col(colIdx(i));
  }
  return result;
}

// Slice rows from is_smoothed array: out = arr(rowIdx)
is_smoothed sliceIsSmoothed(const is_smoothed& arr, const matindex& idx)
{
  int n = static_cast<int>(idx.size());
  is_smoothed result(n);
  for(int i = 0; i < n; i++)
  {
    result(i) = arr(idx(i));
  }
  return result;
}

// ============================================================================
// Base utility functions (from HSmoothBase)
// ============================================================================

trimesh ismember(const trimesh& array1, const std::vector<int>& array2)
{
  std::unordered_map<int, int> dict;
  for(int i = 0; i < static_cast<int>(array2.size()); i++)
  {
    dict.insert({array2[i], i});
  }
  trimesh newTri = trimesh::Zero(array1.rows(), array1.cols());
  for(int col = 0; col < array1.cols(); col++)
  {
    for(int row = 0; row < array1.rows(); row++)
    {
      auto got = dict.find(array1(row, col));
      newTri(row, col) = got->second;
    }
  }
  return newTri;
}

matindex getindex(const std::vector<int>& fromThis)
{
  matindex idx(static_cast<int>(fromThis.size()));
  for(int i = 0; i < static_cast<int>(fromThis.size()); i++)
  {
    idx(i) = fromThis[i];
  }
  return idx;
}

matindex getindex(const std::vector<int>& fromThis, const matindex& inThis)
{
  std::unordered_map<int, int> dict;
  for(int i = 0; i < inThis.rows(); i++)
  {
    dict.insert({inThis(i), i});
  }
  std::vector<int> vtemp;
  for(int i = 0; i < static_cast<int>(fromThis.size()); i++)
  {
    auto got = dict.find(fromThis[i]);
    vtemp.push_back(got->second);
  }
  return getindex(vtemp);
}

matindex getcomplement(const matindex& nSet, int n)
{
  matindex nAll = -1 * matindex::Ones(n, 1);
  for(int i = 0; i < nSet.size(); i++)
  {
    nAll(nSet(i)) = nSet(i);
  }
  std::vector<int> nComplement;
  for(int i = 0; i < nAll.size(); i++)
  {
    if(nAll(i) < 0)
    {
      nComplement.push_back(i);
    }
  }
  return getindex(nComplement);
}

matindex matunion(const matindex& mat1, const matindex& mat2)
{
  std::vector<int> v;
  for(int i = 0; i < mat1.size(); i++)
  {
    v.push_back(mat1(i));
  }
  for(int i = 0; i < mat2.size(); i++)
  {
    v.push_back(mat2(i));
  }
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  return getindex(v);
}

void merge(const meshnode& source, meshnode& target, const matindex& locations)
{
  for(int i = 0; i < source.cols(); i++)
  {
    target.col(locations(i)) = source.col(i);
  }
}

// ============================================================================
// Triangulation class (from HSmoothTri)
// ============================================================================

class Triangulation
{
public:
  Triangulation() = default;

  explicit Triangulation(trimesh& inTri)
  {
    auto [edges, freeBnd] = getEdges(inTri);
    m_FreeBoundary = freeBnd;
    differentiateFaces();
  }

  std::tuple<EdgeList, EdgeList> freeBoundary() const
  {
    return std::make_tuple(m_FreeBoundary, m_FreeBoundarySegments);
  }

  std::tuple<SpMat, matindex> graphLaplacian() const
  {
    std::vector<Triplet> tripletList;
    tripletList.reserve(m_Unique.size() + 2 * m_Dict.size());
    for(auto it = m_Dict.begin(); it != m_Dict.end(); ++it)
    {
      int l = it->first.first;
      int m = it->first.second;
      tripletList.push_back(Triplet(l, m, -1.0));
      tripletList.push_back(Triplet(m, l, -1.0));
    }
    for(int i = 0; i < static_cast<int>(m_DiagCount.size()); i++)
    {
      tripletList.push_back(Triplet(i, i, m_DiagCount[i]));
    }

    SpMat gl(static_cast<int>(m_Unique.size()), static_cast<int>(m_Unique.size()));
    gl.setFromTriplets(tripletList.begin(), tripletList.end());
    gl.makeCompressed();

    // Copy unique to matindex
    std::vector<int> uniqueCopy = m_Unique;
    return std::make_tuple(gl, getindex(uniqueCopy));
  }

private:
  EdgeList m_FreeBoundary;
  EdgeList m_FreeBoundarySegments;
  std::vector<int> m_Unique;
  // Ordered (std::map) so freeBoundary iteration order is identical across platforms/STL
  // implementations. m_Dict order feeds the freeBoundary sequence -> chain-link/segment
  // assignment, so a hash-ordered map (libc++ vs libstdc++) caused cross-platform divergence.
  OrderedEdgeDict<EdgeCount> m_Dict;
  std::vector<double> m_DiagCount;

  void differentiateFaces()
  {
    if(m_FreeBoundary.empty())
    {
      return;
    }
    int start = m_FreeBoundary[0].first;
    std::vector<int> thisSec{0};
    int n = 1;
    while(n < static_cast<int>(m_FreeBoundary.size()))
    {
      if(m_FreeBoundary[n].second == start)
      {
        thisSec.push_back(n);
        m_FreeBoundarySegments.push_back(std::make_pair(thisSec[0], thisSec[1]));
        thisSec.clear();
      }
      else if(thisSec.empty())
      {
        start = m_FreeBoundary[n].first;
        thisSec.push_back(n);
      }
      n++;
    }
  }

  std::tuple<EdgeList, EdgeList> getEdges(const trimesh& inTri)
  {
    for(int i = 0; i < inTri.rows(); i++)
    {
      for(int j = 0; j < inTri.cols(); j++)
      {
        m_Unique.push_back(inTri(i, j));
      }
    }
    std::sort(m_Unique.begin(), m_Unique.end());
    m_Unique.erase(std::unique(m_Unique.begin(), m_Unique.end()), m_Unique.end());

    m_DiagCount = std::vector<double>(m_Unique.size(), 0.0);
    trimesh subTri = ismember(inTri, m_Unique);

    EdgeList edgeList;
    EdgeList freeBoundary;

    for(int i = 0; i < subTri.rows(); i++)
    {
      for(int j = 0; j < 3; j++)
      {
        int l = (j + 3) % 3;
        int m = (j + 4) % 3;
        int thisRow = subTri(i, l);
        int thisCol = subTri(i, m);
        EdgePair ep = std::make_pair(std::min(thisRow, thisCol), std::max(thisRow, thisCol));
        auto got = m_Dict.find(ep);
        if(got == m_Dict.end())
        {
          EdgeCount ec(m_Unique[thisRow], m_Unique[thisCol]);
          m_Dict.insert({ep, ec});
          m_DiagCount[thisRow] += 1.0;
          m_DiagCount[thisCol] += 1.0;
        }
        else
        {
          got->second.count++;
        }
      }
    }

    for(auto it = m_Dict.begin(); it != m_Dict.end(); ++it)
    {
      edgeList.push_back(it->second.origPair);
      if(it->second.count == 1)
      {
        freeBoundary.push_back(it->second.origPair);
      }
    }

    return std::make_tuple(edgeList, fastChainLinkSort(freeBoundary));
  }

  EdgeList fastChainLinkSort(const EdgeList& inList)
  {
    // Use std::map for deterministic iteration order across platforms
    std::map<int, std::vector<int>> windingDict;
    for(int i = 0; i < static_cast<int>(inList.size()); i++)
    {
      int ltemp = inList[i].first;
      int rtemp = inList[i].second;
      auto got = windingDict.find(ltemp);
      if(got == windingDict.end())
      {
        windingDict.insert({ltemp, std::vector<int>{rtemp}});
      }
      else
      {
        got->second.push_back(rtemp);
      }
    }

    EdgeList outList;
    auto it = windingDict.begin();
    while(!windingDict.empty())
    {
      int next = it->second.back();
      outList.push_back(std::make_pair(it->first, next));
      it->second.pop_back();
      if(it->second.empty())
      {
        windingDict.erase(it);
      }
      it = windingDict.find(next);
      if(it == windingDict.end())
      {
        it = windingDict.begin();
      }
    }
    return outList;
  }
};

// ============================================================================
// Core smoothing functions (from HSmoothMain)
// ============================================================================

SpMat laplacian2D(int n, const std::string& type = "serial")
{
  std::vector<Triplet> tripletList;
  tripletList.reserve(3 * n);
  for(int i = 0; i < n; i++)
  {
    tripletList.push_back(Triplet(i, i, -1.0));
    if(i != n - 1)
    {
      tripletList.push_back(Triplet(i, i + 1, 1.0));
    }
  }
  SpMat temp(n, n);
  temp.setFromTriplets(tripletList.begin(), tripletList.end());
  SpMat lap = SpMat(temp.transpose()) * temp;
  if(type == "serial")
  {
    lap.coeffRef(n - 1, n - 1) = 1.0;
  }
  else if(type == "cyclic")
  {
    lap.coeffRef(0, 0) = 2.0;
    lap.coeffRef(0, n - 1) = -1.0;
    lap.coeffRef(n - 1, 0) = -1.0;
  }
  lap.makeCompressed();
  return lap;
}

std::tuple<SpMat, SpMat> analyzeLaplacian(const SpMat& gl)
{
  SpMat d(gl.rows(), gl.cols());
  SpMat a(gl.rows(), gl.cols());
  std::vector<Triplet> dt;
  std::vector<Triplet> at;

  for(int k = 0; k < gl.outerSize(); ++k)
  {
    for(SpMat::InnerIterator it(gl, k); it; ++it)
    {
      if(it.value() < -0.5)
      {
        at.push_back(Triplet(static_cast<int>(it.row()), static_cast<int>(it.col()), it.value()));
      }
      else if(it.value() > 0.5)
      {
        dt.push_back(Triplet(static_cast<int>(it.row()), static_cast<int>(it.col()), it.value()));
      }
    }
  }
  d.setFromTriplets(dt.begin(), dt.end());
  a.setFromTriplets(at.begin(), at.end());
  d.makeCompressed();
  a.makeCompressed();
  return std::make_tuple(d, a);
}

std::tuple<SpMat, Eigen::MatrixXd> getDirichletBVP(const SpMat& gl, const Eigen::MatrixXd& yIn, const matindex& nFixed, const matindex& nMobile)
{
  matindex nAll = matunion(nFixed, nMobile);

  SpMat glRed = sliceSparse(gl, nMobile, nMobile);
  SpMat sm1 = sliceSparse(gl, nAll, nFixed);
  Eigen::MatrixXd sm2 = sliceDenseRows(yIn, nFixed);
  Eigen::MatrixXd sm3 = sm1 * sm2;
  Eigen::MatrixXd fConst = sliceDenseRows(sm3, nMobile);

  return std::make_tuple(glRed, fConst);
}

double getObjFn(Smoother& smth, double feps, const SpMat& fSmallEye, const SpMat& ltl, const Eigen::MatrixXd& ltk, const matindex& nMobile, const Eigen::MatrixXd& yMobile, const SpMat& d,
                const Eigen::MatrixXd& ayIn, Eigen::MatrixXd& yOut)
{
  SpMat bigA = (1.0 - feps) * fSmallEye + feps * ltl;
  Eigen::MatrixXd b = (1.0 - feps) * yMobile - feps * ltk;

  smth.compute(bigA);
  Eigen::MatrixXd ySmooth = smth.solve(b);

  mergeDenseRows(ySmooth, yOut, nMobile);

  Eigen::ArrayXXd yDeltaD = (d * yOut + ayIn).array();
  return (yDeltaD * yDeltaD).sum();
}

meshnode smooth(const meshnode& nodesIn, const matindex& nFixed, SpMat& gl, double fThresh = 0.001, int nIter = 53)
{
  matindex nMobile = getcomplement(nFixed, static_cast<int>(gl.cols()));
  if(nMobile.size() == 0)
  {
    return nodesIn;
  }

  Eigen::MatrixXd data = nodesIn.transpose(); // Dense Nx3

  auto [glRed, fConst] = getDirichletBVP(gl, data, nFixed, nMobile);
  auto [d, a] = analyzeLaplacian(gl);

  Eigen::MatrixXd ayIn = a * data;
  SpMat fSmallEye(nMobile.size(), nMobile.size());
  fSmallEye.setIdentity();

  Eigen::MatrixXd yMobile = sliceDenseRows(data, nMobile);

  SpMat ltl = SpMat(glRed.transpose() * glRed);
  Eigen::MatrixXd ltk = glRed.transpose() * fConst;
  Eigen::MatrixXd yOut = data;

  Smoother smth;

  double fEps = 0.5;
  double fStep = fEps / 2.0;
  int nCount = 1;

  double fobj1 = getObjFn(smth, fEps, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
  double fobj2 = getObjFn(smth, fEps + fThresh, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
  double fslope = (fobj2 - fobj1) / fThresh;

  // Adaptive bisection over the smoothing parameter fEps, matching the reference
  // MATLAB/Python implementation (Smooth.m / HierarchicalSmooth.py):
  //   - iterate while the objective slope is still steep (|fslope| > fThresh)
  //   - step fEps in the direction given by the sign of the slope
  //   - halve the step each iteration (true bisection)
  while(std::fabs(fslope) > fThresh && nCount < nIter)
  {
    if(fslope > 0.0)
    {
      fEps -= fStep;
    }
    else
    {
      fEps += fStep;
    }

    fStep /= 2.0;
    fobj1 = getObjFn(smth, fEps, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
    fobj2 = getObjFn(smth, fEps + fThresh, fSmallEye, ltl, ltk, nMobile, yMobile, d, ayIn, yOut);
    fslope = (fobj2 - fobj1) / fThresh;
    nCount++;
  }

  return yOut.transpose(); // Nx3 -> 3xN
}

meshnode smoothWithType(const meshnode& nodesIn, const std::string& type = "serial", double fThresh = 0.001, int nIter = 53)
{
  SpMat lap = laplacian2D(static_cast<int>(nodesIn.cols()), type);
  std::vector<int> vidx;
  if(type == "serial")
  {
    vidx = std::vector<int>{0, static_cast<int>(lap.cols() - 1)};
  }
  matindex nFixed = getindex(vidx);
  return smooth(nodesIn, nFixed, lap, fThresh, nIter);
}

// ============================================================================
// Volume Solver - the main algorithm orchestrator
// ============================================================================

struct VolumeSolverData
{
  is_smoothed status;
  trimesh mesh;
  meshnode node;
  meshnode nodeSmooth;
  facelabel label;
  nodetype type;
  int maxIterations;
  double error;
  double errorThreshold;
  OrderedEdgeDict<std::vector<int>> boundaryDict;
};

void initVolumeSolver(VolumeSolverData& vs, trimesh&& volumeMesh, meshnode&& surfaceNodes, facelabel&& fLabels, nodetype&& nodeType, int nIterations, double errorThreshold)
{
  vs.mesh = std::move(volumeMesh);
  vs.node = std::move(surfaceNodes);
  vs.label = std::move(fLabels);
  vs.type = std::move(nodeType);
  vs.maxIterations = nIterations;
  vs.errorThreshold = errorThreshold;

  vs.status = is_smoothed(vs.type.size());
  for(int i = 0; i < vs.type.size(); i++)
  {
    vs.status(i) = (vs.type(i) % 10 == 4); // quad junction points considered already smoothed
  }

  vs.nodeSmooth = vs.node;

  // Compute minimum edge length for error threshold
  vs.error = (vs.node.col(vs.mesh(0, 0)).array() - vs.node.col(vs.mesh(0, 1)).array()).matrix().norm();
  vs.error = std::min(vs.error, (vs.node.col(vs.mesh(0, 1)).array() - vs.node.col(vs.mesh(0, 2)).array()).matrix().norm());
  vs.error = std::sqrt(3.0 * vs.error * vs.error);

  // Build boundary dictionary and ensure consistent mesh handedness
  for(int i = 0; i < vs.label.rows(); i++)
  {
    int thisMin = std::min(vs.label(i, 0), vs.label(i, 1));
    int thisMax = std::max(vs.label(i, 0), vs.label(i, 1));
    if(vs.label(i, 0) == thisMin)
    {
      // Flip to ensure consistent handedness
      int temp = vs.mesh(i, 0);
      vs.mesh(i, 0) = vs.mesh(i, 1);
      vs.mesh(i, 1) = temp;
    }
    EdgePair thisPair = std::make_pair(thisMin, thisMax);
    auto got = vs.boundaryDict.find(thisPair);
    if(got == vs.boundaryDict.end())
    {
      std::vector<int> v;
      v.push_back(i);
      vs.boundaryDict.insert({thisPair, v});
    }
    else
    {
      got->second.push_back(i);
    }
  }
}

trimesh sliceMesh(const VolumeSolverData& vs, const std::vector<int>& fromThesePatches)
{
  matindex patchIdx = getindex(fromThesePatches);
  int numPatches = static_cast<int>(patchIdx.size());
  trimesh triSub(numPatches, 3);
  for(int i = 0; i < numPatches; i++)
  {
    triSub(i, 0) = vs.mesh(patchIdx(i), 0);
    triSub(i, 1) = vs.mesh(patchIdx(i), 1);
    triSub(i, 2) = vs.mesh(patchIdx(i), 2);
  }
  return triSub;
}

void markSectionAsComplete(VolumeSolverData& vs, const matindex& idx)
{
  for(int i = 0; i < idx.size(); i++)
  {
    vs.status(idx(i)) = true;
  }
}

Result<> runHierarchicalSmooth(VolumeSolverData& vs, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  int boundaryCount = 1;
  int totalBoundaries = static_cast<int>(vs.boundaryDict.size());

  MessageHelper messageHelper(messageHandler, std::chrono::milliseconds(1000));
  auto throttledMessenger = messageHelper.createThrottledMessenger(std::chrono::milliseconds(1000));

  for(auto it = vs.boundaryDict.begin(); it != vs.boundaryDict.end(); ++it)
  {
    if(shouldCancel)
    {
      return {};
    }

    throttledMessenger.sendThrottledMessage([boundaryCount, totalBoundaries]() { return fmt::format("Processing boundary {} of {}", boundaryCount, totalBoundaries); });

    trimesh triSub = sliceMesh(vs, it->second);
    Triangulation tri(triSub);

    auto [gl, nUniq] = tri.graphLaplacian();
    auto [fb, fbSec] = tri.freeBoundary();

    // Smooth each free boundary segment first
    for(int i = 0; i < static_cast<int>(fbSec.size()); i++)
    {
      if(shouldCancel)
      {
        return {};
      }

      int start = fbSec[i].first;
      int stop = fbSec[i].second;
      int count;
      for(count = start; count <= stop; count++)
      {
        if(vs.type(fb[count].first) % 10 == 4)
        {
          break;
        }
      }

      std::vector<int> vtemp;
      if(count > stop)
      {
        // No quad junctions in this free boundary - smooth without constraints
        for(count = start; count <= stop; count++)
        {
          vtemp.push_back(fb[count].first);
        }
        matindex thisFreeBoundaryIdx = getindex(vtemp);
        meshnode thisFreeBoundary = sliceMeshnodeCols(vs.nodeSmooth, thisFreeBoundaryIdx);
        meshnode thisFreeBoundarySmooth = smoothWithType(thisFreeBoundary, "cyclic");
        merge(thisFreeBoundarySmooth, vs.nodeSmooth, thisFreeBoundaryIdx);
        markSectionAsComplete(vs, thisFreeBoundaryIdx);
      }
      else
      {
        // Triple line sections found - smooth separately
        vtemp.push_back(fb[count].first);
        int thisSize = 1 + (stop - start);
        for(int j = count + 1; j < 1 + count + thisSize; j++)
        {
          int effectiveJ = j % thisSize;
          vtemp.push_back(fb[effectiveJ].first);
          if(vs.type(fb[effectiveJ].first) % 10 == 4)
          {
            // Reached terminal quad point
            matindex thisTripleLineIndex = getindex(vtemp);
            is_smoothed thisStatus = sliceIsSmoothed(vs.status, thisTripleLineIndex);
            if(!thisStatus.all())
            {
              meshnode thisTripleLine = sliceMeshnodeCols(vs.nodeSmooth, thisTripleLineIndex);
              meshnode thisTripleLineSmoothed = smoothWithType(thisTripleLine);
              merge(thisTripleLineSmoothed, vs.nodeSmooth, thisTripleLineIndex);
              markSectionAsComplete(vs, thisTripleLineIndex);
            }
            vtemp.clear();
            vtemp.push_back(fb[effectiveJ].first);
          }
        }
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    // Smooth entire boundary subject to fixed triple points
    meshnode boundaryNode = sliceMeshnodeCols(vs.nodeSmooth, nUniq);
    std::vector<int> fixed;
    for(int i = 0; i < static_cast<int>(fb.size()); i++)
    {
      fixed.push_back(fb[i].first);
    }
    matindex nFixed = getindex(fixed, nUniq);
    meshnode boundaryNodeSmooth = smooth(boundaryNode, nFixed, gl);
    merge(boundaryNodeSmooth, vs.nodeSmooth, nUniq);
    markSectionAsComplete(vs, nUniq);
    boundaryCount++;
  }

  // Validate results - reject nodes with excessive displacement
  Eigen::ArrayXXd fTemp = vs.nodeSmooth.array() - vs.node.array();
  Eigen::ArrayXXd fNorm = (fTemp * fTemp).colwise().sum().sqrt() / vs.error;
  int rejectedCount = 0;
  if((fNorm > vs.errorThreshold).any())
  {
    for(int i = 0; i < vs.status.rows(); i++)
    {
      if(fNorm(0, i) > vs.errorThreshold)
      {
        vs.status(i, 0) = false;
        vs.nodeSmooth.col(i) = vs.node.col(i); // reset to old values
        rejectedCount++;
      }
    }
  }

  if(rejectedCount > 0)
  {
    messageHandler.sendWarningMessage(fmt::format("{} of {} nodes not smoothed due to excessive displacement", rejectedCount, vs.nodeSmooth.cols()));
  }
  else
  {
    messageHandler.sendInfoMessage("All nodes smoothed successfully.");
  }

  return {};
}

} // anonymous namespace

// =============================================================================
// HierarchicalSmooth class implementation
// =============================================================================

HierarchicalSmooth::HierarchicalSmooth(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, HierarchicalSmoothInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

HierarchicalSmooth::~HierarchicalSmooth() noexcept = default;

Result<> HierarchicalSmooth::operator()()
{
  // Get the triangle geometry
  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangleGeometryDataPath);

  // Get data arrays
  const auto& nodeTypeArray = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->nodeTypeArrayPath);
  const auto& faceLabelsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->faceLabelsArrayPath);

  // Get vertex and face data
  auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();
  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const auto& nodeTypeRef = nodeTypeArray.getDataStoreRef();
  const auto& faceLabelsRef = faceLabelsArray.getDataStoreRef();

  IGeometry::MeshIndexType numVertices = triangleGeom.getNumberOfVertices();
  IGeometry::MeshIndexType numFaces = triangleGeom.getNumberOfFaces();

  m_MessageHandler.sendInfoMessage(fmt::format("Hierarchical Smooth: {} vertices, {} faces", numVertices, numFaces));

  // Convert simplnx data to Eigen types
  // meshnode is 3 x N (each column is a vertex)
  meshnode vertices(3, numVertices);
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    vertices(0, i) = static_cast<double>(verticesRef[3 * i + 0]);
    vertices(1, i) = static_cast<double>(verticesRef[3 * i + 1]);
    vertices(2, i) = static_cast<double>(verticesRef[3 * i + 2]);
  }

  // trimesh is N x 3 (each row is a triangle with 3 vertex indices)
  trimesh faces(numFaces, 3);
  for(IGeometry::MeshIndexType i = 0; i < numFaces; i++)
  {
    faces(i, 0) = static_cast<int>(facesRef[3 * i + 0]);
    faces(i, 1) = static_cast<int>(facesRef[3 * i + 1]);
    faces(i, 2) = static_cast<int>(facesRef[3 * i + 2]);
  }

  // facelabel is N x 2
  facelabel fLabels(numFaces, 2);
  for(IGeometry::MeshIndexType i = 0; i < numFaces; i++)
  {
    fLabels(i, 0) = static_cast<int>(faceLabelsRef[2 * i + 0]);
    fLabels(i, 1) = static_cast<int>(faceLabelsRef[2 * i + 1]);
  }

  // nodetype is N x 1
  nodetype nType(numVertices);
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    nType(i) = static_cast<int>(nodeTypeRef[i]);
  }

  // Initialize solver and run (move Eigen matrices to avoid copies)
  VolumeSolverData vs;
  initVolumeSolver(vs, std::move(faces), std::move(vertices), std::move(fLabels), std::move(nType), m_InputValues->maxIterations, m_InputValues->errorThreshold);

  Result<> result = runHierarchicalSmooth(vs, m_ShouldCancel, m_MessageHandler);
  if(result.invalid())
  {
    return result;
  }

  // Write smoothed vertices back to the triangle geometry
  m_MessageHandler.sendInfoMessage("Writing smoothed vertices back to geometry...");
  for(IGeometry::MeshIndexType i = 0; i < numVertices; i++)
  {
    verticesRef[3 * i + 0] = static_cast<float32>(vs.nodeSmooth(0, i));
    verticesRef[3 * i + 1] = static_cast<float32>(vs.nodeSmooth(1, i));
    verticesRef[3 * i + 2] = static_cast<float32>(vs.nodeSmooth(2, i));
  }

  return {};
}
