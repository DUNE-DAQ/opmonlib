
#include "opmonlib/BackendOpMonFacility.hpp"

using namespace dunedaq::opmonlib;

void
BackendOpMonFacility::publish(opmon::OpMonEntry&& e) const
{

  std::lock_guard<std::mutex> lk(m_mutex);
  m_list.push_back(e);
}

BackendOpMonFacility::data_t
BackendOpMonFacility::get_entries(std::regex measurement_filter)
{

  std::unique_lock<std::mutex> lk(m_mutex);
  data_t ret;
  m_list.swap(ret);
  lk.unlock();

  std::smatch values;
  auto it = ret.begin();
  while (it != ret.end()) {
    if (!std::regex_match(it->measurement(), values, measurement_filter)) {
      it = ret.erase(it);
    } else {
      ++it;
    }
  }

  return ret;
}
