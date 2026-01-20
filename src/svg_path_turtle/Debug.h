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

#pragma once

#include "SourceLocation.h"

#include "DebugSink.h"
#include "Messages.h"

#include <vector>
#include <iostream>
#include <map>
#include <string>

class EngineDebugger : public EngineDebugSink
	             , public ParserDebugSink
{
    //////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////

public:
    using Location = ParserStarterKit::Location;

private:
    // Debugging, and also parsing

    struct StatementInfo
    {
	const char *label = nullptr;
	Location loc;

	explicit operator bool() const
	{
	    return static_cast<bool>(loc);
	}

	StatementInfo() = default;

	explicit StatementInfo(const Location &loc)
	    : loc(loc)
	{
	}

	StatementInfo(const char *label, const Location &loc)
	    : loc(loc)
	{
	}
    };

    struct FullDebugInfo : public StatementInfo
    {
	size_t file_id = 0;

	FullDebugInfo() = default;

	FullDebugInfo(size_t file_id, const Location &loc)
	    : StatementInfo(loc)
	    , file_id(file_id)
	{
	}

	FullDebugInfo(size_t file_id, const StatementInfo &info)
	    : StatementInfo(info)
	    , file_id(file_id)
	{
	}
    };

    struct ChunkInfo
    {
	size_t file_id = 0;

	bool is_call_frame = false;

	std::vector<StatementInfo> statements;
    };

    //////////////////////////////////////////////////////
    //
    // Data
    //
    //////////////////////////////////////////////////////

    std::ostream *m_p_trace_stream = nullptr;

    int m_call_trace_level = 0;
    int m_parse_trace_level = 0;
    bool m_report_breakpoints = false;
    bool m_show_stacks = false;

    FullDebugInfo m_source_info;

    std::map<size_t, std::string> m_filenames;

    // This maps chunk_index --> { statement locations }. This data structure
    // is parallel to Chunk::statements in the ExecutionEngine.
    std::vector<ChunkInfo> m_chunks;

    bool m_is_executing = false;

    EngineLocation m_pen_height_error_loc;

    //////////////////////////////////////////////////////
    //
    // Functions
    //
    //////////////////////////////////////////////////////

    //// Internal utility functions

    FullDebugInfo get_debug_info(const EngineLocation &loc) const;

    void show_location(std::ostream &out, const EngineLocation &loc) const;
				       
    void show_trace_point(std::ostream &out,
			  const char *phase,
			  const EngineLocation &loc,
			  const std::string &stack_description);

    //////////////////////////////////////////////////////
    //
    // ParserDebugSink implementation
    //
    //////////////////////////////////////////////////////

    void add_source_file(size_t file_id, std::string filename) override;

    void set_source_location(const SourceLocation &loc,
			     const char *label = nullptr) override;

    //////////////////////////////////////////////////////
    //
    // EngineDebugSink implementation
    //
    //////////////////////////////////////////////////////

    // Parsing

    void handle_new_chunk(size_t chunk_index, bool is_call_frame) override;

    void handle_new_statement(const EngineDebugSink::Info &info) override;

    // Execution

    bool want_stack_description() const override;

    void handle_trace_point(const EngineDebugSink::Info &info) override;

    void handle_pen_height_error(const EngineLocation &loc) override;

    void handle_breakpoint(const EngineLocation &loc) override;

public:

    //////////////////////////////////////////////////////
    //
    // Public interface
    //
    //////////////////////////////////////////////////////

    //// Construction

    void set_call_trace_level(int level = 1);
    void set_parse_trace_level(int level = 1);
    void set_report_breakpoints(bool b = true);
    void set_show_stacks(bool b = true);

    //// Debugging

    // If needs_trace_file() returns true, set_trace_output() must be called
    // with a non-null pointer.
    bool needs_trace_file() const;
    void set_trace_output(std::ostream *out);

    SourceFileLocation
      get_source_file_location(const EngineLocation &loc) const;

    SourceFileLocation get_pen_height_error_loc() const;

    void show_backtrace(const std::vector<EngineLocation> &call_stack,
			const std::string &stack_description);

    void list_chunks(std::ostream &out);
};
