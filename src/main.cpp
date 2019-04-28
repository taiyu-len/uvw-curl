#include "uvw_curl.hpp"
#include "log.hpp"
#include <iostream>
#include <thread>

// sample program using uvw and curl to download a file and print it.
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void* url) {
	LOG() << nmemb << " bytes recievd from " << static_cast<const char*>(url);
	return nmemb;
}
static void add_download(const char* url, CurlMulti &m) {
	LOG() << "Adding url" << url;
	auto easy = CurlEasy();
	easy.setopt(CURLOPT_URL, url);
	easy.setopt(CURLOPT_WRITEFUNCTION, write_callback);
	easy.setopt(CURLOPT_WRITEDATA, static_cast<void const*>(url));
	//easy.setopt(CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)2000);
	m.add_handle(std::move(easy));
}

int main(int argc, char **argv) try {
	if (argc <= 1) {
		return 0;
	}
	auto loop = uvw::Loop::getDefault();
	auto curlg = CurlGlobal();
	auto curlm = CurlMulti(*loop, curlg);
	for (int i = 1; i < argc; ++i) {
		add_download(argv[i], curlm);
	}
	LOG() << "Running loop";
	//std::thread a([&]{ loop->run(); });
	loop->run();
	//a.join();
	return EXIT_SUCCESS;
} catch (std::exception const& e) {
	std::cerr << e.what() << '\n';
	return 1;
}

