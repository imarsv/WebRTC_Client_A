cmake_minimum_required(VERSION 3.12)
project(WebRTC_client_1)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})

#
# libjsoncpp

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})
include_directories(${JSONCPP_INCLUDE_DIRS})


#
# WebRTC
include_directories(/home/mars/sources/webrtc/src)
include_directories(/home/mars/sources/webrtc/src/third_party/abseil-cpp)
include_directories(/home/mars/sources/webrtc/src/third_party/libyuv/include)

add_definitions(-DV8_DEPRECATION_WARNINGS -DUSE_UDEV -DUSE_AURA=1 -DUSE_NSS_CERTS=1 -DUSE_OZONE=1 -DFULL_SAFE_BROWSING -DSAFE_BROWSING_CSD -DSAFE_BROWSING_DB_LOCAL -DCHROMIUM_BUILD -DFIELDTRIAL_TESTING_ENABLED -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DCR_SYSROOT_HASH=4e7db513b0faeea8fb410f70c9909e8736f5c0ab -D_DEBUG -DDYNAMIC_ANNOTATIONS_ENABLED=1 -DWTF_USE_DYNAMIC_ANNOTATIONS=1 -DWEBRTC_ENABLE_PROTOBUF=1 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DHAVE_SCTP -DUSE_BUILTIN_SW_CODECS -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0 -DWEBRTC_POSIX -DWEBRTC_LINUX -DABSL_ALLOCATOR_NOTHROW=1 -DHAVE_WEBRTC_VIDEO -DHAVE_WEBRTC_VOICE)
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations -fno-strict-aliasing --param=ssp-buffer-size=4 -fstack-protector -Wno-builtin-macro-redefined -D__DATE__= -D__TIME__= -D__TIMESTAMP__= -funwind-tables -fPIC -pipe")

#
# Mongoose
add_definitions(-DMG_ENABLE_SSL -DMG_ENABLE_HTTP_WEBSOCKET)

add_executable(client
        main.cpp

        flagdefs.h

        main_main_wnd.h
        main_wnd.h
        main_wnd.cc

        defaults.h
        defaults.cc

        conductor.h
        conductor.cc

        peer_connection_client.h
        peer_connection_client.cc

        mongoose.c
        mongoose.h SignalingManager.cpp SignalingManager.h)

target_link_libraries(client
        ${GTK3_LIBRARIES}
        /home/mars/sources/webrtc/src/out/Default/obj/libwebrtc.a

        dl
        pthread
        ${JSONCPP_LIBRARIES}
        ssl
        crypto)