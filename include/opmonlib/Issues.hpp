/**
 * @file Issues.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_ISSUES_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_ISSUES_HPP_

#include <ers/Issue.hpp>
#include <string>

namespace dunedaq {

ERS_DECLARE_ISSUE(opmonlib,
		  OpmonServiceCreationFailed,
		  "OpmonServiceCreationFailed: " << error,
		  ((std::string)error)
		  ) ;

ERS_DECLARE_ISSUE(opmonlib,
		  OpMonFacilityCreationFailed,
		  "Facility " << type << ": creation failed",
		  ((std::string)type)
		  ) ;

ERS_DECLARE_ISSUE_BASE(opmonlib,
		       OpMonFacilityCreationFailedWithCause,
		       opmonlib::OpMonFacilityCreationFailed,
		       "Facility " << type << ": creation failed because " << cause_message,
		       ((std::string)type),
		       ((std::string)cause_message)
		       )


} // namespace dunedaq

#endif // OPMONLIB_INCLUDE_OPMONLIB_ISSUES_HPP_
