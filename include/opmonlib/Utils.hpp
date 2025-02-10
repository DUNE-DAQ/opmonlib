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
#include <map>

#include <google/protobuf/message.h>
#include "opmonlib/opmon_entry.pb.h"
#include "ers/ers.hpp"
#include <logging/Logging.hpp> // NOTE: if ISSUES ARE DECLARED BEFORE include logging/Logging.hpp, TLOG_DEBUG<<issue wont work.

#include <google/protobuf/util/time_util.h>


namespace dunedaq {
  
  ERS_DECLARE_ISSUE(opmonlib,
		    FailedInfoReconstruction,
		    "Failure to create an object of type " << type,
		    ((std::string)type)
		    )

  ERS_DECLARE_ISSUE_BASE(opmonlib,
			 NameMismatch,
			 FailedInfoReconstruction,
			 "Message " << type
			 << " cannot be reconstructed from entry of type " << entry,
			 ((std::string)type),
			 ((std::string)entry)
			 )

  ERS_DECLARE_ISSUE(opmonlib,
		    MissingField,
		    "Field " << field << " not present in message " << message,
		    ((std::string)field)((std::string)message)
		    )

  
}

namespace dunedaq::opmonlib {

  using CustomOrigin = std::map<std::string, std::string> ;
  using cr_map_type = std::invoke_result<decltype(&dunedaq::opmon::OpMonEntry::data),
					 dunedaq::opmon::OpMonEntry>::type;
  using map_type = std::remove_const<std::remove_reference<cr_map_type>::type>::type;


  /**
   * sets the field called "name" in the message m to the value of "value"
   * This is essentially allows us to use any schema generated message as if it was dynamic object
   */
  template<class T>
  void set_value(google::protobuf::Message & m, const std::string & name, T value);

  template<class T>
  void set_value( const google::protobuf::Reflection & ,
		  google::protobuf::Message & ,
		  const google::protobuf::FieldDescriptor* , T );

  opmon::OpMonId make_origin( const std::string & session, const std::string & app ) ;
  
  dunedaq::opmon::OpMonEntry to_entry(const google::protobuf::Message & m,
				      const CustomOrigin & co);

  template<class M>
  M from_entry( const dunedaq::opmon::OpMonEntry & );

  void from_entry(google::protobuf::Message & m,
		  const dunedaq::opmon::OpMonEntry &,
		  std::string top_block = "");
  
  map_type to_map(const google::protobuf::Message & m,
		  std::string top_block = "");
  
  std::string to_string( const dunedaq::opmon::OpMonId & );

  const dunedaq::opmon::OpMonId & operator += (dunedaq::opmon::OpMonId &,
					       const  std::string & element);
  
  dunedaq::opmon::OpMonId operator + (const dunedaq::opmon::OpMonId &,
				      const  std::string & element ) ;

}  // namespace dunedaq::opmonlib

#include "details/Utils.hxx"

#endif // OPMONLIB_INCLUDE_OPMONLIB_UTILS_HPP_
