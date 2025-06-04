from __future__ import annotations

import logging
import os
import sys
from datetime import datetime, tzinfo
from pathlib import Path

import conffwk
import pytz
from rich.console import Console
from rich.logging import RichHandler
from rich.theme import Theme

from opmonlib.conf import OpMonConf
from opmonlib.opmon_entry_pb2 import OpMonId

logging_log_levels = {
    "CRITICAL": logging.CRITICAL,
    "ERROR": logging.ERROR,
    "WARNING": logging.WARNING,
    "INFO": logging.INFO,
    "DEBUG": logging.DEBUG,
    "NOTSET": logging.NOTSET,
}

logging_log_level_keys = list(logging_log_levels.keys())
logging_log_level_values = list(logging_log_levels.values())

oks_log_levels = {
    "kTopPriority": 0,
    "kEventDriven": 1073741824,
    "kDefault": 2147483648,
    "kLowestPriority": 4294967295,
}

oks_log_level_keys = list(oks_log_levels.keys())
oks_log_level_values = list(oks_log_levels.values())

oks_to_logging_map = {
    "kTopPriority": "ERROR",
    "kEventDriven": "WARNING",
    "kDefault": "INFO",
    "kLowestPriority": "DEBUG",
}

log_level_keys = logging_log_level_keys + oks_log_level_keys
log_level_values = logging_log_level_values + logging_log_level_values

class LogLevelError(Exception):
    """Custom error for unrecognised log level."""
    def __init__(self, level: str | int) -> None:
        if isinstance(level, str):
            err_msg = f"Level '{level}' is not one of the recognised levels ({log_level_keys})."
        elif isinstance(level, int):
            err_msg = f"Level '{level}' is not one of the recognised levels ({log_level_values})."
        else:
            err_msg = f"Level '{level}' is not of a supported type."
        super().__init__(err_msg)

e_log_levels = (
    f"{logging_log_level_keys} python logging or "
    f"{oks_log_level_keys} for oks log levels."
)

CONTEXT_SETTINGS = {"help_option_names": ["-h", "--help"]}
CONSOLE_THEMES = Theme({"info": "dim cyan", "warning": "magenta", "danger": "bold red"})

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


def setup_rich_handler() -> RichHandler:
    """Initialize a Rich handler for terminal logging."""
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
    return handler


def logging_log_level_from_int(level: int) -> str:
    """Get the level name from its int value."""
    if not isinstance(level, int):
        return level
    for k, v in logging_log_levels.items():
        if v == level:
            return k
    for k, v in oks_log_levels.items():
        if v == level:
            return oks_to_logging_map[k]
    err_str = f"Requested log level with value {level} is not standard ({e_log_levels})"
    raise ValueError(err_str) from None


def logging_log_level_from_str(level: str) -> int:
    """Get the level int from its str value."""
    if not isinstance(level, str):
        return level
    for k, v in logging_log_levels.items():
        if k == level.upper():
            return v
    for k in oks_log_levels.keys():
        if k == level:
            return logging_log_levels[oks_to_logging_map[k]]
    err_str = f"Requested log level with value {level} is not standard ({e_log_levels})"
    raise ValueError(err_str) from None


def parse_opmon_conf(
    log: logging.Logger,
    conf: dict[str:str] | "conffwk.dal.OpMonConf",  # noqa: UP037
    uri: dict[str:str] | "conffwk.dal.OpMonURI",  # noqa: UP037.
) -> dict[str:str]:
    """Parse the OpMonConf and OpMonURI."""
    if not conf:
        log.error("Missing opmon configuration, exiting.")
        sys.exit(1)
    if not uri:
        log.error("Missing opmon URI, exiting.")
        sys.exit(1)

    opmon_type = (
        uri.get("type") if isinstance(uri, dict) else getattr(uri, "type", None)
    )
    if opmon_type:
        log.debug("Found OpMon type: %s", opmon_type)
    else:
        log.debug(
            "Missing 'type' in the opmon configuration, [yellow]using default value "
            "'stdout'[/yellow]."
        )
        opmon_type = "stdout"

    path = uri.get("path") if isinstance(uri, dict) else getattr(uri, "path", None)
    if path:
        log.debug("Found OpMon path: %s", path)
    elif opmon_type != "stdout":
        log.error("Missing 'path' in the opmon configuration, exiting.")
        sys.exit(1)
    else:
        if path == []:
            path = ""
        log.debug("No OpMon path required for type 'stdout'.")

    if opmon_type == "stream" and "monkafka" not in path:
        msg = "OpMon 'stream' configuration must publish to kafka, exiting."
        raise ValueError(msg) from None
    if opmon_type != "stream" and "monkafka" in path:
        msg = "To use kafka, the type must be set to stream."
        raise ValueError(msg) from None

    bootstrap = None
    topic = None
    if opmon_type == "file" and not Path(path).parent.is_dir():
        err_str = "Requested directory to put file in does not exist."
        raise ValueError(err_str) from None
    if "monkafka" in path:
        bootstrap, topic = path.split("/", 1)
    if not topic:
        topic = "opmon_stream"
    log.debug("Using OpMon topic: [green]'%s'[/green]", topic)
    log.debug("Using OpMon bootstrap: [green]'%s'[/green]", bootstrap)

    level = (
        conf.get("level") if isinstance(conf, dict) else getattr(conf, "level", None)
    )
    if level:
        log.debug("Found OpMon level: [green]%s[/green]", level)
    else:
        log.debug(
            "Missing 'level' in the OpMon configuration, [yellow]using default "
            "'DEBUG'[/yellow]."
        )
        level = logging.DEBUG

    interval_s = (
        conf.get("interval_s")
        if isinstance(conf, dict)
        else getattr(conf, "interval_s", None)
    )
    if interval_s:
        log.debug("Found OpMon interval_s: %s", interval_s)
    else:
        log.debug(
            "Missing 'interval_s' in the opmon configuration, [yellow]using default "
            "10s[/yellow]."
        )
        interval_s = 10.0

    return OpMonConf(opmon_type, bootstrap, topic, level, interval_s, path)


def to_string(opmon_id: OpMonId) -> str:
    """Map the OpMonId to a string."""
    ret = opmon_id.get("session")
    if not ret:
        err_msg = "Missing session in OpMonId."
        raise ValueError(err_msg) from None

    application = opmon_id.get("application")
    if application:
        ret += "." + application

    substructures = opmon_id.get("substructure")
    for substructure in substructures:
        ret += "." + substructure
    
    return ret


def extract_opmon_file_path(file_path: str, origin: OpMonId | None = None) -> str:
    """Verify the file path can be opened."""
    hook = "://"
    hook_position = file_path.find(hook)
    fname = None
    if hook_position == -1:
        fname = file_path
    else:
        fname = file_path[hook_position + len(hook) :]

    if origin:
        slash_pos = fname.rfind("/")
        if slash_pos == -1:
            dot_pos = fname.find(".")
        else:
            dot_pos = fname.find(".", slash_pos)
        origin = to_string(origin)
        if dot_pos == -1:
            fname += "." + origin + ".json"
        else:
            fname = fname[:dot_pos] + "." + origin + fname[dot_pos:]

    try:
        with open(fname, "a"):
            pass
    except OSError:
        err_str = f"Can not open file {fname}"
        raise OSError(err_str) from None

    return fname
