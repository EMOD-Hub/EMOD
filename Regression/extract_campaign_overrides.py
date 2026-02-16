#!/usr/bin/python

# INPUT: 
#   1) base campaign, and
#   2) modified campaign
#
# OUTPUT: Any events in the modified campaign that are different from or do not exist in the base campaign
#
# NOTE: Events are compaired based on Event_Name using a hash of the json.dumps

import json
import sys
import string
import argparse

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
parser.add_argument("-B", "--base", default='campaign_baseline.json', help="The file path to the base campaign.json.  Default is 'campaign_baseline.json'")
parser.add_argument("-M", "--modified", default='campaign_modified.json', help="The file path to the modified campaign.json.  Default is 'campaign_modified.json'")
parser.add_argument("-S", "--saveto", default="", help="The file path to save the resulting overlay campaign.json.  Default will just print to screen")
parser.add_argument("-V", "--verbose", action="store_true", help="Flag for verbose")
args = parser.parse_args()


base_str = open( args.base ).read()
base_json = json.loads( base_str )
base_event_str_list = strToEventList( base_str )
base_event_map = make_event_map(base_json)

mod_str = open( args.modified ).read()
mod_json = json.loads( mod_str )
mod_event_str_list = strToEventList( mod_str )
mod_event_map = make_event_map(mod_json)

diff_json = {}

for key, value in base_json.items():
    if key == "Events":
        # Skip events
        if args.verbose:
            print("Skipping key: %s" % key)
        continue
    elif key in mod_json:
        if args.verbose:
            print("Key from mod: %s" % key)
        # Use modified value, if any
        diff_json[key] = mod_json[key]
    else:
        if args.verbose:
            print("Key from base: %s" % key)
        # Use base
        diff_json[key] = value

diff_json["Default_Campaign_Path"] = args.base

diff_str = json.dumps(diff_json, indent=4, sort_keys=True )

diff_str_events = ""

for event_str in mod_event_str_list:
    event_json = json.loads(event_str)
    event_name = event_json["Event_Name"]

    mod_hash = mod_event_map[event_name][0]
    if args.verbose:
        print("Event_Name: %s" % event_name)

    if event_name not in base_event_map:
        if args.verbose:
            print("NEW: %s" % event_name)
        diff_str_events += "        %s,\n"%event_str
    elif base_event_map[event_name][0] != mod_hash:
        if args.verbose:
            print("MODIFIED: %s" % event_name)
        diff_str_events += "        %s,\n"%event_str
    elif args.verbose:
        print("MATCH: %s" % event_name)

diff_str_events = diff_str_events[:-2]  # Chop final ",\n"

insert_psn = diff_str.rfind('}')
insert_psn = diff_str.rfind('\n', insert_psn)
diff_str = diff_str[0:insert_psn-1] \
           + ',\n' \
           + '    "Events":\n' \
           + '    [\n' \
           + diff_str_events + "\n" \
           + '    ]\n' \
           + '}'


if len(args.saveto) > 0:
    if args.verbose:
        print("Writing output to %s" % args.saveto)

    with open(args.saveto, 'w') as outfile:
        outfile.write( diff_str )
if len(args.saveto) == 0 or args.verbose:
    print("Here's the resulting difference:")
    print("--------------------------------")
    print(diff_str)
