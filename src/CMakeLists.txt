project (mcu_tmf882x)

# create the project executable for running on the target
add_executable (${PROJECT_NAME} main.c)

set (TMF882X_LIB "tmf882x_lib" CACHE INTERNAL "")

# Configure header for source-visible version info
configure_file (
    "${PROJECT_SOURCE_DIR}/mcu_tmf882x_config.h.in"
    "${PROJECT_BINARY_DIR}/generated/mcu_tmf882x_config.h"
  )

# include paths
include_directories ("${PROJECT_BINARY_DIR}/generated")
include_directories ("${PROJECT_SOURCE_DIR}/../inc")

file (GLOB SOURCES "${PROJECT_SOURCE_DIR}/*.c")
list (REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/main.c")

# create the driver archive
add_library (${TMF882X_LIB} STATIC ${SOURCES})

# generate binary image to build into target
if (BUILTIN_FW)
    add_definitions (-D_HAS_BUILTIN_FW)
    find_program(SREC_EXE srec_cat)
    if (NOT ${SREC_EXE_FOUND})
        message(FATAL_ERROR "'srec_cat' utility not found!")
    endif()
    set (SREC_CMD ${SREC_EXE} ${FW_PATH} -intel @${SREC_CFG})
    set (TOF_BIN_IMAGE_SOURCES
        "${PROJECT_BINARY_DIR}/generated/tof_bin_image.c"
        "${PROJECT_BINARY_DIR}/generated/tof_bin_image.h"
    )

    add_custom_command (OUTPUT ${TOF_BIN_IMAGE_SOURCES}
                        COMMAND ${SREC_CMD}
                        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/generated
                        COMMENT "Generate binary image source files"
                        VERBATIM
    )
    add_custom_target (tof_builtin_fw ALL DEPENDS ${TOF_BIN_IMAGE_SOURCES})
    # create the project executable for running on the target
    add_dependencies (${PROJECT_NAME} tof_builtin_fw)
    add_dependencies (${TMF882X_LIB} tof_builtin_fw)
    target_sources (${PROJECT_NAME} PUBLIC ${TOF_BIN_IMAGE_SOURCES})
endif()

# link in realtime package for clock_gettime()
target_link_libraries (${PROJECT_NAME} ${TMF882X_LIB} ${RT_LIB})

#custom target for generating doxygen documentation
string (REPLACE ";" " " DOC_SOURCES "${SOURCES}")
find_package (Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/../Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_TARGET doxygen)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    #set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/../Doxyfile)
    add_custom_target (${DOXYGEN_TARGET} ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generate Doxygen docs for tmf882x mcu driver"
        VERBATIM)
    set_target_properties (${DOXYGEN_TARGET} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    find_package(LATEX COMPONENTS PDFLATEX)
    if (LATEX_PDFLATEX_FOUND)
        set(PDF_LATEX_IN refman.tex)
        add_custom_command (TARGET ${DOXYGEN_TARGET}
            POST_BUILD
            COMMAND ${PDFLATEX_COMPILER} ${PDF_LATEX_IN}
            # must run twice to fix broken source links in latex
            COMMAND ${PDFLATEX_COMPILER} ${PDF_LATEX_IN}
            COMMAND cp refman.pdf ../TMF882X_Driver_User_Guide.pdf
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../doxygen/latex/
            COMMENT "Create Doxygen PDF file from Latex output"
            VERBATIM)
    endif()
else(DOXYGEN_FOUND)
    message ("Doxygen is required to generate documentation")
endif()
