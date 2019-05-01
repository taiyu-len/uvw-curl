#include "uvw_curl.hpp"
#include "log.hpp"
#include <iostream>
#include <thread>

static void add_download(const char* url, uvw_curl::Multi &m) {
	using namespace uvw_curl;
	auto easy = Easy::create();
	easy->url(url);
	easy->on<Easy::DataEvent>(
	[](Easy::DataEvent x, Easy &y) {
		LOG() << "Recieved " << x.length << " Bytes from " << y.effective_url();
	});
	easy->on<Easy::EndEvent>(
	[](Easy::EndEvent, Easy &y) {
		LOG() << "Finished Recieving from " << y.effective_url();
	});
	easy->on<Easy::ErrorEvent>(
	[](Easy::ErrorEvent const&e, auto const&) {
		LOG() << "Easy Error: " << e.what();
	});
	m.add_handle(std::move(easy));
}

int main(int argc, char **argv) try {
	if (argc <= 1) {
		return 0;
	}
	using namespace uvw_curl;

	TRACE() << "Main";
	auto loop = uvw::Loop::getDefault();
	auto multi = Multi::create(loop, Global::create());
	multi->on<Multi::ErrorEvent>(
	[](Multi::ErrorEvent const&e, auto const&) {
		LOG() << "Multi Error: " << e.what();
	});
	for (int i = 1; i < argc; ++i) {
		add_download(argv[i], *multi);
	}
	LOG() << "Running Loop";
	//std::thread a([&]{ loop->run(); });
	loop->run();
	//a.join();
	return EXIT_SUCCESS;
} catch (std::exception const& e) {
	std::cerr << e.what() << '\n';
	return 1;
}

