import logging
import sys

from google.protobuf.json_format import MessageToJson
from google.protobuf.message import Message as Msg

from opmonlib.conf import OpMonConf
from opmonlib.publisher_base import OpMonPublisherBase
from opmonlib.utils import (
    LoggingFormatter,
    extract_opmon_file_path,
    full_log_format,
    log_level_from_int,
    log_level_from_str,
    setup_rich_handler,
)


class OpMonPublisher(OpMonPublisherBase):
    """Publish operational monitoring metrics to file or stream."""

    def __init__(
        self,
        conf: OpMonConf,
        log_level: int | str = logging.INFO,
        rich_handler: bool = False,
    ) -> None:
        """Construct the object to publish OpMon metrics to stdout."""
        super().__init__()
        self.log = logging.getLogger("OpMonPublisher")
        if isinstance(log_level, str):
            log_level = log_level_from_str(log_level)
        self.log.setLevel(log_level)
        self.log.addHandler(setup_rich_handler())

        self.conf = conf
        if isinstance(self.conf.level, str):
            self.conf.level = log_level_from_str(self.conf.level)

        if self.conf.opmon_type == "stdout":
            if rich_handler:
                handler = setup_rich_handler()
            else:
                handler = logging.StreamHandler(sys.stdout)
                handler.setFormatter(LoggingFormatter(fmt=full_log_format))
        elif self.conf.opmon_type == "file":
            self.conf.path = extract_opmon_file_path(self.conf.path)
            handler = logging.FileHandler(self.conf.path)
        elif self.conf.opmon_type == "stream":
            self.log.error("Type must not be stream to use file or stdout handling.")
            sys.exit(1)
        else:
            self.log.error("Unsupported OpMon type.")
            sys.exit(1)

        self.default_topic = "monitoring." + self.conf.topic
        self.publisher = logging.getLogger(self.default_topic)
        self.publisher.addHandler(handler)
        self.publisher.setLevel(self.conf.level)

        super().__post_init__()
        return

    def publish_message(
        self, logger: logging.Logger, level_name: int | str, message: str
    ) -> None:
        """Log the metric with the appropriate level."""
        method = getattr(logger, log_level_from_int(level_name).lower(), logger.info)
        method(message)
        return

    def publish(
        self,
        session: str,
        application: str,
        message: Msg,
        custom_origin: dict[str, str] | None = None,
        substructure: list[str] | None = None,
        level: int | str | None = None,
    ) -> None:
        """Publish the message to either a file or the terminal."""
        if not isinstance(message, Msg):
            self.log.error("Passed message needs to be of type google.protobuf.message")
            return
        if isinstance(level, str):
            level = log_level_from_int(level)
        if not level:
            level = self.conf.level
        if level < self.conf.level:
            return
        metric = self.to_entry(
            session=session,
            application=application,
            message=message,
            custom_origin=custom_origin,
            substructure=substructure,
        )
        target_topic = self.extract_topic(message)
        target_key = self.extract_key(metric)
        publishing_logger = logging.getLogger(
            f"{self.publisher.name}.{target_topic}.{target_key}"
        )
        self.publish_message(publishing_logger, level, MessageToJson(metric))
        return
