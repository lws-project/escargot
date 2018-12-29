CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# FLAGS
SET (ESCARGOT_CXXFLAGS)
SET (ESCARGOT_LDFLAGS)
SET (ESCARGOT_LIBRARIES)
SET (ESCARGOT_LIBDIRS)

SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_CONFIG}")
SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_COMMON}")
SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_THIRD_PARTY}")
SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_CONFIG}")
SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_COMMON}")

IF (${ESCARGOT_HOST} STREQUAL "linux")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_LINUX}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_LINUX}")
    SET (ESCARGOT_LIBRARIES ${ESCARGOT_LIBRARIES} ${ESCARGOT_LIBRARIES_LINUX})
    SET (ESCARGOT_LIBDIRS ${ESCARGOT_LIBDIRS} ${ESCARGOT_LIBDIRS_LINUX})
ELSEIF (${ESCARGOT_HOST} MATCHES "tizen")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_TIZEN}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_TIZEN}")
    SET (ESCARGOT_LIBRARIES ${ESCARGOT_LIBRARIES} ${ESCARGOT_LIBRARIES_TIZEN})
    SET (ESCARGOT_LIBDIRS ${ESCARGOT_LIBDIRS} ${ESCARGOT_LIBDIRS_TIZEN})
ELSEIF (${ESCARGOT_HOST} MATCHES "android")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_ANDROID}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_ANDROID}")
ENDIF()

IF (${ESCARGOT_ARCH} STREQUAL "x64")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_X64}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_X64}")
ELSEIF (${ESCARGOT_ARCH} STREQUAL "x86")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_X86}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_X86}")
ELSEIF (${ESCARGOT_ARCH} STREQUAL "i686")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_X86}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_X86}")
ELSEIF (${ESCARGOT_ARCH} STREQUAL "arm")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_ARM}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_ARM}")
ELSEIF (${ESCARGOT_ARCH} STREQUAL "aarch64")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_AARCH64}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_AARCH64}")
ELSEIF (${ESCARGOT_ARCH} STREQUAL "x86_64")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_X64}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_X64}")
ENDIF()

IF (${ESCARGOT_MODE} STREQUAL "debug")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_DEBUG}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_DEBUG}")
ELSEIF (${ESCARGOT_MODE} STREQUAL "release")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_RELEASE}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_RELEASE}")
ENDIF()

IF (${ESCARGOT_OUTPUT} STREQUAL "bin")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_BIN}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_BIN}")
ELSEIF (${ESCARGOT_OUTPUT} STREQUAL "shared_lib")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_SHAREDLIB}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_SHAREDLIB}")
ELSEIF (${ESCARGOT_OUTPUT} STREQUAL "static_lib")
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_STATICLIB}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_STATICLIB}")
ENDIF()

IF ("${LTO}" EQUAL 1)
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_LTO}")
    SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_LDFLAGS_LTO}")
    IF (${ESCARGOT_OUTPUT} STREQUAL "bin")
        SET (ESCARGOT_LDFLAGS "${ESCARGOT_LDFLAGS} ${ESCARGOT_CXXFLAGS}")
    ENDIF()
ENDIF()

IF ("${VENDORTEST}" EQUAL 1)
    SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS} ${ESCARGOT_CXXFLAGS_VENDORTEST}")
ENDIF()


