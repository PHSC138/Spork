#include "json.hpp"
#include "statelist.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <typeinfo>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <limits>

// Curlpp
#include <cstdlib>
#include <cerrno>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Exception.hpp>

using json = nlohmann::json;
using std::cout;
using std::endl;

#ifndef DEBUG
#define DEBUG 1
#endif

// Maximum number of threads (given by handout)
#define MAX_THREADS 64

// https://stackoverflow.com/questions/5207550/in-c-is-there-a-way-to-go-to-a-specific-line-in-a-text-file
std::fstream * GotoLine(std::fstream * file, unsigned int num){
    file->seekg(std::ios::beg);
    cout << "GotoLine " << num << endl;
    if (num <= 1) return file;
    for(int i=0; i < num - 1; ++i) {
        file->ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        cout << "newline" << endl;
    }
    return file;
}

void do_http(std::string http_options, std::string address, std::vector<std::string> *payload_files, std::vector<unsigned> *payload_file_sizes, unsigned payload_file_index, 
    unsigned start, unsigned size) {
    cout << "Thread ID "<< std::this_thread::get_id() << " starting up with file_index " << 
    payload_file_index << " start " << start << " and size " << size << endl;

    auto json_options = json::parse(http_options);
    if (DEBUG) cout << "do_http json_options:" << std::endl << json_options.dump(4) << endl;

    std::string http_method = json_options["http_method"];
    transform(http_method.begin(), http_method.end(), http_method.begin(), ::tolower);
    auto http_data = json_options["http_data"];

    // The inner_fstreams will be the file pointers to the inner payload files
    // This is used to do the cartesian product
    // Outer fstream
    std::fstream *outer_fstream = new std::fstream();
    std::vector<std::fstream *> inner_fstreams;
    for (unsigned i = 0; i < payload_files->size(); i++) {
        std::string path = payload_files->at(i);
        if (DEBUG) cout << "Opening " << path << endl;

        if (i == payload_file_index) {
            outer_fstream->open(path, std::ios::in);

            if (!(*outer_fstream)) {
                cout << "Error opening file: " << path <<endl;
                exit(1);
            }

            // Get to proper line number
            outer_fstream = GotoLine(outer_fstream, start);
        } else {
            // Don't include outer fstream in the inner fstreams
            std::fstream * stream = new std::fstream();
            stream->open(path, std::ios::in);

            if (!(*stream)) {
                cout << "Error opening file: " << path <<endl;
                exit(1);
            }

            inner_fstreams.push_back(stream);
        }
    }


    // Remove size of outer loop file from payload_file_sizes
    std::vector<unsigned> inner_sizes = std::vector(*payload_file_sizes);
    inner_sizes.erase(inner_sizes.begin() + payload_file_index);

    // cURLpp to send http post request
    try {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;
        
        request.setOpt(new curlpp::options::Url(address)); 
        request.setOpt(new curlpp::options::Verbose(true)); 
        
        // TODO: list of json headers
        std::list<std::string> header; 
        header.push_back("Content-Type: application/octet-stream"); 
        
        request.setOpt(new curlpp::options::HttpHeader(header)); 
        
        // Loop through outer data
        std::string outer_line; 
        for(unsigned outer_loop_counter = start; outer_loop_counter < start + size; outer_loop_counter++) { 

            // Read outer file line
            std::getline(*outer_fstream, outer_line);
            cout << "Outer_line: " << outer_line << endl; 

            StateList state_list = StateList(inner_sizes);
            std::vector<int> previous_state;
            std::vector<std::string> previous_state_values(inner_sizes.size(), "");

            // Cartesian product loop using a state list
            while (!state_list.is_done()) {
                cout << "Iterations: " << state_list.get_iterations() << endl;

                std::vector<unsigned> current_state = state_list.get_state();

                // Loop through http_data and create key:value json object list with state_list checking for "$x" syntax
                // data will store the kv pairs for the current iteration
                json data;
                for (auto& [key, value] : http_data.items()) {
                    if (DEBUG) cout << "key " << key << ": " << value << endl;
                    if (value.is_string()) {
                        std::string value_string = value.get<std::string>();
                        if (value_string.front() == '$') {
                            // Remove '$'
                            value_string.erase(value_string.begin());
                            // Convert to int
                            unsigned index = std::stoi(value_string);
                            if (index == payload_file_index) {
                                // Set the value to be the outer_line
                                data[key] = outer_line;
                            } else {
                                if (index > payload_file_index) index--;
                                // Find state (current line) of index
                                // If state is 0, go to top, else fstream SHOULD be at top of file, so can just use getline
                                if (DEBUG) cout << "inner_fstreams.size() " << inner_fstreams.size() << " index " << index << endl;
                                std::fstream *current_fstream = inner_fstreams[index];
                                std::string inner_line;
                                cout << "current_state[" << index << "]: " << current_state[index] << endl;
                                if (current_state[index] == 0) current_fstream->seekg(std::ios::beg);

                                if (previous_state.size() == 0 || previous_state[index] != current_state[index]) {
                                    std::getline(*current_fstream, inner_line);
                                    previous_state_values[index] = inner_line;
                                } else {
                                    inner_line = previous_state_values[index];
                                }

                                data[key] = inner_line;
                            }
                        } else {
                            data[key] = value;
                        }
                    } else data[key] = value;
                }

                // TODO: could put this in the other loop
                // Form encoded key0=value0&key1=value1...
                std::string payload_string;
                for (auto& [key, value] : data.items()) 
                    payload_string.append(key + "=" + value.get<std::string>() + "&");

                // Remove the last '&'
                payload_string.pop_back();
                if (DEBUG) cout << "Thread " << std::this_thread::get_id() << " sending " << payload_string << endl;

                if (http_method.compare("post") == 0) {
                    // POST request
                    request.setOpt(new curlpp::options::PostFields(payload_string));
                    request.setOpt(new curlpp::options::PostFieldSize(payload_string.size()));
                } else {
                    // GET request
                    // TODO: put cartesian product into querystring 

                }

                // TODO: cookies (example07)

                // HTTP authorization header
                // request.setOpt(new curlpp::options::UserPwd("user:password"));
                
                request.perform(); 

                // other way to retreive URL
                cout << endl 
                    << "Effective URL: " 
                    << curlpp::infos::EffectiveUrl::get(request)
                    << endl;

                cout << "Response code: " 
                    << curlpp::infos::ResponseCode::get(request) 
                    << endl;
                state_list.next_state();
                std::copy(current_state.begin(), current_state.end(), std::back_inserter(previous_state));
            }
        }
    }
    catch ( curlpp::LogicError & e ) {
        cout << e.what() << endl;
    }
    catch ( curlpp::RuntimeError & e ) {
        cout << e.what() << endl;
    }
}

