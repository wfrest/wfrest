# wfrest JSON Wrapper

A modern, type-safe, and easy-to-use JSON wrapper for C++11 applications.

## Features

- **Simple API**: Intuitive interface for creating, parsing, and manipulating JSON data
- **Type Safety**: Strong type checking with clear error reporting
- **Modern C++ Design**: Leverages C++11 features for a clean and efficient implementation
- **Memory Safety**: Automatic memory management with proper ownership semantics
- **Performance**: Efficient parsing and serialization with minimal overhead
- **Error Handling**: Comprehensive error reporting without exceptions

## Basic Usage

### Creating JSON Values

```cpp
// Create JSON objects
JsonObject user;
user.set("name", "John Doe")
    .set("age", 30)
    .set("is_active", true);

// Create JSON arrays
JsonArray scores;
scores.push_back(95)
      .push_back(87)
      .push_back(92);

// Combine objects and arrays
user.set("scores", scores);

// Create nested objects
JsonObject address;
address.set("city", "New York")
       .set("zip", "10001");
user.set("address", address);

// Use initializer lists for concise creation
JsonObject config = {
    {"debug", true},
    {"max_connections", 100},
    {"server", {
        {"host", "localhost"},
        {"port", 8080}
    }}
};
```

### Parsing JSON

```cpp
// Parse from string
Json data = Json::parse("{\"name\":\"John\",\"age\":30}");
if (data.has_error()) {
    std::cout << "Parse error: " << data.error_message() << std::endl;
    return;
}

// Parse from file
FILE* fp = fopen("config.json", "r");
Json config = Json::parse(fp);
fclose(fp);
if (config.has_error()) {
    std::cout << "File error: " << config.error_message() << std::endl;
    return;
}

// Parse from stream
std::ifstream file("data.json");
Json stream_data = Json::parse(file);
if (stream_data.has_error()) {
    std::cout << "Stream error: " << stream_data.error_message() << std::endl;
    return;
}
```

### Accessing and Modifying JSON Data

```cpp
// Access object properties
std::string name;
if (data["name"].get(name)) {
    std::cout << "Name: " << name << std::endl;
} else {
    std::cout << "Error: " << data.error_message() << std::endl;
}

// Access with type checking
int age = 0;
if (data["age"].get(age)) {
    std::cout << "Age: " << age << std::endl;
}

// Check if a key exists
if (data.has("email")) {
    std::string email;
    data["email"].get(email);
}

// Modify values
data["age"] = 31;
data["email"] = "john.doe@example.com";

// Access array elements
Json arr = Json::parse("[1, 2, 3, 4, 5]");
if (!arr.has_error()) {
    int value;
    if (arr[2].get(value)) {
        std::cout << "Third element: " << value << std::endl;
    }
}

// Iterate through arrays
for (int i = 0; i < arr.size(); i++) {
    int value;
    if (arr[i].get(value)) {
        std::cout << "Element " << i << ": " << value << std::endl;
    }
}

// Add to arrays
arr.push_back(6);
```

### Serializing JSON

```cpp
// Convert to string (compact format)
std::string json_str = data.dump();
std::cout << json_str << std::endl;

// Pretty print with indentation
std::string pretty_json = data.dump(4); // 4 spaces indentation
std::cout << pretty_json << std::endl;
```

## Specialized Classes

### JsonObject

`JsonObject` is a specialized class for JSON objects that provides a fluent interface for setting properties:

```cpp
JsonObject user;
user.set("name", "John")
    .set("age", 30)
    .set("roles", JsonArray{"admin", "user"});
```

### JsonArray

`JsonArray` is a specialized class for JSON arrays that provides a fluent interface for adding elements:

```cpp
JsonArray numbers;
numbers.push_back(1)
       .push_back(2)
       .push_back(3);
```

## Error Handling

The JSON wrapper uses error codes and messages instead of exceptions for error handling:

```cpp
// Check for errors after operations
Json data = Json::parse("{invalid json}");
if (data.has_error()) {
    std::cout << "Error code: " << data.error_code() << std::endl;
    std::cout << "Error message: " << data.error_message() << std::endl;
}

// Error handling for type mismatches
Json value = Json::parse("42");
std::string str;
if (!value.get(str)) {
    std::cout << "Type error: " << value.error_message() << std::endl;
}

// Error handling for array access
Json arr = Json::parse("[1, 2, 3]");
if (arr[5].has_error()) {
    std::cout << "Array error: " << arr.error_message() << std::endl;
}

// Error handling for object operations
Json obj = Json::parse("{}");
if (!obj.ensure_array()) {
    std::cout << "Cannot convert to array: " << obj.error_message() << std::endl;
}
```

## Improvements Over Previous Version

The JSON wrapper has been significantly improved:

1. **Simplified API**: Reduced redundant methods and improved naming for clarity
2. **Better Type Safety**: Added type checking and safe type conversion
3. **Improved Error Handling**: Detailed error messages and error codes
4. **Memory Management**: Simplified ownership model with RAII principles
5. **Modern C++ Features**: Utilized C++11 features and fluent interfaces
6. **Specialized Classes**: Added `JsonObject` and `JsonArray` for specific operations
7. **Consistent Behavior**: Fixed inconsistencies in API behavior

## Implementation Details

The JSON wrapper is built on top of a C-style JSON parser but provides a modern C++ interface. It handles memory management automatically and ensures type safety through template specialization and type traits.

### Performance Considerations

- The wrapper minimizes copying by using move semantics where appropriate
- JSON parsing is done lazily to avoid unnecessary processing
- Memory allocation is optimized for common use cases

