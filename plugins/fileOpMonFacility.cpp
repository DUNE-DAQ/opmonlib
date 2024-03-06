/**
 * @file fileOpMonFacility.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "fileOpMonFacility.hpp"
#include <iostream>


namespace dunedaq::opmonlib {

  fileOpMonFacility::fileOpMonFacility(std::string uri) :
    JSonOpMonFacility(uri) {

    std::string hook = "://";
    auto sep = uri.find(hook);
    std::string fname;
    if (sep == std::string::npos) { // assume filename
      fname = uri;
    } else {
      fname = uri.substr(sep + hook.size());
    }
    
    m_ofs.open(fname, std::ios::out | std::ios::app);
    if (!m_ofs.is_open()) {
      throw BadFile(ERS_HERE, fname);
    }
  }
  

  void fileOpMonFacility::publish(opmon::OpMonEntry && e) const {
    std::string json;
    google::protobuf::util::MessageToJsonString( e, & json,
						 get_json_options() );

    std::unique_lock<std::mutex> lock(m_mutex);

    try {
      m_ofs << json << std::endl << std::flush;
    } catch ( std::ios_base::failure except ) {
      throw OpMonPublishFailure( ERS_HERE,
				 get_URI(), e.measurement(), e.opmon_id(),
				 WritingFailed(ERS_HERE, json, except) );
    }
    
  }
  
} // namespace dunedaq::opmonlib

DEFINE_DUNE_OPMON_FACILITY(dunedaq::opmonlib::fileOpMonFacility)

