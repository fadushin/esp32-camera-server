#!/usr/bin/env python
#
# Copyright (c) dushin.net
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of dushin.net nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY dushin.net ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL dushin.net BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from http_client import HttpClient
import subprocess

def create_option_parser():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option(
        "--host",
        dest="host",
        help="ESP32 cam host (192.168.4.1)",
        type="string",
        default="192.168.4.1"
    )
    parser.add_option(
        "--port",
        dest="port",
        help="Riak port (80)",
        type="int",
        default=80
    )
    parser.add_option(
        "--verbose",
        dest="verbose",
        action="store_true",
        help="verbose mode",
    )
    return parser


def invoke(command):
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    (stdout, stderr) = proc.communicate()
    if proc.returncode != 0:
        raise Exception(
            "An error occurred executing command '%s'.  Return code: %s.  stdout: '%s'.  stderr: '%s'" %
            (command, proc.returncode, stdout, stderr)
        )
    return stdout

def log(message):
    #print("LOG> {}".format(message))
    pass
