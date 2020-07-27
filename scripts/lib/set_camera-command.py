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

import common
from http_client import HttpClient
import base64


def create_option_parser():
    parser = common.create_option_parser()
    parser.add_option(
        "--frame_size",
        dest="frame_size",
        help="QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA"
    )
    parser.add_option(
        "--jpeg_quality",
        dest="jpeg_quality",
        help="JPEG quality (1..63)",
    )
    parser.add_option(
        "--flash",
        dest="flash",
        help="on or off",
    )
    return parser


def main(argv) :
    parser = create_option_parser()
    (options, args) = parser.parse_args()
    try:
        (options, args) = parser.parse_args()
        client = HttpClient(
            options.host, options.port, verbose=options.verbose
        )
        params = {}
        if options.frame_size :
            params['frame_size'] = options.frame_size
        if options.jpeg_quality :
            params['jpeg_quality'] = options.jpeg_quality
        if options.flash :
            params['flash'] = options.flash
        response = client.post("/config/camera", "", params=params)
        client.pretty_print_response(response, False)
        return 0
    except Exception as e:
        import traceback
        traceback.print_exc()
        return -1

if __name__ == "__main__" :
    import sys
    sys.exit(main(sys.argv))
