if(ITK_WRAP_PYTHON)
  itk_python_add_test(
    NAME itkGPUGradientAnisotropicDiffusionImageFilterPythonTest2D
    TEST_DRIVER_ARGS
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/itkGPUGradientAnisotropicDiffusionImageFilterTest.py
      DATA{../../test/Input/BrainProtonDensitySlice.png}
      ${ITK_TEST_OUTPUT_DIR}/gpuGradientAnisotropicDiffusionImageFilterTest2DPython.mha
      2
  )

  itk_python_add_test(
    NAME itkGPUGradientAnisotropicDiffusionImageFilterPythonTest3D
    TEST_DRIVER_ARGS
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/tkGPUGradientAnisotropicDiffusionImageFilterTest.py
      DATA{../../test/Input/HeadMRVolume.mha}
      ${ITK_TEST_OUTPUT_DIR}/gpuGradientAnisotropicDiffusionImageFilterTest3DPython.mha
      3
  )
endif()
