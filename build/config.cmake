CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# CONFIGURATION
#######################################################
SET (ESCARGOT_CXXFLAGS_CONFIG)
SET (ESCARGOT_LDFLAGS_CONFIG)

IF (${ESCARGOT_HOST} STREQUAL "linux")
    SET (CMAKE_C_COMPILER gcc)
    SET (CMAKE_CXX_COMPILER g++)
    SET (ARFLAGS)

ELSEIF (${ESCARGOT_HOST} STREQUAL "android")
    SET (COMPILER_PREFIX "arm-linux-androideabi")
    SET (CMAKE_C_COMPILER ${ANDROID_NDK_STANDALONE}/bin/${COMPILER_PREFIX}-gcc)
    SET (CMAKE_CXX_COMPILER ${ANDROID_NDK_STANDALONE}/bin/${COMPILER_PREFIX}-g++)
    SET (LINK ${ANDROID_NDK_STANDALONE}/bin/${COMPILER_PREFIX}-g++)
    SET (LD ${ANDROID_NDK_STANDALONE}/bin/${COMPILER_PREFIX}-ld)
    SET (AR ${ANDROID_NDK_STANDALONE}/bin/${COMPILER_PREFIX}-ar)

    SET (ESCARGOT_CXXFLAGS_CONFIG "--sysroot=${ANDROID_NDK_STANDALONE}/sysroot")
    SET (ESCARGOT_LDFLAGS_CONFIG  "--sysroot=${ANDROID_NDK_STANDALONE}/sysroot")
    SET (ARFLAGS)

ELSEIF (${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (CMAKE_C_COMPILER gcc)
    SET (CMAKE_CXX_COMPILER g++)
    IF ("${LTO}" EQUAL 1)
#TODO
        SET (ARFLAGS "--plugin=/usr/lib/bfd-plugins/liblto_plugin.so")
    ELSE()
        SET (ARFLAGS)
    ENDIF()

ELSEIF (${ESCARGOT_HOST} MATCHES "tizen")
    IF (NOT DEFINED ${TIZEN_SDK_HOME})
        MESSAGE (FATAL_ERROR "TIZEN_SDK_HOME must be set")
    ENDIF()

    IF (${ESCARGOT_HOST} MATCHES "mobile")
        IF (${ESCARGOT_ARCH} STREQUAL "arm")
            SET (TIZEN_SYSROOT "${TIZEN_SDK_HOME}/platforms/tizen-${TIZEN_VERSION}/mobile/rootstraps/mobile-${TIZEN_VERSION}-device.core")
        ELSEIF (${ESCARGOT_ARCH} STREQUAL "i386")
            SET (TIZEN_SYSROOT "${TIZEN_SDK_HOME}/platforms/tizen-${TIZEN_VERSION}/mobile/rootstraps/mobile-${TIZEN_VERSION}-emulator.core")
        ENDIF()
    ELSEIF (${ESCARGOT_HOST} MATCHES "wearable")
        IF (${ESCARGOT_ARCH} STREQUAL "arm")
            SET (TIZEN_SYSROOT "${TIZEN_SDK_HOME}/platforms/tizen-${TIZEN_VERSION}/wearable/rootstraps/wearable-${TIZEN_VERSION}-device.core")
        ELSEIF (${ESCARGOT_ARCH} STREQUAL "i386")
            SET (TIZEN_SYSROOT "${TIZEN_SDK_HOME}/platforms/tizen-${TIZEN_VERSION}/wearable/rootstraps/wearable-${TIZEN_VERSION}-emulator.core")
        ENDIF()
    ENDIF()

    SET (COMPILER_PREFIX "${ESCARGOT_ARCH}-linux-gnueabi")
    SET (CMAKE_C_COMPILER ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-gcc)
    SET (CMAKE_CXX_COMPILER ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-g++)
    SET (LINK ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-g++)
    SET (LD ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-ld)
    IF (${LTO} EQUAL 1)
      SET (AR ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-gcc-ar)
#TODO
      SET (ARFLAGS "--plugin=${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/libexec/gcc/${COMPILER_PREFIX}/4.6.4/liblto_plugin.so")
    ELSE()
      SET (AR ${TIZEN_SDK_HOME}/tools/${COMPILER_PREFIX}-gcc-4.6/bin/${COMPILER_PREFIX}-ar)
      SET (ARFLAGS)
    ENDIF()

    SET (ESCARGOT_CXXFLAGS_CONFIG "--sysroot=${TIZEN_SYSROOT}")
    SET (ESCARGOT_LDFLAGS_CONFIG "--sysroot=${TIZEN_SYSROOT}")
ENDIF()


#######################################################
# PATH
#######################################################
SET (ESCARGOT_ROOT ${PROJECT_SOURCE_DIR})
SET (ESCARGOT_THIRD_PARTY_ROOT ${ESCARGOT_ROOT}/third_party)
SET (GCUTIL_ROOT ${ESCARGOT_THIRD_PARTY_ROOT}/GCutil)

SET (ESCARGOT_OUTDIR ${ESCARGOT_ROOT}/out/${ESCARGOT_HOST}/${ESCARGOT_ARCH}/${ESCARGOT_TYPE}/${ESCARGOT_MODE})

IF (ESCARGOT_OUTPUT STREQUAL "bin")
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ESCARGOT_OUTDIR}/)
    SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ESCARGOT_OUTDIR}/lib)
    SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ESCARGOT_OUTDIR}/lib)
