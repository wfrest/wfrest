#ifndef WFREST_JSON_H_
#define WFREST_JSON_H_

#include "workflow/json_parser.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <initializer_list>
#include <type_traits>

namespace wfrest
{

// Forward declarations
class Json;
class JsonObject;
class JsonArray;

namespace detail
{
// Type traits for JSON type detection
template <typename T>
struct is_string : std::false_type {};

template <class T, class Traits, class Alloc>
struct is_string<std::basic_string<T, Traits, Alloc>> : std::true_type {};

template <typename C>
struct is_char
    : std::integral_constant<bool, std::is_same<C, char>::value ||
                                   std::is_same<C, char16_t>::value ||
                                   std::is_same<C, char32_t>::value ||
                                   std::is_same<C, wchar_t>::value>
{};

template <typename T>
struct is_number
    : std::integral_constant<bool, std::is_arithmetic<T>::value &&
                                   !std::is_same<T, bool>::value &&
                                   !detail::is_char<T>::value>
{};

// Helper for safe type conversion
template <typename T, typename U>
bool safe_cast(U value, T& result) {
    if (std::is_same<T, U>::value) {
        result = static_cast<T>(value);
        return true;
    }
    
    if (std::is_arithmetic<T>::value && std::is_arithmetic<U>::value) {
        // Check for potential overflow/underflow
        if (value > std::numeric_limits<T>::max() || 
            value < std::numeric_limits<T>::lowest()) {
            return false; // 转换会导致溢出
        }
    }
    
    result = static_cast<T>(value);
    return true;
}

} // namespace detail

/**
 * @class Json
 * @brief A modern C++ wrapper for JSON data
 */
class Json
{
public:
    using Object = JsonObject;
    using Array = JsonArray;

    // 错误代码枚举
    enum ErrorCode {
        NO_ERROR = 0,
        PARSE_ERROR,
        TYPE_ERROR,
        CONVERSION_ERROR,
        INDEX_ERROR,
        KEY_ERROR,
        MEMORY_ERROR,
        IO_ERROR,
        UNKNOWN_ERROR
    };

    // Static factory methods
    static Json parse(const std::string& str);
    static Json parse(std::istream& stream);
    static Json parse(FILE* fp);
    
    // Create null JSON value
    Json();
    
    // Create JSON value from various types
    Json(std::nullptr_t);
    Json(bool value);
    Json(const std::string& value);
    Json(const char* value);
    
    template <typename T, 
              typename = typename std::enable_if<detail::is_number<T>::value>::type>
    Json(T value)
        : node_(json_value_create(JSON_VALUE_NUMBER, static_cast<double>(value))),
          parent_(nullptr), allocated_(true)
    {}
    
    // Copy and move semantics
    Json(const Json& other);
    Json& operator=(const Json& other);
    Json(Json&& other) noexcept;
    Json& operator=(Json&& other) noexcept;
    
    ~Json();

    // Serialization
    std::string dump() const;
    std::string dump(int indent) const;
    
    // Element access
    Json& operator[](const std::string& key);
    const Json& operator[](const std::string& key) const;
    Json& operator[](int index);
    const Json& operator[](int index) const;
    
    // Type checking
    bool is_null() const { return type() == JSON_VALUE_NULL; }
    bool is_number() const { return type() == JSON_VALUE_NUMBER; }
    bool is_boolean() const { 
        int t = type();
        return t == JSON_VALUE_TRUE || t == JSON_VALUE_FALSE; 
    }
    bool is_string() const { return type() == JSON_VALUE_STRING; }
    bool is_object() const { return type() == JSON_VALUE_OBJECT; }
    bool is_array() const { return type() == JSON_VALUE_ARRAY; }
    bool is_valid() const { return node_ != nullptr; }
    
    // Type conversion
    template <typename T>
    bool get(T& result) const;
    
    // Type-specific getters
    bool get_bool(bool& result) const {
        if (json_value_type(node_) == JSON_VALUE_TRUE) {
            result = true;
            return true;
        } else if (json_value_type(node_) == JSON_VALUE_FALSE) {
            result = false;
            return true;
        }
        
        const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not a boolean");
        return false;
    }
    
