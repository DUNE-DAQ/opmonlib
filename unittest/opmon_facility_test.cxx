/**
 * @file opmon_facility_test.cxx Test application that tests and demonstrates
 * the conversion from a generic protobuf message to an opmon entry.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/OpMonFacility.hpp"
#include "opmonlib/Utils.hpp"
#include "opmonlib/info/test.pb.h"

#define BOOST_TEST_MODULE opmon_facility_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <future>

using namespace dunedaq::opmonlib;
using namespace dunedaq::opmon;

BOOST_AUTO_TEST_SUITE(Opmon_Facility_Test)


BOOST_AUTO_TEST_CASE(Invalid_Creation) {

  BOOST_CHECK_THROW( auto service = makeOpMonFacility("invalid://"),
		     OpMonFacilityCreationFailed );

  BOOST_CHECK_NO_THROW( auto service = makeOpMonFacility("") );
  BOOST_CHECK_NO_THROW( auto service = makeOpMonFacility("null://") );
  
}


BOOST_AUTO_TEST_CASE(STD_Cout_facility) {
  
  auto service = makeOpMonFacility("cout"); 

  dunedaq::opmon::TestInfo ti;
  ti.set_int_example( 42 );
  ti.set_float_example( 12.34 );
  ti.set_string_example( "anohter_test" );
  ti.set_bool_example( true );

  BOOST_CHECK_NO_THROW ( service -> publish(  to_entry( ti ) ) ) ;
  
}


BOOST_AUTO_TEST_CASE(null_facility) {
  
  auto service = makeOpMonFacility("null://"); 

  dunedaq::opmon::TestInfo ti;
  ti.set_int_example( 42 );
  ti.set_float_example( 12.34 );
  ti.set_string_example( "null_test" );
  ti.set_bool_example( true );

  BOOST_CHECK_THROW ( service -> publish(  to_entry( ti ) ),
		      dunedaq::opmonlib::OpMonPublishFailure ) ;
  
}


BOOST_AUTO_TEST_CASE(File_facility) {

  BOOST_CHECK_THROW( auto service = makeOpMonFacility("file:///impossible_file.txt"),
		     OpMonFacilityCreationFailed);

  auto service = makeOpMonFacility("file://./test_file.txt");

  auto pub_func = [&](int i){
    auto opmon_id = "unit.test.thread_" + std::to_string(i);
    for (auto j = 0; j < 5; ++j ) {
      dunedaq::opmon::TestInfo ti;
      ti.set_int_example( i*1000 + j );
      ti.set_string_example( "test" );
      auto e = to_entry( ti );
      e.set_opmon_id(opmon_id);
      BOOST_CHECK_NO_THROW( service->publish( std::move(e) ) );
    }
  };

  const int n = 30;
  std::vector<std::future<void>> threads(n);

  for( auto i = 0 ; i < n; ++i ) {
    threads[i] = async(std::launch::async, pub_func, i);
  }
  
  for ( auto & t : threads ) {
    BOOST_CHECK_NO_THROW( t.get() );
  }
  
}


BOOST_AUTO_TEST_SUITE_END()

