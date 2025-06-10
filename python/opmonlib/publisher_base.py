import sys
from abc import ABC, abstractmethod

from google.protobuf.descriptor import FieldDescriptor
from google.protobuf.message import Message as Msg
from google.protobuf.timestamp_pb2 import Timestamp

from opmonlib.conf import OpMonConf
from opmonlib.opmon_entry_pb2 import OpMonEntry, OpMonId, OpMonValue
from opmonlib.utils import (
    LogLevelError,
    logging_log_levels,
    oks_log_levels,
    oks_to_logging_map,
)


class OpMonPublisherBase(ABC):
    """Base class for OpMon publishers."""

    @abstractmethod
    def __init__(self) -> None:
        """Construct the publisher."""
        self.ts = Timestamp()
        pass

    def __post_init__(self) -> None:
        """Perform post-init checks."""
        if not self.log:
            err_msg = "OpMon publisher must have a logger."
            raise (err_msg)
        if not self.conf:
            err_msg = "OpMon publisher must have a configuration."
            raise AttributeError(err_msg)
        if not isinstance(self.conf, OpMonConf):
            err_msg = "OpMon configuration must be of type OpMonConf."
            raise TypeError(err_msg)
        if not self.publisher:
            err_msg = "OpMon publisher must have a publisher"
            raise AttributeError(err_msg)
        if not self.default_topic:
            err_msg = "OpMon publisher must have a default_topic"
            raise AttributeError(err_msg)
        return

    @abstractmethod
    def publish(
        self,
        session: str,
        application: str,
        message: Msg,
        custom_origin: dict[str, str] | None = None,
        substructure: list[str] | None = None,
        level: int | str | None = None,
    ) -> None:
        """Publish an OpMonEntry to the relevant location."""
        pass

    def check_publisher(self) -> None:
        """Validate that the publisher has a valid method to send messages."""
        if not self.publisher:
            missing_producer_err_str = (
                "Improperly initialized OpMonProducer used, nothing will be published."
            )
            self.log.error(missing_producer_err_str)
            sys.exit(1)
        return

    def extract_topic(self, message: Msg) -> str:
        """Extract the target topic from the message."""
        self.check_publisher()
        return self.default_topic

    def make_origin(
        self, session: str, app: str, substructure: dict[str, str] | None
    ) -> OpMonId:
        """Construct and return the OpMonId."""
        return OpMonId(session=session, application=app, substructure=substructure)

    def validate_custom_origin(
        self, custom_origin: dict[str, str] | None = None
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

    def make_data(self, message: Msg, top_block: str = "") -> dict:
        """Map each message entry to the correct data type."""
        message_dict = {}
        for name, descriptor in message.DESCRIPTOR.fields_by_name.items():
            if descriptor.label == FieldDescriptor.LABEL_REPEATED:
                continue  # Repeated values not supported in influxdb
            if descriptor.cpp_type == FieldDescriptor.CPPTYPE_MESSAGE:
                top_block += name + "."
                message_dict = message_dict | self.make_data(
                    getattr(message, name), top_block
                )
            else:
                message_dict[top_block + name] = self.to_map(
                    value=getattr(message, name), field_type=descriptor.cpp_type
                )
        return message_dict

    def to_map(self, value: int | float | bool | str, field_type: int) -> OpMonValue:
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
            case _:
                pass  # Ignore unknown types.
        return formatted_opmonvalue

    def to_entry(
        self,
        session: str,
        application: str,
        message: Msg,
        custom_origin: dict[str, str] | None,
        substructure: list[str] | None,
    ) -> OpMonEntry:
        """Pack all the data that needs to be published to an OpMonEntry."""
        self.ts.GetCurrentTime()
        return OpMonEntry(
            time=self.ts,
            origin=self.make_origin(session, application, substructure),
            custom_origin=self.validate_custom_origin(custom_origin),
            measurement=message.DESCRIPTOR.full_name,
            data=self.make_data(message),
        )

    def log_level_to_int(self, level: str | int) -> int:
        """Convert the log level to the equivalent level in python logging as an int."""
        if isinstance(level, int):
            if level in oks_log_levels.values():
                oks_level_name = next(
                    k for k, v in oks_log_levels.items() if v == level
                )
                return logging_log_levels[oks_to_logging_map[oks_level_name]]
            if level in logging_log_levels.values():
                return level
        elif isinstance(level, str):
            if level in oks_log_levels.keys():
                return logging_log_levels[oks_to_logging_map[level]]
            if level.upper() in logging_log_levels.keys():
                return logging_log_levels[level.upper()]
        raise LogLevelError(level)

    def log_level_to_str(self, level: str | int) -> str:
        """Convert the log level to the equivalent level in python logging as a str."""
        if isinstance(level, str):
            if level in oks_log_levels.keys():
                return oks_to_logging_map[level]
            if level in logging_log_levels.keys():
                return level
        elif isinstance(level, int):
            if level in oks_log_levels.values():
                oks_level_name = next(
                    k for k, v in oks_log_levels.items() if v == level
                )
                return oks_to_logging_map[oks_level_name]
            if level in logging_log_levels.values():
                return next(k for k, v in logging_log_levels.items() if v == level)
        return None
