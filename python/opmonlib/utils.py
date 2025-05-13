import logging
import sys
from pathlib import Path
from datetime import datetime
from typing import Optional
from google.protobuf.message import Message as msg
from opmonlib.opmon_entry_pb2 import OpMonValue, OpMonId, OpMonEntry
from google.protobuf.descriptor import FieldDescriptor as fd
from google.protobuf.timestamp_pb2 import Timestamp

def parse_opmon_conf(log: logging.Logger, conf: dict[str: str], uri: dict[str: str]) -> dict[str: str]:
    """Parse the OpMonConf and OpMonURI."""
    if not conf:
        log.error("Missing opmon configuration, exiting.")
        sys.exit(1)
    if not uri:
        log.error("Missing opmon URI, exiting.")
        sys.exit(1)

    type = uri.get("type")
    if not type:
        log.warning("Missing 'type' in the opmon configuration, using default value 'stdout'.")
        type = "stdout"

    path = uri.get("path")
    if not path:
        log.error("Missing 'path' in the opmon configuration, exiting.")
        sys.exit(1)

    if type is "stream" and not "monkafka" in path:
        raise ValueError("OpMon stream configuration must publish to kafka, exiting.")
    elif type is not "stream" and "monkafka" in path:
        raise ValueError("To use kafka, the type must be set to stream.")

    bootstrap = None
    topic = None
    if type == "file" and not Path(path).parent.is_dir():
            log.error("Requested directory to put file in does not exist.")
            sys.exit(1)
    elif "monkafka" in path:
        bootstrap, topic = path.split("/", 1)
    if not topic:
        topic = "OpMon"

    level = conf.get("level")
    if not level:
        log.warning("Missing 'log_level' in the opmon configuration, using default value 'INFO'.")
        level = logging.INFO

    interval_s = conf.get("interval_s")
    if not interval_s:
        log.error("Missing 'interval_s' in the opmon configuration, exiting.")
        sys.exit(1)

    conf = {
        "type": type,
        "bootstrap": bootstrap,
        "topic": topic,
        "level": level,
        "interval_s": interval_s
    }
    return conf


def map_entry(value, field_type:int) -> OpMonValue:
    formatted_OpMonValue = OpMonValue()
    match field_type:
        case fd.CPPTYPE_INT32:
            formatted_OpMonValue.int4_value = value
        case fd.CPPTYPE_INT64:
            formatted_OpMonValue.int8_value = value
        case fd.CPPTYPE_UINT32:
            formatted_OpMonValue.uint4_value = value
        case fd.CPPTYPE_UINT64:
            formatted_OpMonValue.uint8_value = value
        case fd.CPPTYPE_DOUBLE:
            formatted_OpMonValue.double_value = value
        case fd.CPPTYPE_FLOAT:
            formatted_OpMonValue.float_value = value
        case fd.CPPTYPE_BOOL:
            formatted_OpMonValue.boolean_value = value
        case fd.CPPTYPE_STRING:
            formatted_OpMonValue.string_value = value
        # Ignore unknown types.
    return formatted_OpMonValue

def map_message( message:msg, top_block:str=""):
    message_dict = {}
    for name, descriptor in message.DESCRIPTOR.fields_by_name.items():
        if descriptor.label == fd.LABEL_REPEATED:
            continue # Repeated values not supported in influxdb
        elif descriptor.cpp_type == fd.CPPTYPE_MESSAGE:
            top_block += name + "."
            message_dict = message_dict | map_message(getattr(message, name), top_block)
        else:
            message_dict[top_block + name] = map_entry(getattr(message, name), descriptor.cpp_type)
    return message_dict

def validate_custom_origin(custom_origin:Optional[dict[str,str]] = {}):
    for key, value in custom_origin.items():
        if type(value) != "str":
            try:
                custom_origin[key] = str(value)
            except:
                raise TypeError(f"custom_origin[{key}] is not a string and cannot be converted to one.")
    return custom_origin

def pack_to_OpMonEntry(
    session:str,
    application:str,
    message:msg,
    custom_origin:Optional[dict[str,str]],
    substructure:Optional[list[str]],
    t: Timestamp
) -> OpMonEntry:
    opmon_id = OpMonId(
        session = session,
        application = application,
        substructure = substructure
    )
    opmon_entry = OpMonEntry(
        time = t,
        origin = opmon_id,
        custom_origin = validate_custom_origin(custom_origin),
        measurement = message.DESCRIPTOR.full_name,
        data = map_message(message),
    )
    return opmon_entry