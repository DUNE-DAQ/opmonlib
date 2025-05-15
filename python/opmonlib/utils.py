import logging
import sys
from pathlib import Path

from google.protobuf.descriptor import FieldDescriptor
from google.protobuf.message import Message as Msg
from google.protobuf.timestamp_pb2 import Timestamp

from opmonlib.opmon_entry_pb2 import OpMonEntry, OpMonId, OpMonValue


def parse_opmon_conf(
    log: logging.Logger, conf: dict[str:str], uri: dict[str:str]
) -> dict[str:str]:
    """Parse the OpMonConf and OpMonURI."""
    if not conf:
        log.error("Missing opmon configuration, exiting.")
        sys.exit(1)
    if not uri:
        log.error("Missing opmon URI, exiting.")
        sys.exit(1)

    opmon_type = uri.get("type")
    if not opmon_type:
        log.warning(
            "Missing 'type' in the opmon configuration, using default value 'stdout'."
        )
        opmon_type = "stdout"

    path = uri.get("path")
    if not path:
        log.error("Missing 'path' in the opmon configuration, exiting.")
        sys.exit(1)

    if opmon_type == "stream" and "monkafka" not in path:
        msg = "OpMon stream configuration must publish to kafka, exiting."
        raise ValueError(msg) from None
    if opmon_type != "stream" and "monkafka" in path:
        msg = "To use kafka, the type must be set to stream."
        raise ValueError(msg) from None

    bootstrap = None
    topic = None
    if opmon_type == "file" and not Path(path).parent.is_dir():
        log.error("Requested directory to put file in does not exist.")
        sys.exit(1)
    elif "monkafka" in path:
        bootstrap, topic = path.split("/", 1)
    if not topic:
        topic = "OpMon"

    level = conf.get("level")
    if not level:
        log.warning(
            "Missing 'log_level' in the opmon configuration, using default 'INFO'."
        )
        level = logging.INFO

    interval_s = conf.get("interval_s")
    if not interval_s:
        log.error("Missing 'interval_s' in the opmon configuration, exiting.")
        sys.exit(1)

    return {
        "type": opmon_type,
        "bootstrap": bootstrap,
        "topic": topic,
        "level": level,
        "interval_s": interval_s,
    }


def map_entry(value: int | float | bool | str, field_type: int) -> OpMonValue:
    """Map the data entry to the correct protobuf format."""
    formatted_opmonvalue = OpMonValue()
    match field_type:
        case FieldDescriptor.CPPTYPE_INT32:
            formatted_opmonvalue.int4_value = value
        case FieldDescriptor.CPPTYPE_INT64:
            formatted_opmonvalue.int8_value = value
        case FieldDescriptor.CPPTYPE_UINT32:
            formatted_opmonvalue.uint4_value = value
        case FieldDescriptor.CPPTYPE_UINT64:
            formatted_opmonvalue.uint8_value = value
        case FieldDescriptor.CPPTYPE_DOUBLE:
            formatted_opmonvalue.double_value = value
        case FieldDescriptor.CPPTYPE_FLOAT:
            formatted_opmonvalue.float_value = value
        case FieldDescriptor.CPPTYPE_BOOL:
            formatted_opmonvalue.boolean_value = value
        case FieldDescriptor.CPPTYPE_STRING:
            formatted_opmonvalue.string_value = value
        # Ignore unknown types.
    return formatted_opmonvalue


def map_message(message: Msg, top_block: str = "") -> dict:
    """Map each message entry to the correct data type."""
    message_dict = {}
    for name, descriptor in message.DESCRIPTOR.fields_by_name.items():
        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            continue  # Repeated values not supported in influxdb
        if descriptor.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
            top_block += name + "."
            message_dict = message_dict | map_message(getattr(message, name), top_block)
        else:
            message_dict[top_block + name] = map_entry(
                getattr(message, name), descriptor.cpp_type
            )
    return message_dict


def validate_custom_origin(
    custom_origin: dict[str, str] | None = None,
) -> dict[str, str]:
    """Validate that each custom_origin entry is a str."""
    if custom_origin is None:
        return None
    for key, value in custom_origin.items():
        if not isinstance(value, str):
            try:
                custom_origin[key] = str(value)
            except TypeError:
                msg = "%s is not a string and cannot be converted to one.", key
                raise TypeError(msg) from None
    return custom_origin


def pack_to_opmonentry(
    session: str,
    application: str,
    message: Msg,
    custom_origin: dict[str, str] | None,
    substructure: list[str] | None,
    t: Timestamp,
) -> OpMonEntry:
    """Pack all the data that needs to be published to an OpMonEntry."""
    opmon_id = OpMonId(
        session=session, application=application, substructure=substructure
    )
    return OpMonEntry(
        time=t,
        origin=opmon_id,
        custom_origin=validate_custom_origin(custom_origin),
        measurement=message.DESCRIPTOR.full_name,
        data=map_message(message),
    )
