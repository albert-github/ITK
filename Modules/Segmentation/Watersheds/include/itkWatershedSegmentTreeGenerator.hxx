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
#ifndef itkWatershedSegmentTreeGenerator_hxx
#define itkWatershedSegmentTreeGenerator_hxx

#include <stack>
#include "itkOneWayEquivalencyTable.h"

namespace itk::watershed
{
template <typename TScalar>
SegmentTreeGenerator<TScalar>::SegmentTreeGenerator()
{
  const typename SegmentTreeType::Pointer st = static_cast<SegmentTreeType *>(this->MakeOutput(0).GetPointer());
  this->SetNumberOfRequiredOutputs(1);
  this->ProcessObject::SetNthOutput(0, st.GetPointer());
  m_MergedSegmentsTable = OneWayEquivalencyTableType::New();
}

template <typename TScalar>
auto
SegmentTreeGenerator<TScalar>::MakeOutput(DataObjectPointerArraySizeType itkNotUsed(idx)) -> DataObjectPointer
{
  return SegmentTreeType::New().GetPointer();
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::SetFloodLevel(double val)
{
  // Clamp level between 0.0 and 1.0
  if (val > 1.0)
  {
    m_FloodLevel = 1.0;
  }
  else if (val < 0.0)
  {
    m_FloodLevel = 0.0;
  }
  else
  {
    m_FloodLevel = val;
  }

  // Has this flood level increased over a level that has already been
  // calculated? If not, then we don't set the modified flag.  A decrease in
  // the flood level does not require re-execution of the filter.
  if (m_HighestCalculatedFloodLevel < m_FloodLevel)
  {
    this->Modified();
  }
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::GenerateData()
{
  // Reset persistent ivars.
  m_MergedSegmentsTable->Clear();
  this->GetOutputSegmentTree()->Clear();

  const typename SegmentTableType::Pointer input = this->GetInputSegmentTable();
  auto                                     mergeList = SegmentTreeType::New();
  auto                                     seg = SegmentTableType::New();
  if (m_ConsumeInput) // do not copy input
  {
    input->Modified();
    input->SortEdgeLists();

    if (m_Merge)
    {
      this->MergeEquivalencies();
    }

    this->CompileMergeList(input, mergeList);
    this->ExtractMergeHierarchy(input, mergeList);
  }
  else
  {
    seg->Copy(*input); // copy the input
    seg->SortEdgeLists();
    if (m_Merge)
    {
      this->MergeEquivalencies();
    }
    this->CompileMergeList(seg, mergeList);
    this->ExtractMergeHierarchy(seg, mergeList);
  }
  this->UpdateProgress(1.0);

  // Keep track of the highest flood level we calculated
  if (m_FloodLevel > m_HighestCalculatedFloodLevel)
  {
    m_HighestCalculatedFloodLevel = m_FloodLevel;
  }
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::MergeEquivalencies()
{
  const typename SegmentTableType::Pointer segTable = this->GetInputSegmentTable();
  auto threshold = static_cast<ScalarType>(m_FloodLevel * segTable->GetMaximumDepth());
  const typename EquivalencyTableType::Pointer eqTable = this->GetInputEquivalencyTable();
  eqTable->Flatten();
  IdentifierType counter = 0;

  segTable->PruneEdgeLists(threshold);

  for (auto it = eqTable->Begin(); it != eqTable->End(); ++it)
  {
    MergeSegments(segTable, m_MergedSegmentsTable, it->first, it->second); // Merge first INTO second.
    // deletes first
    if ((counter % 10000) == 0)
    {
      segTable->PruneEdgeLists(threshold);
      m_MergedSegmentsTable->Flatten();
      counter = 0;
    }

    ++counter;
  }
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::CompileMergeList(SegmentTableTypePointer segments, SegmentTreeTypePointer mergeList)
{
  // Region A will flood Region B (B will merge with A) at a flood level L
  // when all of the following conditions are true:
  // 1) Depth of B < L
  // 2) A is across the lowest edge of B
  auto threshold = static_cast<ScalarType>(m_FloodLevel * segments->GetMaximumDepth());
  m_MergedSegmentsTable->Flatten();

  segments->PruneEdgeLists(threshold);

  for (auto segment_ptr = segments->Begin(); segment_ptr != segments->End(); ++segment_ptr)
  {
    const IdentifierType labelFROM = segment_ptr->first;

    // Must take into account any equivalencies that have already been
    // recorded.
    if (segment_ptr->second.edge_list.empty())
    {
      // This is to defend against the referencing below.  This was causing an assert error.
      itkGenericExceptionMacro("CompileMergeList:: An unexpected and fatal error has occurred.");
    }
    IdentifierType labelTO = m_MergedSegmentsTable->RecursiveLookup(segment_ptr->second.edge_list.front().label);
    while (labelTO == labelFROM) // Pop off any bogus merges with ourself
    {                            // that may have been left in this list.
      segment_ptr->second.edge_list.pop_front();
      labelTO = m_MergedSegmentsTable->RecursiveLookup(segment_ptr->second.edge_list.front().label);
    }

    // Add this merge to our list if its saliency is below
    // the threshold.
    typename SegmentTreeType::merge_t tempMerge;
    tempMerge.from = labelFROM;
    tempMerge.to = labelTO;
    tempMerge.saliency = segment_ptr->second.edge_list.front().height - segment_ptr->second.min;
    if (tempMerge.saliency < threshold)
    {
      mergeList->PushBack(tempMerge);
    }
  }

  // Heapsort the list
  using MergeComparison = typename SegmentTreeType::merge_comp;
  std::make_heap(mergeList->Begin(), mergeList->End(), MergeComparison());
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::ExtractMergeHierarchy(SegmentTableTypePointer segments, SegmentTreeTypePointer heap)
{
  const typename SegmentTreeType::Pointer list = this->GetOutputSegmentTree();

  // Merges segments up to a specified flood level according to the information
  // in the heap of merges.  As two segments are merged, calculates a new
  // possible merges and pushes it onto the heap.
  auto threshold = static_cast<ScalarType>(m_FloodLevel * segments->GetMaximumDepth());

  using MergeComparison = typename SegmentTreeType::merge_comp;

  if (heap->Empty())
  {
    return;
  }
  auto initHeapSize = static_cast<double>(heap->Size());

  unsigned int                        counter = 0;
  typename SegmentTreeType::ValueType topMerge = heap->Front();

  while ((!heap->Empty()) && (topMerge.saliency <= threshold))
  {
    ++counter;            // Every so often we should eliminate
    if (counter == 10000) // all the recursion in our records
    {                     // of which segments have merged.
      counter = 0;
      segments->PruneEdgeLists(threshold); // also we want to
      // keep the edge list size under control
    }
    if ((counter % 10000) == 0)
    {
      m_MergedSegmentsTable->Flatten();
    }

    if ((counter % 1000) == 0)
    {
      this->UpdateProgress(1.0 - ((static_cast<double>(heap->Size())) / initHeapSize));
    }

    std::pop_heap(heap->Begin(), heap->End(), MergeComparison());
    heap->PopBack(); // Popping the heap moves the top element to the end
                     // of the container structure, so we delete that here.

    // Recursively find the segments we are about to merge
    // (the labels identified here may have merged already)
    const IdentifierType fromSegLabel = m_MergedSegmentsTable->RecursiveLookup(topMerge.from);
    const IdentifierType toSegLabel = m_MergedSegmentsTable->RecursiveLookup(topMerge.to);

    // If the two segments do not resolve to the same segment and the
    // "TO" segment has never been merged, then then merge them.
    // Otherwise, ignore this particular entry.
    if (fromSegLabel == topMerge.from && fromSegLabel != toSegLabel)
    {
      typename SegmentTableType::DataType * toSeg = segments->Lookup(toSegLabel);

      topMerge.from = fromSegLabel;
      topMerge.to = toSegLabel;
      list->PushBack(topMerge); // Record this merge for posterity

      // Merge the segments
      Self::MergeSegments(segments, m_MergedSegmentsTable, fromSegLabel, toSegLabel);

      // Now check for new possible merges in A.
      // All we have to do is look at the front of the
      // ordered list.

      // Recursively look up the label to which we might
      // be merging to.
      if (!toSeg->edge_list.empty())
      {
        typename SegmentTreeType::ValueType tempMerge;
        tempMerge.from = toSegLabel; // The new, composite segment
        tempMerge.to = m_MergedSegmentsTable->RecursiveLookup(toSeg->edge_list.front().label);
        while (tempMerge.to == tempMerge.from)
        { // We don't want to merge to ourself.
          toSeg->edge_list.pop_front();
          tempMerge.to = m_MergedSegmentsTable->RecursiveLookup(toSeg->edge_list.front().label);
        }
        tempMerge.saliency = (toSeg->edge_list.front().height) - toSeg->min;

        heap->PushBack(tempMerge);
        std::push_heap(heap->Begin(), heap->End(), MergeComparison());
      }
    }
    if (!heap->Empty())
    {
      topMerge = heap->Front();
    }
  }
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::PruneMergeSegments(SegmentTableTypePointer           segments,
                                                  OneWayEquivalencyTableTypePointer eqT,
                                                  const IdentifierType              FROM,
                                                  const IdentifierType              TO,
                                                  ScalarType)
{
  // Lookup both entries.
  typename SegmentTableType::segment_t * from_seg = segments->Lookup(FROM);
  typename SegmentTableType::segment_t * to_seg = segments->Lookup(TO);

  if (from_seg == 0 || to_seg == 0)
  {
    itkGenericExceptionMacro("PruneMergeSegments:: An unexpected and fatal error has occurred.");
  }

  // Compare the minimum values.
  if ((from_seg->min) < (to_seg->min))
  {
    (to_seg->min) = (from_seg->min);
  }

  // Add all the "FROM" edges to the "TO" edges.

  // We MUST eliminate redundant edges to prevent the
  // segment table from growing to unmanageable sizes.
  // Since the lists are sorted, the lowest height
  // value will be added first, so any redundant edges
  // can simply be eliminated.

  // References in adjacent edge tables are not replaced,
  // but rather will be resolved later through the one-way
  // equivalency table.

  HashMapType seen_table;

  typename SegmentTableType::edge_list_t::iterator edgeTOi = to_seg->edge_list.begin();
  typename SegmentTableType::edge_list_t::iterator edgeFROMi = from_seg->edge_list.begin();
  while (edgeTOi != to_seg->edge_list.end() && edgeFROMi != from_seg->edge_list.end())
  {
    // Recursively resolve the labels we are seeing
    IdentifierType labelTO = eqT->RecursiveLookup(edgeTOi->label);
    IdentifierType labelFROM = eqT->RecursiveLookup(edgeFROMi->label);

    // Ignore any labels already in this list and
    // any pointers back to ourself.
    // This step is necessary to keep the edge lists from
    // growing exponentially in size as segments merge.
    if (seen_table.find(labelTO) != seen_table.end() || labelTO == FROM)
    {
      typename SegmentTableType::edge_list_t::iterator edgeTEMPi = edgeTOi;
      ++edgeTEMPi;
      to_seg->edge_list.erase(edgeTOi);
      edgeTOi = edgeTEMPi;
      continue;
    }
    if (seen_table.find(labelFROM) != seen_table.end() || labelFROM == TO)
    {
      ++edgeFROMi;
      continue;
    }

    // Fix any changed labels since we have the chance
    if (labelTO != edgeTOi->label)
    {
      edgeTOi->label = labelTO;
    }
    if (labelFROM != edgeFROMi->label)
    {
      edgeFROMi->label = labelFROM;
    }

    // Which edge is next in the list?
    if (edgeFROMi->height < edgeTOi->height)
    {
      to_seg->edge_list.insert(edgeTOi, *edgeFROMi);
      seen_table.insert(HashMapType::value_type(labelFROM, true));
      ++edgeFROMi;
    }
    else
    {
      seen_table.insert(HashMapType::value_type(labelTO, true));
      ++edgeTOi;
    }
  }

  // Process tail of the FROM list.
  while (edgeFROMi != from_seg->edge_list.end())
  {
    IdentifierType labelFROM = eqT->RecursiveLookup(edgeFROMi->label);
    if (seen_table.find(labelFROM) != seen_table.end() || labelFROM == TO)
    {
      ++edgeFROMi;
    }
    else
    {
      if (labelFROM != edgeFROMi->label)
      {
        edgeFROMi->label = labelFROM;
      }
      to_seg->edge_list.push_back(*edgeFROMi);
      seen_table.insert(HashMapType::value_type(labelFROM, true));
      ++edgeFROMi;
    }
  }

  // Process tail of the TO list.
  while (edgeTOi != to_seg->edge_list.end())
  {
    IdentifierType labelTO = eqT->RecursiveLookup(edgeTOi->label);
    if (seen_table.find(labelTO) != seen_table.end() || labelTO == FROM)
    {
      typename SegmentTableType::edge_list_t::iterator edgeTEMPi = edgeTOi;
      ++edgeTEMPi;
      to_seg->edge_list.erase(edgeTOi);
      edgeTOi = edgeTEMPi;
    }
    else
    {
      if (labelTO != edgeTOi->label)
      {
        edgeTOi->label = labelTO;
      }
      seen_table.insert(HashMapType::value_type(labelTO, true));
      ++edgeTOi;
    }
  }

  // Now destroy the segment that was merged.
  segments->Erase(FROM);

  // Record this equivalency
  eqT->Add(FROM, TO);
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::MergeSegments(SegmentTableTypePointer           segments,
                                             OneWayEquivalencyTableTypePointer eqT,
                                             const IdentifierType              FROM,
                                             const IdentifierType              TO)
{
  HashMapType seen_table;

  // Lookup both entries.
  typename SegmentTableType::segment_t * from_seg = segments->Lookup(FROM);
  typename SegmentTableType::segment_t * to_seg = segments->Lookup(TO);

  if (from_seg == nullptr || to_seg == nullptr)
  {
    itkGenericExceptionMacro("itk::watershed::SegmentTreeGenerator::MergeSegments:: An unexpected and fatal error "
                             "has occurred. This is probably the result of overthresholding of the input image.");
  }

  // Compare the minimum values.
  if ((from_seg->min) < (to_seg->min))
  {
    (to_seg->min) = (from_seg->min);
  }

  // Add all the "FROM" edges to the "TO" edges.

  // We MUST eliminate redundant edges to prevent the
  // segment table from growing to unmanageable sizes.
  // Since the lists are sorted, the lowest height
  // value will be added first, so any redundant edges
  // can simply be eliminated.

  // References in adjacent edge tables are not replaced,
  // but rather will be resolved later through the one-way
  // equivalency table.

  auto edgeTOi = to_seg->edge_list.begin();
  auto edgeFROMi = from_seg->edge_list.begin();
  while (edgeTOi != to_seg->edge_list.end() && edgeFROMi != from_seg->edge_list.end())
  {
    // Recursively resolve the labels we are seeing
    const IdentifierType labelTO = eqT->RecursiveLookup(edgeTOi->label);
    const IdentifierType labelFROM = eqT->RecursiveLookup(edgeFROMi->label);

    // Ignore any labels already in this list and
    // any pointers back to ourself.
    // This step is necessary to keep the edge lists from
    // growing exponentially in size as segments merge.
    if (seen_table.find(labelTO) != seen_table.end() || labelTO == FROM)
    {
      auto edgeTEMPi = edgeTOi;
      ++edgeTEMPi;
      to_seg->edge_list.erase(edgeTOi);
      edgeTOi = edgeTEMPi;
      continue;
    }
    if (seen_table.find(labelFROM) != seen_table.end() || labelFROM == TO)
    {
      ++edgeFROMi;
      continue;
    }

    // Fix any changed labels since we have the chance
    if (labelTO != edgeTOi->label)
    {
      edgeTOi->label = labelTO;
    }
    if (labelFROM != edgeFROMi->label)
    {
      edgeFROMi->label = labelFROM;
    }

    // Which edge is next in the list?
    if (edgeFROMi->height < edgeTOi->height)
    {
      to_seg->edge_list.insert(edgeTOi, *edgeFROMi);
      seen_table.insert(HashMapType::value_type(labelFROM, true));
      ++edgeFROMi;
    }
    else
    {
      seen_table.insert(HashMapType::value_type(labelTO, true));
      ++edgeTOi;
    }
  }

  // Process tail of the FROM list.
  while (edgeFROMi != from_seg->edge_list.end())
  {
    const IdentifierType labelFROM = eqT->RecursiveLookup(edgeFROMi->label);
    if (seen_table.find(labelFROM) != seen_table.end() || labelFROM == TO)
    {
      ++edgeFROMi;
    }
    else
    {
      if (labelFROM != edgeFROMi->label)
      {
        edgeFROMi->label = labelFROM;
      }
      to_seg->edge_list.push_back(*edgeFROMi);
      seen_table.insert(HashMapType::value_type(labelFROM, true));
      ++edgeFROMi;
    }
  }

  // Process tail of the TO list.
  while (edgeTOi != to_seg->edge_list.end())
  {
    const IdentifierType labelTO = eqT->RecursiveLookup(edgeTOi->label);
    if (seen_table.find(labelTO) != seen_table.end() || labelTO == FROM)
    {
      auto edgeTEMPi = edgeTOi;
      ++edgeTEMPi;
      to_seg->edge_list.erase(edgeTOi);
      edgeTOi = edgeTEMPi;
    }
    else
    {
      if (labelTO != edgeTOi->label)
      {
        edgeTOi->label = labelTO;
      }
      seen_table.insert(HashMapType::value_type(labelTO, true));
      ++edgeTOi;
    }
  }

  // Now destroy the segment that was merged.
  segments->Erase(FROM);

  // Record this equivalency
  eqT->Add(FROM, TO);
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  //  typename SegmentTableType::Pointer  inputPtr  =
  // this->GetInputSegmentTable();
  //  typename SegmentTreeType::Pointer outputPtr =
  // this->GetOutputSegmentTree();

  //  if ( !inputPtr || !outputPtr )
  //    {
  //    return;
  //    }
}

template <typename TScalar>
void
SegmentTreeGenerator<TScalar>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "FloodLevel: " << m_FloodLevel << std::endl;
  os << indent << "Merge: " << m_Merge << std::endl;
  os << indent << "ConsumeInput: " << m_ConsumeInput << std::endl;
  os << indent << "HighestCalculatedFloodLevel: " << m_HighestCalculatedFloodLevel << std::endl;
}
} // namespace itk::watershed

#endif
