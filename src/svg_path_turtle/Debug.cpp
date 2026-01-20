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

#include "Debug.h"
#include "Messages.h"

#include <iostream>
#include <format>
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
//
//  EngineDebugger
//
/////////////////////////////////////////////////////////////////////////////

void EngineDebugger::set_call_trace_level(int level)
{
    m_call_trace_level = level;
}

void EngineDebugger::set_parse_trace_level(int level)
{
    m_parse_trace_level = level;
}

void EngineDebugger::set_report_breakpoints(bool b)
{
    m_report_breakpoints = b;
}

void EngineDebugger::set_show_stacks(bool b)
{
    m_show_stacks = b;
}

bool EngineDebugger::needs_trace_file() const
{
    return m_call_trace_level || m_report_breakpoints;
}

void EngineDebugger::set_trace_output(std::ostream *out)
{
    assert(out);

    m_p_trace_stream = out;
}

EngineDebugger::FullDebugInfo
  EngineDebugger::get_debug_info(const EngineLocation &loc) const
{
    assert(loc.chunk_index < m_chunks.size());
    assert(loc.statement_index < m_chunks[loc.chunk_index].statements.size());

    const ChunkInfo &info = m_chunks[loc.chunk_index];

    return { info.file_id, info.statements[loc.statement_index] };
}

SourceFileLocation
  EngineDebugger::get_source_file_location(const EngineLocation &loc) const
{
    SourceFileLocation where;

    if(loc.chunk_index < m_chunks.size())
    {
	const ChunkInfo &c = m_chunks[loc.chunk_index];

	where.filename = m_filenames.at(c.file_id);

	if(loc.statement_index < c.statements.size())
	    where.loc = c.statements[loc.statement_index].loc;
    }

    return where;
}

SourceFileLocation EngineDebugger::get_pen_height_error_loc() const
{
    return get_source_file_location(m_pen_height_error_loc);
}

void EngineDebugger::list_chunks(std::ostream &out)
{
    out << " --------- Chunks --------------------------------\n";

    for(size_t i = 0; i < m_chunks.size(); ++i)
    {
	ChunkInfo &c = m_chunks[i];

	Location loc;

	if(!c.statements.empty())
	    loc = c.statements[0].loc;

	out << i << ": ";

	if(!loc)
	    // Note: this depends on m_chunks having empty cells at the start.
	    // See comment in handle_new_chunk().
	    out << "builtin command function" << '\n';
	else
	{
	    out << (c.is_call_frame ? "command function" : "local block");

	    out << " ----- "
		<< std::format("{} statement(s)", c.statements.size())
		<< " -----\n";

	    for(const auto &info : m_chunks[i].statements)
		out << "  line "
		    << info.loc.linenum << ":"
		    << info.loc.charnum << " "
		    << info.label << "\n";
	}
    }

    out << " --------- End of chunks -------------------------\n";
}

void EngineDebugger::show_location(std::ostream &out,
				    const EngineLocation &loc) const
{
    auto where = get_source_file_location(loc);

    report_location(out, where);
}

void EngineDebugger::show_trace_point(std::ostream &out,
				       const char *phase,
				       const EngineLocation &loc,
				       const std::string &stack_description)
{
    show_location(out, loc);

    // OPTIMIZE - 'info' could be used to provide info to show_location()
    auto info = get_debug_info(loc);

    if(phase)
    {
	out << phase;

	if(info.label)
	    out << " " << info.label;

	out << ": ";
    }

    if(m_show_stacks && !stack_description.empty())
	out << ' ' << stack_description;

    out << '\n';

    out.flush();
}

