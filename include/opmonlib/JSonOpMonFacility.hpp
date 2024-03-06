/**
 * @file JSonOpMonFacility.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_JSONOPMONFACILITY_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_JSONOPMONFACILITY_HPP_

#include "opmonlib/OpMonFacility.hpp"
#include <google/protobuf/util/json_util.h>


namespace dunedaq::opmonlib {

class JSonOpMonFacility : public OpMonFacility
{
public:
  explicit JSonOpMonFacility(std::string uri)
    : OpMonFacility(uri) {

    m_opt.add_whitespace = true;
    m_opt.preserve_proto_field_names = true;
  }
  
protected:
  const auto & get_json_options() const { return m_opt; }
  
private:
  google::protobuf::util::JsonPrintOptions m_opt;
  
};

} // namespace dunedaq::opmonlib

#endif //OPMONLIB_INCLUDE_OPMONLIB_JSONOPMONFACILITY_HPP_

