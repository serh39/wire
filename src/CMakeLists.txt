set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

file(GLOB_RECURSE LIBWIRE_HEADERS ../include/*.hpp)
file(GLOB LIBWIRE_SOURCES *.cpp internal/*.cpp internal/tcp/*.cpp tcp/*.cpp udp/*.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring for Linux platform")
    set(LIBWIRE_PLATFORM "LINUX")
    file(GLOB_RECURSE LIBWIRE_PLATFORM_SOURCES posix/*.cpp epoll/*.cpp)
elseif(UNIX)
    message(STATUS "Configuring for generic POSIX platform")
    set(LIBWIRE_PLATFORM "POSIX")
    file(GLOB_RECURSE LIBWIRE_PLATFORM_SOURCES posix/*.cpp select/*.cpp)
elseif(WIN32)
    message(STATUS "Configuring for Win32 platform.")
    set(LIBWIRE_PLATFORM "WIN32")
    file(GLOB_RECURSE LIBWIRE_PLATFORM_SOURCES win32/*.cpp)
else()
    message(WARNING "libwire doesn't supports your target platform, trying POISX files anyway...")
    set(LIBWIRE_PLATFORM "POSIX")
endif()

set(LIBWIRE_ALL_SOURCES ${LIBWIRE_SOURCES} ${LIBWIRE_PLATFORM_SOURCES} PARENT_SCOPE)
set(LIBWIRE_ALL_HEADERS ${LIBWIRE_HEADERS} ${LIBWIRE_PLATFORM_HEADERS} PARENT_SCOPE)

add_library(libwire ${LIBWIRE_SOURCES} ${LIBWIRE_PLATFORM_SOURCES})
target_include_directories(libwire PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/>
    )

if(WIN32)
    target_compile_definitions(libwire PUBLIC -DWINVER=0x601 -D_WIN32_WINNT=0x601)
    target_include_directories(libwire PRIVATE ${PROJECT_SOURCE_DIR}/include/libwire_win32)
    target_link_libraries(libwire PUBLIC ws2_32)
endif()

include(GNUInstallDirs)

install(TARGETS libwire
        EXPORT libwire-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
