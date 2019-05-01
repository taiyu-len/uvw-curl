#include "log.hpp"
#include "uvw_curl.hpp"
#include <iomanip>
#include <iostream>
static void add_download( const char* url, uvw_curl::Multi &m )
{
	using namespace uvw_curl;
	auto easy = Easy::create();
	easy->url(url);
	easy->header_events(true);
	easy->on<Easy::HeaderEvent>(
	[](Easy::HeaderEvent x, Easy& y) {
		LOG() << "Header " << y.effective_url();
		std::printf("%*s", static_cast<int>(x.length), x.data);
	});
	easy->on<Easy::ErrorEvent>(
	[](Easy::ErrorEvent const&e, Easy const& x) {
		LOG() << "Easy Error: " << e.what() << '\n' << x.error_buffer.data() << '\n';
	});
	m.add_handle(std::move(easy));
}

int main(int argc, char** argv) {
	using namespace uvw_curl;
	if (argc == 1) return 0;
	auto loop  = uvw::Loop::getDefault();
	auto multi = Multi::create(loop, Global::create());
	for (int i = 1; i < argc; ++i) {
		add_download(argv[i], *multi);
	}
	loop->run();
}

