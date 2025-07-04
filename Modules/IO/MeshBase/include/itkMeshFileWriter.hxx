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
#ifndef itkMeshFileWriter_hxx
#define itkMeshFileWriter_hxx

#include "itkCommand.h"
#include "itkDataObject.h"
#include "itkMeshConvertPixelTraits.h"
#include "itkMeshIOFactory.h"
#include "itkObjectFactoryBase.h"
#include "itkMakeUniqueForOverwrite.h"

#include "vnl/vnl_vector.h"


namespace itk
{
template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::SetInput(const InputMeshType * input)
{
  this->ProcessObject::SetNthInput(0, const_cast<TInputMesh *>(input));
}

template <typename TInputMesh>
auto
MeshFileWriter<TInputMesh>::GetInput() -> const InputMeshType *
{
  if (this->GetNumberOfInputs() < 1)
  {
    return nullptr;
  }

  return static_cast<TInputMesh *>(this->ProcessObject::GetInput(0));
}

template <typename TInputMesh>
auto
MeshFileWriter<TInputMesh>::GetInput(unsigned int idx) -> const InputMeshType *
{
  return static_cast<TInputMesh *>(this->ProcessObject::GetInput(idx));
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::Write()
{
  const InputMeshType * input = this->GetInput();

  itkDebugMacro("Writing an mesh file");

  // Make sure input is available
  if (input == nullptr)
  {
    itkExceptionMacro("No input to writer!");
  }

  // Make sure that we can write the file given the name
  if (m_FileName.empty())
  {
    throw MeshFileWriterException(__FILE__, __LINE__, "FileName must be specified", ITK_LOCATION);
  }

  if (!(m_UserSpecifiedMeshIO && !m_MeshIO.IsNull()))
  {
    // Try creating via factory
    if (m_MeshIO.IsNull())
    {
      itkDebugMacro("Attempting factory creation of MeshIO for file: " << m_FileName);
      m_MeshIO = MeshIOFactory::CreateMeshIO(m_FileName.c_str(), MeshIOFactory::IOFileModeEnum::WriteMode);
      m_FactorySpecifiedMeshIO = true;
    }
    else
    {
      if (m_FactorySpecifiedMeshIO && !m_MeshIO->CanWriteFile(m_FileName.c_str()))
      {
        itkDebugMacro("MeshIO exists but doesn't know how to write file:" << m_FileName);
        itkDebugMacro("Attempting creation of MeshIO with a factory for file:" << m_FileName);
        m_MeshIO = MeshIOFactory::CreateMeshIO(m_FileName.c_str(), MeshIOFactory::IOFileModeEnum::WriteMode);
        m_FactorySpecifiedMeshIO = true;
      }
    }
  }

  if (m_MeshIO.IsNull())
  {
    MeshFileWriterException e(__FILE__, __LINE__);
    std::ostringstream      msg;
    msg << " Could not create IO object for file " << m_FileName << std::endl
        << "  Tried to create one of the following:" << std::endl;
    {
      for (auto & allobject : ObjectFactoryBase::CreateAllInstance("itkMeshIOBase"))
      {
        auto * io = dynamic_cast<MeshIOBase *>(allobject.GetPointer());
        msg << "    " << io->GetNameOfClass() << std::endl;
      }
    }

    msg << "  You probably failed to set a file suffix, or" << std::endl
        << "    set the suffix to an unsupported type." << std::endl;
    e.SetDescription(msg.str().c_str());
    e.SetLocation(ITK_LOCATION);
    throw e;
  }

  // NOTE: this const_cast<> is due to the lack of const-correctness
  // of the ProcessObject.
  auto * nonConstInput = const_cast<InputMeshType *>(input);

  // Update the input.
  // Streaming is not supported at this time.
  nonConstInput->SetRequestedRegionToLargestPossibleRegion();
  nonConstInput->Update();

  if (m_FileTypeIsBINARY)
  {
    m_MeshIO->SetFileType(IOFileEnum::BINARY);
  }
  else
  {
    m_MeshIO->SetFileType(IOFileEnum::ASCII);
  }

  if (m_UseCompression)
  {
    m_MeshIO->UseCompressionOn();
  }
  else
  {
    m_MeshIO->UseCompressionOff();
  }

  // Setup the MeshIO
  m_MeshIO->SetFileName(m_FileName.c_str());

  // Whether write points
  if (input->GetPoints() && input->GetNumberOfPoints())
  {
    m_MeshIO->SetUpdatePoints(true);
    m_MeshIO->SetNumberOfPoints(input->GetNumberOfPoints());
    m_MeshIO->SetPointDimension(TInputMesh::PointDimension);
    m_MeshIO->SetPointComponentType(MeshIOBase::MapComponentType<typename TInputMesh::PointType::ValueType>::CType);
  }

  // Whether write cells
  if (input->GetCells() && input->GetNumberOfCells())
  {
    SizeValueType cellsBufferSize = 2 * input->GetNumberOfCells();
    for (typename TInputMesh::CellsContainerConstIterator ct = input->GetCells()->Begin();
         ct != input->GetCells()->End();
         ++ct)
    {
      cellsBufferSize += ct->Value()->GetNumberOfPoints();
    }
    m_MeshIO->SetCellBufferSize(cellsBufferSize);
    m_MeshIO->SetUpdateCells(true);
    m_MeshIO->SetNumberOfCells(input->GetNumberOfCells());
    m_MeshIO->SetCellComponentType(MeshIOBase::MapComponentType<typename TInputMesh::PointIdentifier>::CType);
  }

  // Whether write point data
  if (input->GetPointData() && input->GetPointData()->Size())
  {
    m_MeshIO->SetUpdatePointData(true);
    m_MeshIO->SetNumberOfPointPixels(input->GetPointData()->Size());
    // m_MeshIO->SetNumberOfPointPixelComponents(MeshConvertPixelTraits<typename
    // TInputMesh::PixelType>::GetNumberOfComponents());
    m_MeshIO->SetPixelType(input->GetPointData()->Begin().Value(), true);
  }

  // Whether write cell data
  if (input->GetCellData() && input->GetCellData()->Size())
  {
    m_MeshIO->SetUpdateCellData(true);
    m_MeshIO->SetNumberOfCellPixels(input->GetCellData()->Size());
    // m_MeshIO->SetNumberOfCellPixelComponents(MeshConvertPixelTraits<typename
    // TInputMesh::CellPixelType>::GetNumberOfComponents());
    m_MeshIO->SetPixelType(input->GetCellData()->Begin().Value(), false);
  }

  this->InvokeEvent(StartEvent());

  // Write mesh information
  m_MeshIO->WriteMeshInformation();

  // write points
  if (input->GetPoints() && input->GetNumberOfPoints())
  {
    WritePoints();
  }

  // Write cells
  if (input->GetCells() && input->GetNumberOfCells())
  {
    WriteCells();
  }

  // Write point data
  if (input->GetPointData() && input->GetPointData()->Size())
  {
    WritePointData();
  }

  // Write cell data
  if (input->GetCellData() && input->GetCellData()->Size())
  {
    WriteCellData();
  }

  // Write to disk
  m_MeshIO->Write();

  // Notify end event observers
  this->InvokeEvent(EndEvent());

  // Release upstream data if requested
  this->ReleaseInputs();
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::WritePoints()
{
  const InputMeshType * input = this->GetInput();

  itkDebugMacro("Writing points: " << m_FileName);
  const SizeValueType pointsBufferSize = input->GetNumberOfPoints() * TInputMesh::PointDimension;
  using ValueType = typename TInputMesh::PointType::ValueType;
  const auto buffer = make_unique_for_overwrite<ValueType[]>(pointsBufferSize);
  CopyPointsToBuffer(buffer.get());
  m_MeshIO->WritePoints(buffer.get());
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::WriteCells()
{
  itkDebugMacro("Writing cells: " << m_FileName);

  const SizeValueType cellsBufferSize = m_MeshIO->GetCellBufferSize();
  using PointIdentifierType = typename TInputMesh::PointIdentifier;
  const auto buffer = make_unique_for_overwrite<PointIdentifierType[]>(cellsBufferSize);
  CopyCellsToBuffer(buffer.get());
  m_MeshIO->WriteCells(buffer.get());
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::WritePointData()
{
  const InputMeshType * input = this->GetInput();

  itkDebugMacro("Writing point data: " << m_FileName);

  if (input->GetPointData()->Size())
  {
    const SizeValueType numberOfComponents =
      input->GetPointData()->Size() * MeshConvertPixelTraits<typename TInputMesh::PixelType>::GetNumberOfComponents(
                                        input->GetPointData()->Begin().Value());

    using ValueType = typename itk::NumericTraits<typename TInputMesh::PixelType>::ValueType;
    const auto buffer = make_unique_for_overwrite<ValueType[]>(numberOfComponents);
    CopyPointDataToBuffer(buffer.get());
    m_MeshIO->WritePointData(buffer.get());
  }
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::WriteCellData()
{
  const InputMeshType * input = this->GetInput();

  itkDebugMacro("Writing cell data: " << m_FileName);

  if (input->GetCellData()->Size())
  {
    const SizeValueType numberOfComponents =
      input->GetCellData()->Size() * MeshConvertPixelTraits<typename TInputMesh::CellPixelType>::GetNumberOfComponents(
                                       input->GetCellData()->Begin().Value());

    using ValueType = typename itk::NumericTraits<typename TInputMesh::CellPixelType>::ValueType;
    const auto buffer = make_unique_for_overwrite<ValueType[]>(numberOfComponents);
    CopyCellDataToBuffer(buffer.get());
    m_MeshIO->WriteCellData(buffer.get());
  }
}

template <typename TInputMesh>
template <typename Output>
void
MeshFileWriter<TInputMesh>::CopyPointsToBuffer(Output * data)
{
  const typename InputMeshType::PointsContainer * points = this->GetInput()->GetPoints();

  SizeValueType                                     index{};
  typename TInputMesh::PointsContainerConstIterator pter = points->Begin();

  while (pter != points->End())
  {
    for (unsigned int jj = 0; jj < TInputMesh::PointDimension; ++jj)
    {
      data[index++] = static_cast<Output>(pter.Value()[jj]);
    }

    ++pter;
  }
}

template <typename TInputMesh>
template <typename Output>
void
MeshFileWriter<TInputMesh>::CopyCellsToBuffer(Output * data)
{
  // Get input mesh pointer
  const typename InputMeshType::CellsContainer * cells = this->GetInput()->GetCells();

  // Define required variables

  // For each cell
  SizeValueType                                    index{};
  typename TInputMesh::CellsContainerConstIterator cter = cells->Begin();
  while (cter != cells->End())
  {
    typename TInputMesh::CellType * cellPtr = cter.Value();

    // Write the cell type
    switch (cellPtr->GetType())
    {
      case CellGeometryEnum::VERTEX_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::VERTEX_CELL);
        break;
      case CellGeometryEnum::LINE_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::LINE_CELL);
        break;
      case CellGeometryEnum::POLYLINE_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::POLYLINE_CELL);
        break;
      case CellGeometryEnum::TRIANGLE_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::TRIANGLE_CELL);
        break;
      case CellGeometryEnum::QUADRILATERAL_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::QUADRILATERAL_CELL);
        break;
      case CellGeometryEnum::POLYGON_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::POLYGON_CELL);
        break;
      case CellGeometryEnum::TETRAHEDRON_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::TETRAHEDRON_CELL);
        break;
      case CellGeometryEnum::HEXAHEDRON_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::HEXAHEDRON_CELL);
        break;
      case CellGeometryEnum::QUADRATIC_EDGE_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::QUADRATIC_EDGE_CELL);
        break;
      case CellGeometryEnum::QUADRATIC_TRIANGLE_CELL:
        data[index++] = static_cast<Output>(CellGeometryEnum::QUADRATIC_TRIANGLE_CELL);
        break;
      default:
        itkExceptionMacro("Unknown mesh cell");
    }

    // The second element is number of points for each cell
    data[index++] = cellPtr->GetNumberOfPoints();
    // Others are point identifiers in the cell
    const typename TInputMesh::PointIdentifier * ptIds = cellPtr->GetPointIds();
    const unsigned int                           numberOfPoints = cellPtr->GetNumberOfPoints();
    for (unsigned int ii = 0; ii < numberOfPoints; ++ii)
    {
      data[index++] = static_cast<Output>(ptIds[ii]);
    }

    ++cter;
  }
}

