#!/usr/bin/python
# /usr/local/bin/wdt_start_poll

import serial
import time
import sys

WDT_DEVICE = "/dev/ttyACM0"

interval=60
target=240

class CommandBlocked(NotImplementedError):
    pass

class VersionMismatch(NotImplementedError):
    pass

class CommandNotSensibleInThisState(NotImplementedError):
    pass

class DigiDog(object):
    """Abstraction of DigiDog device."""

    ARGMAP={
        "A": "timer.armed",
        "C": "timer.current",
        "F": "timer.fired",
        "H": "device.pinout",
        "I": "device.output-levels",
        "J": "timer.locked",
        "K": "device.internal-watchdog",
        "L": "timer.fired.lifetime",
        "M": "target.recovery-method.firmware",
        "N": "target.recovery-method",
        "O": "device.serial",
        "P": "command.executed",
        "Q": "command.blocked",
        "R": "target.reset-timings",
        "S": "timer.start",
        "T": "timer.start.firmware",
        "U": "device.unit-id",
        "V": "device.version",
        "W": "debug.method",
        "X": "command.not-implemented",
        "Z": "target.power-timings",
    }

    @staticmethod
    def parse_result( result ):
        parser_map = {
            "target.power-timings": DigiDog._parse_power_timings,
            "target.reset-timings": DigiDog._parse_int_single,
            "device.internal-watchdog": DigiDog._parse_int_single,
            "target.recovery-method": DigiDog._parse_recovery_method,
            "target.recovery-method.firmware": DigiDog._parse_recovery_method,
            "device.pinout": DigiDog._parse_pinout,
            "device.output-levels": DigiDog._parse_output_levels,
            "timer.armed": DigiDog._parse_bool_single,
            "timer.fired": DigiDog._parse_bool_single,
            "timer.fired.lifetime": DigiDog._parse_int_single,
            "timer.locked": DigiDog._parse_bool_single,
            "timer.start": DigiDog._parse_int_single,
            "timer.start.firmware": DigiDog._parse_int_single,
            "timer.current": DigiDog._parse_int_single,

        }
        parsed = {}
        for char, items in result.items():
            if char in DigiDog.ARGMAP:
                name = DigiDog.ARGMAP[char]
                if name in parser_map:
                    items = parser_map[name](items)
                parsed[name] = items
            else:
                if "unknown" not in parsed:
                    parsed["unknown"] = {}
                parsed["unknown"][char] = items
        return parsed

    @staticmethod
    def _parse_power_timings( items ):
        new_items = []
        for item in items:
            try:
                press1, pause, press2 = item.split(",")
            except ValueError:
                next
            new_items.append({"press1": press1, "pause": pause, "press2": press2})
        return new_items[0]

    @staticmethod
    def _parse_output_levels( items ):
        for item in items:
            try:
                reset, power = item.split(" ")
                reset_on, reset_off = reset.split(",")
                power_on, power_off = power.split(",")
            except ValueError:
                next
            return {
                "reset": {
                    "on": "HIGH" if int(reset_on) else "LOW",
                    "off": "HIGH" if int(reset_off) else "LOW",
                    },
                "power": {
                    "on": "HIGH" if int(power_on) else "LOW",
                    "off": "HIGH" if int(power_off) else "LOW",
                    }
                }
    @staticmethod
    def _parse_pinout( items ):
        new_items = []
        for item in items:
            try:
                reset, power, led = item.split(",")
            except ValueError:
                next
            new_items.append({"reset": reset, "power": power, "led": led})
        return new_items[0]

    @staticmethod
    def _parse_recovery_method( items ):
        new_items = []
        for item in items:
            if item:
                new_items.append("power")
            else:
                new_items.append("reset")
        return new_items[0]

    @staticmethod
    def _parse_bool_single( items ):
        new_items = []
        for item in items:
            new_items.append(bool(int(item)))
        return new_items[0]

    @staticmethod
    def _parse_int_single( items ):
        new_items = []
        for item in items:
            new_items.append(int(item))
        return new_items[0]

    def __init__(self, device):
        """Contructor for Watchdog abstraction."""
        self._device = device

    def _communicate(self, write):
        """Communicate with the device. Send 'write', return lines as array."""
        with serial.Serial(self._device, 9600, xonxoff=False, rtscts=False, timeout=.2) as sdev:
            sdev.write(write)
            time.sleep(0.05)
            timeout = False
            data = ""
            while not timeout:
                curr = sdev.read()
                if not curr:
                    timeout = True
                else:
                    data = data + curr
            lines = [x for x in data.split("\n") if x != ""]
            return lines

    def command(self, command):
        """Send command to device, return results as dict of lists."""

        lines = self._communicate(command)
        commands = {}
        for line in lines:
            try:
                key, value = line.split(":", 1)
            except ValueError:
                key = "unknown"
                value = line
            if key not in commands:
                commands[key] = []
            commands[key].append(value.strip())
        return DigiDog.parse_result(commands)

    def version(self):
        """Return version number."""
        results = self.command("V")
        if "device.version" in results:
            return int(results["device.version"].pop())
        else:
            return 0

    def command_with_version(self, command, version=2):
        """Execute command with if version >= $version."""
        device_version = self.version()
        if device_version >= int(version):
            return self.command(command)
        else:
            raise VersionMismatch("Version requested ({}) was not met by device ({}) - Command '{}'".format(version, device_version, command))

    def blocked_commands(self):
        """Return list of blocked commands on device as list of characters."""
        results = self.command_with_version("Q", 2)
        if "command.blocked" not in results:
            return []
        else:
            blocked_commands = results["command.blocked"]
            try:
                blocked_commands.remove("Q")
            except ValueError:
                raise ValueError("List of blocked commands is not complete.")
            return blocked_commands

    def arm(self):
        """Arm timer by starting it. It may be disallowed to stop it again depending on firmware configuration."""
        results = self.command("X")
        return results

    def disarm(self):
        """Disarm timer, if it is allowed."""
        results = self.command("x")
        if "command.blocked" in results and "x" in results["command.blocked"]:
            raise CommandBlocked("Could not disarm timer because it was not allowed.")
        if "command.executed" in results and "x" in results["command.executed"]:
            return True

    def trigger(self):
        """Trigger a timer reset to keep the device alive."""
        if self.get_timer_armed():
            results = self.command("R")
            return results
        else:
            raise CommandNotSensibleInThisState("Timer is not running. It does not make any sense to trigger it.")

    def timer_up(self):
        """Increase the timer interval."""
        results = self.command_with_version("+", 2)
        if "command.blocked" in results and "+" in results["command.blocked"]:
            raise CommandBlocked("Timer can not be adjusted.")
        return results["timer.start"]

    def timer_down(self):
        """Increase the timer interval."""
        results = self.command_with_version("-", 2)
        if "command.blocked" in results and "-" in results["command.blocked"]:
            raise CommandBlocked("Timer can not be adjusted.")
        return results["timer.start"]

    def set_timer(self, value):
        """Try to set timer to a defined value. If the value can not be met accurately,
           the timer will be set to a value just above it. If the timer can not be set
           to a value high enough, it is set to the highest value possible. The new timer
           value is returned."""
        blocked = self.blocked_commands()
        over = False
        under = False
        stasis = False
        timer_set = False
        timer = self.get_timer_start()
        last = timer
        if value <= 0 or value >=65535:
            raise ValueError("Requested timer value of '{}' implausible. Sensible values are from 0 to 65535".format(value))
        while not timer_set:
            # Save old timer value to see if it was modified
            last = timer
            if timer > value:
                # If timer is over requested value, decrease timer
                timer = self.timer_down()
                # If timer is now below max, set low tide flag, reset high tide flag
                if timer < value:
                    under = True
                    over = False
            elif timer < value:
                # If timer is under requested value, increase timer
                timer = self.timer_up()
                # If timer is now above max, set high tide flag, reset low tide flag
                if timer > value:
                    under = False
                    over = True
            if last == timer:
                timer_set = True
            if timer == value:
                timer_set = True
            if over and not under:
                timer_set = True
        return timer

    def get_timer_start(self):
        """Request timer start value."""
        return self.get_status()["timer.start"]

    def get_timer_current(self):
        """Request current timer value."""
        return self.get_status()["timer.current"]

    def get_timer_armed(self):
        """Request current timer value."""
        return self.get_status()["timer.armed"]

    def get_config(self):
        """Fetch configuration from device"""
        return self.command_with_version("C", 2)

    def get_status(self):
        """Fetch operational status from device"""
        return self.command_with_version("S", 2)

    def eeprom_save(self):
        """Write values to EEPROM if allowed"""
        results = self.command_with_version(">", 2)
        return results

    def eeprom_restore(self):
        """Read values from EEPROM and reset counters - if allowed.
           TODO: Check if fired counter can be reject."""
        results = self.command_with_version("<", 2)
        return results

    def lock(self):
        """Set timer lock if supported."""
        results = self.command_with_version("L", 2)
        if "command.blocked" in results and "L" in results["command.blocked"]:
            raise CommandBlocked("Cannot lock Timer. Command blocked.")
        if "command.executed" in results and "L" in results["command.executed"]:
            return True
        else:
            return False


