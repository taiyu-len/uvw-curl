#include "uvw_curl.hpp"
#include "log.hpp"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <regex>
#include <string>
#include <vector>

static void add_download(std::string const&, uvw_curl::Multi&, bool links = true );
static void download_images(std::string& input, uvw_curl::Multi &m)
{
	static auto rgx = std::regex(
		"https?://[\\w./]*\\.(jpg|png|gif)",
		std::regex::optimize);
	auto first = std::sregex_iterator(input.begin(), input.end(), rgx);
	auto last  = std::sregex_iterator();
	if (first == last)
	{
		return;
	}
	auto suffix = std::ssub_match();
	while (first != last)
	{
		// add found links to download
		suffix = first->suffix();
		add_download(first->str(), m, false);
		++first;
	}
	// drop searched area of input string
	input = suffix;
}


void add_download(std::string const& url, uvw_curl::Multi &m, bool links)
{
	using namespace uvw_curl;
	auto easy = Easy::create();
	easy->url(url);
	if (links) {
		LOG() << "Download links from " << url;
		auto data = std::make_shared<std::string>();
		easy->on<Easy::DataEvent>(
		[data] (Easy::DataEvent const& e, Easy const&)
		{
			data->append(e.data, e.data + e.length);
		});
		easy->on<Easy::EndEvent>(
		[data] (Easy::EndEvent const&, Easy const& y)
		{
			download_images(*data, *y.multi);
		});
	} else {
		auto idx = url.rfind('/');
		if (idx == std::string::npos) {
			LOG() << "Invalid Url:" << url;
			return;
		}
		auto filename = url.substr(idx + 1);
		auto file = fopen(filename.c_str(), "wx");
		if (file== nullptr) {
			LOG() << "failed to create file " << filename;
			return;
		}
		LOG() << "Adding download for:" << filename;
		easy->on<Easy::DataEvent>(
		[file](Easy::DataEvent const& e, Easy & x) {
			LOG() << "Recieved " << std::setw(5) << e.length
			<< " Bytes from " << x.effective_url();
			fwrite(e.data, e.length, 1, file);
		});
		easy->once<Easy::EndEvent>(
		[file](Easy::EndEvent const&, Easy & x) {
			LOG() << "Finished downloading " << x.effective_url();
			fclose(file);
		});
		easy->on<Easy::ErrorEvent>(
		[](Easy::ErrorEvent const&e, Easy & x) {
			LOG() << "Error: " << e.what();
		});
	}
	m.add_handle(std::move(easy));
}

int main(int argc, char** argv)
{
	using namespace uvw_curl;
	if (argc == 1) return 0;
	auto loop  = uvw::Loop::getDefault();
	auto multi = Multi::create(loop, Global::create());
	multi->on<Multi::ErrorEvent>(
	[](Multi::ErrorEvent const&e, Multi & x) {
		LOG() << "Error: " << e.what();
	});
	for (int i = 1; i < argc; ++i)
	{
		add_download(argv[i], *multi);
	}
	TRACE() << "Start Loop";
	loop->run();
	LOG() << "Finish Loop";
}

