/**
 * @file Utils.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/Utils.hpp"

dunedaq::opmon::OpMonEntry dunedaq::opmonlib::to_entry(const google::protobuf::Message & m) {

  dunedaq::opmon::OpMonEntry entry;
  entry.set_measurement( m.GetTypeName() );
  
  const auto * descriptor_p = m.GetDescriptor();
  const auto & des = *descriptor_p;
  
  const auto * reflection_p = m.GetReflection();
  const auto & ref = *reflection_p;
  
  using namespace google::protobuf;

  auto count = des.field_count();
  for ( int i = 0; i < count; ++i ) {
    const auto * field_p = des.field(i);
    if ( field_p -> is_repeated() ) continue;
    
    auto name = field_p -> name();
    auto type = field_p -> cpp_type();
    dunedaq::opmon::OpMonValue value;
    bool success = true;
    switch (type) {
    case FieldDescriptor::CppType::CPPTYPE_INT32:
      value.set_int4_value( ref.GetInt32(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_INT64:
      value.set_int8_value( ref.GetInt64(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_UINT32:
      value.set_uint4_value( ref.GetUInt32(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_UINT64:
      value.set_uint8_value( ref.GetUInt64(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
      value.set_double_value( ref.GetDouble(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_FLOAT:
      value.set_float_value( ref.GetFloat(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_BOOL:
      value.set_boolean_value( ref.GetBool(m, field_p) );
      break;
    case FieldDescriptor::CppType::CPPTYPE_STRING:
      value.set_string_value( ref.GetString(m, field_p) );
      break;
    default:
      success = false;
      break;
    } // switch on the cpp type                                                                         
    
      // it seems the following lines are causing the linking issue with the current system               
    if ( success )
      (*entry.mutable_data())[name] = value;
  }

  return entry;
}

