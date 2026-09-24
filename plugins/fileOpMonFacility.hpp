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

#include <condition_variable>
#include <fstream>
#include <mutex>

namespace dunedaq {

ERS_DECLARE_ISSUE(opmonlib, BadFile, "Can not open file: " << filename, ((std::string)filename))

ERS_DECLARE_ISSUE(opmonlib, WritingFailed, "Failed to write. Message: " << message, ((std::string)message))

ERS_DECLARE_ISSUE(opmonlib,
                  FacilityStopRequested,
                  "Facility about to be destroyed and cannot write messages anymore.",
                  ERS_EMPTY)

ERS_DECLARE_ISSUE(opmonlib,
                  FileClosedBeforeWritingComplete,
                  "Facility was destroyed after " << milliseconds << " ms with " << counter
                                                  << " entries yet to be written",
                  ((uint16_t)milliseconds)((uint16_t)counter))

} // namespace dunedaq

namespace dunedaq::opmonlib {

class fileOpMonFacility : public JSonOpMonFacility
{

public:
  explicit fileOpMonFacility(std::string uri, OptionalOrigin);
  ~fileOpMonFacility();

  void publish(opmon::OpMonEntry&& e) const override;

private:
  void write(opmon::OpMonEntry&& e) const noexcept;

  mutable std::ofstream m_ofs;
  mutable std::mutex m_mutex;
  mutable std::condition_variable m_writing_variable;
  mutable std::atomic<uint16_t> m_writing_counter{ 0 };
  mutable std::atomic<bool> m_stop_request{ false };
};

} // namespace dunedaq::opmonlib

#endif // OPMONLIB_PLUGIN_FILEOPMONFACILITY_HPP_
