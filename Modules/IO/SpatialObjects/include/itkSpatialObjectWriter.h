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
#ifndef itkSpatialObjectWriter_h
#define itkSpatialObjectWriter_h

#include "itkMetaSceneConverter.h"
#include "itkMetaConverterBase.h"
#include "itkSpatialObject.h"

namespace itk
{
/** \class SpatialObjectWriter
 *
 * \brief TODO
 * \ingroup ITKIOSpatialObjects
 */
template <unsigned int VDimension = 3,
          typename PixelType = unsigned char,
          typename TMeshTraits = DefaultStaticMeshTraits<PixelType, VDimension, VDimension>>
class ITK_TEMPLATE_EXPORT SpatialObjectWriter : public Object
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(SpatialObjectWriter);

  /** SmartPointer type alias support */
  using Self = SpatialObjectWriter;
  using Pointer = SmartPointer<Self>;

  /** Run-time type information (and related methods). */
  using Superclass = Object;

  using SpatialObjectType = SpatialObject<VDimension>;
  using SpatialObjectPointer = typename SpatialObjectType::Pointer;
  using SpatialObjectConstPointer = typename SpatialObjectType::ConstPointer;

  /** base type for MetaConverters -- bidirectional conversion btw
   *  SpatialObject & MetaObject
   */
  using MetaConverterBaseType = MetaConverterBase<VDimension>;

  using MetaSceneConverterType = MetaSceneConverter<VDimension, PixelType, TMeshTraits>;

  /** Method for creation through the object factory */
  itkNewMacro(Self);

  itkOverrideGetNameOfClassMacro(SpatialObjectWriter);

  /** Load a tube file. */
  void
  Update();

  /** Set the filename  */
  itkSetStringMacro(FileName);

  /** Get the filename */
  itkGetStringMacro(FileName);

  /** Set the Input  */
  void
  SetInput(const SpatialObjectType * input)
  {
    m_SpatialObject = input;
  }

  itkSetMacro(BinaryPoints, bool);
  itkGetConstMacro(BinaryPoints, bool);
  itkBooleanMacro(BinaryPoints);

  /** Set the version of MetaIO to use.
   *    Version 0 cannot accurately represented ImageSpatialObjects.
   *    Version 1 fixes bugs in Version 0, but introduces new tag-value
   *      pairs that might throw warnings on older readers. */

  void
  SetMetaIOVersion(unsigned int ver);
  unsigned int
  GetMetaIOVersion() const;

  void
  SetTransformPrecision(unsigned int precision);

  unsigned int
  GetTransformPrecision();

  /** Set/Get if the images should be written in a different file */
  /** @ITKStartGrouping */
  itkSetMacro(WriteImagesInSeparateFile, bool);
  itkGetConstMacro(WriteImagesInSeparateFile, bool);
  itkBooleanMacro(WriteImagesInSeparateFile);
  /** @ITKEndGrouping */
  /** Add a converter for a new MetaObject/SpatialObject type */
  void
  RegisterMetaConverter(const char *            metaTypeName,
                        const char *            spatialObjectTypeName,
                        MetaConverterBaseType * converter);

protected:
  std::string m_FileName{};
  bool        m_BinaryPoints{};
  bool        m_WriteImagesInSeparateFile{};

  SpatialObjectWriter();
  ~SpatialObjectWriter() override = default;

private:
  SpatialObjectConstPointer m_SpatialObject{};

  typename MetaSceneConverterType::Pointer m_MetaToSpatialConverter{};
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkSpatialObjectWriter.hxx"
#endif

#endif // itkSpatialObjectWriter_h
