CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12 FATAL_ERROR)

#######################################################
# CONFIGURATION
#######################################################
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
# FLAGS FOR TARGET
#######################################################
SET (ESCARGOT_LIBRARIES)
SET (ESCARGOT_INCDIRS)
INCLUDE (${ESCARGOT_ROOT}/build/target.cmake)

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
    ${ESCARGOT_CXXFLAGS})

SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

# ESCARGOT COMMON LDFLAGS
SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS} -fvisibility=hidden)

# bdwgc
IF (${ESCARGOT_MODE} STREQUAL "debug")
    SET (ESCARGOT_DEFINITIONS_COMMON ${ESCARGOT_DEFINITIONS_COMMON} -DGC_DEBUG)
ENDIF()

IF (${ESCARGOT_OUTPUT} STREQUAL "shared_lib" AND ${ESCARGOT_HOST} STREQUAL "android")
    SET (ESCARGOT_LDFLAGS ${ESCARGOT_LDFLAGS} -shared)
ENDIF()

IF (ESCARGOT_SMALL_CONFIG)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DESCARGOT_SMALL_CONFIG)
ENDIF()

IF (NOT DEFINED ESCARGOT_LIBICU_SUPPORT)
    SET (ESCARGOT_LIBICU_SUPPORT OFF)
ENDIF()

IF (NOT DEFINED ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN)
    SET (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN OFF)
ENDIF()

#######################################################
# FLAGS FOR ADDITIONAL FUNCTION
#######################################################
IF (ESCARGOT_LIBICU_SUPPORT)
    IF (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN)
        SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_ICU -DENABLE_INTL -DENABLE_RUNTIME_ICU_BINDER)
        SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_INTL_DISPLAYNAMES -DENABLE_INTL_NUMBERFORMAT -DENABLE_INTL_PLURALRULES)
        SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_INTL_RELATIVETIMEFORMAT -DENABLE_INTL_LISTFORMAT)
    ELSE()
        IF (NOT ${ESCARGOT_HOST} STREQUAL "windows")
            # windows icu cannot support these feature yet.(~10.0.18362)
            SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_INTL_DISPLAYNAMES -DENABLE_INTL_NUMBERFORMAT -DENABLE_INTL_PLURALRULES)
            SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_INTL_RELATIVETIMEFORMAT -DENABLE_INTL_LISTFORMAT)

            PKG_CHECK_MODULES (ICUI18N REQUIRED icu-i18n)
            PKG_CHECK_MODULES (ICUUC REQUIRED icu-uc)
        ENDIF()

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

IF (ESCARGOT_USE_CUSTOM_LOGGING)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_CUSTOM_LOGGING)
ELSEIF (${ESCARGOT_HOST} STREQUAL "tizen" OR ${ESCARGOT_HOST} STREQUAL "tizen_obs")
    PKG_CHECK_MODULES (DLOG REQUIRED dlog)
    SET (ESCARGOT_LIBRARIES ${ESCARGOT_LIBRARIES} ${DLOG_LIBRARIES})
    SET (ESCARGOT_INCDIRS ${ESCARGOT_INCDIRS} ${DLOG_INCLUDE_DIRS})
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} ${DLOG_CFLAGS_OTHER})
ENDIF()

SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_COMPRESSIBLE_STRING)
IF (ESCARGOT_SMALL_CONFIG)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DLZ4_MEMORY_USAGE=16 -DLZ4_HEAPMODE=1)
ENDIF()

SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_RELOADABLE_STRING)

IF (ESCARGOT_CODE_CACHE)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_CODE_CACHE)
ENDIF()

IF (ESCARGOT_WASM)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_WASM)
ENDIF()

IF (ESCARGOT_THREADING)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_THREADING -DGC_THREAD_ISOLATE)
ENDIF()

IF (ESCARGOT_EXPORT_ALL)
    SET (ESCARGOT_CXXFLAGS ${ESCARGOT_CXXFLAGS} -fvisibility=default)
ENDIF()

IF (ESCARGOT_TCO)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_TCO)
ENDIF()

IF (ESCARGOT_TEMPORAL)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DENABLE_TEMPORAL)
ENDIF()

IF (ESCARGOT_TEST)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DESCARGOT_ENABLE_TEST)
ENDIF()

#######################################################
# FLAGS FOR $(MODE) : debug/release
#######################################################
# DEBUG FLAGS
SET (ESCARGOT_DEFINITIONS_DEBUG -D_GLIBCXX_DEBUG -DGC_DEBUG)

# RELEASE FLAGS
SET (ESCARGOT_DEFINITIONS_RELEASE -DNDEBUG)

# SHARED_LIB FLAGS
SET (ESCARGOT_CXXFLAGS_SHAREDLIB -fPIC)
SET (ESCARGOT_LDFLAGS_SHAREDLIB -ldl)

# STATIC_LIB FLAGS
SET (ESCARGOT_CXXFLAGS_STATICLIB -fPIC -DESCARGOT_EXPORT=)

# SHELL FLAGS
SET (ESCARGOT_CXXFLAGS_SHELL -DESCARGOT_EXPORT=)

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
IF (ESCARGOT_DEBUGGER)
    SET (ESCARGOT_DEFINITIONS ${ESCARGOT_DEFINITIONS} -DESCARGOT_DEBUGGER)
ENDIF()