dev = DigiDog("/dev/ttyACM0")
print dev.set_timer(1200)
timer = dev.get_timer_start()

dev.arm()
try:
    dev.lock()
except Exception as e:
    print "Could not lock timer due to exception {}".format(e)

while True:
    try:
        remaining = dev.get_timer_current()/10
        print "{}s remaining".format(remaining)
        print dev.trigger()
        sleep = timer/50
        print "Sleeping {}s for {}s timer".format(sleep, timer/10)
        time.sleep(sleep)
    except KeyboardInterrupt:
        print dev.disarm()
        break
    except CommandNotSensibleInThisState:
        print "Could not trigger timer because it was not running. Starting timer..."
        try:
            dev.arm()
            dev.lock()
        except Exception as e:
            print "Could not restart timer due to exception: {}".format(e)
    except Exception as e:
        print "Could not reset timer due to exception: {}".format(e)

sys.exit(0)

def calculate_bounds(reset_interval, target_timeout):
    """Calculate a range for the point in time when to reset watchdog."""
    upper_bound = target_timeout + reset_interval
    lower_bound = target_timeout - reset_interval
    if lower_bound < 30:
        lower_bound = 30
    if lower_bound > upper_bound:
        lower_bound = upper_bound
        upper_bound = upper_bound + reset_interval
    return [lower_bound, upper_bound]


