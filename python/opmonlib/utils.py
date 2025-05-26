import logging
from dataclasses import dataclass
import sys
from pathlib import Path

from google.protobuf.descriptor import FieldDescriptor
from google.protobuf.message import Message as Msg
from google.protobuf.timestamp_pb2 import Timestamp

from opmonlib.opmon_entry_pb2 import OpMonEntry, OpMonId, OpMonValue

@dataclass
class OpMonConf:
    opmon_type: str
    bootstrap: str
    topic: str
    level: int
    interval_s: float
    path: str

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
    log.warning(path)
    if not path:
        log.error("Missing 'path' in the opmon configuration, exiting.")
        sys.exit(1)

    if opmon_type == "stream" and "monkafka" not in path:
        msg = "OpMon 'stream' configuration must publish to kafka, exiting."
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
        topic = "opmon_stream"

    level = conf.get("level")
    if not level:
        log.warning(
            "Missing 'log_level' in the opmon configuration, using default 'DEBUG'."
        )
        level = logging.DEBUG

    interval_s = conf.get("interval_s")
    if not interval_s:
        log.warning(
            "Missing 'interval_s' in the opmon configuration, using default 10s."
        )
        interval_s = 10.0

    return OpMonConf(
        opmon_type,
        bootstrap,
        topic,
        level,
        interval_s,
        path
    )

def to_string(id: OpMonId) -> str:
    ret = id.get("session")
    if not ret:
        err_msg = "Missing session in OpMonId."
        raise ValueError(err_msg) from None

    application = id.get("application")
    if application:
        ret += "." + application

    substructures = id.get("substructure")
    for substructure in substructures:
        ret += "." + substructure
    return ret

def to_map(value: int | float | bool | str, field_type: int) -> OpMonValue:
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


def make_data(message: Msg, top_block: str = "") -> dict:
    """Map each message entry to the correct data type."""
    message_dict = {}
    for name, descriptor in message.DESCRIPTOR.fields_by_name.items():
        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            continue  # Repeated values not supported in influxdb
        if descriptor.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
            top_block += name + "."
            message_dict = message_dict | make_data(getattr(message, name), top_block)
        else:
            message_dict[top_block + name] = to_map(
                value=getattr(message, name),
                field_type=descriptor.cpp_type
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

def make_origin(session: str, app: str) -> OpMonId:
    opmonid = OpMonId(
        session = session,
        application = app
    )
    return opmonid

def to_entry(
    session: str,
    application: str,
    message: Msg,
    custom_origin: dict[str, str] | None,
    substructure: list[str] | None,
    t: Timestamp,
) -> OpMonEntry:
    """Pack all the data that needs to be published to an OpMonEntry."""
    return OpMonEntry(
        time=t,
        origin=make_origin(session, application),
        custom_origin=validate_custom_origin(custom_origin),
        measurement=message.DESCRIPTOR.full_name,
        data=make_data(message),
    )
