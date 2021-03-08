#include <iostream>
#include <string>

#include "opmonlib/InfoManager.hpp"
#include "opmonlib/InfoCollector.hpp"
#include "JsonInfluxConverter/InfoCollector.h"

using namespace dunedaq::opmonlib;
using namespace std;


InfoManager::InfoManager( std::string service ) {
  m_service = service;
  m_running.store(false);
}

void InsertToDB(std::vector<std::string> insertsVector)
{
    for (int i = 0; i < insertsVector.size(); i++)
    {
        std::string resp;
        // query from table
        influxdb_cpp::server_info si("127.0.0.1", 8086, "mydb", "admin", "admin");
        influxdb_cpp::query(resp, insertsVector[i], si);
        std::cout << resp << std::endl;
    }

}

void InfoManager::publish_info( int level ) {

  // Use m_mod_mgr and m_time_interval_sec here?
  // add? 

  //nlohmann::json j = gather_info(level);
  
  //std::cout <<  j.dump(2) << std::endl;  // print json?

    //#include <filesystem>
    //#include <influxdb.hpp>
    //#include <nlohmann/json.hpp>

    //Json to TSDB variables and class instanciation
    //std::string path = "C:/Users/yadonon/Projects/jsontoinfluxdb/converterC/converterC/jsonFiles";
    std::string path = "yourpath";
    std::vector<std::string> insertsVector;
    std::vector<std::string> tagSetVector;
    tagSetVector.push_back(".class_name=");
    std::string timeVariableName = ".time=";
    JsonConverter jsonConverter;

    //Goes through json files at indicated location
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().extension() == ".json")
        {
            jsonConverter.setInsertsVector(false, tagSetVector, timeVariableName, entry.path().string());
            InsertToDB(jsonConverter.getInsertsVector());
        }
    }

}


nlohmann::json InfoManager::gather_info( int level ) {

  nlohmann::json j;
  dunedaq::opmonlib::InfoCollector ic;
  // FIXME: check against nullptr!
  m_ip->gather_stats(ic, level);       
  j = ic.get_collected_infos();
  
  return j;

}


void InfoManager::set_provider( opmonlib::InfoProvider& p ){

  // Set the data member to point to selected InfoProvider
  m_ip = &p;

}

void InfoManager::start(uint32_t interval_sec, uint32_t level) {
  m_running.store(true);
  if(interval_sec > 0)
    m_thread = std::thread(&InfoManager::run, this, interval_sec, level);
}

void InfoManager::run(uint32_t interval_sec, uint32_t level) { 
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

void InfoManager::stop() {
  m_running.store(false);
  m_thread.join();
}

