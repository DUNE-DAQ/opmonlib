/**
 * @file nullOpMonFacility.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/OpMonFacility.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace dunedaq {

  ERS_DECLARE_ISSUE( opmonlib,
		     NullOpMonFacilityInvoked,
		     "Something invoked a null OpMonFacility",
		     ERS_EMPTY )
}

namespace dunedaq::opmonlib {

class nullOpMonFacility : public OpMonFacility
{
public:
  explicit nullOpMonFacility(std::string uri) : OpMonFacility(uri) {;}
  
  void publish(opmon::OpMonEntry && e) const override {
    throw OpMonPublishFailure(ERS_HERE,
			      get_URI(), e.measurement(), e.opmon_id(),
			      NullOpMonFacilityInvoked(ERS_HERE) );
  }

};

} // namespace dunedaq::opmonlib

DEFINE_DUNE_OPMON_FACILITY(dunedaq::opmonlib::nullOpMonFacility)
