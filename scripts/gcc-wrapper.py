#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2011, Code Aurora Forum. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Code Aurora nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Invoke gcc, looking for warnings, and causing a failure if there are
# non-whitelisted warnings.

import re
import os
import sys
import subprocess

# Note that gcc uses unicode, which may depend on the locale.  TODO:
# force LANG to be set to en_US.UTF-8 to get consistent warnings.

allowed_warnings = set([
    "alignment.c:720",
    "async.c:127",
    "async.c:283",
    "decompress_bunzip2.c:511",
    "dm.c:1118",
    "dm.c:1146",
    "dm-table.c:1065",
    "dm-table.c:1071",
    "ehci-dbg.c:44",
    "ehci-dbg.c:88",
    "ehci-hcd.c:1048",
    "ehci-hcd.c:423",
    "ehci-hcd.c:614",
    "ehci-hub.c:109",
    "ehci-hub.c:1265",
    "ehci-msm.c:156",
    "ehci-msm.c:201",
    "ehci-msm.c:455",
    "eventpoll.c:1118",
    "gspca.c:1509",
    "ioctl.c:4673",
    "main.c:305",
    "main.c:734",
    "nf_conntrack_netlink.c:762",
    "nf_nat_standalone.c:118",
    "return_address.c:61",
    "scan.c:749",
    "smsc.c:257",
    "yaffs_guts.c:1571",
    "yaffs_guts.c:600",
 ])

# Capture the name of the object file, can find it.
ofile = None

warning_re = re.compile(r'''(.*/|)([^/]+\.[a-z]+:\d+):(\d+:)? warning:''')
def interpret_warning(line):
    """Decode the message from gcc.  The messages we care about have a filename, and a warning"""
    line = line.rstrip('\n')
    m = warning_re.match(line)
    if m and m.group(2) not in allowed_warnings:
        print "error, forbidden warning:", m.group(2)

        # If there is a warning, remove any object if it exists.
        if ofile:
            try:
                os.remove(ofile)
            except OSError:
                pass
        sys.exit(1)

def run_gcc():
    args = sys.argv[1:]
    # Look for -o
    try:
        i = args.index('-o')
        global ofile
        ofile = args[i+1]
    except (ValueError, IndexError):
        pass

    compiler = sys.argv[0]

    proc = subprocess.Popen(args, stderr=subprocess.PIPE)
    for line in proc.stderr:
        print line,
# LGE_CHANGE [2011.07.24] M3S : disable forbidden warnign check routine 
#       interpret_warning(line)

    result = proc.wait()

    return result

if __name__ == '__main__':
    status = run_gcc()
    sys.exit(status)
