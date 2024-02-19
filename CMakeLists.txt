cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CHAIN_TYPE    BTC )
set(MYSQL_TEST    ON )
set(GLOG_TEST     OFF)
set(BUILD_TESTING OFF)

set(BUILD_BTCPOOL_DB OFF)

set(CHAIN_SRC_ROOT /work/bitcoin-0.16.3)

project(MyProject)


###################################### Options ######################################
message("") # empty line
message("------------------------------- Options -------------------------------")

# Package name postfix will be different with some optional feature enabled.
# This option can be defined to change the default package name.
if(NOT POOL__DEB_PACKNAME_POSTFIX)
    set(POOL__DEB_PACKNAME_POSTFIX "")
endif()

#
# Build Type: -DCMAKE_BUILD_TYPE=Debug|Release
# Default: Release
#
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("-- Build Type: ${CMAKE_BUILD_TYPE} (-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})")
IF(CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_CXX_FLAGS "-g -O0 -Wall")
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  set(CMAKE_CXX_FLAGS "-g -O2 -Wall")
ELSE()
  set(CMAKE_CXX_FLAGS "-g -Wall")
ENDIF()

# set C++ standard
set(CMAKE_CXX_STANDARD 17)
# stop building after the first error
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdelete-non-virtual-dtor -Wfatal-errors")

# The number of concurrent jobs when compiling a third-party library
if(JOBS)
  message("-- Concurrent Jobs: ${JOBS} (-DJOBS=${JOBS})")
  set(MAKE_JOBS "-j${JOBS}") 
else()
  message("-- Concurrent Jobs: 1 (-DJOBS=1)")
  set(MAKE_JOBS "") 
endif()

option(USE_CUDA "Use cuda in build" ON)
# Use Nvidia CUDA in build
if(USE_CUDA)
  message("-- Use Nvidia CUDA in build: Enabled (-DUSE_CUDA=ON)")
else()
  message("-- Use Nvidia CUDA in build: Disabled (-DUSE_CUDA=OFF)")
  message("    WARNING: Bytom share checking will be too slow without Nvidia CUDA.")
endif()

#
# Chain Type
#
set(SUPPORTED_CHAIN_TYPES "BTC|BCH|BSV|UBTC|LTC|ZEC")
if(NOT CHAIN_TYPE)
  message(FATAL_ERROR "Chain type not defined! (-DCHAIN_TYPE=${SUPPORTED_CHAIN_TYPES})")
endif()



# add chain type definitions
add_definitions(-DCHAIN_TYPE_${CHAIN_TYPE})
add_definitions(-DCHAIN_TYPE_STR="${CHAIN_TYPE}")

# add a macro that needed by chain's source code
add_definitions(-DHAVE_CONFIG_H)


###
# options for install & package
###

# install prefix
if(NOT POOL__INSTALL_PREFIX)
  set(POOL__INSTALL_PREFIX ${POOL__DEFAULT_INSTALL_PREFIX})
endif()
message("-- Install Prefix: ${POOL__INSTALL_PREFIX} (-DPOOL__INSTALL_PREFIX=${POOL__INSTALL_PREFIX})")
set(CMAKE_INSTALL_PREFIX ${POOL__INSTALL_PREFIX})

# Debian/Ubuntu software package
if(POOL__GENERATE_DEB_PACKAGE)
  message("-- Generate Debian/Ubuntu software package: Enabled (-DPOOL__GENERATE_DEB_PACKAGE=ON)")
else()
  message("-- Generate Debian/Ubuntu software package: Disabled (-DPOOL__GENERATE_DEB_PACKAGE=OFF)")
endif()


###################################### Libs ######################################
message("") # empty line
message("------------------------------- Libs -------------------------------")

#
# chain's libs
#
if(CHAIN_TYPE STREQUAL "BCH")
  set(BITCOIN_LIBRARIES ${BITCOIN_LIBRARIES}
    "${CHAIN_SRC_ROOT}/src/libbitcoin_common.a"
    "${CHAIN_SRC_ROOT}/src/libbitcoin_consensus.a"
    "${CHAIN_SRC_ROOT}/src/libbitcoin_util.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto_base.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto_shani.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto_avx2.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto_sse41.a")