def blocked_commands(device):
    """Request blocked commands device. Returns a list of Strings."""
    version_ok = False
    these_blocked_cmds = []
    with serial.Serial(device, 9600, xonxoff=False, rtscts=False, timeout=1) as this_wdt_dev:
        this_wdt_dev.write("V")
        for sline in lines_from_device(this_wdt_dev):
            if sline.startswith("V:"):
                if sline.split("V:")[1].strip() >= "2":
                    version_ok = True
                else:
                    version_ok = False
        if version_ok:
            this_wdt_dev.write("Q")
            for sline in lines_from_device(this_wdt_dev):
                if sline.startswith("Q:"):
                    command = sline.split(":")[1].strip()
                    if not command == "Q":
                        these_blocked_cmds.append(command)
                else:
                    print("Unknown response to Q - {}".format(sline))
    return these_blocked_cmds


def lines_from_device(device):
    """Read all available lines from a serial device and return list of strings."""
    timeout = False
    data = ""
    while not timeout:
        curr = device.read()
        if not curr:
            timeout = True
        else:
            data = data + curr
    lines = [x for x in data.split("\n") if x != ""]
    return lines


detected = False

with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
    wdt_dev.write("V")
    for line in lines_from_device(wdt_dev):
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
    for line in lines_from_device(wdt_dev):
        if line.startswith("N:"):
            if line.split("N:")[1].strip() == "1":
                method = "power cycle"
            else:
                method = "reset"
            print "The device is configured to recover the system by {}".format(method)
    wdt_dev.write("S")
    for line in lines_from_device(wdt_dev):
        if line.startswith("F:"):
            if line.split(":")[1].strip() == "1":
                print "Watchdog fired since last activation"
            else:
                print "Watchdog did not fire since last activation"
    wdt_dev.write("X")
    for line in lines_from_device(wdt_dev):
        if line.startswith("A:"):
            if line.split(":")[1].strip() == "1":
                print "Watchdog successfully enabled"
            else:
                print "Failed to enable watchdog"
        elif line.startswith("S:"):
            print "Timeout approximately {}s".format(int(int(line.split(":")[1])/10))
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

