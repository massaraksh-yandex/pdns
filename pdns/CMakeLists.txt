cmake_minimum_required (VERSION 2.8)
project(pdns-recursor)

set(CMAKE_BUILD_TYPE Debug)

set(SBINDIR "${DESTDIR}/usr/sbin")
set(BINDIR ${DESTDIR}/usr/bin)
set(MANDIR ${DESTDIR}/usr/share/man/man1)
set(SYSCONFDIR ${DESTDIR}/etc/powerdns)
set(LOCALSTATEDIR ${DESTDIR}/var/run)
set(DOCDIR "docs")

find_program(RAGEL ragel)
find_program(XMLTO xmlto) # add asciidoc
find_library(RT_LIBRARY NAMES rt)
find_package(Boost 1.40 REQUIRED)
find_package(Lua51 REQUIRED)

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

message(STATUS "Program 'RAGEL' : ${RAGEL}")
message(STATUS "Program 'XMLTO' : ${XMLTO}")

message(STATUS "DESTDIR: " ${DESTDIR})
message(STATUS "SBINDIR: " ${SBINDIR})
message(STATUS "BINDIR: " ${BINDIR})
message(STATUS "MANDIR: " ${MANDIR})
message(STATUS "SYSCONFDIR: " ${SYSCONFDIR})
message(STATUS "LOCALSTATEDIR: " ${LOCALSTATEDIR})
message(STATUS "Lua headers: " ${LUA_INCLUDE_DIR})
message(STATUS "Boost headers: ${Boost_INCLUDE_DIR}")

add_definitions(-DLOCALSTATEDIR="${LOCALSTATEDIR}")
add_definitions(-DSYSCONFDIR="${SYSCONFDIR}")
add_definitions(-D_GNU_SOURCE -DHAVE_STRCASESTR -DPDNS_ENABLE_LUA -DHAVE_LUA)

set(GENERAL_FLAGS "${GENERAL_FLAGS} -pthread -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GENERAL_FLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GENERAL_FLAGS} -O0 -g")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${GENERAL_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${GENERAL_FLAGS} -O3")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bsymbolic-functions -rdynamic -lrt")

include_directories(ext/rapidjson/include)
include_directories(ext/polarssl-1.3.2/include)
include_directories(${LUA_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})

# Building pdns_recursor
set(RECURSOR pdns_recursor)

if(RAGEL)
    add_custom_command(OUTPUT dnslabeltext.cc COMMAND ragel ARGS dnslabeltext.rl -o dnslabeltext.cc)
else()
    message(WARNING "RAGEL was not found. File dnslabeltext.cc will not be generated. You need to create it manually")
endif()

add_executable(${RECURSOR} syncres.cc  misc.cc unix_utility.cc qtype.cc logger.cc
arguments.cc lwres.cc pdns_recursor.cc recursor_cache.cc dnsparser.cc
dnswriter.cc dnsrecords.cc rcpgenerator.cc base64.cc zoneparser-tng.cc
rec_channel.cc rec_channel_rec.cc selectmplexer.cc sillyrecords.cc
dns_random.cc ext/polarssl-1.3.2/library/aes.c ext/polarssl-1.3.2/library/padlock.c dnslabeltext.cc
lua-pdns.cc lua-recursor.cc randomhelper.cc recpacketcache.cc dns.cc
reczones.cc base32.cc nsecrecords.cc json.cc json_ws.cc epollmplexer.cc
rrl_impl.cc rrl.cc rrl_logger.cc rrl_cleaning.cc rrl_params.cc rrl_map.cc rrl_structures.cc
rrl_functions.cc rrl_config.cc)
target_link_libraries(${RECURSOR} ${LUA_LIBRARIES})
target_link_libraries(${RECURSOR} ${RT_LIBRARY})

# Creating man-pages
if(XMLTO)
    add_custom_command(OUTPUT ${DOCDIR}/pdns_recursor.1 COMMAND make -C docs pdns_recursor.1)
    add_custom_command(OUTPUT ${DOCDIR}/rec_control.1 COMMAND make -C docs rec_control.1)
    add_custom_target(docs ALL DEPENDS ${DOCDIR}/rec_control.1 ${DOCDIR}/pdns_recursor.1)
    install(FILES ${DOCDIR}/pdns_recursor.1 ${DOCDIR}/rec_control.1 DESTINATION ${MANDIR})
else()
    message(WARNING "XMLTO was not found. Man pages will not be generated")
endif()

# Building rec_control
set(REC_CONTROL rec_control)
add_executable(${REC_CONTROL} rec_channel.cc rec_control.cc arguments.cc misc.cc
unix_utility.cc logger.cc qtype.cc)

# Creating config file and places it in the buid directory
add_custom_command(OUTPUT recursor.conf-dist COMMAND ./pdns_recursor --config > recursor.conf-dist)
add_custom_target(config ALL DEPENDS recursor.conf-dist)
add_dependencies(config ${RECURSOR})

install(PROGRAMS ${RECURSOR} DESTINATION ${SBINDIR})
install(PROGRAMS ${REC_CONTROL} DESTINATION ${BINDIR})
install(FILES pdns-recursor_logcheck DESTINATION ${DESTDIR}/etc/logcheck/ignore.d.server RENAME pdns-recursor)
install(FILES pdns-recursor_default DESTINATION ${DESTDIR}/etc/default RENAME pdns-recursor)
install(FILES pdns-recursor.init.d DESTINATION ${DESTDIR}/etc/init.d RENAME pdns-recursor)
install(FILES recursor.conf-dist DESTINATION ${DESTDIR}/etc/powerdns RENAME recursor.conf)
