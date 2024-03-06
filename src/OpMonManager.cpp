/**
 * @file OpMonManager.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <chrono>

#include <opmonlib/OpMonManager.hpp>
#include <logging/Logging.hpp>

using namespace dunedaq::opmonlib;


OpMonManager::OpMonManager( std::string session,
			    std::string name,
			    std::string opmon_facility_uri)
  : MonitorableObject( name, session) {

  m_facility = makeOpMonFacility(opmon_facility_uri);
  
}


OpMonManager::~OpMonManager() {
  stop();
}


void OpMonManager::start(std::chrono::seconds interval, opmon_level level) {

  if (m_thread_p) {
    try {
      m_thread_p->stop_working_thread();
    } catch (const utilities::ThreadingIssue & ti){
      ers::warning( ti );
    }
  }

  TLOG() << "Starting a new monitoring thread" ;

  auto running_function = std::bind( & OpMonManager::run, this, std::placeholders::_1, interval, level);
  m_thread_p.reset(new dunedaq::utilities::WorkerThread( running_function ) );

  auto name = get_opmon_id();
  name += ".opmon";

  m_thread_p->start_working_thread(name);
}



void OpMonManager::stop() {

  if ( m_thread_p ) {
    m_thread_p->stop_working_thread();
  } else {
    throw MonitoringThreadNotSet(ERS_HERE);
  }
  
}
  


void OpMonManager::run(std::atomic<bool> & running,
		       std::chrono::seconds interval, opmon_level level ) {

  auto sleep_interval = std::chrono::milliseconds(100);
  
  auto last_collection_time = std::chrono::steady_clock::now();
  
  while( running.load() ) {
    
    std::this_thread::sleep_for(sleep_interval);
    auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - last_collection_time);
    
    if ( time_span >= interval ) {
      last_collection_time = std::chrono::steady_clock::now();
      publish( collect(level) );
      // there is no catch here because collect is supposed to catch all possible exceptions
      // In this way we should garantee the collection of metrics on the system
    }
  }
}
