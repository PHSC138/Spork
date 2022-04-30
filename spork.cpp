#include "json.hpp"
#include "statelist.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <typeinfo>
#include <vector>

// Curlpp
#include <cerrno>
#include <cstdlib>

#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

using json = nlohmann::json;
using std::cout;
using std::endl;

#ifndef DEBUG
#define DEBUG 0
#endif

// Maximum number of threads (given by handout)
#define MAX_THREADS 64

// Goes to line in file stream
// https://stackoverflow.com/questions/5207550/in-c-is-there-a-way-to-go-to-a-specific-line-in-a-text-file
std::fstream *GotoLine(std::fstream *file, unsigned int num) {
  file->seekg(std::ios::beg);
  if (num <= 1)
    return file;
  for (int i = 0; i < num - 1; ++i) {
    file->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return file;
}

void do_http(std::string http_options, std::string address, std::vector<std::string> *payload_files,
             std::vector<unsigned> *payload_file_sizes, unsigned payload_file_index, unsigned start, unsigned size) {

  if (DEBUG)
    cout << "Thread ID " << std::this_thread::get_id() << " starting up with file_index " << payload_file_index
         << " start " << start << " and size " << size << endl;

  // Re parse json since it was passed in with a string
  auto json_options = json::parse(http_options);
  if (DEBUG)
    cout << "do_http json_options:" << std::endl << json_options.dump(4) << endl;

  // TODO: try catch
  std::string http_method = json_options["http_method"];
  transform(http_method.begin(), http_method.end(), http_method.begin(), ::tolower);
  auto http_data = json_options["http_data"];

  // The outer fstream has the most lines out of all the files
  std::fstream *outer_fstream = new std::fstream();
  // The inner_fstreams will be the file pointers to the payload files we have to do the cartesian product on
  std::vector<std::fstream *> inner_fstreams;
  // Open an fstream for all the files
  for (unsigned i = 0; i < payload_files->size(); i++) {
    std::string path = payload_files->at(i);
    if (DEBUG)
      cout << "Opening " << path << endl;

    // If it's the file with the most lines, open the outer_fstream
    if (i == payload_file_index) {
      outer_fstream->open(path, std::ios::in);

      if (!(*outer_fstream)) {
        cout << "Error opening file: " << path << endl;
        exit(1);
      }

      // Get to starting line number
      outer_fstream = GotoLine(outer_fstream, start);
    } else {
      // Open inner fstreams
      std::fstream *stream = new std::fstream();
      stream->open(path, std::ios::in);

      if (!(*stream)) {
        cout << "Error opening file: " << path << endl;
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

    request.setOpt(new curlpp::options::Verbose(true));

    // TODO: list of json headers
    std::list<std::string> header;
    header.push_back("Content-Type: application/x-www-form-urlencoded");

    request.setOpt(new curlpp::options::HttpHeader(header));

    // String to store the current outer_line
    std::string outer_line;
    // Loop through outer data
    for (unsigned outer_loop_counter = start; outer_loop_counter < start + size; outer_loop_counter++) {
      // Read outer file line
      std::getline(*outer_fstream, outer_line);
      if (DEBUG)
        cout << "Outer_line: " << outer_line << endl;

      // StateList is a Class that stores the vector of the current state of the cartesian product
      StateList state_list = StateList(inner_sizes);

      // Note: could be merged into StateList
      // Store the previous state to cache updates
      std::vector<int> previous_state;

      // Note: could be merged into StateList
      // The cached lines will be stored here
      std::vector<std::string> previous_state_values(inner_sizes.size(), "");

      // Cartesian product loop using StateList
      while (!state_list.is_done()) {
        if (DEBUG)
          cout << "Iterations: " << state_list.get_iterations() << endl;

        std::vector<unsigned> current_state = state_list.get_state();

        // Loop through http_data and create key:value json object list with state_list checking for "$x" syntax
        // data variable will store the kv pairs for the current iteration
        json data;
        for (auto &[key, value] : http_data.items()) {
          if (DEBUG)
            cout << "key " << key << ": " << value << endl;
          // Values with '$' will be strings
          if (value.is_string()) {
            // Get string from json object
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
                // If the index is larger than the the payload_file_index, we have to fix the
                // offset since we removed the payload_file_index
                if (index > payload_file_index)
                  index--;

                if (DEBUG)
                  cout << "inner_fstreams.size() " << inner_fstreams.size() << " index " << index << endl;

                // Get fstream corresponding to index
                std::fstream *current_fstream = inner_fstreams[index];

                if (DEBUG)
                  cout << "current_state[" << index << "]: " << current_state[index] << endl;

                // Find cartesian product state (current line) of index
                // If state is 0, go to first line of file, else fstream SHOULD be at the
                // right point of file, so we can just use getline
                std::string inner_line;
                if (current_state[index] == 0)
                  current_fstream->seekg(std::ios::beg);
                // Check for first iteration, or if the previous_state has changed
                if (previous_state.size() == 0 || previous_state[index] != current_state[index]) {
                  // Get new line
                  std::getline(*current_fstream, inner_line);
                  // Update the cached lines
                  previous_state_values[index] = inner_line;
                } else {
                  inner_line = previous_state_values[index];
                }

                // Store the key and value into data
                data[key] = inner_line;
              }
            } else
              data[key] = value;
          } else
            data[key] = value;
        }

        // TODO: could put this in the other loop to save another iteration through the json
        // Compute the form encoded payload_string from the data json
        // Form encoded key0=value0&key1=value1...
        std::string payload_string;
        for (auto &[key, value] : data.items())
          payload_string.append(key + "=" + value.get<std::string>() + "&");

        // Remove the extra '&' added to the end of the string
        payload_string.pop_back();

        if (DEBUG)
          cout << "Thread " << std::this_thread::get_id() << " sending " << payload_string << endl;

        if (http_method.compare("post") == 0) {
          request.setOpt(new curlpp::options::Url(address));
          // POST request
          request.setOpt(new curlpp::options::PostFields(payload_string));
          request.setOpt(new curlpp::options::PostFieldSize(payload_string.size()));
        } else {
          // GET request
          request.setOpt(new curlpp::options::Url(address + "?" + payload_string));
        }

        // TODO: cookies (example07)

        // Send the request!
        request.perform();

        // Copy current_state to previous state
        std::copy(current_state.begin(), current_state.end(), std::back_inserter(previous_state));

        // Advance state list to next state
        state_list.next_state();

        // TODO: store results into file
        // Print results
        cout << curlpp::infos::ResponseCode::get(request) << " " << curlpp::infos::EffectiveUrl::get(request) << " "
             << payload_string << endl;
      }
    }
  } catch (curlpp::LogicError &e) {
    cout << e.what() << endl;
  } catch (curlpp::RuntimeError &e) {
    cout << e.what() << endl;
  }
}

int main(int argc, char **argv) {
  // Check argument size
  if (argc != 2) {
    std::string output;
    if (argc > 2)
      output = "Too many arguments!";
    if (argc < 2)
      output = "Not enough arguments!";
    cout << output << std::endl << "Usage: ./spork config_file.json" << endl;
    exit(1);
  }

  // Open configuration json file
  std::fstream config_file;
  config_file.open(argv[1], std::ios::in);
  if (!config_file) {
    cout << "Could not open configuration file: " << argv[1] << endl;
    exit(1);
  }

  // Read configuration json file
  std::ostringstream string_stream;
  string_stream << config_file.rdbuf();

  // Parse the configuration json
  nlohmann::basic_json config;
  try {
    config = json::parse(string_stream.str());
  } catch (const nlohmann::detail::parse_error &e) {
    cout << "Error parsing json from " << argv[1] << endl;
    exit(1);
  }

  // Print out configuration file
  if (DEBUG)
    cout << config.dump(4) << endl;

  // Get address, protocol, and options from json
  std::string address;
  std::string protocol;
  nlohmann::json protocol_options;
  unsigned int num_threads;

  try {
    address = config["address"];
  } catch (const nlohmann::detail::type_error &e) {
    std::cerr << "Missing configuration option: address" << endl;
    exit(1);
  }
  try {
    protocol = config["protocol"];
  } catch (const nlohmann::detail::type_error &e) {
    std::cerr << "Missing configuration option: protocol" << endl;
    exit(1);
  }
  try {
    protocol_options = config["protocol_options"];
  } catch (const nlohmann::detail::type_error &e) {
    std::cerr << "Missing configuration option: protocol_options" << endl;
    exit(1);
  }
  try {
    // Current number of threads
    num_threads = config["threads"];
    if (num_threads > MAX_THREADS) {
      cout << "Threads provided " << num_threads << " > MAX_THREADS " << MAX_THREADS << endl;
      exit(1);
    }
  } catch (const nlohmann::detail::type_error &e) {
    std::cerr << "Missing configuration option: threads" << endl;
    exit(1);
  }
  try {
    config["payload"];
  } catch (const nlohmann::detail::type_error &e) {
    std::cerr << "Missing configuration option: payload" << endl;
    exit(1);
  }

  // data_files stores paths to files with payload data
  std::vector<std::string> data_files;
  // data_file_sizes stores the number of lines in that file **can be off by 1 if no newline at end of file**
  std::vector<unsigned> data_file_sizes;

  // Index of the file with the longest lines (most data)
  unsigned max_file_index = 0;

  // Open payload files using json key value
  for (auto &[key, value] : config["payload"].items()) {
    // Open file read only
    if (DEBUG)
      cout << "Opening: " << value << endl;
    std::fstream file = std::fstream();
    file.open(value, std::ios::in);
    if (!file) {
      cout << "Could not open: " << value << endl;
      exit(1);
    }

    // Store path value
    data_files.push_back(value);

    // Find number of elements in newline separated files
    data_file_sizes.push_back(std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n'));
    file.close();

    if (DEBUG)
      cout << "File size: " << data_file_sizes.back() << endl;

    // Update largest file index
    if (data_file_sizes.back() > data_file_sizes[max_file_index])
      max_file_index = data_file_sizes.size() - 1;
  }

  // Lowercase protocol
  transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
  if (protocol.compare("http") == 0) {
    if (DEBUG)
      cout << "Detected http" << endl;

    // Split largest file by number of threads to evenly distribute work
    div_t div_result = std::div((int)(data_file_sizes[max_file_index]), (int)num_threads);
    unsigned split_data_size = div_result.quot;
    unsigned split_data_size_remainder = div_result.rem;

    if (DEBUG)
      cout << "Each thread will get " << split_data_size << " entries from " << max_file_index << " remainder of "
           << split_data_size_remainder << " will be split between first threads" << endl;

    // Start the threads!
    // Store them here
    std::vector<std::thread> threads;
    // Line number to start at
    unsigned start_position = 1;
    for (unsigned i = 0; i < num_threads; i++) {
      unsigned final_data_size = split_data_size;

      // Distribute remainder to first threads
      if (split_data_size_remainder > 0) {
        final_data_size++;
        split_data_size_remainder--;
      }

      // Start thread and store result into threads vector
      threads.emplace_back(std::thread(do_http, protocol_options.dump(), address, &data_files, &data_file_sizes,
                                       max_file_index, start_position, final_data_size));

      // Increment start line
      start_position += final_data_size;
    }

    // Join all the threads
    unsigned joined_thread_count = 0;
    while (joined_thread_count < num_threads) {
      for (std::thread &current_thread : threads) {
        // Synchronize the threads
        if (current_thread.joinable()) {
          if (DEBUG)
            cout << "Joining thread " << current_thread.get_id() << endl;
          current_thread.join();
          joined_thread_count++;
        }
      }
    }
  } else {
    // TODO: support raw (nc / ncat / raw sockets?)
    cout << "Unsupported protocol " << protocol << " only supporting http currently" << endl;
    exit(1);
  }
}
