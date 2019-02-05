# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             qosmos_native

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/checksum.c
             src/main/cpp/checksum_test.c
             src/main/cpp/code.c
             src/main/cpp/config.c
             src/main/cpp/epoll.c
             src/main/cpp/file.c
             src/main/cpp/fmemopen.c
             src/main/cpp/gre_packet.c
             src/main/cpp/hash.c
             src/main/cpp/hash_map.c
             src/main/cpp/icmp_packet.c
             src/main/cpp/ip_address.c
             src/main/cpp/ip_packet.c
             src/main/cpp/ip_prefix.c
             src/main/cpp/link_layer.c
             src/main/cpp/logging.c
             src/main/cpp/mpls_packet.c
             src/main/cpp/net_utils.c
             src/main/cpp/netdev.c
             src/main/cpp/open_memstream.c
             src/main/cpp/packet.c
             src/main/cpp/packet_checksum.c
             src/main/cpp/packet_parser.c
             src/main/cpp/packet_parser_test.c
             src/main/cpp/packet_socket_linux.c
             src/main/cpp/packet_socket_pcap.c
             src/main/cpp/packet_to_string.c
             src/main/cpp/packet_to_string_test.c
             src/main/cpp/packetdrill.c
             src/main/cpp/pipe.c
             src/main/cpp/run.c
             src/main/cpp/run_command.c
             src/main/cpp/run_packet.c
             src/main/cpp/run_system_call.c
             src/main/cpp/script.c
             src/main/cpp/so_testing.c
             src/main/cpp/socket.c
             src/main/cpp/symbols_freebsd.c
             src/main/cpp/symbols_linux.c
             src/main/cpp/symbols_netbsd.c
             src/main/cpp/symbols_openbsd.c
             src/main/cpp/system.c
             src/main/cpp/tcp_options.c
             src/main/cpp/tcp_options_iterator.c
             src/main/cpp/tcp_options_to_string.c
             src/main/cpp/tcp_packet.c
             src/main/cpp/types.c
             src/main/cpp/udp_packet.c
             src/main/cpp/wire_client.c
             src/main/cpp/wire_client_netdev.c
             src/main/cpp/wire_conn.c
             src/main/cpp/wire_protocol.c
             src/main/cpp/wire_server.c
             src/main/cpp/wire_server_netdev.c
             src/main/cpp/wrap.c
             src/main/cpp/flowmaps.c
             src/main/cpp/qosmos_native.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       qosmos_native

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )