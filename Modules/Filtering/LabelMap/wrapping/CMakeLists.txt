itk_wrap_module(ITKLabelMap)

set(
  WRAPPER_SUBMODULE_ORDER
  itkLabelObjectLine
  itkLabelObject
  itkShapeLabelObject
  itkStatisticsLabelObject
  ITKLabelMapBase
  itkLabelMapFilter
  itkInPlaceLabelMapFilter
  itkChangeRegionLabelMapFilter
)

# Save default includes so they can be reset afterwards
set(tmp_default_includes ${WRAPPER_DEFAULT_INCLUDE})

# Add itkStatisticsLabelObject.h, it is not automatically detected
# (but it maybe should)
list(APPEND WRAPPER_DEFAULT_INCLUDE "itkStatisticsLabelObject.h")

itk_auto_load_and_end_wrap_submodules()

# Reset the WRAPPER_DEFAULT_INCLUDE to original value
set(WRAPPER_DEFAULT_INCLUDE ${tmp_default_includes})
