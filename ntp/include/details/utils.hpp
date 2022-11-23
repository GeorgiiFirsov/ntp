/**
 * @file utils.hpp
 * @brief Some useful implementation stuff
 */

#pragma once


namespace ntp::details {

/**
 * @brief Provides SEH-safe way to call functions from Win32 ThreadPool API
 * 
 * @tparam function Pointer to function to call
 * @tparam Args... Types of arguments
 * @param args Arbitrary number of arguments passed to the function
 * @returns Win32 error code
 */
template<auto function, typename... Args>
DWORD SafeThreadpoolCall(Args&&... args) noexcept
{
	__try
	{
		function(std::forward<Args>(args)...);
		return ERROR_SUCCESS;
	}
	__except
	{
		return GetExceptionCode();
	}
}

} // namespace ntp::details