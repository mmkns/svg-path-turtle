/*
 *  MIT License
 *  
 *  Copyright (c) 2026 mmkns
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "Options.h"
#include "version.h"

#include <string>
#include <cstring>
#include <utility>

//////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//////////////////////////////////////////////////////////////////////////////

static const char *s_command_name = "COMMAND_NAME_NOT_YET_KNOWN";

static void exit_w_usage(const std::string &msg = {})
{
    if(!msg.empty())
	std::cerr << "ERROR: " << msg << '\n';

    std::cerr << "Usage: " << s_command_name
	      << " [OPTION]... [INFILE] [OUTFILE]" << '\n';

    std::cerr << R"(
Output
 --optimize           - drop unnecessary whitespace in output
 --decimal-places <N> - decimal places in output
 --prettyprint        - each SVG command on a separate line
 --no-pen-error       - disable the pen height warning

Debugging
 -s                   - wrap output in basic 500x500 SVG file
 --svg-out "w h [bg-color path-fill path-stroke stroke-width linejoin linecap]"
		      - same as -s, but configurable.
			 Defaults:
			  background color   = white
			  path fill color    = lightblue
			  path stroke color  = black
			  stroke-width       = 1.5
			  linejoin           = round
			  linecap            = round

 --debug              - line numbers on all errors; backtrace on exceptions
 --trace              - trace execution
 --trace-parse        - trace parsing
 --show-breaks        - show when the 'breakpoint' command is encountered
 --list-chunks        - show list of all functions and local blocks

Other
 -h,--help            - show this help
 --version            - print program version

If INFILE is "-" or "" or not present, defaults to stdin.
If OUTFILE is "-" or "" or not present, defaults to stdout.

)";

    exit(1);
}

static void exit_w_version()
{
    std::cout << "svg_path_turtle version "
	      << SVG_PATH_TURTLE_VERSION
	      << "\n";
    exit(0);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Command line
//
//////////////////////////////////////////////////////////////////////////////

void Options::parse_command_line(int argc, char **argv)
{
    s_command_name = argv[0];

    bool end_of_options = false;
    bool has_input_file = false;
    bool has_output_file = false;

    for(int i = 1; i < argc; ++i)
    {
	const char *arg = argv[i];

	auto opt = [arg, end_of_options](const char *s)
		    {
		       	return !end_of_options && strcmp(arg, s) == 0;
		    };

	if(!end_of_options && opt("--"))    end_of_options = true;
	else if(opt("--help") || opt("-h")) exit_w_usage();
	else if(opt("--version"))           exit_w_version();
	else if(opt("--debug"))             debug = true;
	else if(opt("--trace"))             ++call_trace_level;
	else if(opt("--trace-parse"))       ++parse_trace_level;
	else if(opt("--list-chunks"))       list_chunks = true;
	else if(opt("--show-breaks"))       report_breakpoints = true;
	else if(opt("--optimize"))          optimize = true;
	else if(opt("--prettyprint"))       prettyprint = true;
	else if(opt("--no-pen-error"))      disable_pen_warning = true;
	else if(opt("-s"))                  svg_out.enable();
	else if(opt("--decimal-places"))
	{
	    ++i;
	    if(i == argc)
		exit_w_usage("--decimal-places requires a number");

	    try
	    {
		decimal_places = std::stol(argv[i]);
	    }
	    catch(...)
	    {
		exit_w_usage("--decimal-places: invalid number");
	    }
	}
	else if(opt("--svg-out"))
	{
	    ++i;
	    if(i == argc)
		exit_w_usage("--svg-out requires width,height[,...]");

	    if(!svg_out.configure(argv[i]))
		exit_w_usage("Invalid config for --svg-out option");
	}
	else if(!end_of_options && arg[0] == '-' && arg[1])
	    exit_w_usage("Unrecognized option: " + std::string(arg));
	else if(!std::exchange(has_input_file, true))
	    input_filename = arg;
	else if(!std::exchange(has_output_file, true))
	    output_filename = arg;
	else
	    exit_w_usage("Too many filenames.");
    }

    if(call_trace_level || parse_trace_level || list_chunks || report_breakpoints)
	debug = true;

    if(optimize && prettyprint)
	exit_w_usage("Only one of --optimize or --prettyprint is allowed");
}
