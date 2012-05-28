/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

namespace concurrit {

enum ExecutionModeType {MODE_SINGLE, MODE_SERVER, MODE_CLIENT};

class Config {
public:
	static bool OnlyShowHelp;
	static int ExitOnFirstExecution;
	static bool DeleteCoveredSubtrees;
	static char* SaveDotGraphToFile;
	static long MaxWaitTimeUSecs;
	static bool RunUncontrolled;
	static char* TestLibraryFile;
	static bool IsStarNondeterministic;
	static bool KeepExecutionTree;
//	static bool TrackAlternatePaths;
	static int MaxTimeOutsBeforeDeadlock;
	static bool ManualInstrEnabled;
	static bool PinInstrEnabled;
	static bool ReloadTestLibraryOnRestart;
	static bool MarkEndingBranchesCovered;
	static bool SaveExecutionTraceToFile;
	static ExecutionModeType ExecutionMode;
	static bool ParseCommandLine(int argc = -1, char **argv = NULL);
	static bool ParseCommandLine(const main_args& args);
};

} // end namespace


#endif /* CONFIG_H_ */
