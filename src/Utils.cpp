/**
 * @file Utils.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "opmonlib/Utils.hpp"

dunedaq::opmon::OpMonId
dunedaq::opmonlib::make_origin(const std::string& session, const std::string& app)
{
  opmon::OpMonId origin;
  origin.set_session(session);
  origin.set_application(app);
  return origin;
}

dunedaq::opmon::OpMonEntry
dunedaq::opmonlib::to_entry(const google::protobuf::Message& m, const CustomOrigin& co)
{

  dunedaq::opmon::OpMonEntry entry;
  entry.set_measurement(m.GetTypeName());

  *entry.mutable_data() = dunedaq::opmonlib::to_map(m);

  for (const auto& [tag, value] : co) {
    (*entry.mutable_custom_origin())[tag] = value;
  }

  return entry;
}

dunedaq::opmonlib::map_type
dunedaq::opmonlib::to_map(const google::protobuf::Message& m, std::string top_block)
{

  dunedaq::opmonlib::map_type ret;

  const auto* descriptor_p = m.GetDescriptor();
  const auto& des = *descriptor_p;

  const auto* reflection_p = m.GetReflection();
  const auto& ref = *reflection_p;

  using namespace google::protobuf;

  auto count = des.field_count();
  for (int i = 0; i < count; ++i) {
    const auto* field_p = des.field(i);
    if (field_p->is_repeated())
      continue;

    auto name = top_block.empty() ? field_p->name() : top_block + '.' + field_p->name();

    auto type = field_p->cpp_type();
    dunedaq::opmon::OpMonValue value;
    bool success = true;
    switch (type) {
      case FieldDescriptor::CppType::CPPTYPE_INT32:
        value.set_int4_value(ref.GetInt32(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_INT64:
        value.set_int8_value(ref.GetInt64(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_UINT32:
        value.set_uint4_value(ref.GetUInt32(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_UINT64:
        value.set_uint8_value(ref.GetUInt64(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
        value.set_double_value(ref.GetDouble(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_FLOAT:
        value.set_float_value(ref.GetFloat(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_BOOL:
        value.set_boolean_value(ref.GetBool(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_STRING:
        value.set_string_value(ref.GetString(m, field_p));
        break;
      case FieldDescriptor::CppType::CPPTYPE_MESSAGE: {
        auto data = dunedaq::opmonlib::to_map(ref.GetMessage(m, field_p), name);
        ret.insert(data.begin(), data.end());
        success = false;
        break;
      }
      default:
        success = false;
        break;
    } // switch on the cpp type

    if (success)
      ret[name] = value;
  }

  return ret;
}

void
dunedaq::opmonlib::from_entry(google::protobuf::Message& m, const dunedaq::opmon::OpMonEntry& e, std::string top_block)
{

  const auto* descriptor_p = m.GetDescriptor();
  const auto& des = *descriptor_p;

  const auto* reflection_p = m.GetReflection();
  const auto& ref = *reflection_p;

  using namespace google::protobuf;

  auto count = des.field_count();
  for (int i = 0; i < count; ++i) {
    const auto* field_p = des.field(i);
    if (field_p->is_repeated())
      continue;

    auto name = top_block.empty() ? field_p->name() : top_block + '.' + field_p->name();

    auto type = field_p->cpp_type();

    if (type == FieldDescriptor::CppType::CPPTYPE_MESSAGE) {
      auto message = ref.MutableMessage(&m, field_p);
      from_entry(*message, e, name);
    } else {

      auto value = e.data().find(name)->second;

      switch (type) {
        case FieldDescriptor::CppType::CPPTYPE_INT32:
          ref.SetInt32(&m, field_p, value.int4_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_INT64:
          ref.SetInt64(&m, field_p, value.int8_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_UINT32:
          ref.SetUInt32(&m, field_p, value.uint4_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_UINT64:
          ref.SetUInt64(&m, field_p, value.uint8_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
          ref.SetDouble(&m, field_p, value.double_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_FLOAT:
          ref.SetFloat(&m, field_p, value.float_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_BOOL:
          ref.SetBool(&m, field_p, value.boolean_value());
          break;
        case FieldDescriptor::CppType::CPPTYPE_STRING:
          ref.SetString(&m, field_p, value.string_value());
          break;
        default:
          break;
      } // switch on the cpp type
    } // else if it's a message
  } // loop over the fields
}

std::string
dunedaq::opmonlib::to_string(const dunedaq::opmon::OpMonId& id)
{

  std::string ret(id.session());

  if (!id.application().empty()) {
    ret += '.';
    ret += id.application();
  }

  for (int i = 0; i < id.substructure().size(); ++i) {
    ret += '.';
    ret += id.substructure()[i];
  }

  return ret;
}

const dunedaq::opmon::OpMonId&
dunedaq::opmonlib::operator+=(dunedaq::opmon::OpMonId& id, const std::string& element)
{

  if (element.empty())
    return id;

  if (id.session().empty()) {
    id.set_session(element);
    return id;
  }

  if (id.application().empty()) {
    id.set_application(element);
    return id;
  }

  id.add_substructure(element);
  return id;
}

dunedaq::opmon::OpMonId
dunedaq::opmonlib::operator+(const dunedaq::opmon::OpMonId& id, const std::string& element)
{

  auto ret = id;
  ret += element;
  return ret;
}

//---------------------------------------------------
// template specifications for setters by name
//---------------------------------------------------
template<>
void
dunedaq::opmonlib::set_value<int32_t>(const google::protobuf::Reflection& r,
                                      google::protobuf::Message& m,
                                      const google::protobuf::FieldDescriptor* f_p,
                                      int32_t value)
{

  r.SetInt32(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<int64_t>(const google::protobuf::Reflection& r,
                                      google::protobuf::Message& m,
                                      const google::protobuf::FieldDescriptor* f_p,
                                      int64_t value)
{

  r.SetInt64(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<uint32_t>(const google::protobuf::Reflection& r,
                                       google::protobuf::Message& m,
                                       const google::protobuf::FieldDescriptor* f_p,
                                       uint32_t value)
{

  r.SetUInt32(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<uint64_t>(const google::protobuf::Reflection& r,
                                       google::protobuf::Message& m,
                                       const google::protobuf::FieldDescriptor* f_p,
                                       uint64_t value)
{

  r.SetUInt64(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<double>(const google::protobuf::Reflection& r,
                                     google::protobuf::Message& m,
                                     const google::protobuf::FieldDescriptor* f_p,
                                     double value)
{

  r.SetDouble(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<float>(const google::protobuf::Reflection& r,
                                    google::protobuf::Message& m,
                                    const google::protobuf::FieldDescriptor* f_p,
                                    float value)
{

  r.SetFloat(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<bool>(const google::protobuf::Reflection& r,
                                   google::protobuf::Message& m,
                                   const google::protobuf::FieldDescriptor* f_p,
                                   bool value)
{

  r.SetBool(&m, f_p, value);
}

template<>
void
dunedaq::opmonlib::set_value<std::string>(const google::protobuf::Reflection& r,
                                          google::protobuf::Message& m,
                                          const google::protobuf::FieldDescriptor* f_p,
                                          std::string value)
{

  r.SetString(&m, f_p, value);
}
