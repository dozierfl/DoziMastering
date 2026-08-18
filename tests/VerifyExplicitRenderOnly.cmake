file(READ "${SOURCE_ROOT}/src/mastering_app/MasteringComponent.cpp" component_source)
file(READ "${SOURCE_ROOT}/src/mastering_app/MasteringComponent.h" component_header)

if(NOT component_source MATCHES "previewButton\\.onClick=\\[this\\]\\{renderPreview\\(\\);\\};")
    message(FATAL_ERROR "Render Preview must remain wired to the explicit preview button")
endif()

if(component_source MATCHES "presetAudition" OR component_header MATCHES "presetAudition")
    message(FATAL_ERROR "Processor changes must not schedule an automatic offline render")
endif()

string(REGEX REPLACE "previewButton\\.onClick=\\[this\\]\\{renderPreview\\(\\);\\};" "" source_without_explicit_render "${component_source}")
if(source_without_explicit_render MATCHES "renderPreview\\(\\);")
    message(FATAL_ERROR "Only the explicit Render Preview control may invoke renderPreview")
endif()
