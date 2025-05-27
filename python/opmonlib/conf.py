from dataclasses import dataclass


@dataclass
class OpMonConf:
    """Define all OpMon configuration parameters."""

    opmon_type: str
    bootstrap: str
    topic: str
    level: int
    interval_s: float
    path: str
