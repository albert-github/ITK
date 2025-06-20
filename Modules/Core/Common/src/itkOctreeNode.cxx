/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/* Local Include Files */
#include "itkOctree.h"

namespace itk
{
/*
 * ====================================================================================
 * ====================================================================================
 * ====================================================================================
 */
OctreeNode::OctreeNode() = default;

OctreeNode::~OctreeNode() { this->RemoveChildren(); }

OctreeNode &
OctreeNode::GetChild(const LeafIdentifierEnum ChildID) const
{
  return *m_Branch->GetLeaf(ChildID);
}

OctreeNode &
OctreeNode::GetChild(const LeafIdentifierEnum ChildID)
{
  return *m_Branch->GetLeaf(ChildID);
}

long
OctreeNode::GetColor() const
{
  if (m_Parent != nullptr)
  {
    const long x = m_Branch - m_Parent->GetColorTable();

    //
    // you'll want to indicate that the branch
    // is a subtree and not in fact a color.
    if (x >= 0 && x < m_Parent->GetColorTableSize())
    {
      return x;
    }
  }
  return -1;
}

void
OctreeNode::SetColor(int color)
{
  if (m_Parent != nullptr)
  {
    this->RemoveChildren();
    m_Branch = const_cast<OctreeNodeBranch *>(m_Parent->GetColorTable()) + color;
  }
}

void
OctreeNode::SetBranch(OctreeNodeBranch * NewBranch)
{
  this->RemoveChildren();
  m_Branch = NewBranch;
}

/**
 * \brief Determines if the current node is colored
 * \return  bool true if this node is colored
 */
bool
OctreeNode::IsNodeColored() const
{
  if (m_Parent != nullptr)
  {
    const OctreeNodeBranch * colorTable = m_Parent->GetColorTable();
    const OctreeNodeBranch * first = &(colorTable[0]);
    const OctreeNodeBranch * last = &(colorTable[m_Parent->GetColorTableSize() - 1]);

    return this->m_Branch >= first && this->m_Branch <= last;
  }

  return false;
}

void
OctreeNode::RemoveChildren()
{
  if (!this->IsNodeColored())
  {
    delete m_Branch;
    if (m_Parent != nullptr)
    {
      m_Branch = &(const_cast<OctreeNodeBranch *>(m_Parent->GetColorTable())[0]);
    }
    else
    {
      m_Branch = nullptr;
    }
  }
}
} // namespace itk
