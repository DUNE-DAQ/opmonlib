/**
 * @file moniotorable_object_test.cxx Test application that tests MonitorableObject.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/info/test.pb.h"
#include "opmonlib/MonitorableObject.hpp"
#include "opmonlib/OpMonManager.hpp"

#define BOOST_TEST_MODULE monitorable_object_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <vector>

using namespace dunedaq::opmonlib;
using namespace dunedaq::opmon;

BOOST_AUTO_TEST_SUITE(Monitorable_Object_Test)

struct my_fixture {

  class TestObject : public MonitorableObject {

  public:
    using MonitorableObject::register_child;
    TestObject() : MonitorableObject() {;}
  };

  my_fixture()
    : manager("test", "manager")
    , node_p(new TestObject) {;}  
  
  OpMonManager manager;
  std::shared_ptr<TestObject> node_p;
   
};


BOOST_FIXTURE_TEST_CASE( opmon_ids, my_fixture ) {

  BOOST_CHECK_EQUAL( node_p->get_opmon_id(), "" );
  BOOST_CHECK_EQUAL( manager.get_opmon_id(), "test.manager" );
  
}




BOOST_AUTO_TEST_SUITE_END()