void EngineDebugger::show_backtrace(
			    const std::vector<EngineLocation> &call_stack,
			    const std::string &stack_description)
{
    if(call_stack.empty())
	std::cerr << "Backtrace: empty! (Internal Error)\n";
    else
    {
	std::cerr << "\n";
	std::cerr << "---- Backtrace: --------------------------\n";

	for(size_t i = 0; i < call_stack.size(); ++i)
	{
	    const EngineLocation &pc = call_stack[i];

	    if(pc.chunk_index == EngineLocation::no_chunk)
		std::cerr << "Internal error: unrecognized chunk";
	    else if(pc.chunk_index >= m_chunks.size())
		std::cerr << "Internal error: bad chunk index";
	    else
		show_location(std::cerr, pc);

	    if(i == 0)
		std::cerr << "main";
	    else if(m_chunks[pc.chunk_index].is_call_frame)
		std::cerr << "command function";
	    else
		std::cerr << "local block";

	    std::cerr << '\n';

	    assert(pc.chunk_index < m_chunks.size());
	}

	if(!stack_description.empty())
	{
	    std::cerr << "------------------------------------------\n";

	    std::cerr << "Stacks: " << stack_description << '\n';
	}

	std::cerr << "---- End of backtrace: -------------------\n";
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  ParserDebugSink implementation
//
/////////////////////////////////////////////////////////////////////////////


void EngineDebugger::add_source_file(size_t file_id, std::string filename)
{
    m_source_info.file_id = file_id;

    [[maybe_unused]]
    auto res = m_filenames.emplace(file_id, filename);

    assert(res.second);
}

void EngineDebugger::set_source_location(const SourceLocation &loc,
					  const char *label)
{
    m_source_info.file_id = loc.file_id;
    m_source_info.loc.linenum = loc.linenum;
    m_source_info.loc.charnum = loc.charnum;
    m_source_info.label = label;
}

/////////////////////////////////////////////////////////////////////////////
//
//  EngineDebugSink implementation
//
/////////////////////////////////////////////////////////////////////////////


void EngineDebugger::handle_new_chunk(size_t chunk_index, bool is_call_frame)
{
    assert(chunk_index != EngineLocation::no_chunk);
    assert(chunk_index >= m_chunks.size());

    // Note: since builtins are not passed in here, this ends up allocating
    // around 30 empty cells.  However, they're not large, and so switching to a
    // map or hash would probably waste more space anyway.

    m_chunks.resize(chunk_index + 1);

    m_chunks[chunk_index].file_id = m_source_info.file_id;

    m_chunks[chunk_index].is_call_frame = is_call_frame;
}

void EngineDebugger::handle_new_statement(const EngineDebugSink::Info &info)
{
    assert(info.loc.chunk_index < m_chunks.size());

    auto &statements = m_chunks[info.loc.chunk_index].statements;

    statements.emplace_back(m_source_info);

    if(m_parse_trace_level)
	show_trace_point(std::cerr, "Parse", info.loc, info.stack_description);
}

bool EngineDebugger::want_stack_description() const
{
    return (m_parse_trace_level || m_call_trace_level) && m_show_stacks;
}

void EngineDebugger::handle_trace_point(const EngineDebugSink::Info &info)
{
    if(m_call_trace_level)
    {
	assert(m_p_trace_stream);

	// Note: trace is normally interleaved with the actual SVG output,
	// so the user can map lines of turtle code to that output.  For
	// that reason, it is helpful to emit a newline here.

	*m_p_trace_stream << '\n'; // separate trace from actual output

	show_trace_point(*m_p_trace_stream,
			 "Run", info.loc, info.stack_description);

	if(m_call_trace_level > 1)
	  *m_p_trace_stream << std::format(" Turtle: xyd={:.2f},{:.2f},{:.2f}"
					           " ixy={:.2f},{:.2f}\n",
					    info.turtle.get_x(),
					    info.turtle.get_y(),
					    info.turtle.get_dir(),
					    info.turtle.get_initial_x(),
					    info.turtle.get_initial_y());
    }
}

void EngineDebugger::handle_pen_height_error(const EngineLocation &loc)
{
    m_pen_height_error_loc = loc;
}

void EngineDebugger::handle_breakpoint(const EngineLocation &loc)
{
    if(m_report_breakpoints)
    {
	assert(m_p_trace_stream);

	show_location(*m_p_trace_stream, loc);
	*m_p_trace_stream << "--------- breakpoint ----------------\n";
	m_p_trace_stream->flush();
    }
}
