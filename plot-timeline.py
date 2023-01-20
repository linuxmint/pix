#!/usr/bin/env python3

import math
import optparse
import os
import re
import sys
import cairo

FONT_NAME = "Bitstream Vera Sans"
FONT_SIZE = 10
PIXELS_PER_SECOND = 1000
PIXELS_PER_LINE = 12
PLOT_WIDTH = 1400
TIME_SCALE_WIDTH = 20
SYSCALL_MARKER_WIDTH = 20
LOG_TEXT_XPOS = 300
LOG_MARKER_WIDTH = 20
BACKGROUND_COLOR = (0, 0, 0)

# list of strings to ignore in the plot
ignore_strings = [
#    "nautilus_directory_async_state_changed"
    ]

# list of pairs ("string", (r, g, b)) to give a special color to some strings
special_colors = [
    ("nautilus_window_size_allocate", (1, 1, 1)),
    ("STARTING MAIN LOOP", (1, 0, 0)),
    ("directory_load_callback", (1, 1, 0)),
    ("nautilus_directory_new", (1, 1, 0)),
    ("nautilus_directory_async_state_changed", (1, 1, 0)),
    ("fm-directory-view.c: finish_loading", (1, 0.5, 0)),
    ("fm-directory-view.c", (1, 1, 0)),
    ("fm_icon_view_begin_loading", (1, 0, 1)),
    ("location_has_really_changed", (1, 0, 1)),
    ("update_for_new_location", (1, 0, 1)),
    ("expose_event", (0, 1, 1)),
    ("fm_icon_view_add_file", (0, 1, 1)),
    ]

def get_special_color (string):
    for sc in special_colors:
        if string.find (sc[0]) >= 0:
            return sc[1]

    return None

def string_has_substrings (string, substrings):
    for i in substrings:
        if string.find (i) >= 0:
            return True

    return False

# basic (non-strace) format
# Example:
# doozerd: store 18:53:24.599973 store.go:266: apply set 4 /doozer/slot/1 a0019c85.9ac8ceaa 4 <nil>
doozer_regex = re.compile (r'^doozerd:.*(\d+):(\d+):(\d+.\d+) +(.*)')
doozer_hour_group = 1
doozer_min_group  = 2
doozer_sec_group  = 3
doozer_log_group  = 4

# assumes "strace -ttt -f"
mark_regex = re.compile (r'^\d+ +(\d+\.\d+) +access\("MARK: ([^:]*: )(.*)", F_OK.*')
mark_timestamp_group = 1
mark_program_group = 2
mark_log_group = 3

# 3273  1141862703.998196 execve("/usr/bin/dbus-launch", ["/usr/bin/dbus-launch", "--sh-syntax", "--exit-with-session", "/usr/X11R6/bin/gnome"], [/* 61 vars */]) = 0
# 3275  1141862704.003623 execve("/home/devel/bin/dbus-daemon", ["dbus-daemon", "--fork", "--print-pid", "8", "--print-address", "6", "--session"], [/* 61 vars */]) = -1 ENOENT (No such file or directory)
complete_exec_regex = re.compile (r'^(\d+) +(\d+\.\d+) +execve\("(.*)", \[".*= (0|-1 ENOENT \(No such file or directory\))$')
complete_exec_pid_group = 1
complete_exec_timestamp_group = 2
complete_exec_command_group = 3
complete_exec_result_group = 4

# 3283  1141862704.598008 execve("/opt/gnome/lib/GConf/2/gconf-sanity-check-2", ["/opt/gnome/lib/GConf/2/gconf-san"...], [/* 66 vars */] <unfinished ...>
unfinished_exec_regex = re.compile (r'^(\d+) +(\d+\.\d+) +execve\("(.*)", \[".*<unfinished \.\.\.>$')
unfinished_exec_pid_group = 1
unfinished_exec_timestamp_group = 2
unfinished_exec_command_group = 3

# 3283  1141862704.598704 <... execve resumed> ) = 0
# 3309  1141862707.027481 <... execve resumed> ) = -1 ENOENT (No such file or directory)
resumed_exec_regex = re.compile (r'^(\d+) +(\d+\.\d+) +<\.\.\. execve resumed.*= (0|-1 ENOENT \(No such file or directory\))$')
resumed_exec_pid_group = 1
resumed_exec_timestamp_group = 2
resumed_exec_result_group = 3

success_result = "0"

class BaseMark:
    colors = 0, 0, 0
    def __init__(self, timestamp, log):
        self.timestamp = timestamp
        self.log = log
        self.timestamp_ypos = 0
        self.log_ypos = 0