try:
    with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
        wdt_dev.write("V")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("V:"):
                if line.split("V:")[1].strip() >= "1":
                    print("Detected protocol version >=1.0 Watchdog on port {}".format(WDT_DEVICE))
                    detected = True
                else:
                    print("Detected protocol version is not supported")
            if line.startswith("U:"):
                if line.split("U:")[1].strip() == "0":
                    print("Detected unknown device on port {}".format(WDT_DEVICE))
                elif line.split("U:")[1].strip() == "1":
                    print("Detected DigiSpark based DigiDog on port {}".format(WDT_DEVICE))
                else:
                    print("Detected protocol version is not known. This may or may not cause problems.")
        if not detected:
            sys.exit(1)
        wdt_dev.write("C")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("N:"):
                if line.split("N:")[1].strip() == "1":
                    method = "power cycle"
                else:
                    method = "reset"
                print("The device is configured to recover the system by {}".format( method ))
        wdt_dev.write("S")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("F:"):
                if line.split(":")[1].strip() == "1":
                    print("Watchdog fired since last activation")
                else:
                    print("Watchdog did not fire since last activation")
        wdt_dev.write("X")
        for line in lines_from_device( wdt_dev ):
            if line.startswith("A:"):
                if line.split(":")[1].strip() == "1":
                    print("Watchdog successfully enabled")
                else:
                    print("Failed to enable watchdog")
            elif line.startswith("S:"):
                print("Timeout approximately {}s".format( int(int(line.split(":")[1])/10) ))
    blocked_cmds = blocked_commands(WDT_DEVICE)
    if "-" in blocked_cmds or "+" in blocked_cmds:
        print("Device forbids adjusting the watchdog - disabled")
        disable_adjust = True
    else:
        disable_adjust = False
    if "x" in blocked_cmds:
        print("Watchdog forbids stopping the timer! Can not stop timer on exit!")
        disable_stop = True
    else:
        disable_stop = False

    notice_disable_adjust = True

    ## Loop for resetting the watchdog
    while True:
        lower, upper = calculate_bounds(interval, target)
        with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
            wdt_dev.write("S")
            print("resetting timer...")
            wdt_dev.write("R")
            for line in lines_from_device(wdt_dev):
                if line.startswith("C:"):
                    print("Time left was approximately {}s".format( int(int(line.split(":")[1])/10) ))
                    if not disable_adjust:
                        if int(int(line.split(":")[1])/10) > upper:
                            wdt_dev.write("-")
                        if int(int(line.split(":")[1])/10) < lower:
                            wdt_dev.write("+")
                        for iline in lines_from_device(wdt_dev):
                            if iline.startswith("S:"):
                                new_time = int(int(iline.split(":")[1])/10)
                                print("Adjusted watchdog to {}s. target is {}s".format(new_time, target))
                    else:
                        if notice_disable_adjust:
                            print("Not adjusting watchdog - device won't let us.")
                            notice_disable_adjust = False

        time.sleep(interval)

except KeyboardInterrupt:
    with serial.Serial(WDT_DEVICE, 9600, xonxoff=False, rtscts=False, timeout=1) as wdt_dev:
        wdt_dev.write("x")
        for line in lines_from_device(wdt_dev):
            if line.startswith("A:"):
                if line.split(":")[1].strip() == "0":
                    print("Watchdog successfully disabled")
                else:
                    print("Failed to disable watchdog")
            if line.startswith("Q:x"):
                print("Watchdog can not be disabled. Timer is still running.")
