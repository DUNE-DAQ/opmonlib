cmake_minimum_required(VERSION 3.12)
project(opmonlib VERSION 1.3.6)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(cetlib REQUIRED)
find_package(ers REQUIRED)
find_package(logging REQUIRED)
find_package(nlohmann_json REQUIRED)

set(OPMONLIB_DEPENDENCIES ${CETLIB} ${CETLIB_EXCEPT} ers::ers logging::logging nlohmann_json::nlohmann_json)

##############################################################################
# Main library

daq_add_library(*.cpp LINK_LIBRARIES ${OPMONLIB_DEPENDENCIES})

##############################################################################
# Plugins

daq_add_plugin(stdoutOpmonService duneOpmonService LINK_LIBRARIES opmonlib)
daq_add_plugin(fileOpmonService duneOpmonService LINK_LIBRARIES opmonlib)

##############################################################################
# Integration tests

#daq_add_application(opmonlib_test opmonlib_test.cpp TEST LINK_LIBRARIES opmonlib)

##############################################################################
# No unit tests written

##############################################################################

daq_install()