int main(int argc, char** argv) {
    if (DEBUG) {
        for(int i = 0; i < argc; i++) cout << argv[i] << " ";
        cout << "(" << argc << ")" << endl;
    }
    if (argc != 2) {
        std::string output;
        if (argc > 2) output = "Too many arguments!";
        if (argc < 2) output = "Not enough arguments!";
        cout << output << std::endl << "Usage: ./spork config_file.json" << endl;
        exit(1);
    }

    // Read configuration json file
    std::fstream config_file;
    config_file.open(argv[1], std::ios::in);
    if (!config_file) {
		cout << "Could not open configuration file: " << argv[1] << endl;
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
        cout << "Error parsing json from " << argv[1] << endl;
        exit(1);
    }

    if (DEBUG) cout << config.dump(4) << endl;

    // Set up as much of payload as possible -- libtins http / raw
    std::string address = config["address"];
    std::string protocol = config["protocol"];
    auto protocol_options = config["protocol_options"];
    // Current number of threads
    unsigned int num_threads = config["threads"];
    if (num_threads > MAX_THREADS) { 
        cout << "Threads provided " << num_threads << " > MAX_THREADS " << MAX_THREADS << endl;
        exit(1);
    }

    std::vector<std::string> data_files;
    std::vector<unsigned> data_file_sizes;
    unsigned max_file_index = 0;
    // even easier with structured bindings (C++17)
    // Open payload files
    for (auto& [key, value] : config["payload"].items()) {
        if (DEBUG) cout << "Opening: " << value << endl;
        std::fstream file = std::fstream();
        file.open(value, std::ios::in);

        if (!file) {
            cout << "Could not open: " << value << endl;
            exit(1);
        }

        data_files.push_back(value);
        
        // Find number of elements in newline separated files
        data_file_sizes.push_back(std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n'));
        file.close();

        if (DEBUG) cout << "File size: " << data_file_sizes.back() << endl;
        
        // Update largest file index
        if (data_file_sizes.back() > data_file_sizes[max_file_index]) max_file_index = data_file_sizes.size() - 1;
    }


    // if (DEBUG) std::cout << "protocol_options type: " << typeid(protocol_options).name() << std::endl;

    // Lowercase protocol 
    transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
    if (protocol.compare("http") == 0) {
        if (DEBUG) cout << "Detected http" << endl;

        // Split largest file by num_procs
        div_t div_result = std::div((int) (data_file_sizes[max_file_index]), (int)num_threads);
        unsigned split_data_size = div_result.quot;
        unsigned split_data_size_remainder = div_result.rem;

        if (DEBUG) cout << "Each thread will get " << split_data_size << " entries from " << max_file_index 
        << " remainder of " << split_data_size_remainder << " will be split between first threads" << endl;

        // Do the threading
        std::vector<std::thread> threads;
        unsigned start_position = 1;
        for (unsigned i = 0; i < num_threads; i++) {
            unsigned final_data_size = split_data_size;

            // Distribute remainder to first threads
            if(split_data_size_remainder > 0) {
                final_data_size++;
                split_data_size_remainder--;
            }

            // index, start, size
            // std::thread current_thread(do_http, protocol_options.dump(), address, &data_files, max_file_index, start_position, final_data_size);
            threads.emplace_back(std::thread(do_http, protocol_options.dump(), address, &data_files, &data_file_sizes, max_file_index, start_position, final_data_size));
            start_position += final_data_size;
        }

        unsigned joined_thread_count = 0;
        while (joined_thread_count < num_threads) {
            for (std::thread &current_thread : threads) {
                // synchronize threads:
                if (current_thread.joinable()) {
                    if (DEBUG) cout << "Joining thread " << current_thread.get_id() << endl;
                    current_thread.join();
                    joined_thread_count++;
                }
            }
        }
    } else {
        // TODO: support raw (nc / ncat / raw sockets?)
        cout << "Unsupported protocol " << protocol <<  " only supporting http currently" << endl;
        exit(1);
    }
}