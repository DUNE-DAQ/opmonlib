/**
 * @file TestOpMonManager.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_TESTOPMONMANAGER_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_TESTOPMONMANAGER_HPP_

#include "opmonlib/OpMonManager.hpp"
#include <cstddef>

#include "opmonlib/BackendOpMonFacility.hpp"

namespace dunedaq {

}

namespace dunedaq::opmonlib {
/*
 * This is a specification of the OpMonManager designed to spy on the metrics
 * generated. It's supposed to be used in tests.
 */
class TestOpMonManager : public OpMonManager
{

public:
  explicit TestOpMonManager(std::string session = "NULL", std::string name = "tree")
    : OpMonManager(session, name, std::make_shared<BackendOpMonFacility>())
  {
    ;
  }

  virtual ~TestOpMonManager() = default;

  auto get_backend_facility() { return std::dynamic_pointer_cast<BackendOpMonFacility>(get_opmon_facility()); }

  // we expose the collect so we can harvest the metrics at will
  using OpMonManager::collect;
};

} // namespace dunedaq::opmonlib

#endif // OPMONLIB_INCLUDE_OPMONLIB_TESTOPMONMANAGER_HPP_
