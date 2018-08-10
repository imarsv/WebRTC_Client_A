cmake_minimum_required(VERSION 3.12)
project(WebRTC_client_1)

set(CMAKE_CXX_STANDARD 11)

# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

#
# libjsoncpp

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
#link_libraries(${JSONCPP_LIBRARIES})
INCLUDE_DIRECTORIES(${JSONCPP_INCLUDE_DIRS})


#
# WebRTC
include_directories(/home/mars/sources/webrtc/src)
include_directories(/home/mars/sources/webrtc/src/third_party/abseil-cpp)
include_directories(/home/mars/sources/webrtc/src/third_party/libyuv/include)

add_definitions(-DV8_DEPRECATION_WARNINGS -DUSE_UDEV -DUSE_AURA=1 -DUSE_NSS_CERTS=1 -DUSE_OZONE=1 -DFULL_SAFE_BROWSING -DSAFE_BROWSING_CSD -DSAFE_BROWSING_DB_LOCAL -DCHROMIUM_BUILD -DFIELDTRIAL_TESTING_ENABLED -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DCR_SYSROOT_HASH=4e7db513b0faeea8fb410f70c9909e8736f5c0ab -D_DEBUG -DDYNAMIC_ANNOTATIONS_ENABLED=1 -DWTF_USE_DYNAMIC_ANNOTATIONS=1 -D_GLIBCXX_DEBUG=1 -DWEBRTC_ENABLE_PROTOBUF=1 -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE -DHAVE_SCTP -DUSE_BUILTIN_SW_CODECS -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0 -DWEBRTC_POSIX -DWEBRTC_LINUX -DABSL_ALLOCATOR_NOTHROW=1 -DHAVE_WEBRTC_VIDEO -DHAVE_WEBRTC_VOICE)

set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations -fno-strict-aliasing --param=ssp-buffer-size=4 -fstack-protector -Wno-builtin-macro-redefined -D__DATE__= -D__TIME__= -D__TIMESTAMP__= -funwind-tables -fPIC -pipe")

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
        peer_connection_client.cc)


# -lX11 -lXcomposite -lXext -lXrender -latomic -ldl -lpthread -lrt -lgmodule-2.0 -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0 -lm
# Link the target to the GTK+ libraries
TARGET_LINK_LIBRARIES(client
        ${GTK3_LIBRARIES}
        /home/mars/sources/webrtc/src/out/Default/obj/libwebrtc.a

        dl
        pthread
        jsoncpp)