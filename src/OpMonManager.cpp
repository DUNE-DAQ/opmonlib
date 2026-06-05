/**
 * @file OpMonManager.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <chrono>

#include <opmonlib/OpMonManager.hpp>
#include "logging/Logging.hpp"

using namespace dunedaq::opmonlib;

OpMonManager::OpMonManager( std::string session,
			    std::string name,
			    facility_ptr_t f_ptr)
  : MonitorableObject( name, session) {

  m_facility.store(f_ptr);
  
}

void OpMonManager::start_monitoring() {

  if ( ! m_cfg ) 
    throw MissingConfiguration(ERS_HERE);

  TLOG() << "Starting a new monitoring thread with interval " << m_cfg.load()->get_interval().count() << " seconds, at level " << get_opmon_level();

  auto running_function = std::bind( & OpMonManager::run, this, std::placeholders::_1);
  m_thread = std::jthread( running_function );
  auto handle = m_thread.native_handle();
  auto thread_name = "opmon";
  auto rc =  pthread_setname_np(handle, thread_name);    
   if (rc != 0) {
    ers::warning(ThreadNameTooLong(ERS_HERE, thread_name));
  }
}
  
void OpMonManager::run(std::stop_token stoken ) {

  auto sleep_interval = std::chrono::milliseconds(100);
  auto reporting_interval = m_cfg.load()->get_interval();  
  auto last_collection_time = std::chrono::steady_clock::now();
  
  while( ! stoken.stop_requested() ) {
    
    std::this_thread::sleep_for(sleep_interval);
    auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - last_collection_time);

    if ( time_span < reporting_interval ) continue;

    if ( stoken.stop_requested() ) break;
      
    last_collection_time = std::chrono::steady_clock::now();
    publish( collect() );
    // there is no catch here because collect is supposed to catch all possible exceptions
    // In this way we should garantee the collection of metrics on the system
    
  }

  TLOG() << "Exiting the monitoring thread";
}


