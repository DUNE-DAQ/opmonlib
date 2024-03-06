/** 
 * @file OpMonFacility.hpp 
 *                         
 * This is part of the DUNE DAQ Application Framework, copyright 2020. 
 * Licensing/copyright details are in the COPYING file that you should have 
 * received with this code.     
 */

#include "opmonlib/OpMonFacility.hpp"
#include "logging/Logging.hpp"

using namespace dunedaq::opmonlib;

std::shared_ptr<OpMonFacility> dunedaq::opmonlib::makeOpMonFacility(std::string const& facility) {

  TLOG() << "FACILITY = " << facility;

  const std::string hook = "://";
  auto sep = facility.find(hook);
  std::string scheme;
  if (sep == std::string::npos) { // simple path   
    scheme = "stdout";
  } else { // with scheme 
    scheme = facility.substr(0, sep);
  }

  
  std::string plugin_name = scheme + "OpMonFacility";
  static cet::BasicPluginFactory bpf("duneOpMonFacility", "make");
  std::shared_ptr<OpMonFacility> os_ptr;
  
  try {
    os_ptr = bpf.makePlugin<std::shared_ptr<OpMonFacility>>(plugin_name, facility);
  } catch (const ers::Issue& iexpt) {
    throw OpMonFacilityCreationFailed(ERS_HERE, plugin_name, iexpt);
  } catch (const cet::exception& cexpt) {
    throw OpMonFacilityCreationFailedWithCause(ERS_HERE, plugin_name, cexpt.what());
  } catch (...) { // NOLINT JCF Jan-27-2021 violates letter of the law but not the spirit    
    throw OpMonFacilityCreationFailed(ERS_HERE, plugin_name);
  }
  return os_ptr;
}