    template <typename T>
    typename std::enable_if<detail::is_number<T>::value, bool>::type
    get(T& result) const {
        if (!is_number()) {
            const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not a number");
            return false;
        }
        return detail::safe_cast(json_value_number(node_), result);
    }
    
    template <typename T>
    typename std::enable_if<detail::is_string<T>::value, bool>::type
    get(T& result) const {
        if (!is_string()) {
            const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not a string");
            return false;
        }
        result = std::string(json_value_string(node_));
        return true;
    }
    
    bool get_null(std::nullptr_t& result) const {
        if (!is_null()) {
            const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not null");
            return false;
        }
        result = nullptr;
        return true;
    }
    
    bool get_object(JsonObject& result) const;
    bool get_array(JsonArray& result) const;
    
    template <typename T>
    T get() const {
        T result{};
        get(result);
        return result;
    }
    
    template <typename T>
    operator T() const { return get<T>(); }
    
    // Object operations
    bool has(const std::string& key) const;
    void erase(const std::string& key);
    
    template <typename T>
    bool set(const std::string& key, const T& value);
    
    // Array operations
    template <typename T>
    bool push_back(const T& value);
    void erase(int index);
    
    // Common operations
    int size() const;
    bool empty() const;
    void clear();
    std::string type_str() const;
    int type() const { return json_value_type(node_); }
    
    // Iterators
    class iterator;
    class const_iterator;
    
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    
    // Error handling
    std::string last_error() const { return error_message_; }
    ErrorCode error_code() const { return error_code_; }
    bool has_error() const { return error_code_ != NO_ERROR; }
    void clear_error() { error_code_ = NO_ERROR; error_message_.clear(); }

protected:
    // Internal constructor for creating from raw json_value_t
    Json(json_value_t* node, bool allocated);
    
    // Internal helper methods
    bool ensure_object();
    bool ensure_array();
    
    void set_error(ErrorCode code, const std::string& message) {
        error_code_ = code;
        error_message_ = message;
    }
    
    // Serialization helpers
    static void value_to_string(const json_value_t* val, int spaces, int depth,
                              std::string* out_str);
    static void string_to_string(const char* str, std::string* out_str);
    static void number_to_string(double number, std::string* out_str);
    static void array_to_string(const json_array_t* arr, int spaces, int depth,
                              std::string* out_str);
    static void object_to_string(const json_object_t* obj, int spaces, int depth,
                               std::string* out_str);

private:
    json_value_t* node_ = nullptr;
    const json_value_t* parent_ = nullptr;
    bool allocated_ = false;
    std::string parent_key_;
    std::string error_message_;
    ErrorCode error_code_ = NO_ERROR;
    
    // Helper for memory management
    void destroy_node();
    
    // Friends
    friend class JsonObject;
    friend class JsonArray;
    friend std::ostream& operator<<(std::ostream& os, const Json& json);
};

/**
 * @class JsonObject
 * @brief Specialized Json class for object operations
 */
class JsonObject : public Json
{
public:
    JsonObject();
    
    // Initialize with key-value pairs
    JsonObject(std::initializer_list<std::pair<std::string, Json>> items);
    
    // Object-specific operations
    template <typename T>
    JsonObject& set(const std::string& key, const T& value);
    
    // Iteration
    class iterator;
    class const_iterator;
    
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
};

/**
 * @class JsonArray
 * @brief Specialized Json class for array operations
 */
class JsonArray : public Json
{
public:
    JsonArray();
    
    // Initialize with values
    JsonArray(std::initializer_list<Json> items);
    
    // Array-specific operations
    template <typename T>
    JsonArray& push_back(const T& value);
    
    // Iteration
    class iterator;
    class const_iterator;
    
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
};

// Stream operator
inline std::ostream& operator<<(std::ostream& os, const Json& json)
{
    return (os << json.dump());
}

} // namespace wfrest

#endif // WFREST_JSON_H_
