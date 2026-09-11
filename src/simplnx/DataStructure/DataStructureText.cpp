/**
 * @file DataStructureText.cpp
 * @brief Implements text hierarchy export for DataStructure.
 */

#include "DataStructure.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructureExportUtilities.hpp"

#include <ostream>
#include <ranges>
#include <string_view>
#include <vector>

namespace nx::core
{
namespace
{
constexpr std::string_view k_Delimiter = "|--";

/**
 * @brief Writes an object name with control characters escaped.
 * @param outputStream Stream that receives the escaped name.
 * @param name Unescaped object name.
 */
void writeEscapedTextName(std::ostream& outputStream, std::string_view name)
{
  for(const unsigned char character : name)
  {
    switch(character)
    {
    case '\\':
      outputStream << "\\\\";
      break;
    case '\n':
      outputStream << "\\n";
      break;
    case '\r':
      outputStream << "\\r";
      break;
    case '\t':
      outputStream << "\\t";
      break;
    case '\b':
      outputStream << "\\b";
      break;
    case '\f':
      outputStream << "\\f";
      break;
    default:
      outputStream << (character < 0x20 || character == 0x7F ? '?' : static_cast<char>(character));
      break;
    }
  }
}

/**
 * @brief Writes hierarchy indentation without allocating a depth-sized string.
 * @param outputStream Stream that receives the spaces.
 * @param depth Zero-based hierarchy depth.
 */
void writeIndent(std::ostream& outputStream, usize depth)
{
  constexpr std::string_view kSpaces = "                                                                ";
  usize remainingSpaces = depth * 2;
  while(remainingSpaces >= kSpaces.size())
  {
    outputStream << kSpaces;
    remainingSpaces -= kSpaces.size();
  }
  outputStream << kSpaces.substr(0, remainingSpaces);
}

/**
 * @class PendingTextNode
 * @brief Stores one object and its depth for iterative text export.
 */
class PendingTextNode
{
public:
  /**
   * @brief Creates a pending text node.
   * @param objectPtr Object to export.
   * @param depth Zero-based hierarchy depth.
   */
  PendingTextNode(const DataObject* objectPtr, usize depth)
  : m_ObjectPtr(objectPtr)
  , m_Depth(depth)
  {
  }

  /**
   * @brief Gets the object to export.
   * @return Non-owning pointer to the object.
   */
  [[nodiscard]] const DataObject* getObjectPtr() const
  {
    return m_ObjectPtr;
  }

  /**
   * @brief Gets the hierarchy depth.
   * @return Zero-based hierarchy depth.
   */
  [[nodiscard]] usize getDepth() const
  {
    return m_Depth;
  }

private:
  const DataObject* m_ObjectPtr = nullptr;
  usize m_Depth = 0;
};

} // namespace

void DataStructure::exportHierarchyAsText(std::ostream& outputStream) const
{
  const std::vector<const DataObject*> rootObjectPtrs = data_structure_export::GetSortedObjectPointers(m_RootGroup);
  std::vector<PendingTextNode> pendingNodes;
  pendingNodes.reserve(rootObjectPtrs.size());
  for(const DataObject* objectPtr : std::ranges::reverse_view(rootObjectPtrs))
  {
    pendingNodes.emplace_back(objectPtr, 0);
  }

  while(!pendingNodes.empty())
  {
    const PendingTextNode pendingNode = pendingNodes.back();
    pendingNodes.pop_back();

    const DataObject* objectPtr = pendingNode.getObjectPtr();
    writeIndent(outputStream, pendingNode.getDepth());
    outputStream << k_Delimiter;
    writeEscapedTextName(outputStream, objectPtr->getName());
    outputStream << '\n';

    const auto* groupPtr = dynamic_cast<const BaseGroup*>(objectPtr);
    if(groupPtr == nullptr)
    {
      continue;
    }

    const std::vector<const DataObject*> childObjectPtrs = data_structure_export::GetSortedObjectPointers(groupPtr->getDataMap());
    for(const DataObject* childPtr : std::ranges::reverse_view(childObjectPtrs))
    {
      pendingNodes.emplace_back(childPtr, pendingNode.getDepth() + 1);
    }
  }
}
} // namespace nx::core
