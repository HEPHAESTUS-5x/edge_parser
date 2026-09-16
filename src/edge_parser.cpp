/*
  ____                                   _             _       
 / ___|  ___  _   _ _ __ ___ ___   _ __ | |_   _  __ _(_)_ __  
 \___ \ / _ \| | | | '__/ __/ _ \ | '_ \| | | | |/ _` | | '_ \ 
  ___) | (_) | |_| | | | (_|  __/ | |_) | | |_| | (_| | | | | |
 |____/ \___/ \__,_|_|  \___\___| | .__/|_|\__,_|\__, |_|_| |_|
                                  |_|            |___/         

Creation date: 2026-09-016T12:33:35.802+0200
# NOTICE: MADS Version 2.4.0
*/

// Mandatory included headers
#include <source.hpp>
#include <nlohmann/json.hpp>
#include <pugg/Kernel.h>
// other includes as needed here
#include <mosquittopp.h>
#include <thread>
#include <queue>
#include <mutex>
#include <sstream>

// Define the name of the plugin
#ifndef PLUGIN_NAME
#define PLUGIN_NAME "edge_parser"
#endif
#define NO_ERROR "No Error"

// Load the namespaces
using namespace std;
using json = nlohmann::json;
using namespace mosqpp;

class Edge_parserPlugin : public Source<json>, public mosquittopp {

public:
  using Source::Source;

  string kind() override { return PLUGIN_NAME; }

  return_type setup() {
    return_type status = return_type::success;
    if (_connected) return status;
    string host = _params["broker_host"];
    string topic = _params["topic"];
    int port = _params["broker_port"];

    lib_init();
    reinitialise("EdgeParser-bridge", true);
    loop_start();
    if (connect_async(host.c_str(), port, 5) != MOSQ_ERR_SUCCESS) {
      cerr << "Failed to connect to MQTT broker" << endl;
      status = return_type::critical;
    } 
    return status;
  }

  ~Edge_parserPlugin() {
    disconnect();
    mosqpp::lib_cleanup();
  }

  void on_connect(int rc) override {
    if (!_params["silent"])
      cerr << "Connected with code " << rc << endl;
    subscribe(NULL, _params["topic"].get<string>().c_str(), _params["QoS"].get<int>());
    _connected = true;
    return;
  }

  void on_subscribe(int mid, int qos_count, const int *granted_qos) override {
    if (!_params["silent"])
      cerr << "Subscribed to " << _params["topic"].get<string>() << endl;
    return;
  }

  void on_disconnect(int rc) override {
    if (!_params["silent"])
      cerr << "Disconnected with code " << rc << endl;
    _connected = false;
    return;
  }

void on_message(const struct mosquitto_message *message) override {
    if (message -> payloadlen == 0) return;

    string raw_str((char *)(message -> payload), message -> payloadlen);
    
    try {
      json j_container = json::parse(raw_str);
      if (j_container.contains("payload") && j_container["payload"].contains("raw_content")) {
        raw_str = j_container["payload"]["raw_content"].get<string>();
      }
    } catch (...) {}

    lock_guard<mutex> lock(_queue_mutex);
    size_t added_count = 0;
    size_t search_pos = 0;

    // scan HFData array in the raw string and extract each sample
    while (true) {
      size_t pos = raw_str.find("\"HFData\":", search_pos);
      if (pos == string::npos) break;

      size_t start_arr = raw_str.find('[', pos);
      if (start_arr == string::npos) break;

      int bracket_depth = 0;
      string current_item = "";
      bool capturing = false;
      size_t end_block_pos = start_arr;

      for (size_t i = start_arr; i < raw_str.length(); ++i) {
        char c = raw_str[i];
        if (c == '[') {
          if (bracket_depth == 1) {
            capturing = true;
            current_item = "";
          }
          bracket_depth++;
        }
        
        if (capturing) {
          current_item += c;
        }

        if (c == ']') {
          bracket_depth--;
          if (bracket_depth == 1 && capturing) {
            capturing = false;
            try {
              json j_row = json::parse(current_item);
              if (j_row.is_array() && j_row.size() >= 7) {
                _sample_queue.push(j_row);
                added_count++;
              }
            } catch (...) {}
          } else if (bracket_depth == 0) {
            end_block_pos = i;
            break;
          }
        }
      }

      search_pos = end_block_pos + 1;
    }

    if (!_params["silent"] && added_count > 0)
      cerr << endl << "Added " << added_count << " samples to the queue" << endl;
    
    return;
  }

  // one sample at each loop cycle
  return_type get_output(json &out, vector<unsigned char> *blob = nullptr) override {
    if (!_connected) {
      reconnect_async();
      while (!_connected) {
        this_thread::sleep_for(chrono::milliseconds(100));
      }
    }

    json current_sample;
    {
      lock_guard<mutex> lock(_queue_mutex);
      if (_sample_queue.empty()) {
        // skip if the queue is empty, but do not block the MADS loop
        return return_type::retry;
      }
      current_sample = _sample_queue.front();
      _sample_queue.pop();
    }

    out.clear();
    out["sample_index"] = current_sample[0];
    out["X"] = current_sample[1];
    out["Y"] = current_sample[2];
    out["Z"] = current_sample[3];
    out["SP"] = current_sample[4];
    out["A"] = current_sample[5];
    out["C"] = current_sample[6];

    if (!_agent_id.empty()) out["agent_id"] = _agent_id;
    return return_type::success;
  }

  void set_params(const json &params) override {
    Source::set_params(params);
    _params["broker_host"] = "localhost";
    _params["broker_port"] = 1883;
    _params["silent"] = false;
    _params["QoS"] = 2;
    _params["topic"] = "#";
    _params.merge_patch(params);
    
    setup();
    while (!_connected) {
      this_thread::sleep_for(chrono::milliseconds(100));
    }
  }

  map<string, string> info() override {
    return {
      {"Broker:", _params["broker_host"].get<string>() + ":" + to_string(_params["broker_port"])},
      {"Topic:", _params["topic"].get<string>()},
      {"Buffer Size:", to_string(_sample_queue.size())}
    };
  };

private:
  queue<json> _sample_queue;
  mutex _queue_mutex;
  bool _connected = false;
};

MADS_REGISTER_PLUGINS(Edge_parserPlugin)

int main(int argc, char const *argv[]) {
  Edge_parserPlugin plugin;
  json output, params;

  params["broker_host"] = "localhost";
  params["broker_port"] = 1883;
  params["topic"] = "#";
  params["silent"] = false;

  plugin.set_params(params);

  while (true) {
    if (plugin.get_output(output) == return_type::success) {
      cout << "Output: " << output.dump(2) << endl;
    } else {
      this_thread::sleep_for(chrono::milliseconds(2));
    }
  }

  return 0;
}