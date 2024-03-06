/**
 * @file stdoutOpMonFacility.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <logging/Logging.hpp>
#include "opmonlib/JSonOpMonFacility.hpp"

#include <google/protobuf/util/json_util.h>

#include <iostream>
#include <memory>
#include <string>

namespace dunedaq::opmonlib {

class stdoutOpMonFacility : public JSonOpMonFacility
{
public:
  explicit stdoutOpMonFacility(std::string uri)
    : JSonOpMonFacility(uri) { ; }
  
  void publish(opmon::OpMonEntry && e) const override {
    std::string json;
    google::protobuf::util::MessageToJsonString( e, & json, get_json_options() );
    TLOG() << "OpMon metric: " << json ;
  }

};

} // namespace dunedaq::opmonlib

DEFINE_DUNE_OPMON_FACILITY(dunedaq::opmonlib::stdoutOpMonFacility)
