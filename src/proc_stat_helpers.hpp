#pragma once

#include <cstdio>
#include <cstring>
#include <optional>

#include <sys/types.h>

namespace gamescope::proc_stat
{
// The command name in /proc/<pid>/stat is wrapped in parentheses but not
// escaped, so it can hold spaces and ')' of its own; only the last ')' ends it.
// The state is 'Z' both for a zombie and for a process whose main thread has
// exited while other threads still run; only num_threads tells them apart.
inline std::optional<pid_t> ParseLiveParentPid( const char *pszStatLine )
{
	const char *pszCommEnd = strrchr( pszStatLine, ')' );
	if ( !pszCommEnd )
		return std::nullopt;

	char chState = 0;
	int nParentPid = -1;
	long lThreads = 0;
	int nFields = sscanf( pszCommEnd + 1,
		" %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %ld",
		&chState, &nParentPid, &lThreads );
	if ( nFields < 2 )
		return std::nullopt;

	if ( chState == 'Z' && ( nFields < 3 || lThreads <= 1 ) )
		return std::nullopt;

	return pid_t( nParentPid );
}
}