elseif(CHAIN_TYPE STREQUAL "ZEC")
  execute_process(
      COMMAND "${CHAIN_SRC_ROOT}/depends/config.guess"
      OUTPUT_VARIABLE ZCASH_PLATFORM
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(BITCOIN_LIBRARIES
    "${CHAIN_SRC_ROOT}/src/libbitcoin_common.a"
    "${CHAIN_SRC_ROOT}/src/libbitcoin_util.a"
    "${CHAIN_SRC_ROOT}/src/libzcash.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto.a"
    "${CHAIN_SRC_ROOT}/src/snark/libsnark.a"
    "${CHAIN_SRC_ROOT}/depends/${ZCASH_PLATFORM}/lib/libsodium.a"
    "${CHAIN_SRC_ROOT}/depends/${ZCASH_PLATFORM}/lib/librustzcash.a")

else()
  set(BITCOIN_LIBRARIES ${BITCOIN_LIBRARIES}
    "${CHAIN_SRC_ROOT}/src/libbitcoin_common.a"
    "${CHAIN_SRC_ROOT}/src/libbitcoin_consensus.a"
    "${CHAIN_SRC_ROOT}/src/libbitcoin_util.a"
    "${CHAIN_SRC_ROOT}/src/crypto/libbitcoin_crypto.a")
endif()

list(GET BITCOIN_LIBRARIES 0 BITCOIN_LIBRARIE)
if (NOT EXISTS ${BITCOIN_LIBRARIE})
  message(STATUS "building ${CHAIN_NAME}...")

  if(CHAIN_TYPE STREQUAL "ZEC")
    execute_process(WORKING_DIRECTORY "${CHAIN_SRC_ROOT}" COMMAND ./zcutil/build.sh --disable-tests ${MAKE_JOBS})

  else()
    execute_process(WORKING_DIRECTORY "${CHAIN_SRC_ROOT}" COMMAND ./autogen.sh)
  
    if (NOT CHAIN_ENABLE_WALLET)
      execute_process(WORKING_DIRECTORY "${CHAIN_SRC_ROOT}" COMMAND ./configure --with-gui=no --disable-wallet --disable-tests --disable-bench ${CHAIN_EXTRA_FLAGS})
    else()
      execute_process(WORKING_DIRECTORY "${CHAIN_SRC_ROOT}" COMMAND ./configure --with-gui=no --disable-tests --disable-bench ${CHAIN_EXTRA_FLAGS})
    endif()

    execute_process(WORKING_DIRECTORY "${CHAIN_SRC_ROOT}" COMMAND make ${MAKE_JOBS})
  endif()

endif()

foreach(BITCOIN_LIBRARIE ${BITCOIN_LIBRARIES})
  if (NOT EXISTS ${BITCOIN_LIBRARIE})
    message(FATAL_ERROR "${BITCOIN_LIBRARIE} not exists!")
  endif()
endforeach()


#
# libsecp256k1
#
set(secp256k1_DIR "${CHAIN_SRC_ROOT}/src/secp256k1")
set(secp256k1_LIBRARIES "${secp256k1_DIR}/.libs/libsecp256k1.a")
if (NOT EXISTS ${secp256k1_LIBRARIES})
  message(STATUS "build secp256k1...")
  execute_process(WORKING_DIRECTORY "${secp256k1_DIR}" COMMAND ./autogen.sh)
  execute_process(WORKING_DIRECTORY "${secp256k1_DIR}" COMMAND ./configure --enable-module-recovery)
  execute_process(WORKING_DIRECTORY "${secp256k1_DIR}" COMMAND make ${MAKE_JOBS})
endif()
if (NOT EXISTS ${secp256k1_LIBRARIES})
  message(FATAL_ERROR "build secp256k1 failed!")
endif()


#
# other libraries
#
if(CHAIN_TYPE STREQUAL "ZEC")
  FIND_PACKAGE(OpenMP REQUIRED)
endif()

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(LibConfig REQUIRED)
find_package(Glog REQUIRED)
find_package(libzmq REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Boost 1.47.0 COMPONENTS thread filesystem system regex program_options chrono REQUIRED)
find_package(LibPthread REQUIRED)

if(USE_CUDA)
  find_package(CUDA 9.1)
  if(NOT CUDA_FOUND)
    message(WARNING "cuda 9.1 not found! USE_CUDA set to OFF")
    set(USE_CUDA OFF)
  endif()
endif()

# find_package(MYSQL REQUIRED)
# # Force to use discovered OPENSSL_CRYPTO_LIBRARY & OPENSSL_SSL_LIBRARY
# string(REPLACE "-lcrypto" "" MYSQL_LIBRARIES ${MYSQL_LIBRARIES})
# string(REPLACE "-lssl" "" MYSQL_LIBRARIES ${MYSQL_LIBRARIES})
# string(STRIP ${MYSQL_LIBRARIES} MYSQL_LIBRARIES)
# add include directory
# include_directories(3rdparty/mysql/include)
# add library directory
link_directories(3rdparty/mysql/lib64)


########################## Generate version number #############################

# The base version
# The update of this version number may not be timely.
# We will get the version number from the git tag.
SET(BPOOL_VERSION_MAJOR "2")
SET(BPOOL_VERSION_MINOR "3")
SET(BPOOL_VERSION_PATCH "0")

# Get the current abbreviated commit hash of the working branch
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git describe --tag --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE BPOOL_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if (BPOOL_GIT_DESCRIBE)
  message("-- Git describe: ${BPOOL_GIT_DESCRIBE}")

  # Split git describe likes `v2.3.2-0-g2d76329` to list
  # Remove prefix `v`
  string(REGEX REPLACE "^[^0-9]+" "" BPOOL_GIT_DESCRIBE ${BPOOL_GIT_DESCRIBE})
  # Split to list
  string(REPLACE "." ";" BPOOL_GIT_DESCRIBE_LIST ${BPOOL_GIT_DESCRIBE})

  list(GET BPOOL_GIT_DESCRIBE_LIST 0 BPOOL_VERSION_MAJOR)
  list(REMOVE_AT BPOOL_GIT_DESCRIBE_LIST 0)
  list(GET BPOOL_GIT_DESCRIBE_LIST 0 BPOOL_VERSION_MINOR)
  list(REMOVE_AT BPOOL_GIT_DESCRIBE_LIST 0)
  # The tag may be `v2.2.1.1-bitcoin-0-gb9fb89d`
  # So the BPOOL_VERSION_PATCH should be `1.1-bitcoin-0-gb9fb89d`
  # We need to merge the remaining items with `.`
  string(REPLACE ";" "." BPOOL_VERSION_PATCH "${BPOOL_GIT_DESCRIBE_LIST}")

elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  message("-- Git describe is empty, the base version will be used.")
  message("-- It is recommended to add a tag to the current commit.")
  execute_process(
      COMMAND git log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE BPOOL_GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(BPOOL_GIT_COMMIT_HASH)
    set(BPOOL_VERSION_PATCH "${BPOOL_VERSION_PATCH}-${BPOOL_GIT_COMMIT_HASH}")
  endif()
endif()

set(BPOOL_VERSION_STR "${BPOOL_VERSION_MAJOR}.${BPOOL_VERSION_MINOR}.${BPOOL_VERSION_PATCH}")

message("-- version string: ${BPOOL_VERSION_STR}")

configure_file(src/config/bpool-version.h.in src/config/bpool-version.h)


# package version of cpack
SET(CPACK_PACKAGE_VERSION_MAJOR "${BPOOL_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${BPOOL_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${BPOOL_VERSION_PATCH}")


###################################### Targets ######################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/src
  3rdparty
  src
  test
  ${CHAIN_SRC_ROOT}/src
  ${CHAIN_SRC_ROOT}/src/config
  ${CHAIN_SRC_ROOT}/src/secp256k1/include
  ${LIBZMQ_INCLUDE_DIR}
  3rdparty/mysql/include)

set(THIRD_LIBRARIES
        gtest pthread
        mysqlcppconn
        ${LIBZMQ_LIBRARIES}         
        ${GLOG_LIBRARIES}
 )

if(MYSQL_TEST)
    file(GLOB_RECURSE MYSQL_SOURCES src/mysqltest/*.cc)
    add_executable(mysqlTest ${MYSQL_SOURCES})
    target_link_libraries(mysqlTest ${THIRD_LIBRARIES})
endif()

if(GLOG_TEST)
    file(GLOB_RECURSE GLOG_SOURCES src/glogtest/GlogTest.cc)
    add_executable(glogTest ${GLOG_SOURCES})
    target_link_libraries(glogTest ${THIRD_LIBRARIES})
endif()

if(BUILD_BTCPOOL_DB)
    file(GLOB_RECURSE BTCPOOL_DB_SOURCES src/*.cc)
    add_executable(btcpooldbtest ${BTCPOOL_DB_SOURCES})
    target_link_libraries(btcpooldbtest ${THIRD_LIBRARIES})
endif()

if(BUILD_TESTING)
    file(GLOB_RECURSE TEST_SOURCES test/*.cc)
    add_executable(unittest ${TEST_SOURCES})
    target_link_libraries(unittest ${THIRD_LIBRARIES})
endif()