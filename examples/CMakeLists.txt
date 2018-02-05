macro(libwire_example namespace name)
    add_executable(${namespace}-${name} ${ARGN})
    target_link_libraries(${namespace}-${name} libwire)
    add_dependencies(examples ${namespace}-${name})
    if(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR}/examples)
        set_target_properties(${namespace}-${name}
            PROPERTIES OUTPUT_NAME ${name})
    endif()
endmacro()

add_custom_target(examples)

add_subdirectory(tcp/)
add_subdirectory(udp/)

libwire_example(dns query dns_query.cpp)

