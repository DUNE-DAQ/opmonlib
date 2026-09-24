/**
 * @file opmon_facility_test.cxx Test application that tests and demonstrates
 * the conversion from a generic protobuf message to an opmon entry.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "NullOpMonFacility.hpp"
#include "opmonlib/OpMonFacility.hpp"
#include "opmonlib/Utils.hpp"
#include "opmonlib/opmon/test.pb.h"

#define BOOST_TEST_MODULE opmon_facility_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <future>

using namespace dunedaq::opmonlib;
using namespace dunedaq::opmon;

BOOST_AUTO_TEST_SUITE(Opmon_Facility_Test)

BOOST_AUTO_TEST_CASE(Invalid_Creation)
{

  BOOST_CHECK_THROW(auto service = makeOpMonFacility("invalid://"), OpMonFacilityCreationFailed);

  BOOST_CHECK_NO_THROW(auto service = makeOpMonFacility(""));
}

BOOST_AUTO_TEST_CASE(STD_Cout_facility)
{

  auto service = makeOpMonFacility("cout");

  dunedaq::opmon::TestInfo ti;
  ti.set_int_example(42);
  ti.set_float_example(12.34);
  ti.set_string_example("anohter_test");
  ti.set_bool_example(true);

  dunedaq::opmon::ComplexInfo ci;
  ci.set_another_float(1.23);
  *ci.mutable_sub_message() = ti;

  BOOST_CHECK_NO_THROW(service->publish(to_entry(ci, {})));
}

BOOST_AUTO_TEST_CASE(null_facility)
{

  auto service = std::make_shared<NullOpMonFacility>();

  dunedaq::opmon::TestInfo ti;
  ti.set_int_example(42);
  ti.set_float_example(12.34);
  ti.set_string_example("null_test");
  ti.set_bool_example(true);

  BOOST_CHECK_THROW(service->publish(to_entry(ti, {})), dunedaq::opmonlib::OpMonPublishFailure);
}

BOOST_AUTO_TEST_CASE(File_facility)
{

  BOOST_CHECK_THROW(auto service = makeOpMonFacility("file:///impossible_file.txt"), OpMonFacilityCreationFailed);

  auto service = makeOpMonFacility("file://./test_file", make_origin("test", "app"));

  auto pub_func = [&](int i) {
    dunedaq::opmon::OpMonId id;
    id += "unit";
    id += "test";
    id += "thread_" + std::to_string(i);
    for (auto j = 0; j < 30; ++j) {
      dunedaq::opmon::TestInfo ti;
      ti.set_int_example(i * 1000 + j);
      ti.set_string_example("test");
      auto e = to_entry(ti, {});
      *e.mutable_origin() = id;
      BOOST_CHECK_NO_THROW(service->publish(std::move(e)));
    }
  };

  const int n = 50;
  std::vector<std::future<void>> threads(n);

  for (auto i = 0; i < n; ++i) {
    threads[i] = async(std::launch::async, pub_func, i);
  }

  for (auto& t : threads) {
    BOOST_CHECK_NO_THROW(t.get());
  }
}

BOOST_AUTO_TEST_SUITE_END()
