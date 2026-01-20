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

#include "EasyParser.h"

#include "Turtle.h"
#include "Signature.h"
#include "Engine.h"
#include "Names.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "Debug.h"
#include "BasicSVG.h"
#include "FileUtil.h"
#include "Options.h"

#include <string>
#include <iomanip>
#include <iostream>

//////////////////////////////////////////////////////////////////////////////
//
//  Execution error reporter
//
//    This holds the ref & ptr needed to properly report errors that
//    occur during execution.
//
//////////////////////////////////////////////////////////////////////////////

class EngineErrorReporter
{
    ExecutionEngine &m_engine;
    EngineDebugger *m_debugger;

public:
    EngineErrorReporter(ExecutionEngine &engine,
			EngineDebugger *debugger)
	: m_engine(engine)
	, m_debugger(debugger)
    {
    }

    void error_exit(const std::string &msg)
    {
	SourceFileLocation loc;

	if(m_debugger)
	{
	    auto engine_loc = m_engine.get_engine_location();

	    loc = m_debugger->get_source_file_location(engine_loc);
	}

	report_message(std::cerr, loc, "Error", msg);

	if(m_debugger)
	{
	    auto [call_stack, stack_description] = m_engine.get_backtrace();

	    m_debugger->show_backtrace(call_stack, stack_description);
	}

	exit(1);
    }

    void report_pen_height_error()
    {
	if(m_engine.had_pen_height_error())
	{
	    SourceFileLocation loc;

	    if(m_debugger)
		loc = m_debugger->get_pen_height_error_loc();

	    ::report_message(std::cerr, loc, "Warning",
		   "Pen height became negative. Results may be incorrect.");
	}
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//  Utility RAII class for outputting entire SVG file
//
//////////////////////////////////////////////////////////////////////////////

class SvgOutRAII
{
    const SVGConfig &svg_out;

    std::ostream &out;

public:
    SvgOutRAII(const SvgOutRAII &) = delete;
    SvgOutRAII &operator=(const SvgOutRAII &) = delete;

    SvgOutRAII(const SVGConfig &svg_out, std::ostream &out)
	: svg_out(svg_out)
	, out(out)
    {
	if(svg_out)
	    svg_out.output_header(out);
    }

    ~SvgOutRAII()
    {
	if(svg_out)
	    svg_out.output_footer(out);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//  Main
//
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    Options opt;

    opt.parse_command_line(argc, argv);

    // Prepare Debugger

    std::unique_ptr<EngineDebugger> debugger;

    if(opt.debug)
    {
	debugger = std::make_unique<EngineDebugger>();

	debugger->set_call_trace_level(opt.call_trace_level);
	debugger->set_parse_trace_level(opt.parse_trace_level);
	debugger->set_report_breakpoints(opt.report_breakpoints);
	debugger->set_show_stacks(true);
    }

    // Prepare Execution Engine

    Outfile output_file(opt.output_filename);

    ExecutionEngine engine(output_file, debugger.get());

    engine.set_decimal_places(opt.decimal_places);

    if(opt.optimize)
	engine.set_output_format(OstreamTurtle::optimized_output);
    else if(opt.prettyprint)
	engine.set_output_format(OstreamTurtle::prettyprint_output);

    // Parse 

    size_t main_chunk_index = ExecutionEngine::no_chunk;

    {
	Infile input_file(opt.input_filename);

	Lexer lex(input_file);

	Parser p(lex, engine, debugger.get());

	p.set_filename(opt.input_filename);

	p.parse();

	main_chunk_index = p.get_main();
    }

    if(debugger && opt.list_chunks)
	debugger->list_chunks(std::cerr);

    // Execute 

    EngineErrorReporter reporter(engine, debugger.get());

    try
    {
	SvgOutRAII write_svg(opt.svg_out, output_file);

	if(debugger && debugger->needs_trace_file())
	    // Note: debugger trace output is interleaved with the SVG output on
	    // the same ostream, so the user can map lines of turtle code to the
	    // SVG code that is produced by them.
	    debugger->set_trace_output(output_file.get_ptr());

	engine.execute_main(main_chunk_index);
    }
    catch(const SvgPathTurtle::ParallelLinesException&)
    {
	reporter.error_exit("Parallel lines in q or Q command.");
    }
    catch(const SvgPathTurtle::InvalidReflectionException&)
    {
	reporter.error_exit("Invalid reflection arguments x==0 and y==0.");
    }
    catch(const SvgPathTurtle::EmptyTurtleStackException&)
    {
	reporter.error_exit("Empty stack in 'pop' command.");
    }
    catch(const SvgPathTurtle::EmptyMatrixStackException&)
    {
	reporter.error_exit("Empty stack in 'pop_matrix' command.");
    }
    catch(const ExecutionEngine::InfiniteRecursionException&)
    {
	reporter.error_exit("Stack overflow - probably due to infinitely "
			    "recursive user-defined command function");
    }
    catch(const std::runtime_error &err)
    {
	reporter.error_exit(err.what());
    }
    catch(...)
    {
	reporter.error_exit("Unknown error");
    }

    if(!opt.disable_pen_warning)
	reporter.report_pen_height_error();
}
