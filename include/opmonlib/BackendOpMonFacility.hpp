#ifndef OPMONLIB_INCLUDE_OPMONLIB_BACKENDOPMONFACILITY_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_BACKENDOPMONFACILITY_HPP_

#include "opmonlib/OpMonFacility.hpp"
#include "opmonlib/Utils.hpp"

#include <list>
#include <mutex>
#include <regex>

namespace dunedaq::opmonlib {

class BackendOpMonFacility : public OpMonFacility
{

  using entry_t = dunedaq::opmon::OpMonEntry;
  using data_t = std::list<entry_t>;

public:
  BackendOpMonFacility()
    : OpMonFacility("backend://")
  {
    ;
  }

  void publish(entry_t&& e) const override;

  data_t get_entries(std::regex measurement_filter = std::regex(".*"));

private:
  mutable data_t m_list;
  mutable std::mutex m_mutex;
};
}

#endif // OPMONLIB_INCLUDE_OPMONLIB_BACKENDOPMONFACILITY_HPP_
