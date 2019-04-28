#include "uvw_curl.hpp"
#include "log.hpp"
#include <iostream>
#include <thread>

static void add_download(const char* url, uvw_curl::CurlMulti &m) {
	using namespace uvw_curl;
	auto easy = CurlEasy::create();
	easy->setopt(CURLOPT_URL, url);
	easy->on<CurlEasy::XferInfoEvent>(
	[](CurlEasy::XferInfoEvent x, CurlEasy &y) {
		auto url = y.getinfo<const char*>(CURLINFO_EFFECTIVE_URL);
		LOG() << "Recieved " << x.dlnow << " / " << x.dltotal << " Bytes from " << url;
	});
	easy->on<CurlEasy::EndEvent>(
	[](CurlEasy::EndEvent x, CurlEasy &y) {
		auto url = y.getinfo<const char*>(CURLINFO_EFFECTIVE_URL);
		LOG() << "Finished Recieving from " << url;
	});
	m.add_handle(std::move(easy));
}

int main(int argc, char **argv) try {
	if (argc <= 1) {
		return 0;
	}
	using namespace uvw_curl;

	auto loop = uvw::Loop::getDefault();
	auto multi = CurlMulti::create(loop, CurlGlobal::create());
	for (int i = 1; i < argc; ++i) {
		add_download(argv[i], *multi);
	}
	TRACE() << "Running loop";
	//std::thread a([&]{ loop->run(); });
	loop->run();
	//a.join();
	return EXIT_SUCCESS;
} catch (std::exception const& e) {
	std::cerr << e.what() << '\n';
	return 1;
}

