CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# CONFIGURATION
#######################################################
IF (EXISTS "${PROJECT_SOURCE_DIR}/build/cmake/toolchain_${ESCARGOT_HOST}_${ESCARGOT_ARCH}.cmake")
    INCLUDE ("${PROJECT_SOURCE_DIR}/build/cmake/toolchain_${ESCARGOT_HOST}_${ESCARGOT_ARCH}.cmake")
ELSE()
    MESSAGE (FATAL_ERROR "Error: unsupported target")
ENDIF()

# CONFIGURE ESCARGOT VERSION
FIND_PACKAGE(Git)
IF (GIT_FOUND)
    EXECUTE_PROCESS (
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE ESCARGOT_BUILD_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()
IF ((NOT DEFINED ESCARGOT_BUILD_VERSION) OR (ESCARGOT_BUILD_VERSION STREQUAL ""))
    FILE (STRINGS "${PROJECT_SOURCE_DIR}/RELEASE_VERSION" ESCARGOT_BUILD_VERSION)
ENDIF()
MESSAGE(STATUS "Escargot Build Version: ${ESCARGOT_BUILD_VERSION}")
CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/src/EscargotInfo.h.in ${PROJECT_SOURCE_DIR}/src/EscargotInfo.h @ONLY)

#######################################################
# PATH
#######################################################
SET (ESCARGOT_ROOT ${PROJECT_SOURCE_DIR})
SET (ESCARGOT_THIRD_PARTY_ROOT ${ESCARGOT_ROOT}/third_party)
SET (GCUTIL_ROOT ${ESCARGOT_THIRD_PARTY_ROOT}/GCutil)


#######################################################
# FLAGS FOR COMMON
#######################################################
# ESCARGOT COMMON CXXFLAGS
SET (ESCARGOT_DEFINITIONS
    ${ESCARGOT_DEFINITIONS}
    -DESCARGOT
)

SET (CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
SEPARATE_ARGUMENTS(CXXFLAGS_FROM_ENV)
SET (CFLAGS_FROM_ENV $ENV{CFLAGS})
SEPARATE_ARGUMENTS(CFLAGS_FROM_ENV)

SET (ESCARGOT_CXXFLAGS
    ${CXXFLAGS_FROM_ENV}
    ${ESCARGOT_CXXFLAGS}
    -std=c++11 -g3
    -fno-math-errno
    -fdata-sections -ffunction-sections
    -fno-omit-frame-pointer
    -fvisibility=hidden
    -Wno-unused-parameter
    -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof
    -Wno-deprecated-declarations
)

IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} -frounding-math -fsignaling-nans -Wno-unused-but-set-variable -Wno-unused-but-set-parameter)
ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -Wno-parentheses-equality -Wno-dynamic-class-memaccess -Wno-deprecated-register -Wno-expansion-to-defined -Wno-return-type -Wno-overloaded-virtual -Wno-unused-private-field)
ELSE()
    MESSAGE (FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " is Unsupported Compiler")
ENDIF()

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} -Wno-attributes -Wno-class-memaccess -Wno-deprecated-copy -Wno-cast-function-type -Wno-stringop-truncation -Wno-pessimizing-move -Wno-ignored-qualifiers)
endif()

SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

# ESCARGOT COMMON LDFLAGS
SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS} -fvisibility=hidden)

IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS})
ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS})
ELSE()
    MESSAGE (FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " is Unsupported Compiler")
ENDIF()



# bdwgc
IF (${ESCARGOT_MODE} STREQUAL "debug")
    SET (ESCARGOT_DEFINITIONS_COMMON ${ESCARGOT_DEFINITIONS_COMMON} -DGC_DEBUG)
ENDIF()

#######################################################
# FLAGS FOR $(ESCARGOT_HOST)
#######################################################

IF (${ESCARGOT_OUTPUT} STREQUAL "shared_lib" AND ${ESCARGOT_HOST} STREQUAL "android")
    SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS} -shared)
ENDIF()

IF (NOT DEFINED ESCARGOT_SMALL_CONFIG)
    SET (ESCARGOT_SMALL_CONFIG OFF)
ENDIF()

IF (NOT DEFINED ESCARGOT_GC_SMALL_CONFIG)
    SET (ESCARGOT_GC_SMALL_CONFIG OFF)
ENDIF()

IF (${ESCARGOT_SMALL_CONFIG} STREQUAL "ON")
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DESCARGOT_SMALL_CONFIG)
ENDIF()

IF (NOT DEFINED ESCARGOT_LIBICU_SUPPORT)
    SET (ESCARGOT_LIBICU_SUPPORT ON)
ENDIF()

IF (NOT DEFINED ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN)
    SET (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN ON)
ENDIF()


IF (${ESCARGOT_HOST} STREQUAL "android")
    SET (ESCARGOT_LIBICU_SUPPORT OFF)
