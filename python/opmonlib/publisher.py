import logging
import os
import sys
from datetime import datetime, tzinfo

import pytz
from google.protobuf.message import Message as Msg
from rich.console import Console
from rich.logging import RichHandler
from rich.theme import Theme

from opmonlib.opmon_entry_pb2 import OpMonEntry
from opmonlib.utils import parse_opmon_conf

CONTEXT_SETTINGS = {"help_option_names": ["-h", "--help"]}
CONSOLE_THEMES = Theme({"info": "dim cyan", "warning": "magenta", "danger": "bold red"})

log_levels = {
    "CRITICAL": logging.CRITICAL,
    "ERROR": logging.ERROR,
    "WARNING": logging.WARNING,
    "INFO": logging.INFO,
    "DEBUG": logging.DEBUG,
    "NOTSET": logging.NOTSET,
}
# TODO: for production, remove the filename
full_log_format = "%(asctime)s %(levelname)s %(filename)s %(name)s %(message)s"
# TODO: for production, remove the filename
rich_log_format = "%(filename)s %(name)s %(message)s"
# TODO: include timezone as %Z when the RichHandler starts supporting it in the tty.
date_time_format = "[%Y/%m/%d %H:%M:%S]"
time_zone = pytz.utc


class LoggingFormatter(logging.Formatter):
    """Custom logging formatter for DUNE DAQ applications."""

    def __init__(
        self,
        fmt: str = full_log_format,
        datefmt: str = date_time_format,
        tz: tzinfo = time_zone,
    ) -> None:
        """Construct the logging formatter."""
        super().__init__(fmt, datefmt)
        self.tz = tz
        self.datefmt = datefmt

    def formatTime(self, record: logging.LogRecord, datefmt: str) -> str:  # noqa: N802
        """Apply the correct formatting to the log record date and time."""
        date_time = datetime.fromtimestamp(record.created, self.tz)
        return date_time.strftime(self.datefmt)

    def format(self, record: logging.LogRecord) -> logging.LogRecord:
        """Apply the correct formatting to the log record."""
        record.asctime = self.formatTime(record, self.datefmt)
        # TODO: for production, remove filename and lineno entries
        component_width = 30
        file_lineno = f"{record.filename}:{record.lineno}"
        record.filename = file_lineno.ljust(component_width)[:component_width]
        component_width = 45
        name_colon = f"{record.name}:"
        if name_colon.startswith("drunc."):
            name_colon = name_colon.replace("drunc.", "")
        record.name = name_colon.ljust(component_width)[:component_width]
        component_width = 10
        level_name = record.levelname
        record.levelname = level_name.ljust(component_width)[:component_width]
        return super().format(record)


class OpMonPublisher:
    """Publish operational monitoring metrics to file or stream."""

    def __init__(
        self,
        conf: dict[str:str],
        uri: dict[str:str],
        log_level: int = logging.INFO,
        rich_handler: bool = True,
    ) -> None:
        """Construct the object to publish OpMon metrics to stdout."""
        self.log = logging.getLogger("OpMonPublisher")
        self.log.setLevel(log_level)

        opmon_conf = parse_opmon_conf(self.log, conf, uri)
        self.type = opmon_conf["type"]
        self.bootstrap = opmon_conf["bootstrap"]
        self.level = opmon_conf["level"]
        self.interval_s = opmon_conf["interval_s"]
        self.default_topic = "monitoring." + opmon_conf["topic"]

        self.opmon_producer = logging.getLogger("monitoring.%s", self.default_topic)
        if self.type == "stdout":
            if rich_handler:
                try:
                    width = os.get_terminal_size()[0]
                except OSError:
                    width = 150
                handler = RichHandler(
                    console=Console(width=width),
                    omit_repeated_times=False,
                    markup=True,
                    rich_tracebacks=True,
                    show_path=False,
                    tracebacks_width=width,
                )
                handler.setFormatter(LoggingFormatter(fmt=rich_log_format))
            else:
                handler = logging.StreamHandler(sys.stdout)
                handler.setFormatter(LoggingFormatter(fmt=full_log_format))
        elif self.type == "file":
            handler = logging.FileHandler(self.path)
            handler.setFormatter(LoggingFormatter(fmt=full_log_format))
        else:
            self.log.error("Unsupported OpMon type.")
            sys.exit(1)
        self.log.addHandler(handler)
        return

    def extract_topic(self, message: Msg) -> str:
        """Extract the target topic from the message."""
        if not self.producer:
            self.log.warning(
                "Improperly initialized OpMonProducer used, nothing will be published."
            )
            return None
        return self.default_topic

    def extract_key(self, opmon_entry: OpMonEntry) -> str:
        """Extract  the key from the OpMonEntry."""
        if not self.producer:
            self.log.warning(
                "Improperly initialized OpMonProducer used, nothing will be published."
            )
            return None
        key = str(opmon_entry.origin.session)
        if opmon_entry.origin.application != "":
            key += "." + opmon_entry.origin.application
        for substructure_id in opmon_entry.origin.substructure:
            key += "." + substructure_id
        key += "/" + str(opmon_entry.measurement)
        return key

    def publish(self, message: Msg, metric: OpMonEntry) -> None:
        """Send an OpMonEntry to Kafka."""
        target_topic = self.extract_topic(message)
        target_key = self.extract_key(metric)

        self.producer.send(target_topic, value=metric, key=target_key)
        return
