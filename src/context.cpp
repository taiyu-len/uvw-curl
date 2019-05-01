#include "context.hpp"
#include <memory>
namespace uvw_curl
{
Context::Context(Key, Multi& multi, curl_socket_t s) noexcept
: multi(multi)
, s(s)
, poll(multi._timer->loop().resource<uvw::PollHandle>(s))
{
	auto err = curl_multi_assign(multi._handle.get(), s, this);
	if (err)
	{
		multi.publish(Multi::ErrorEvent{err});
	}
	poll->on<uvw::CloseEvent>(
	[this] (auto const&, auto const&) {
		delete this;
	});
	poll->on<uvw::PollEvent>(
	[this] (uvw::PollEvent const& e, auto const&) {
		int running_handles;
		int flags = 0;
		if (e.flags & uvw::PollHandle::Event::READABLE) {
			flags |= CURL_CSELECT_IN;
		}
		if (e.flags & uvw::PollHandle::Event::WRITABLE) {
			flags |= CURL_CSELECT_OUT;
		}
		auto err = curl_multi_socket_action(
			this->multi._handle.get(), this->s, flags,
			&running_handles);
		if (err != 0)
		{
			this->multi.publish(ErrorEvent{err});
		}
		this->multi.check_info();
	});
}

Context::~Context() noexcept
{
	curl_multi_assign(multi._handle.get(), s, nullptr);
}
} // namespace uvw_curl