ENDIF()


#######################################################
# FLAGS FOR COMMON
#######################################################
# ESCARGOT COMMON CXXFLAGS
SET (ESCARGOT_CXXFLAGS_COMMON "$ENV{CXXFLAGS}")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -DESCARGOT")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -std=c++0x -g3")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -fno-math-errno -I${ESCARGOT_ROOT}/src/")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -fdata-sections -ffunction-sections")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -frounding-math -fsignaling-nans")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -fno-omit-frame-pointer")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -fvisibility=hidden")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -Wno-deprecated-declarations")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -Wno-implicit-fallthrough")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -DESCARGOT_ENABLE_TYPEDARRAY")
SET (ESCARGOT_CXXFLAGS_COMMON "${ESCARGOT_CXXFLAGS_COMMON} -DESCARGOT_ENABLE_PROMISE")


# ESCARGOT COMMON LDFLAGS
SET (ESCARGOT_LDFLAGS_COMMON "-fvisibility=hidden")


# THIRD_PARTY COMMON CXXFLAGS
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY)

# bdwgc
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${GCUTIL_ROOT}/bdwgc/include/")
IF (${ESCARGOT_MODE} STREQUAL "debug")
    SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -DGC_DEBUG")
ENDIF()

# GCutil
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${GCUTIL_ROOT}/")

# checked arithmetic
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${ESCARGOT_THIRD_PARTY_ROOT}/checked_arithmetic/")

# v8's fast-dtoa
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${ESCARGOT_THIRD_PARTY_ROOT}/double_conversion/")

# rapidjson
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${ESCARGOT_THIRD_PARTY_ROOT}/rapidjson/include/")

# yarr
SET (ESCARGOT_CXXFLAGS_THIRD_PARTY "${ESCARGOT_CXXFLAGS_THIRD_PARTY} -I${ESCARGOT_THIRD_PARTY_ROOT}/yarr/")


#######################################################
# FLAGS FOR $(ESCARGOT_HOST)
#######################################################
FIND_PACKAGE (PkgConfig REQUIRED)

# LINUX CXXFLAGS
SET (ESCARGOT_CXXFLAGS_LINUX)
SET (ESCARGOT_CXXFLAGS_LINUX "${ESCARGOT_CXXFLAGS_LINUX} -fno-rtti")
SET (ESCARGOT_CXXFLAGS_LINUX "${ESCARGOT_CXXFLAGS_LINUX} -DENABLE_ICU -DENABLE_INTL")


# LINUX LDFLAGS
SET (ESCARGOT_LDFLAGS_LINUX)
SET (ESCARGOT_LDFLAGS_LINUX "${ESCARGOT_LDFLAGS_LINUX} -lpthread -lrt")


