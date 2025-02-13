cmake_minimum_required(VERSION 3.12)
project(opmonlib VERSION 2.0.3)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

#find_package(okdalgen REQUIRED)
find_package(cetlib REQUIRED)
find_package(ers REQUIRED)
find_package(logging REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(absl REQUIRED)
find_package(Protobuf REQUIRED CONFIG)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package(utilities)
find_package(confmodel)

set(OPMONLIB_DEPENDENCIES ${CETLIB} ${CETLIB_EXCEPT} ers::ers logging::logging nlohmann_json::nlohmann_json protobuf::libprotobuf absl::log_internal_check_op utilities::utilities confmodel::confmodel )

##############################################################################
# Schemas

daq_protobuf_codegen( *.proto opmon/*.proto )

##############################################################################
# Main library

daq_add_library(*.cpp LINK_LIBRARIES ${OPMONLIB_DEPENDENCIES} )

##############################################################################
# Plugins

daq_add_plugin(stdoutOpMonFacility duneOpMonFacility LINK_LIBRARIES opmonlib)
daq_add_plugin(fileOpMonFacility   duneOpMonFacility LINK_LIBRARIES opmonlib)

##############################################################################
# Integration tests

##############################################################################
# unit tests
daq_add_unit_test( schema_conversion_test   LINK_LIBRARIES opmonlib )
daq_add_unit_test( opmon_facility_test      LINK_LIBRARIES opmonlib )
daq_add_unit_test( monitorable_object_test  LINK_LIBRARIES opmonlib )



##############################################################################

daq_install()