class SimpleMark(BaseMark):
    colors = 1.0, 0, 0
    pass

class AccessMark(BaseMark):
    pass

class LastMark(BaseMark):
    colors = 1.0, 0, 0

class FirstMark(BaseMark):
    colors = 1.0, 0, 0

class ExecMark(BaseMark):
#    colors = 0.75, 0.33, 0.33
    colors = (1.0, 0.0, 0.0)
    def __init__(self, timestamp, log, is_complete, is_resumed):
#        if is_complete:
        text = 'execve: '
#        elif is_resumed:
#            text = 'execve resumed: '
#        else:
#            text = 'execve started: '

        text = text + os.path.basename(log)
        BaseMark.__init__(self, timestamp, text)

class Metrics:
    def __init__(self):
        self.width = 0
        self.height = 0

# don't use black or red
palette = [
    (0.12, 0.29, 0.49),
    (0.36, 0.51, 0.71),
    (0.75, 0.31, 0.30),
    (0.62, 0.73, 0.38),
    (0.50, 0.40, 0.63),
    (0.29, 0.67, 0.78),
    (0.96, 0.62, 0.34),
    (1.0 - 0.12, 1.0 - 0.29, 1.0 - 0.49),
    (1.0 - 0.36, 1.0 - 0.51, 1.0 - 0.71),
    (1.0 - 0.75, 1.0 - 0.31, 1.0 - 0.30),
    (1.0 - 0.62, 1.0 - 0.73, 1.0 - 0.38),
    (1.0 - 0.50, 1.0 - 0.40, 1.0 - 0.63),
    (1.0 - 0.29, 1.0 - 0.67, 1.0 - 0.78),
    (1.0 - 0.96, 1.0 - 0.62, 1.0 - 0.34)
    ]

class SyscallParser:
    def __init__ (self):
        self.pending_execs = []
        self.syscalls = []

    def search_pending_execs (self, search_pid):
        n = len (self.pending_execs)
        for i in range (n):
            (pid, timestamp, command) = self.pending_execs[i]
            if pid == search_pid:
                return (i, timestamp, command)

        return (None, None, None)

    def add_line (self, str):
        m = doozer_regex.search (str)
        if m:
            H = int (m.group(doozer_hour_group))
            M = int (m.group(doozer_min_group))
            S = float (m.group(doozer_sec_group))

            timestamp = (H*3600)+(M*60)+(S)
            command = m.group (doozer_log_group)
            self.syscalls.append (SimpleMark (timestamp, command))

            return

        m = mark_regex.search (str)
        if m:
            timestamp = float (m.group (mark_timestamp_group))
            program = m.group (mark_program_group)
            text = program + m.group (mark_log_group)
            if text == 'last':
                self.syscalls.append (LastMark (timestamp, text))
            elif text == 'first':
                self.syscalls.append (FirstMark (timestamp, text))
            else:
#                if program == "nautilus: ":
                if not string_has_substrings (text, ignore_strings):
                    s = AccessMark (timestamp, text)
                    c = get_special_color (text)
                    if c:
                        s.colors = c
                    else:
                        program_hash = program.__hash__ ()
                        s.colors = palette[program_hash % len (palette)]

                    self.syscalls.append (s)

            return

        m = complete_exec_regex.search (str)
        if m:
            result = m.group (complete_exec_result_group)
            if result == success_result:
                pid = m.group (complete_exec_pid_group)
                timestamp = float (m.group (complete_exec_timestamp_group))
                command = m.group (complete_exec_command_group)
                self.syscalls.append (ExecMark (timestamp, command, True, False))

            return

        m = unfinished_exec_regex.search (str)
        if m:
            pid = m.group (unfinished_exec_pid_group)
            timestamp = float (m.group (unfinished_exec_timestamp_group))
            command = m.group (unfinished_exec_command_group)
            self.pending_execs.append ((pid, timestamp, command))
#            self.syscalls.append (ExecMark (timestamp, command, False, False))
            return

        m = resumed_exec_regex.search (str)
        if m:
            pid = m.group (resumed_exec_pid_group)
            timestamp = float (m.group (resumed_exec_timestamp_group))
            result = m.group (resumed_exec_result_group)

            (index, old_timestamp, command) = self.search_pending_execs (pid)
            if index == None:
                print("Didn't find pid %s in pending_execs!" % pid)
                sys.exit (1)

            del self.pending_execs[index]

            if result == success_result:
                self.syscalls.append (ExecMark (timestamp, command, False, True))

