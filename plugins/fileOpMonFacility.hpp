/**
 * @file fileOpMonFacility.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_PLUGIN_FILEOPMONFACILITY_HPP_
#define OPMONLIB_PLUGIN_FILEOPMONFACILITY_HPP_


#include <opmonlib/JSonOpMonFacility.hpp>

#include <fstream>
#include <mutex>

namespace dunedaq {

ERS_DECLARE_ISSUE(opmonlib,
		  BadFile,
		  "Can not open file: " << filename,
		  ((std::string)filename) )

ERS_DECLARE_ISSUE(opmonlib,
		  WritingFailed,
		  "Failed to write. Message: " << message,
		  ((std::string)message)
		  )

} // namespace dunedaq

namespace dunedaq::opmonlib {

class fileOpMonFacility
  : public JSonOpMonFacility {
  
public:
  explicit fileOpMonFacility(std::string uri);

  void publish(opmon::OpMonEntry && e) const override;

private:
  mutable std::ofstream m_ofs;
  mutable std::mutex m_mutex;
  
};
  
} // namespace dunedaq::opmonlib


#endif // OPMONLIB_PLUGIN_FILEOPMONFACILITY_HPP_
