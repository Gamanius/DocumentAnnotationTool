add_library(DocantoPDFLib STATIC
    PDF.cpp
 "PDFContext.cpp")

target_include_directories(DocantoPDFLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/pdf> 
)

target_include_directories(DocantoPDFLib PRIVATE ${MUPDF_INCLUDE_PATH})

target_link_libraries(DocantoPDFLib PUBLIC ${MUPDF_LIB})