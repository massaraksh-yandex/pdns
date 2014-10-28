cmake_minimum_required (VERSION 2.8)
project(pdns-recursor)

set(CMAKE_BUILD_TYPE Release)

set(SBINDIR "${DESTDIR}/usr/sbin")
set(BINDIR ${DESTDIR}/usr/bin)
set(MANDIR ${DESTDIR}/usr/share/man/man1)
set(SYSCONFDIR ${DESTDIR}/etc/powerdns)
set(LOCALSTATEDIR ${DESTDIR}/var/run)
set(DOCDIR "docs")

message(STATUS "DESTDIR: " ${DESTDIR})
message(STATUS "SBINDIR: " ${SBINDIR})
message(STATUS "BINDIR: " ${BINDIR})
message(STATUS "MANDIR: " ${MANDIR})
message(STATUS "SYSCONFDIR: " ${SYSCONFDIR})
message(STATUS "LOCALSTATEDIR: " ${LOCALSTATEDIR})

find_library(LUA_LIBRARY NAMES lua5.1)

add_definitions(-DLOCALSTATEDIR="${LOCALSTATEDIR}")
add_definitions(-DSYSCONFDIR="${SYSCONFDIR}")
add_definitions(-DVERSION="3.5.2-with_rrl")
add_definitions(-D_GNU_SOURCE -DHAVE_STRCASESTR -DPDNS_ENABLE_LUA -DHAVE_LUA)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -pthread -Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pthread -Wall -g")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -pthread -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -pthread -Wall")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bsymbolic-functions -rdynamic")

include_directories(ext/rapidjson/include)
include_directories(ext/polarssl-1.3.2/include)
include_directories(/usr/include/lua5.1)
include_directories(${BOOST_DIR})

set(RECURSOR pdns_recursor)
add_executable(${RECURSOR} syncres.cc  misc.cc unix_utility.cc qtype.cc logger.cc
arguments.cc lwres.cc pdns_recursor.cc recursor_cache.cc dnsparser.cc
dnswriter.cc dnsrecords.cc rcpgenerator.cc base64.cc zoneparser-tng.cc
rec_channel.cc rec_channel_rec.cc selectmplexer.cc sillyrecords.cc
dns_random.cc ext/polarssl-1.3.2/library/aes.c ext/polarssl-1.3.2/library/padlock.c dnslabeltext.cc
lua-pdns.cc lua-recursor.cc randomhelper.cc recpacketcache.cc dns.cc
reczones.cc base32.cc nsecrecords.cc json.cc rrl_impl.cc rrl.cc json_ws.cc epollmplexer.cc)
target_link_libraries(${RECURSOR} ${LUA_LIBRARY})
add_custom_command(OUTPUT dnslabeltext.cc COMMAND ragel ARGS dnslabeltext.rl -o dnslabeltext.cc)
add_custom_command(OUTPUT pdns_recursor COMMAND make ARGS -C docs pdns_recursor.1)
add_custom_command(OUTPUT rec_control COMMAND make ARGS -C docs rec_control.1)

add_custom_command(OUTPUT dnslabeltext.cc COMMAND ragel ARGS dnslabeltext.rl -o dnslabeltext.cc)

add_custom_command(OUTPUT ${DOCDIR}/pdns_recursor.1 COMMAND make -C docs pdns_recursor.1)
add_custom_command(OUTPUT ${DOCDIR}/rec_control.1 COMMAND make -C docs rec_control.1)
add_custom_target(docs ALL DEPENDS ${DOCDIR}/rec_control.1 ${DOCDIR}/pdns_recursor.1)

set(REC_CONTROL rec_control)
add_executable(${REC_CONTROL} rec_channel.cc rec_control.cc arguments.cc misc.cc
unix_utility.cc logger.cc qtype.cc)

install(PROGRAMS ${RECURSOR} DESTINATION ${SBINDIR})
install(PROGRAMS ${REC_CONTROL} DESTINATION ${BINDIR})
add_custom_command(TARGET ${RECURSOR} POST_BUILD COMMAND ${SBINDIR}/pdns_recursor --config > recursor.conf-dist)
install(FILES ${DOCDIR}/pdns_recursor.1 ${DOCDIR}/rec_control.1 DESTINATION ${MANDIR})
install(FILES pdns-recursor_logcheck DESTINATION ${DESTDIR}/etc/logcheck/ignore.d.server RENAME pdns-recursor)
install(FILES pdns-recursor_default DESTINATION ${DESTDIR}/etc/default RENAME pdns-recursor)
install(FILES pdns-recursor.init.d DESTINATION ${DESTDIR}/etc/init.d RENAME pdns-recursor)
install(FILES recursor.conf-dist DESTINATION ${SYSCONFDIR} RENAME recursor.conf)