# SOURCE FILES
FILE (GLOB_RECURSE ESCARGOT_SRC ${ESCARGOT_ROOT}/src/*.cpp)
FILE (GLOB YARR_SRC ${ESCARGOT_THIRD_PARTY_ROOT}/yarr/*.cpp)
FILE (GLOB DOUBLE_CONVERSION_SRC ${ESCARGOT_THIRD_PARTY_ROOT}/double_conversion/*.cc)

IF (NOT ${ESCARGOT_OUTPUT} STREQUAL "bin")
    LIST (REMOVE_ITEM ESCARGOT_SRC ${ESCARGOT_ROOT}/src/shell/Shell.cpp ${ESCARGOT_ROOT}/src/shell/GlobalObjectBuiltinTestFunctions.cpp)
ENDIF()

SET (ESCARGOT_SRC_LIST
    ${ESCARGOT_SRC}
    ${YARR_SRC}
    ${DOUBLE_CONVERSION_SRC}
)


# GC LIBRARY (static) only for binary output
IF (${ESCARGOT_OUTPUT} STREQUAL "bin")
    SET (GC_CFLAGS_COMMON "-g3 -fdata-sections -ffunction-sections -DHAVE_CONFIG_H -DESCARGOT -DIGNORE_DYNAMIC_LOADING -DGC_DONT_REGISTER_MAIN_STATIC_DATA -Wno-unused-variable")

    IF (${ESCARGOT_ARCH} STREQUAL "x86")
        SET (GC_CFLAGS_ARCH "-m32")
        SET (GC_LDFLAGS_ARCH "-m32")
    ELSEIF (${ESCARGOT_ARCH} STREQUAL "arm")
        SET (GC_CFLAGS_ARCH "-march=armv7-a -mthumb -finline-limit=64")
    ENDIF()

    IF (${ESCARGOT_MODE} STREQUAL "debug")
        SET (GC_CFLAGS_MODE "-O0 -DGC_DEBUG")
    ELSE()
        SET (GC_CFLAGS_MODE "-O2")
    ENDIF()

    SET (GC_CFLAGS "${GC_CFLAGS_COMMON} ${GC_CFLAGS_ARCH} ${GC_CFLAGS_MODE} $ENV{CFLAGS}")
    SET (GC_LDFLAGS "${GC_LDFLAGS_ARCH} ${GC_CFLAGS}")

    SET (GC_CONFFLAGS_COMMON --enable-munmap --disable-parallel-mark --enable-large-config --disable-pthread --disable-threads)
    IF (${ESCARGOT_MODE} STREQUAL "debug")
        SET (GC_CONFFLAGS_MODE --enable-debug --enable-gc-debug)
    ELSE()
        SET (GC_CONFFLAGS_MODE --disable-debug --disable-gc-debug)
    ENDIF()
    SET (GC_CONFFLAGS
        ${GC_CONFFLAGS_COMMON}
        ${GC_CONFFLAGS_MODE}
    )

    SET (GC_BUILDDIR ${GCUTIL_ROOT}/bdwgc/out/${ESCARGOT_HOST}/${ESCARGOT_ARCH}/${ESCARGOT_MODE}.static)
    SET (GC_TARGET ${GC_BUILDDIR}/.libs/libgc.a)

    ADD_CUSTOM_COMMAND (OUTPUT ${GC_TARGET}
            COMMENT "BUILD GC"
            WORKING_DIRECTORY ${GCUTIL_ROOT}/bdwgc
            COMMAND autoreconf -vif
            COMMAND automake --add-missing
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GC_BUILDDIR}
            COMMAND cd ${GC_BUILDDIR} && ../../../../configure ${GC_CONFFLAGS} CFLAGS=${GC_CFLAGS} LDFLAGS=${GC_LDFLAGS}
            COMMAND cd ${GC_BUILDDIR} && make -j
    )

    ADD_CUSTOM_TARGET (gc
            DEPENDS ${GC_TARGET}
            COMMAND echo "GC TARGET"
    )
ENDIF()


# BUILD
IF (${ESCARGOT_OUTPUT} STREQUAL "bin")
    ADD_EXECUTABLE (${ESCARGOT_TARGET} ${ESCARGOT_SRC_LIST})
    ADD_DEPENDENCIES (${ESCARGOT_TARGET} gc)

    TARGET_LINK_LIBRARIES (${ESCARGOT_TARGET} ${ESCARGOT_LIBRARIES} ${GC_TARGET})
    TARGET_INCLUDE_DIRECTORIES (${ESCARGOT_TARGET} PUBLIC ${ESCARGOT_LIBDIRS})
    SET_TARGET_PROPERTIES (${ESCARGOT_TARGET} PROPERTIES
                           COMPILE_FLAGS "${ESCARGOT_CXXFLAGS} $ENV{CXXFLAGS}"
                           LINK_FLAGS "${ESCARGOT_LDFLAGS}")

    ADD_CUSTOM_COMMAND (TARGET ${ESCARGOT_TARGET} POST_BUILD
                        COMMAND cp ${ESCARGOT_OUTDIR}/${ESCARGOT_TARGET} .)

ELSEIF (${ESCARGOT_OUTPUT} STREQUAL "shared_lib")
    ADD_LIBRARY (${ESCARGOT_TARGET} SHARED ${ESCARGOT_SRC_LIST})

    TARGET_LINK_LIBRARIES (${ESCARGOT_TARGET} ${ESCARGOT_LIBRARIES})
    TARGET_INCLUDE_DIRECTORIES (${ESCARGOT_TARGET} PUBLIC ${ESCARGOT_LIBDIRS})
    SET_TARGET_PROPERTIES (${ESCARGOT_TARGET} PROPERTIES
                           COMPILE_FLAGS "${ESCARGOT_CXXFLAGS} $ENV{CXXFLAGS}"
                           LINK_FLAGS "${ESCARGOT_LDFLAGS}")
ELSEIF (${ESCARGOT_OUTPUT} STREQUAL "static_lib")
    ADD_LIBRARY (${ESCARGOT_TARGET} STATIC ${ESCARGOT_SRC_LIST})

    SET_TARGET_PROPERTIES (${ESCARGOT_TARGET} PROPERTIES
                           COMPILE_FLAGS "${ESCARGOT_CXXFLAGS} $ENV{CXXFLAGS}")
ENDIF()
