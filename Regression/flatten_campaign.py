#!/usr/bin/python

# INPUT: 
#   1) base campaign, and
#   2) overlay campaign
#
# OUTPUT: Base campaign PLUS overlay.
#
# NOTE: Events are compaired based on Event_Name.  Events in base but not in the overlay pass through.  Events in overlay but not base also pass through.  For events appearing in both files, the overlay is used.

import json
import sys
import argparse
import string

def make_event_map(json_in):
    event_map = {}
    for event in json_in["Events"]:
        try:
            event_map[event['Event_Name']] = (hash( json.dumps(event, sort_keys=True) ), event)
        except KeyError as e:
            print(event)
            raise Exception( 'The event printed above is missing the required %s key' % str(e) )
    return event_map

def strToEventList( str_in ):
    events_idx = string.find(str_in, 'Events')  # -1 is not found
    start_idx = string.find(str_in, '[', events_idx)  # -1 is not found

    psn = start_idx+1
    square_count = 1
    curly_count = 0
    events_str_array = []
    while square_count > 0 and psn <= len(str_in):
        cur = str_in[psn]

        #if args.verbose:
        #    print (psn, cur, square_count, curly_count, len(events_str_array))

        if cur == '[':
            square_count += 1
        elif cur == ']':
            square_count -= 1
        elif cur == '{':
            if curly_count == 0:
                begin_psn = psn
            curly_count += 1
        elif cur == '}':
            if curly_count == 1:
                events_str_array.append( str_in[begin_psn:psn+1] )
            curly_count -= 1
        psn += 1

    return events_str_array


parser = argparse.ArgumentParser()
parser.add_argument("-O", "--overlay", default='campaign_overlay.json', help="The overlay campaign.json.  Default is campaign_overlay.json")
parser.add_argument("-B", "--base", default="", help="The base campaign.json.  If blank or not specified, defauly will be read from the Default_Campaign_Path in the overlay")
parser.add_argument("-S", "--saveto", default="", help="Filename for the merged campaign.json.  Default will just print to screen")
parser.add_argument("-V", "--verbose", action="store_true", help="Flag for verbose")
args = parser.parse_args()


overlay_str = open( args.overlay).read()
overlay_json = json.loads( overlay_str )
overlay_event_map = make_event_map(overlay_json)
overlay_event_str_list = strToEventList( overlay_str )

try:
    base_fn = args.base
    if( len(args.base) == 0 ):
        base_fn = overlay_json["Default_Campaign_Path"]
except KeyError as e:
    raise Exception( 'The overlay campaign file %s is missing a required key: %s' % (sys.argv[1], str(e)) )

base_str = open( base_fn ).read()
base_json = json.loads( base_str )
base_event_map = make_event_map(base_json)
base_event_str_list = strToEventList( base_str )

merged_json = {}

for key, value in base_json.items():
    if key == "Events":
        # Skip events
        if args.verbose:
            print("Skipping key: %s" % key)
        continue
    elif key in overlay_json:
        if args.verbose:
            print("Key from overlay: %s" % key)
        # Use modified value, if any
        merged_json[key] = overlay_json[key]
    else:
        if args.verbose:
            print("Key from base: %s" % key)
        # Use base
        merged_json[key] = value

merged_json["Default_Campaign_Path"] = base_fn

merged_str = json.dumps(merged_json, indent=4, sort_keys=True )

merged_events_str = ""

overlay_event_name_to_string_map = {}
for overlay_event_str in overlay_event_str_list:
    overlay_event_json = json.loads(overlay_event_str)
    event_name = overlay_event_json["Event_Name"]
    overlay_event_name_to_string_map[event_name] = overlay_event_str

for overlay_event_str in overlay_event_str_list:
    overlay_event_json = json.loads(overlay_event_str)
    event_name = overlay_event_json["Event_Name"]

    if event_name not in base_event_map:
        if args.verbose:
            print("USE OVERLAY (EXCLUSIVE): %s" % event_name)
        merged_events_str += "        %s,\n"%overlay_event_str


for base_event_str in base_event_str_list:
    base_event_json = json.loads(base_event_str)
    event_name = base_event_json["Event_Name"]

    base_hash = base_event_map[event_name][0]

    if event_name in overlay_event_map:
        if args.verbose :
            print("USE OVERLAY: %s" % event_name)
        overlay_event_str = overlay_event_name_to_string_map[event_name]
        merged_events_str += "        %s,\n"%overlay_event_str
    else:
        if args.verbose :
            print("USE BASE: %s" % event_name)
        merged_events_str += "        %s,\n"%base_event_str

merged_events_str = merged_events_str[:-2]  # Chop final ",\n"


insert_psn = merged_str.rfind('}')
insert_psn = merged_str.rfind('\n', insert_psn)
merged_str = merged_str[0:insert_psn-1] \
           + ',\n' \
           + '    "Events":\n' \
           + '    [\n' \
           + merged_events_str + "\n" \
           + '    ]\n' \
           + '}'



if len(args.saveto) > 0:
    if args.verbose:
        print("Writing output to %s" % args.saveto)

    with open(args.saveto, 'w') as outfile:
        outfile.write( merged_str )
if len(args.saveto) == 0 or args.verbose:
    print("Here's the merged campaign:")
    print("--------------------------------")
    print(merged_str)
