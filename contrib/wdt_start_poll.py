#!/usr/bin/python
# /usr/local/bin/wdt_start_poll

import serial
import time
import sys

WDT_DEVICE="/dev/ttyACM0"

interval=60
target=120


def calculate_bounds( reset_interval, target_timeout ):
    upper_bound = target_timeout + reset_interval
    lower_bound = target_timeout - reset_interval
    if lower_bound < 30:
        lower_bound = 30
    if lower_bound > upper_bound:
        lower_bound = upper_bound
        upper_bound = upper_bound + reset_interval
    return [ lower_bound, upper_bound ]

def blocked_commands( device ):
    version_ok=False
    these_blocked_cmds = []
    with serial.Serial(device, 9600, xonxoff=False, rtscts=False, timeout=1) as this_wdt_dev:
        this_wdt_dev.write("V")
        for sline in lines_from_device( this_wdt_dev ):
            if sline.startswith("V:"):
                if sline.split("V:")[1].strip() >= "2":
                    version_ok=True
                else:
                    version_ok=False
        if version_ok:
            this_wdt_dev.write("Q")
            for sline in lines_from_device( this_wdt_dev ):
                if sline.startswith("Q:"):
                    command = sline.split(":")[1].strip()
                    if not command == "Q":
                        these_blocked_cmds.append(command)
                else:
                    print "Unknown response to Q - {}".format(sline)
    return these_blocked_cmds


def lines_from_device( device ):
    timeout = False
    data = ""
    while not timeout:
        curr = device.read()
        if not curr:
            timeout = True
        else:
            data = data + curr
    lines = [ x for x in data.split("\n") if x != "" ]
    return lines

detected = False
try:
    with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
        wdt_dev.write("V")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("V:"):
                if line.split("V:")[1].strip() >= "1":
                    print "Detected protocol version >=1.0 Watchdog on port {}".format(WDT_DEVICE)
                    detected = True
                else:
                    print "Detected protocol version is not supported"
            if line.startswith("U:"):
                if line.split("U:")[1].strip() == "0":
                    print "Detected unknown device on port {}".format(WDT_DEVICE)
                elif line.split("U:")[1].strip() == "1":
                    print "Detected DigiSpark based DigiDog on port {}".format(WDT_DEVICE)
                else:
                    print "Detected protocol version is not known. This may or may not cause problems."
        if not detected:
            sys.exit(1)
        wdt_dev.write("C")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("N:"):
                if line.split("N:")[1].strip() == "1":
                    method = "power cycle"
                else:
                    method = "reset"
                print "The device is configured to recover the system by {}".format( method )
        wdt_dev.write("S")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("F:"):
                if line.split(":")[1].strip() == "1":
                    print "Watchdog fired since last activation"
                else:
                    print "Watchdog did not fire since last activation"
        wdt_dev.write("X")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("A:"):
                if line.split(":")[1].strip() == "1":
                    print "Watchdog successfully enabled"
                else:
                    print "Failed to enable watchdog"
            elif line.startswith("S:"):
                print "Timeout approximately {}s".format( int(int(line.split(":")[1])/10) )
    blocked_cmds = blocked_commands(WDT_DEVICE)
    if "-" in blocked_cmds or "+" in blocked_cmds:
        print "Device forbids adjusting the watchdog - disabled"
        disable_adjust = True
    else:
        disable_adjust = False
    if "x" in blocked_cmds:
        print "Watchdog forbids stopping the timer! Can not stop timer on exit!"
        disable_stop = True
    else:
        disable_stop = False

    notice_disable_adjust = True

    ## Loop for resetting the watchdog
    while True:
        lower, upper = calculate_bounds( interval, target )
        with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
            wdt_dev.write("S")
            print "resetting timer..."
            wdt_dev.write("R")
            for line in lines_from_device( wdt_dev ):
                if line.startswith("C:"):
                    print "Time left was approximately {}s".format( int(int(line.split(":")[1])/10) )
                    if not disable_adjust:
                        if int(int(line.split(":")[1])/10) > upper:
                            wdt_dev.write("-")
                        if int(int(line.split(":")[1])/10) < lower:
                            wdt_dev.write("+")
                        for iline in lines_from_device( wdt_dev ):
                            if iline.startswith("S:"):
                                new_time = int(int(iline.split(":")[1])/10)
                                print "Adjusted watchdog to {}s. target is {}s".format(new_time, target)
                    else:
                        if notice_disable_adjust:
                            print "Not adjusting watchdog - device won't let us."
                            notice_disable_adjust = False

        time.sleep(interval)

except KeyboardInterrupt:
    with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
        wdt_dev.write("x")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("A:"):
                if line.split(":")[1].strip() == "0":
                    print "Watchdog successfully disabled"
                else:
                    print "Failed to disable watchdog"
            if line.startswith("Q:x"):
                print "Watchdog can not be disabled. Timer is still running."