template <typename TInputMesh>
template <typename Output>
void
MeshFileWriter<TInputMesh>::CopyPointDataToBuffer(Output * data)
{
  const typename InputMeshType::PointDataContainer * pointData = this->GetInput()->GetPointData();

  //  typename TInputMesh::PixelType value = NumericTraits< typename
  // TInputMesh::PixelType >::ZeroValue();
  // TODO? NumericTraitsVariableLengthVectorPixel should define ZeroValue()
  // Should define NumericTraitsArrayPixel

  const unsigned int numberOfComponents =
    MeshConvertPixelTraits<typename TInputMesh::PixelType>::GetNumberOfComponents(pointData->Begin().Value());

  SizeValueType                                          index = 0;
  typename TInputMesh::PointDataContainer::ConstIterator pter = pointData->Begin();
  while (pter != pointData->End())
  {
    for (unsigned int jj = 0; jj < numberOfComponents; ++jj)
    {
      data[index++] =
        static_cast<Output>(MeshConvertPixelTraits<typename TInputMesh::PixelType>::GetNthComponent(jj, pter.Value()));
    }

    ++pter;
  }
}

template <typename TInputMesh>
template <typename Output>
void
MeshFileWriter<TInputMesh>::CopyCellDataToBuffer(Output * data)
{
  const typename InputMeshType::CellDataContainer * cellData = this->GetInput()->GetCellData();

  //  typename TInputMesh::CellPixelType value = NumericTraits< typename
  // TInputMesh::CellPixelType >::ZeroValue();
  // TODO? NumericTraitsVariableLengthVectorPixel should define ZeroValue()
  // Should define NumericTraitsArrayPixel

  const unsigned int numberOfComponents =
    MeshConvertPixelTraits<typename TInputMesh::CellPixelType>::GetNumberOfComponents(cellData->Begin().Value());
  SizeValueType                                         index = 0;
  typename TInputMesh::CellDataContainer::ConstIterator cter = cellData->Begin();
  while (cter != cellData->End())
  {
    for (unsigned int jj = 0; jj < numberOfComponents; ++jj)
    {
      data[index++] = static_cast<Output>(
        MeshConvertPixelTraits<typename TInputMesh::CellPixelType>::GetNthComponent(jj, cter.Value()));
    }
    ++cter;
  }
}

template <typename TInputMesh>
void
MeshFileWriter<TInputMesh>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << m_FileName << std::endl;

  itkPrintSelfObjectMacro(MeshIO);

  itkPrintSelfBooleanMacro(UserSpecifiedMeshIO);
  itkPrintSelfBooleanMacro(FactorySpecifiedMeshIO);
  itkPrintSelfBooleanMacro(UseCompression);
  itkPrintSelfBooleanMacro(FileTypeIsBINARY);
}
} // end namespace itk

#endif
