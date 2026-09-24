#ifndef OPMONLIB_SRC_NULLOPMONFACILITY_HPP_
#define OPMONLIB_SRC_NULLOPMONFACILITY_HPP_

#include "opmonlib/OpMonFacility.hpp"
#include "opmonlib/Utils.hpp"

namespace dunedaq {

ERS_DECLARE_ISSUE(opmonlib, NullOpMonFacilityInvoked, "Something invoked a null OpMonFacility", ERS_EMPTY)
}

namespace dunedaq::opmonlib {

class NullOpMonFacility : public OpMonFacility
{

public:
  NullOpMonFacility()
    : OpMonFacility("Null://")
  {
    ;
  }

  void publish(opmon::OpMonEntry&& e) const override
  {
    throw OpMonPublishFailure(ERS_HERE,
                              get_URI(),
                              e.measurement(),
                              dunedaq::opmonlib::to_string(e.origin()),
                              NullOpMonFacilityInvoked(ERS_HERE));
  }
};

} // namespace dunedaq::opmonlib

#endif // OPMONLIB_SRC_NULLOPMONFACILITY_HPP_
