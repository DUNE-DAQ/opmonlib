/**
 * @file Utils.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_UTILS_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_UTILS_HPP_


#include <string>
#include <list>

#include <google/protobuf/message.h>
#include "opmonlib/opmon_entry.pb.h"

#include <google/protobuf/util/time_util.h>

namespace dunedaq::opmonlib {

  dunedaq::opmon::OpMonEntry to_entry(const google::protobuf::Message & m);

}  // namespace dunedaq::opmonlib

#endif // OPMONLIB_INCLUDE_OPMONLIB_UTILS_HPP_