def parse_strace(filename):
    parser = SyscallParser ()

    for line in open(filename, "r").readlines():
        if line == "":
            break

        parser.add_line (line)

    return parser.syscalls

def normalize_timestamps(syscalls):

    first_timestamp = syscalls[0].timestamp

    for syscall in syscalls:
        syscall.timestamp -= first_timestamp

def compute_syscall_metrics(syscalls):
    num_syscalls = len(syscalls)

    metrics = Metrics()
    metrics.width = PLOT_WIDTH

    last_timestamp = syscalls[num_syscalls - 1].timestamp
    num_seconds = int(math.ceil(last_timestamp))
    metrics.height = max(num_seconds * PIXELS_PER_SECOND,
                         num_syscalls * PIXELS_PER_LINE)

    text_ypos = 0

    for syscall in syscalls:
        syscall.timestamp_ypos = syscall.timestamp * PIXELS_PER_SECOND
        syscall.log_ypos = text_ypos + FONT_SIZE

        text_ypos += PIXELS_PER_LINE

    return metrics

def plot_time_scale(surface, ctx, metrics):
    num_seconds = int((metrics.height + PIXELS_PER_SECOND - 1) / PIXELS_PER_SECOND)

    ctx.set_source_rgb(0.5, 0.5, 0.5)
    ctx.set_line_width(1.0)

    for i in range(num_seconds):
        ypos = i * PIXELS_PER_SECOND

        ctx.move_to(0, ypos + 0.5)
        ctx.line_to(TIME_SCALE_WIDTH, ypos + 0.5)
        ctx.stroke()

        ctx.move_to(0, ypos + 2 + FONT_SIZE)
        ctx.show_text("%d s" % i)

def plot_syscall(surface, ctx, syscall):
    ctx.set_source_rgb(*syscall.colors)

    # Line

    ctx.move_to(TIME_SCALE_WIDTH, syscall.timestamp_ypos)
    ctx.line_to(TIME_SCALE_WIDTH + SYSCALL_MARKER_WIDTH, syscall.timestamp_ypos)
    ctx.line_to(LOG_TEXT_XPOS - LOG_MARKER_WIDTH, syscall.log_ypos - FONT_SIZE / 2 + 0.5)
    ctx.line_to(LOG_TEXT_XPOS, syscall.log_ypos - FONT_SIZE / 2 + 0.5)
    ctx.stroke()

    # Log text

    ctx.move_to(LOG_TEXT_XPOS, syscall.log_ypos)
    ctx.show_text("%8.5f: %s" % (syscall.timestamp, syscall.log))

def plot_syscalls_to_surface(syscalls, metrics):
    num_syscalls = len(syscalls)

    surface = cairo.ImageSurface(cairo.FORMAT_RGB24,
                                 metrics.width, metrics.height)

    ctx = cairo.Context(surface)
    ctx.select_font_face(FONT_NAME)
    ctx.set_font_size(FONT_SIZE)

    # Background

    ctx.set_source_rgb (*BACKGROUND_COLOR)
    ctx.rectangle(0, 0, metrics.width, metrics.height)
    ctx.fill()

    # Time scale

    plot_time_scale(surface, ctx, metrics)

    # Contents

    ctx.set_line_width(1.0)

    for syscall in syscalls:
        plot_syscall(surface, ctx, syscall)

    return surface

def main(args):
    option_parser = optparse.OptionParser(
        usage="usage: %prog -o output.png <strace.txt>")
    option_parser.add_option("-o",
                             "--output", dest="output",
                             metavar="FILE",
                             help="Name of output file (output is a PNG file)")

    options, args = option_parser.parse_args()

    if not options.output:
        print('Please specify an output filename with "-o file.png" or "--output=file.png".')
        return 1

    if len(args) != 1:
        print('Please specify only one input filename, which is an strace log taken with "strace -ttt -f"')
        return 1

    in_filename = args[0]
    out_filename = options.output

    syscalls = []
    for syscall in parse_strace(in_filename):
        syscalls.append(syscall)
        if isinstance(syscall, FirstMark):
            syscalls = []
        elif isinstance(syscall, LastMark):
            break

    if not syscalls:
        print('No marks in %s, add access("MARK: ...", F_OK)' % in_filename)
        return 1

    normalize_timestamps(syscalls)
    metrics = compute_syscall_metrics(syscalls)

    surface = plot_syscalls_to_surface(syscalls, metrics)
    surface.write_to_png(out_filename)

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
