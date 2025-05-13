import logging
import sys

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
    if "monkafka" in path:
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