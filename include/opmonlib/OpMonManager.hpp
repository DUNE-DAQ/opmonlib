/**
 * @file OpMonManager.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_OPMONMANAGER_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_OPMONMANAGER_HPP_

#include <cstddef>
#include <thread>

#include "opmonlib/MonitorableObject.hpp"
#include "opmonlib/Utils.hpp"

#include "confmodel/OpMonConf.hpp"
#include <logging/Logging.hpp> // NOTE: if ISSUES ARE DECLARED BEFORE include logging/Logging.hpp, TLOG_DEBUG<<issue wont work.

namespace dunedaq {

  ERS_DECLARE_ISSUE( opmonlib,
		     ThreadNameTooLong,
		     "The name " << name << " is too long for a thread name",
		     ((std::string)name)
		     )
  
  ERS_DECLARE_ISSUE( opmonlib,
                     MonitoringThreadNotSet,
                     "Monitoring thread not set",
                     ERS_EMPTY )

  ERS_DECLARE_ISSUE( opmonlib,
		     FailedMonitoringThread,
		     "Monitoring thread failed to start",
		     ERS_EMPTY )

  ERS_DECLARE_ISSUE( opmonlib,
		     MissingConfiguration,
		     "Confiration is not set",
		     ERS_EMPTY )
}



namespace dunedaq::opmonlib {

class OpMonManager : protected MonitorableObject 
{

  friend class TestOpMonManager;
  
public:

  /*
   * Construtor expected to be used for standard operations, 
   * yet, general developers should not concern themselves with this as the framework handles this for them
   */
  explicit OpMonManager(std::string session,
			std::string name,
			std::string opmon_facility_uri = "stdout") :
    OpMonManager( session, name, makeOpMonFacility(opmon_facility_uri, make_origin(session, name)) ){;}
  
  virtual ~OpMonManager() = default;
  
  using MonitorableObject::get_opmon_id;
  using MonitorableObject::get_opmon_level;
  using MonitorableObject::register_node;
  using MonitorableObject::set_opmon_level;
  
  // data collecting loop
  void start_monitoring(); 
  // The stop command is not necessary.
  // The stop is invoked during the destruction of the thread
  // the method requires a valid configuration because the time period is taken from there

  void set_opmon_conf( const confmodel::OpMonConf* c ) {
    m_cfg.store(c);
    set_opmon_level( m_cfg.load()->get_level() );
  }
  
protected:
  using MonitorableObject::collect;

  //obtain the opmon facility
  auto get_opmon_facility() const { return m_facility.load(); }

  OpMonManager(std::string session,
	       std::string name,
	       facility_ptr_t );
  
  void run( std::stop_token ); // function used by the jthread

private:

  std::jthread m_thread;
  std::atomic<const confmodel::OpMonConf*> m_cfg{nullptr}; 

};

 
} // namespace dunedaq::opmonlib

#endif // OPMONLIB_INCLUDE_OPMONLIB_OPMONMANAGER_HPP_