# LINUX LIBRARIES
SET (ESCARGOT_LIBRARIES_LINUX)
SET (ESCARGOT_LIBDIRS_LINUX)
IF (${ESCARGOT_ARCH} STREQUAL "x64" OR ${ESCARGOT_ARCH} STREQUAL "x86")
    PKG_CHECK_MODULES (ICUI18N REQUIRED icu-i18n)
    PKG_CHECK_MODULES (ICUUC REQUIRED icu-uc)
    SET (ESCARGOT_LIBRARIES_LINUX ${ESCARGOT_LIBRARIES_LINUX} ${ICUI18N_LIBRARIES} ${ICUUC_LIBRARIES})
    SET (ESCARGOT_LIBDIRS_LINUX "${ESCARGOT_LIBDIRS_LINUX} ${ICUI18N_INCLUDE_DIRS} ${ICUUC_INCLUDE_DIRS}")
    SET (ESCARGOT_CXXFLAGS_LINUX "${ESCARGOT_CXXFLAGS_LINUX} ${ICUI18N_CFLAGS_OTHER} ${ICUUC_CFLAGS_OTHER}")
ENDIF()

# TIZEN CXXFLAGS
SET (ESCARGOT_CXXFLAGS_TIZEN)
SET (ESCARGOT_CXXFLAGS_TIZEN "${ESCARGOT_CXXFLAGS_TIZEN} -DESCARGOT_SMALL_CONFIG=1 -DESCARGOT_TIZEN")
IF (${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (ESCARGOT_CXXFLAGS_TIZEN "${ESCARGOT_CXXFLAGS_TIZEN} -DENABLE_ICU -DENABLE_INTL")
ELSEIF (${ESCARGOT_HOST} MATCHES "tizen_")
    SET (ESCARGOT_CXXFLAGS_TIZEN "${ESCARGOT_CXXFLAGS_TIZEN} -DENABLE_ICU -DENABLE_INTL")
    IF (${ESCARGOT_ARCH} STREQUAL "arm" OR ${ESCARGOT_ARCH} STREQUAL "i386")
        SET (ESCARGOT_CXXFLAGS_TIZEN "${ESCARGOT_CXXFLAGS_TIZEN} -I${ESCARGOT_ROOT}/deps/tizen/include")
    ENDIF()
ENDIF()


# TIZEN LDFLAGS
SET (ESCARGOT_LDFLAGS_TIZEN)
SET (ESCARGOT_LDFLAGS_TIZEN "${ESCARGOT_LDFLAGS_TIZEN} -lpthread -lrt")
IF (${ESCARGOT_HOST} MATCHES "tizen_" AND NOT ${ESCARGOT_HOST} STREQUAL "tizen_obs")
    IF (${ESCARGOT_ARCH} STREQUAL "arm")
        SET (ESCARGOT_LDFLAGS_TIZEN "${ESCARGOT_LDFLAGS_TIZEN} -Ldeps/tizen/lib/tizen-wearable-${TIZEN_VERSION}-target-arm")
        SET (ESCARGOT_LDFLAGS_TIZEN "${ESCARGOT_LDFLAGS_TIZEN} -licuio -licui18n -licuuc -licudata")
    ELSEIF (${ESCARGOT_ARCH} STREQUAL "i386")
        SET (ESCARGOT_LDFLAGS_TIZEN "${ESCARGOT_LDFLAGS_TIZEN} -Ldeps/tizen/lib/tizen-wearable-${TIZEN_VERSION}-emulator-x86")
        SET (ESCARGOT_LDFLAGS_TIZEN "${ESCARGOT_LDFLAGS_TIZEN} -licuio -licui18n -licuuc -licudata")
    ENDIF()
ENDIF()


# TIZEN LIBRARIES
SET (ESCARGOT_LIBRARIES_TIZEN)
SET (ESCARGOT_LIBDIRS_TIZEN)
IF (${ESCARGOT_HOST} STREQUAL "tizen_obs")
    PKG_CHECK_MODULES (DLOG REQUIRED dlog)
    PKG_CHECK_MODULES (ICUI18N REQUIRED icu-i18n)
    PKG_CHECK_MODULES (ICUUC REQUIRED icu-uc)
    SET (ESCARGOT_LIBRARIES_TIZEN ${ESCARGOT_LIBRARIES_TIZEN} ${DLOG_LIBRARIES} ${ICUI18N_LIBRARIES} ${ICUUC_LIBRARIES})
    SET (ESCARGOT_LIBDIRS_TIZEN "${ESCARGOT_LIBDIRS_TIZEN} ${DLOG_INCLUDE_DIRS} ${ICUI18N_INCLUDE_DIRS} ${ICUUC_INCLUDE_DIRS}")
    SET (ESCARGOT_CXXFLAGS_TIZEN "${ESCARGOT_CXXFLAGS_TIZEN} ${DLOG_CFLAGS_OTHER} ${ICUI18N_CFLAGS_OTHER} ${ICUUC_CFLAGS_OTHER}")
ENDIF()


# ANDROID CXXFLAGS
SET (ESCARGOT_CXXFLAGS_ANDROID)
SET (ESCARGOT_CXXFLAGS_ANDROID "${ESCARGOT_CXXFLAGS_ANDROID} -fPIE -mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=neon -DANDROID=1")
IF ("${REACT_NATIVE}" EQUAL 1)
    SET (ESCARGOT_CXXFLAGS_ANDROID "${ESCARGOT_CXXFLAGS_ANDROID} -UESCARGOT_ENABLE_PROMISE")
    SET (ESCARGOT_CXXFLAGS_ANDROID "${ESCARGOT_CXXFLAGS_ANDROID} -frtti -std=c++11")
ENDIF()


# ANDROID LDFLAGS
SET (ESCARGOT_LDFLAGS_ANDROID)
SET (ESCARGOT_LDFLAGS_ANDROID "${ESCARGOT_LDFLAGS_ANDROID} -fPIE -pie  -march=armv7-a -Wl,--fix-cortex-a8 -llog")
IF (${ESCARGOT_OUTPUT} STREQUAL "shared_lib")
    SET (ESCARGOT_LDFLAGS_ANDROID "${ESCARGOT_LDFLAGS_ANDROID} -shared")
ENDIF()


#######################################################
# FLAGS FOR $(ARCH) : x64/x86/arm
#######################################################
# x64 CXXFLAGS
SET (ESCARGOT_CXXFLAGS_X64)
SET (ESCARGOT_CXXFLAGS_X64 "${ESCARGOT_CXXFLAGS_X64} -DESCARGOT_64=1")


# x64 LDFLAGS
SET (ESCARGOT_LDFLAGS_X64)


# x86 CXXFLAGS
SET (ESCARGOT_CXXFLAGS_X86)
SET (ESCARGOT_CXXFLAGS_X86 "${ESCARGOT_CXXFLAGS_X86} -DESCARGOT_32=1")
IF (NOT ${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (ESCARGOT_CXXFLAGS_X86 "${ESCARGOT_CXXFLAGS_X86} -m32 -mfpmath=sse -msse -msse2")
ENDIF()


# x86 LDFLAGS
SET (ESCARGOT_LDFLAGS_X86)
IF (NOT ${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (ESCARGOT_LDFLAGS_X86 "${ESCARGOT_LDFLAGS_X86} -m32")
ENDIF()


# arm CXXFLAGS
SET (ESCARGOT_CXXFLAGS_ARM)
SET (ESCARGOT_CXXFLAGS_ARM "${ESCARGOT_CXXFLAGS_ARM} -DESCARGOT_32=1")
IF (NOT ${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (ESCARGOT_CXXFLAGS_ARM "${ESCARGOT_CXXFLAGS_ARM} -march=armv7-a -mthumb")
ENDIF()


# arm LDFLAGS
SET (ESCARGOT_LDFLAGS_ARM)


# aarch64 CXXFLAGS
SET (ESCARGOT_CXXFLAGS_AARCH64)
SET (ESCARGOT_CXXFLAGS_AARCH64 "${ESCARGOT_CXXFLAGS_AARCH64} -DESCARGOT_64=1")


#######################################################
# flags for $(MODE) : debug/release
#######################################################
# DEBUG CXXFLAGS
SET (ESCARGOT_CXXFLAGS_DEBUG)
SET (ESCARGOT_CXXFLAGS_DEBUG "${ESCARGOT_CXXFLAGS_DEBUG} -O0 -D_GLIBCXX_DEBUG -Wall -Wextra -Werror")
IF (${ESCARGOT_HOST} STREQUAL "tizen_obs")
    SET (ESCARGOT_CXXFLAGS_DEBUG "${ESCARGOT_CXXFLAGS_DEBUG} -O1")
ENDIF()


# DEBUG LDFLAGS
SET (ESCARGOT_LDFLAGS_DEBUG)


# RELEASE CXXFLAGS
SET (ESCARGOT_CXXFLAGS_RELEASE)
SET (ESCARGOT_CXXFLAGS_RELEASE "${ESCARGOT_CXXFLAGS_RELEASE} -O2 -DNDEBUG -fno-stack-protector")
IF (${ESCARGOT_HOST} MATCHES "tizen")
    SET (ESCARGOT_CXXFLAGS_RELEASE "${ESCARGOT_CXXFLAGS_RELEASE} -Os -finline-limit=64")
ENDIF()


# RELEASE LDFLAGS
SET (ESCARGOT_LDFLAGS_RELEASE)


#######################################################
# FLAGS FOR $(ESCARGOT_OUTPUT) : bin/shared_lib/static_lib
#######################################################
# BIN CXXFLAGS
SET (ESCARGOT_CXXFLAGS_BIN)
SET (ESCARGOT_CXXFLAGS_BIN "${ESCARGOT_CXXFLAGS_BIN} -DESCARGOT_STANDALONE")
SET (ESCARGOT_CXXFLAGS_BIN "${ESCARGOT_CXXFLAGS_BIN} -DESCARGOT_SHELL")


# BIN LDFLAGS
SET (ESCARGOT_LDFLAGS_BIN)
SET (ESCARGOT_LDFLAGS_BIN "${ESCARGOT_LDFLAGS_BIN} -Wl,--gc-sections")


# SHARED_LIB CXXFLAGS
SET (ESCARGOT_CXXFLAGS_SHAREDLIB)
SET (ESCARGOT_CXXFLAGS_SHAREDLIB "${ESCARGOT_CXXFLAGS_SHAREDLIB} -fPIC")


# SHARED_LIB LDFLAGS
SET (ESCARGOT_LDFLAGS_SHAREDLIB)
SET (ESCARGOT_LDFLAGS_SHAREDLIB "${ESCARGOT_LDFLAGS_SHAREDLIB} -ldl")


# STATIC_LIB CXXFLAGS
SET (ESCARGOT_CXXFLAGS_STATICLIB)
SET (ESCARGOT_CXXFLAGS_STATICLIB "${ESCARGOT_CXXFLAGS_STATICLIB} -fPIC")


# STATIC_LIB LDFLAGS
SET (ESCARGOT_LDFLAGS_STATICLIB)
SET (ESCARGOT_LDFLAGS_STATICLIB "${ESCARGOT_LDFLAGS_STATICLIB} -Wl,--gc-sections")


#######################################################
# FLAGS FOR LTO
#######################################################
# LTO CXXFLAGS
SET (ESCARGOT_CXXFLAGS_LTO)
SET (ESCARGOT_CXXFLAGS_LTO "${ESCARGOT_CXXFLAGS_LTO} -flto -ffat-lto-objects")

# LTO LDFLAGS
SET (ESCARGOT_LDFLAGS_LTO)
SET (ESCARGOT_LDFLAGS_LTO "${ESCARGOT_LDFLAGS_LTO} -flto")


#######################################################
# FLAGS FOR TEST
#######################################################
SET (ESCARGOT_CXXFLAGS_VENDORTEST)
SET (ESCARGOT_CXXFLAGS_VENDORTEST "${ESCARGOT_CXXFLAGS_VENDORTEST} -DESCARGOT_ENABLE_VENDORTEST")