ENDIF()

#######################################################
# FLAGS FOR ADDITIONAL FUNCTION
#######################################################

FIND_PACKAGE (PkgConfig REQUIRED)
IF (${ESCARGOT_LIBICU_SUPPORT} STREQUAL "ON")
    IF (${ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN} STREQUAL "ON")
        SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_ICU -DENABLE_INTL -DENABLE_RUNTIME_ICU_BINDER)
    ELSE()
        PKG_CHECK_MODULES (ICUI18N REQUIRED icu-i18n)
        PKG_CHECK_MODULES (ICUUC REQUIRED icu-uc)

        SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_ICU -DENABLE_INTL)

        IF (${ESCARGOT_HOST} STREQUAL "darwin")
            FOREACH (ICU_LDFLAG ${ICUI18N_LDFLAGS} ${ICUUC_LDFLAGS})
                SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS} ${ICU_LDFLAG})
            ENDFOREACH()
        ELSE()
            SET (ESCARGOT_LIBRARIES ${ESCARGOT_LIBRARIES} ${ICUI18N_LIBRARIES} ${ICUUC_LIBRARIES})
        ENDIF()
        SET (ESCARGOT_INCDIRS ${ESCARGOT_INCDIRS} ${ICUI18N_INCLUDE_DIRS} ${ICUUC_INCLUDE_DIRS})
        SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} ${ICUI18N_CFLAGS_OTHER} ${ICUUC_CFLAGS_OTHER})
    ENDIF()
ENDIF()

IF (${ESCARGOT_HOST} STREQUAL "tizen_obs")
    PKG_CHECK_MODULES (DLOG REQUIRED dlog)
    SET (ESCARGOT_LIBRARIES ${ESCARGOT_LIBRARIES} ${DLOG_LIBRARIES})
    SET (ESCARGOT_INCDIRS ${ESCARGOT_INCDIRS} ${DLOG_INCLUDE_DIRS})
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} ${DLOG_CFLAGS_OTHER})
ENDIF()

SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_COMPRESSIBLE_STRING)
IF (${ESCARGOT_SMALL_CONFIG} STREQUAL "ON")
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DLZ4_MEMORY_USAGE=16 -DLZ4_HEAPMODE=1)
ENDIF()

SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_RELOADABLE_STRING)

IF (ESCARGOT_CODE_CACHE)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_CODE_CACHE)
ENDIF()

#######################################################
# flags for $(MODE) : debug/release
#######################################################
# DEBUG FLAGS
SET (ESCARGOT_CXXFLAGS_DEBUG -O0 -Wall -Wextra -Werror ${ESCARGOT_CXXFLAGS_DEBUG})
SET (ESCARGOT_DEFINITIONS_DEBUG -D_GLIBCXX_DEBUG -DGC_DEBUG)


# RELEASE FLAGS
SET (ESCARGOT_CXXFLAGS_RELEASE -O2 -fno-stack-protector ${ESCARGOT_CXXFLAGS_RELEASE})
SET (ESCARGOT_DEFINITIONS_RELEASE -DNDEBUG)


# SHARED_LIB FLAGS
SET (ESCARGOT_CXXFLAGS_SHAREDLIB -fPIC)
SET (ESCARGOT_LDFLAGS_SHAREDLIB -ldl)


# STATIC_LIB FLAGS
SET (ESCARGOT_CXXFLAGS_STATICLIB -fPIC -DESCARGOT_EXPORT=)
SET (ESCARGOT_LDFLAGS_STATICLIB -Wl,--gc-sections)

# SHELL FLAGS
SET (ESCARGOT_CXXFLAGS_SHELL -DESCARGOT_EXPORT=)
SET (ESCARGOT_LDFLAGS_SHELL -Wl,--gc-sections)


#######################################################
# FLAGS FOR TEST
#######################################################
SET (ESCARGOT_DEFINITIONS_TEST -DESCARGOT_ENABLE_TEST)


#######################################################
# FLAGS FOR MEMORY PROFILING
#######################################################
SET (PROFILER_FLAGS)

IF (ESCARGOT_PROFILE_BDWGC)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DPROFILE_BDWGC)
ENDIF()

IF (ESCARGOT_MEM_STATS)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DESCARGOT_MEM_STATS)
ENDIF()

IF (ESCARGOT_VALGRIND)
    SET (PROFILER_FLAGS ${PROFILER_FLAGS} -DESCARGOT_VALGRIND)
ENDIF()

#######################################################
# FLAGS FOR DEBUGGER
#######################################################
SET (DEBUGGER_FLAGS)

IF (ESCARGOT_DEBUGGER)
    SET (DEBUGGER_FLAGS ${DEBUGGER_FLAGS} -DESCARGOT_DEBUGGER)
ENDIF()
