/**
 * @file fileOpMonFacility.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "fileOpMonFacility.hpp"
#include "opmonlib/Utils.hpp"
#include <iostream>
#include <thread>

#include "logging/Logging.hpp"


namespace dunedaq::opmonlib {

  fileOpMonFacility::fileOpMonFacility(std::string uri, OptionalOrigin o) :
    JSonOpMonFacility(uri) {

    std::string hook = "://";
    auto sep = uri.find(hook);
    std::string fname;
    if (sep == std::string::npos) { // assume filename
      fname = uri;
    } else {
      fname = uri.substr(sep + hook.size());
    }

    if (o) {
      auto slash_pos = fname.find_last_of('/');
      auto dot_pos = slash_pos == std::string::npos ? fname.find_first_of('.') : fname.find_first_of('.', slash_pos);

      auto origin = to_string( o.value() );
      if (dot_pos == std::string::npos) {
	fname += '.' + origin + ".json";
      } else {
	fname.insert(dot_pos, '.' + origin );
      }

    }
    
    m_ofs.open(fname, std::ios::out | std::ios::app);
    if (!m_ofs.is_open()) {
      throw BadFile(ERS_HERE, fname);
    }
  }
  
  fileOpMonFacility::~fileOpMonFacility() {

    std::chrono::milliseconds max_delay( m_writing_counter.load());
    
    m_stop_request.store(true);
       
    std::unique_lock<std::mutex> lock(m_mutex);
    auto ret = m_writing_variable.wait_for(lock,
					   max_delay,
					   [&](){ return m_writing_counter.load() == 0; } ) ;
    if ( ! ret ) {
      ers::error(FileClosedBeforeWritingComplete(ERS_HERE,
						 max_delay.count(), m_writing_counter.load()) );
    }
  }

  void fileOpMonFacility::publish(opmon::OpMonEntry && e) const {

    if ( m_stop_request.load() ) {
      throw OpMonPublishFailure( ERS_HERE,
				 get_URI(), e.measurement(),
				 to_string(e.origin()),
				 FacilityStopRequested(ERS_HERE) );
      
    }
    
    ++m_writing_counter;
    std::thread (& fileOpMonFacility::write,
		 this, std::move(e)).detach();
    
  }

  void fileOpMonFacility::write(opmon::OpMonEntry && e) const noexcept {
    std::string json;
    google::protobuf::util::MessageToJsonString( e, & json,
						 get_json_options() );

    std::unique_lock<std::mutex> lock(m_mutex);
    m_writing_variable.wait(lock, [&](){ return m_writing_counter.load() > 0;} );
    
    try {
      m_ofs << json << std::endl << std::flush;
    } catch ( const std::ios_base::failure & except ) {
      ers::error( OpMonPublishFailure( ERS_HERE,
				       get_URI(), e.measurement(),
				       to_string(e.origin()),
				       WritingFailed(ERS_HERE, json, except) ) );
    }

    --m_writing_counter;
    m_writing_variable.notify_one();
  }


  
} // namespace dunedaq::opmonlib

DEFINE_DUNE_OPMON_FACILITY(dunedaq::opmonlib::fileOpMonFacility)

