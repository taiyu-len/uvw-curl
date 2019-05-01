#ifndef UVW_CURL_ZSTRING_HPP
#define UVW_CURL_ZSTRING_HPP
#include <string>
namespace uvw_curl
{
/** Light reference wrapper around zero terminated strings. */
struct zstring
{
	zstring() = default;
	zstring(const char* s): string(s) {};
	zstring(std::string const& s): string(s.c_str()) {};

	operator std::string() const { return {string}; }
	operator const char*() const { return string; }

	const char* string = "";
};
} // namespace uvw_curl

#endif // UVW_CURL_ZSTRING_HPP
