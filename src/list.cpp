#include "list.hpp"
#include <curl/curl.h>
namespace uvw_curl
{
List::List()
:_list(nullptr, &curl_slist_free_all)
{}

List::List( curl_slist* list)
:_list(list, &curl_slist_free_all)
{}

List::operator curl_slist*() const noexcept
{
	return _list.get();
}

void List::append(const char* string) noexcept
{
	auto ptr = _list.release();
	_list.reset(curl_slist_append(ptr, string));
}

void List::clear() noexcept
{
	_list.reset();
}

} // namespace uvw_curl
