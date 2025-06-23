add_library(DocantoPDFLib STATIC
    PDF.cpp
)

target_include_directories(DocantoPDFLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/pdf> 
)

target_include_directories(DocantoPDFLib PRIVATE ${MUPDF_INCLUDE_PATH})

find_library(
    MUPDF_LIB
    NAMES mupdf libmupdf
    PATHS ${MUPDF_DEBUG_BUILD_PATH}
    NO_DEFAULT_PATH
)

target_link_libraries(DocantoPDFLib PUBLIC ${MUPDF_LIB})