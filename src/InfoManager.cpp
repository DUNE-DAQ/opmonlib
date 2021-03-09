#include <iostream>
#include <string>

#include "opmonlib/InfoManager.hpp"
#include "opmonlib/InfoCollector.hpp"
#include "opmonlib/OpmonService.hpp"

using namespace std;

namespace dunedaq::opmonlib {

InfoManager::InfoManager(std::string service) 
{
  m_service = opmonlib::makeOpmonService(service);
  m_running.store(false);
}


void InfoManager::publish_info( int level ) 
{

  nlohmann::json j = gather_info(level);
 
  m_service->publish(j);

}


nlohmann::json InfoManager::gather_info( int level ) 
{

  nlohmann::json j;
  dunedaq::opmonlib::InfoCollector ic;
  // FIXME: check against nullptr!
  m_ip->gather_stats(ic, level);       
  j = ic.get_collected_infos();
  
  return j;

}


void InfoManager::set_provider( opmonlib::InfoProvider& p )
{

  // Set the data member to point to selected InfoProvider
  m_ip = &p;

}


void InfoManager::start(uint32_t interval_sec, uint32_t level) 
{
  m_running.store(true);
  if(interval_sec > 0)
    m_thread = std::thread(&InfoManager::run, this, interval_sec, level);
}


void InfoManager::run(uint32_t interval_sec, uint32_t level) 
{ 
 uint32_t countdown = 10 * interval_sec;
 while(m_running.load()) {
    if (countdown > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      --countdown;
    }
    else {
      publish_info(level);
      countdown = 10 * interval_sec;
    }     
 }
}

void InfoManager::stop()
{
  m_running.store(false);
  m_thread.join();
}

} // namespace dunedaq::opmonlib
