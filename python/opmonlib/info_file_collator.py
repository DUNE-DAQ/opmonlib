#!/usr/bin/env python3

import json

def collate_info_files(output_file, json_files, console):
    
    console.log(f"Reading specified JSON files and outputting collated value traces to {output_file.name}")

    jsons = []
    jd = json.JSONDecoder()
    for jf in json_files:
        console.log(f"Reading info JSON file {jf.name}")
        text = jf.read()
        idx=0
        while idx < len(text):
            res = jd.raw_decode(text, idx)
            jsons.append(res[0])
            idx = res[1] + 2

    #console.log(jsons)

    data = {}

    # format is session.application.substructure.name: value

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

        measurement = jsonobj["measurement"].replace("dunedaq.","").replace("opmon.","")
        if measurement not in objref:
            objref[measurement] = {}
        objref = objref[measurement]

        customOrigin = ""
        if "custom_origin" in jsonobj:
            first = True
            for k,v in jsonobj["custom_origin"].items():
                if not first:
                    customOrigin += "."
                customOrigin += f"{k}:{v}"
                first = False

        if customOrigin != "":
            if customOrigin not in objref:
                objref[customOrigin] = {}
            objref = objref[customOrigin]


        for datapoint in jsonobj["data"]:
            if datapoint not in objref:
                objref[datapoint] = {}

            for value in jsonobj["data"][datapoint]:
                objref[datapoint][jsonobj["time"]]  = jsonobj["data"][datapoint][value]

    json.dump(data, output_file, indent=4, sort_keys=True)
    console.log(f"Operation complete")


