/**
 * @file schema_conversion_test.cxx Test application that tests and demonstrates
 * the conversion from a generic protobuf message to an opmon entry.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/Utils.hpp"
#include "opmonlib/opmon/test.pb.h"

#define BOOST_TEST_MODULE schema_conversion_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <google/protobuf/util/json_util.h>

using namespace dunedaq::opmonlib;
using namespace dunedaq::opmon;

BOOST_AUTO_TEST_SUITE(Schema_Conversion_Test)

BOOST_AUTO_TEST_CASE(conversion)
{

  const int64_t int_value = 394;
  const float float_value = 3.14;
  const double double_value = 6.28;
  const std::string string_value = "my_test";
  const bool bool_value = true;
  const auto tag_name = "channel";
  const auto tag_value = "365";

  dunedaq::opmon::TestInfo ti;
  ti.set_int_example(int_value);
  ti.set_float_example(double_value);
  ti.set_string_example(string_value);
  ti.set_bool_example(bool_value);

  dunedaq::opmon::ComplexInfo ci;
  ci.set_another_float(float_value);
  *ci.mutable_sub_message() = ti;
  ci.mutable_r_field()->Add(42);

  auto test_entry = to_entry(ti, {});
  auto complex_entry = to_entry(ci, { { tag_name, tag_value } });

  // std::string res;
  // google::protobuf::util::MessageToJsonString( test_entry, & res );
  // std::cout << res << std::endl;

  // res.clear();
  // google::protobuf::util::MessageToJsonString( complex_entry, & res );
  // std::cout << res << std::endl;

  BOOST_TEST(test_entry.data().size() == 4);    // check that all the entry of the simple schema are in
  BOOST_TEST(complex_entry.data().size() == 5); // check that repeated entries are not added,
                                                // but nested structures are

  // checks that the values are preserved correctly
  auto value = test_entry.data().find("int_example");
  auto final_int = value->second.int8_value();
  BOOST_TEST(final_int == int_value);

  value = test_entry.data().find("float_example");
  auto final_double = value->second.double_value();
  BOOST_TEST(final_double == double_value);

  value = test_entry.data().find("string_example");
  auto final_string = value->second.string_value();
  BOOST_TEST(final_string == string_value);

  value = test_entry.data().find("bool_example");
  auto final_bool = value->second.boolean_value();
  BOOST_TEST(final_bool == bool_value);

  value = complex_entry.data().find("another_float");
  auto final_float = value->second.float_value();
  BOOST_TEST(final_float == float_value);

  value = complex_entry.data().find("sub_message.string_example");
  final_string = value->second.string_value();
  BOOST_TEST(final_string == string_value);

  BOOST_TEST(complex_entry.custom_origin().begin()->first == tag_name);
  BOOST_TEST(complex_entry.custom_origin().begin()->second == tag_value);

  // inverse
  auto reco_ti = from_entry<dunedaq::opmon::TestInfo>(test_entry);
  BOOST_TEST(reco_ti.int_example() == int_value);
  BOOST_CHECK_THROW(auto failed_reco_ti = from_entry<dunedaq::opmon::TestInfo>(complex_entry),
                    dunedaq::opmonlib::NameMismatch);
  BOOST_CHECK_NO_THROW(auto succ_reco_ci = from_entry<dunedaq::opmon::ComplexInfo>(complex_entry));
}

BOOST_AUTO_TEST_CASE(name_setters)
{

  const int64_t int_value = 394;
  const float float_value = 3.14;
  const double double_value = 6.28;
  const std::string string_value = "my_test";
  const bool bool_value = true;

  dunedaq::opmon::TestInfo ti;

  BOOST_CHECK_THROW(set_value(ti, "non_existing_field", double_value), dunedaq::opmonlib::MissingField);

  set_value(ti, "string_example", string_value);
  BOOST_TEST(string_value == ti.string_example());

  set_value(ti, "float_example", double_value);
  BOOST_TEST(double_value == ti.float_example());

  set_value(ti, "int_example", int_value);
  BOOST_TEST(int_value == ti.int_example());
}

BOOST_AUTO_TEST_SUITE_END()
