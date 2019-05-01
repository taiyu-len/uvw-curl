#include "log.hpp"
#include "uvw_curl.hpp"
#include <iomanip>
#include <iostream>
#include <cctype>
static void dump(std::ostream& os, const char * data, size_t size)
{
	auto flags = os.flags();
	const int width = 0x10;

	os << std::hex << std::setfill('0');
	for (size_t i = 0; i < size; i+= width) {
		for (int c = 0; c < width; ++c) {
			if (i+c < size) {
				auto ch = static_cast<unsigned char>(data[i+c]);
				os << std::setw(2) << int(ch) << ' ';
			} else {
				os << "   ";
			}
		}
		for (int c = 0; (c < width) && (i+c < size); ++c) {
			os << ((data[i+c] >= 0x20 && data[i+c] < 0x80) ? data[i+c] : '.');
		}
		os << '\n';
	}
	os.flags(flags);
}

static void trace(uvw_curl::Easy::DebugEvent const& x, uvw_curl::Easy const& e)
{
	const char* text = nullptr;
	switch (x.type) {
	case CURLINFO_TEXT: std::cerr << "== Info: " << x.data;
	default: return;
	case CURLINFO_HEADER_OUT:   text = "=> Send header"; break;
	case CURLINFO_DATA_OUT:     text = "=> Send data"; break;
	case CURLINFO_SSL_DATA_OUT: text = "=> Send SSL data"; break;
	case CURLINFO_HEADER_IN:    text = "<= Recv header"; break;
	case CURLINFO_DATA_IN:      text = "<= Recv data"; break;
	case CURLINFO_SSL_DATA_IN:  text = "<= Recv SSL data"; break;
	}
	std::cerr << std::setfill(' ') << std::left;
	std::cerr << text << " " << std::setw(10) << x.length << " bytes  (";
	std::cerr << "0x" << std::hex << std::setw(8) << x.length << std::dec << ")\n";
	dump(std::cerr, x.data, x.length);
}


static void add_download( const char* url, uvw_curl::Multi &m )
{
	using namespace uvw_curl;
	auto easy = Easy::create();
	easy->url(url);
	easy->verbose(true);
	easy->on<Easy::DebugEvent>(&trace);
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
