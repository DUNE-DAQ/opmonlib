#!/usr/bin/env python3
import json

import click
from rich.console import Console

def collate_info_files(
    json_files: list, console: Console = None, output_file: click.File = None
) -> dict:
    """Collate the json information into an output file."""
    if console is not None and output_file is not None:
        console.log(
            f"Reading specified JSON files and outputting collated value traces to {output_file.name}"
        )

    jsons = []
    jd = json.JSONDecoder()
    for jf in json_files:
        if console is not None:
            console.log(f"Reading info JSON file {jf.name}")
        if "pathlib.PosixPath" in str(type(jf)):
            text = jf.read_text()
        else:
            text = jf.read()
        idx = 0
        while idx < len(text):
            res = jd.raw_decode(text, idx)
            jsons.append(res[0])
            idx = res[1]
            while idx < len(text) and text[idx] != '{':
                idx += 1
    data = {}

    for jsonobj in jsons:
        session = jsonobj["origin"]["session"]
        application = jsonobj["origin"]["application"]

        if session not in data:
            data[session] = {}

        if application not in data[session]:
            data[session][application] = {}

        objref = data[session][application]
        if "substructure" in jsonobj["origin"]:
            for sub in jsonobj["origin"]["substructure"]:
                if sub not in objref:
                    objref[sub] = {}
                objref = objref[sub]

        measurement = (
            jsonobj["measurement"].replace("dunedaq.", "").replace("opmon.", "")
        )
        if measurement not in objref:
            objref[measurement] = {}
        objref = objref[measurement]

        custom_origin = ""
        if "custom_origin" in jsonobj:
            first = True
            for k, v in jsonobj["custom_origin"].items():
                if not first:
                    custom_origin += "."
                custom_origin += f"{k}:{v}"
                first = False

        if custom_origin != "":
            if custom_origin not in objref:
                objref[custom_origin] = {}
            objref = objref[custom_origin]

        for datapoint in jsonobj["data"]:
            if datapoint not in objref:
                objref[datapoint] = {}

            for value in jsonobj["data"][datapoint]:
                objref[datapoint][jsonobj["time"]] = jsonobj["data"][datapoint][value]

    if output_file is not None:
        json.dump(data, output_file, indent=4, sort_keys=True)
    if console is not None:
        console.log("Operation complete")
    return data
