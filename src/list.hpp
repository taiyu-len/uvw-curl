#ifndef UVW_CURL_LIST_HPP
#define UVW_CURL_LIST_HPP
#include <memory>
#include <curl/curl.h>

namespace uvw_curl
{
// Wrapper around curl_slist type
// does not manage lifetimes of passed in strings.
struct List
{
	List();
	explicit List(curl_slist*);
	operator curl_slist*() const noexcept;

	void append(const char* string) noexcept;
	void clear() noexcept;
private:
	std::unique_ptr<curl_slist, void(*)(curl_slist*)> _list;
};

} // namespace uvw_curl

#endif // UVW_CURL_LIST_HPP
