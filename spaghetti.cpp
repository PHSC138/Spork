#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <typeinfo>
#include <string>
#include <vector>
#include <sstream>

// Curlpp
#include <cstdlib>
#include <cerrno>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

using json = nlohmann::json;

// Handled by Makefile
#ifndef DEBUG
#define DEBUG 0
#endif

// Maximum number of threads (given by handout)
#define MAX_THREADS 64
// Current number of threads
unsigned int num_threads;

int main(int argc, char** argv) {
    if (DEBUG) {
        for(int i = 0; i < argc; i++) std::cout << argv[i] << " ";
        std::cout << "(" << argc << ")" << std::endl;
    }
    if (argc != 2) {
        std::string output;
        if (argc > 2) output = "Too many arguments!";
        if (argc < 2) output = "Not enough arguments!";
        std::cout << output << std::endl << "Usage: ./spaghetti config_file.json" << std::endl;
        exit(1);
    }

    // Read configuration json file
    std::fstream config_file;
    config_file.open(argv[1], std::ios::in);
    if (!config_file) {
		std::cout << "Could not open configuration file: " << argv[1] << std::endl;
        exit(1);
	}

    // Read configuration file
    std::ostringstream string_stream;
    string_stream << config_file.rdbuf();

    // Parse the json
    nlohmann::basic_json config;
    try {
        config = json::parse(string_stream.str());
    } catch (const nlohmann::detail::parse_error& e) {
        std::cout << "Error parsing json from " << argv[1] << std::endl;
        exit(1);
    }

    if (DEBUG) std::cout << config.dump(4) << std::endl;

    // Set up as much of payload as possible -- libtins http / raw
    std::string address = config["address"];
    std::string protocol = config["protocol"];
    auto protocol_options = config["protocol_options"];

    std::vector<std::fstream *> data_files;
    std::vector<int> data_file_sizes;
    // even easier with structured bindings (C++17)
    // Open payload files
    for (auto& [key, value] : config["payload"].items()) {
        if (DEBUG) std::cout << "Opening: " << value << std::endl;
        std::fstream *file = new std::fstream;
        file->open(value, std::ios::in);

        if (!(*file)) {
            std::cout << "Could not open: " << value << std::endl;
            exit(1);
        }

        data_files.push_back(file);
        data_file_sizes.push_back(std::count(std::istreambuf_iterator<char>(*file), std::istreambuf_iterator<char>(), '\n'));
        if (DEBUG) std::cout << "File size: " << data_file_sizes.back() << std::endl;
    }


    // if (DEBUG) std::cout << "protocol_options type: " << typeid(protocol_options).name() << std::endl;

    // Lowercase protocol 
    transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol.compare("http") == 0) {
        if (DEBUG) std::cout << "Detected http" << std::endl;
        std::string http_method = protocol_options["http_method"];
        transform(http_method.begin(), http_method.end(), http_method.begin(), ::tolower);
        auto http_data = protocol_options["http_data"];

        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;
            
            request.setOpt(new curlpp::options::Url(address)); 
            request.setOpt(new curlpp::options::Verbose(true)); 
            
            std::list<std::string> header; 
            header.push_back("Content-Type: application/octet-stream"); 
            
            request.setOpt(new curlpp::options::HttpHeader(header)); 
            
            request.setOpt(new curlpp::options::PostFields("abcd"));
            request.setOpt(new curlpp::options::PostFieldSize(5));

            request.setOpt(new curlpp::options::UserPwd("user:password"));
            
            request.perform(); 
        }
        catch ( curlpp::LogicError & e ) {
            std::cout << e.what() << std::endl;
        }
        catch ( curlpp::RuntimeError & e ) {
            std::cout << e.what() << std::endl;
        }

    } else {
        // TODO: support raw
        std::cout << "Unsupported protocol " << protocol <<  " only supporting http currently" << std::endl;
        exit(1);
    }

    // even easier with structured bindings (C++17)
    // for (auto& [key, value] : o.items()) {
    //   std::cout << key << " : " << value << "\n";
    // }

    // Open read only file descriptors to payload data files
    // Split based off of num_elements -- find the largest file and split by number of processors -- this will be the "outer loop"

    // std::ifstream inFile("file"); 
    // std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');

        // Case for elements > num_processors
        // Case for elements == num_processors
        // Case for elements < num_processors
}