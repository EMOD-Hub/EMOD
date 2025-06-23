#!/usr/bin/python

import regression_utils as ru

def application( json_config_path ):
    ru.flattenConfig( json_config_path )
    return "config.json"
