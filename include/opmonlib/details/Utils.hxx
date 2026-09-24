template<class M>
M
dunedaq::opmonlib::from_entry(const dunedaq::opmon::OpMonEntry& e)
{

  M ret;

  if (ret.GetTypeName() != e.measurement()) {
    throw opmonlib::NameMismatch(ERS_HERE, ret.GetTypeName(), e.measurement());
  }

  from_entry(ret, e);

  return ret;
}

template<class T>
void
dunedaq::opmonlib::set_value(google::protobuf::Message& m, const std::string& name, T value)
{

  const auto* descriptor_p = m.GetDescriptor();
  const auto& des = *descriptor_p;

  auto field_p = des.FindFieldByName(name);
  if (!field_p) {
    throw MissingField(ERS_HERE, name, m.GetTypeName());
  }

  const auto* reflection_p = m.GetReflection();
  const auto& ref = *reflection_p;

  set_value(ref, m, field_p, value);
}
