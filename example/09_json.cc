#include "workflow/WFFacilities.h"
#include <csignal>
#include <iostream>
#include <fstream>
#include "wfrest/HttpServer.h"

using namespace wfrest;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

// Helper function: Write JSON object to temporary file
std::string write_json_to_temp_file(const Json& json)
{
    std::string filename = "/tmp/wfrest_json_example.json";
    std::ofstream file(filename);
    file << json.dump(4);
    file.close();
    return filename;
}

// Helper function: Demonstrate error handling
void handle_json_error(const Json& json, const std::string& operation)
{
    if (json.has_error())
    {
        std::cerr << "Error during " << operation << ": "
                  << "Code: " << json.error_code() << ", "
                  << "Message: " << json.last_error() << std::endl;
    }
}

int main()
{
    signal(SIGINT, sig_handler);

    HttpServer svr;

    // Basic JSON object creation and access
    // curl -v http://ip:port/json1
    svr.GET("/json1", [](const HttpReq *req, HttpResp *resp)
    {
        // Create JSON object using JsonObject
        JsonObject json;
        json.set("number", 123)
            .set("string", "test json")
            .set("boolean", true)
            .set("null_value", nullptr);
        
        // Create nested object
        JsonObject nested;
        nested.set("nested_key", "nested_value")
              .set("nested_number", 456);
        json.set("nested_object", nested);
        
        resp->Json(json);
    });

    // Using JSON arrays
    // curl -v http://ip:port/json2
    svr.GET("/json2", [](const HttpReq *req, HttpResp *resp)
    {
        // Create JSON array
        JsonArray array;
        array.push_back(1)
             .push_back(2)
             .push_back(3)
             .push_back("string in array")
             .push_back(true);
        
        // Create object containing array
        JsonObject json;
        json.set("numbers", array);
        
        // Create nested array using initializer list
        json.set("matrix", JsonArray{
            JsonArray{1, 2, 3},
            JsonArray{4, 5, 6},
            JsonArray{7, 8, 9}
        });
        
        resp->Json(json);
    });

    // Parse valid JSON string
    // curl -v http://ip:port/json3
    svr.GET("/json3", [](const HttpReq *req, HttpResp *resp)
    {
        std::string valid_text = R"(
        {
            "numbers": [1, 2, 3],
            "object": {
                "key1": "value1",
                "key2": 42,
                "key3": true
            },
            "string": "Hello, World!"
        }
        )";
        
        Json parsed = Json::parse(valid_text);
        if (!parsed.has_error())
        {
            // Access parsed JSON
            std::string result = "Parsed JSON successfully:\n";
            
            // Access array elements
            int second_number;
            if (parsed["numbers"][1].get(second_number))
            {
                result += "Second number: " + std::to_string(second_number) + "\n";
            }
            
            // Access nested object
            std::string key1_value;
            if (parsed["object"]["key1"].get(key1_value))
            {
                result += "object.key1: " + key1_value + "\n";
            }
            
            // Check types
            result += "Type of 'string': " + parsed["string"].type_str() + "\n";
            result += "Type of 'numbers': " + parsed["numbers"].type_str() + "\n";
            
            resp->String(result);
        }
        else
        {
            resp->String("Parse error: " + parsed.last_error());
        }
    });

    // Parse invalid JSON string and handle errors
    // curl -v http://ip:port/json4
    svr.GET("/json4", [](const HttpReq *req, HttpResp *resp)
    {
        std::string invalid_text = R"(
        {
            "strings": ["extra", "comma", ]
        }
        )";
        
        Json parsed = Json::parse(invalid_text);
        if (parsed.has_error())
        {
            resp->String("Parse error detected:\nError code: " + 
                         std::to_string(parsed.error_code()) + 
                         "\nError message: " + parsed.last_error());
        }
        else
        {
            resp->Json(parsed);
        }
    });

    // Type conversion and type checking
    // curl -v http://ip:port/json5
    svr.GET("/json5", [](const HttpReq *req, HttpResp *resp)
    {
        JsonObject json;
        json.set("integer", 42)
            .set("double", 3.14)
            .set("string", "42")
            .set("boolean", true);
        
        std::string result = "Type conversions:\n";
        
        // Correct type conversion
        int int_val;
        if (json["integer"].get(int_val))
        {
            result += "integer as int: " + std::to_string(int_val) + "\n";
        }
        
        // Try different type conversions
        std::string str_val;
        if (json["integer"].get(str_val))
        {
            result += "integer as string: " + str_val + "\n";
        }
        else
        {
            result += "Failed to convert integer to string: " + json.last_error() + "\n";
        }
        
        // Try converting string to number
        int str_as_int;
        if (json["string"].get(str_as_int))
        {
            result += "string as int: " + std::to_string(str_as_int) + "\n";
        }
        else
        {
            result += "Failed to convert string to int: " + json.last_error() + "\n";
        }
        
        resp->String(result);
    });

    // File operations: Write JSON to file and read
    // curl -v http://ip:port/json6
    svr.GET("/json6", [](const HttpReq *req, HttpResp *resp)
    {
        // Create complex JSON object
        JsonObject config;
        config.set("app_name", "WFRest JSON Example")
              .set("version", "1.0.0")
              .set("debug", true)
              .set("max_connections", 1000)
              .set("timeout_ms", 30000)
              .set("servers", JsonArray{
                  JsonObject{{"host", "server1.example.com"}, {"port", 8080}},
                  JsonObject{{"host", "server2.example.com"}, {"port", 8081}}
              });
        
        // Write to file
        std::string filename = write_json_to_temp_file(config);
        
        // Read from file
        FILE* fp = fopen(filename.c_str(), "r");
        Json loaded = Json::parse(fp);
        fclose(fp);
        
        std::string result;
        if (loaded.has_error())
        {
            result = "Error loading JSON from file: " + loaded.last_error();
        }
        else
        {
            result = "Successfully loaded JSON from file:\n";
            result += "App name: " + loaded["app_name"].get<std::string>() + "\n";
            result += "Server count: " + std::to_string(loaded["servers"].size()) + "\n";
            
            // Access objects in array
            std::string host1 = loaded["servers"][0]["host"].get<std::string>();
            int port1 = loaded["servers"][0]["port"].get<int>();
            result += "First server: " + host1 + ":" + std::to_string(port1) + "\n";
        }
        
        resp->String(result);
    });

    // Receive and modify JSON
    // curl -X POST http://ip:port/json7 -H 'Content-Type: application/json' -d '{"user":{"name":"John","age":30},"active":true}'
    svr.POST("/json7", [](const HttpReq *req, HttpResp *resp)
    {
        if (req->content_type() != APPLICATION_JSON)
        {
            resp->String("NOT APPLICATION_JSON");
            return;
        }
        
        Json received = req->json();
        if (received.has_error())
        {
            resp->String("Invalid JSON received: " + received.last_error());
            return;
        }
        
        // Modify received JSON
        if (received.has("user"))
        {
            // Add new field
            received["user"]["email"] = "john@example.com";
            
            // Modify existing field
            if (received["user"].has("age"))
            {
                int age;
                if (received["user"]["age"].get(age))
                {
                    received["user"]["age"] = age + 1; // Increase age
                }
            }
            
            // Add an access history array
            JsonArray history;
            history.push_back("Logged in")
                  .push_back("Profile updated")
                  .push_back("Password changed");
            received["user"]["history"] = history;
        }
        
        // Add server timestamp
        received["server_time"] = "2023-06-15T12:34:56Z";
        
        // Return modified JSON
        resp->Json(received);
    });

    // Error handling examples
    // curl -v http://ip:port/json8
    svr.GET("/json8", [](const HttpReq *req, HttpResp *resp)
    {
        std::string result = "Error handling examples:\n\n";
        
        // 1. Parse error
        Json parse_error = Json::parse("{invalid json}");
        result += "1. Parse error:\n";
        if (parse_error.has_error())
        {
            result += "   Code: " + std::to_string(parse_error.error_code()) + "\n";
            result += "   Message: " + parse_error.last_error() + "\n\n";
        }
        
        // 2. Type error
        Json number_json(42);
        std::string str_val;
        result += "2. Type conversion error:\n";
        if (!number_json.get(str_val))
        {
            result += "   Message: " + number_json.last_error() + "\n\n";
        }
        
        // 3. Array index out of bounds
        JsonArray array;
        array.push_back(1).push_back(2).push_back(3);
        result += "3. Array index error:\n";
        int out_of_bounds;
        if (!array[5].get(out_of_bounds))
        {
            result += "   Message: " + array.last_error() + "\n\n";
        }
        
        // 4. Object/Array type error
        Json simple_value("string");
        result += "4. Object/Array conversion error:\n";
        if (!simple_value.ensure_object())
        {
            result += "   Message: " + simple_value.last_error() + "\n";
        }
        
        resp->String(result);
    });

    if (svr.start(8888) == 0)
    {
        std::cout << "Server started on port 8888" << std::endl;
        std::cout << "Try the following endpoints:" << std::endl;
        std::cout << "  GET  /json1 - Basic JSON object creation" << std::endl;
        std::cout << "  GET  /json2 - JSON array operations" << std::endl;
        std::cout << "  GET  /json3 - Parse valid JSON" << std::endl;
        std::cout << "  GET  /json4 - Parse invalid JSON and handle errors" << std::endl;
        std::cout << "  GET  /json5 - Type conversion and checking" << std::endl;
        std::cout << "  GET  /json6 - File operations" << std::endl;
        std::cout << "  POST /json7 - Receive and modify JSON" << std::endl;
        std::cout << "  GET  /json8 - Error handling examples" << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        wait_group.wait();
        svr.stop();
    }
    else
    {
        fprintf(stderr, "Cannot start server");
        exit(1);
    }
    return 0;
}
