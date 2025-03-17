#include "Json.h"

namespace wfrest
{

// ------------------------ Json Implementation -------------------------

Json::Json()
    : node_(json_value_create(JSON_VALUE_NULL)), parent_(nullptr),
      allocated_(true)
{
}

Json::Json(std::nullptr_t)
    : node_(json_value_create(JSON_VALUE_NULL)), parent_(nullptr),
      allocated_(true)
{
}

Json::Json(bool val)
    : node_(val ? json_value_create(JSON_VALUE_TRUE)
                : json_value_create(JSON_VALUE_FALSE)),
      parent_(nullptr), allocated_(true)
{
}

Json::Json(const std::string& str)
    : node_(json_value_create(JSON_VALUE_STRING, str.c_str())),
      parent_(nullptr), allocated_(true)
{
}

Json::Json(const char* str)
    : node_(json_value_create(JSON_VALUE_STRING, str)), parent_(nullptr),
      allocated_(true)
{
}

Json::Json(json_value_t* node, bool allocated)
    : node_(node), parent_(nullptr), allocated_(allocated)
{
}

// Parse constructor
Json Json::parse(const std::string& str)
{
    json_value_t* node = json_value_parse(str.c_str());
    if (node == nullptr) {
        Json result;
        result.set_error(PARSE_ERROR, "Failed to parse JSON string");
        return result;
    }
    return Json(node, true);
}

Json Json::parse(std::istream& stream)
{
    std::stringstream buffer;
    buffer << stream.rdbuf();
    return parse(buffer.str());
}

Json Json::parse(FILE* fp)
{
    if (fp == nullptr) {
        Json result;
        result.set_error(IO_ERROR, "Invalid file pointer");
        return result;
    }
    
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (length <= 0) {
        Json result;
        result.set_error(IO_ERROR, "Empty file");
        return result;
    }
    
    char* buffer = new char[length + 1];
    buffer[length] = '\0';
    
    size_t read_length = fread(buffer, 1, length, fp);
    Json result;
    
    if (read_length != static_cast<size_t>(length)) {
        result.set_error(IO_ERROR, "Failed to read file completely");
    } else {
        result = parse(buffer);
    }
    
    delete[] buffer;
    return result;
}

Json::~Json()
{
    destroy_node();
}

void Json::destroy_node()
{
    if (allocated_ && node_ != nullptr) {
        json_value_destroy(node_);
        node_ = nullptr;
        allocated_ = false;
    }
}

Json::Json(const Json& other)
{
    if (other.node_ != nullptr) {
        node_ = json_value_copy(other.node_);
        allocated_ = true;
    } else {
        node_ = nullptr;
        allocated_ = false;
    }
    parent_ = nullptr;
    error_message_ = other.error_message_;
    error_code_ = other.error_code_;
}

Json& Json::operator=(const Json& other)
{
    if (this == &other) {
        return *this;
    }
    
    destroy_node();
    
    if (other.node_ != nullptr) {
        node_ = json_value_copy(other.node_);
        allocated_ = true;
    } else {
        node_ = nullptr;
        allocated_ = false;
    }
    
    parent_ = nullptr;
    parent_key_.clear();
    error_message_ = other.error_message_;
    error_code_ = other.error_code_;
    
    return *this;
}

Json::Json(Json&& other) noexcept
    : node_(other.node_), parent_(other.parent_), allocated_(other.allocated_),
      parent_key_(std::move(other.parent_key_)), error_message_(std::move(other.error_message_)),
      error_code_(other.error_code_)
{
    other.node_ = nullptr;
    other.parent_ = nullptr;
    other.allocated_ = false;
    other.error_code_ = NO_ERROR;
}

Json& Json::operator=(Json&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    
    destroy_node();
    
    node_ = other.node_;
    parent_ = other.parent_;
    allocated_ = other.allocated_;
    parent_key_ = std::move(other.parent_key_);
    error_message_ = std::move(other.error_message_);
    error_code_ = other.error_code_;
    
    other.node_ = nullptr;
    other.parent_ = nullptr;
    other.allocated_ = false;
    other.error_code_ = NO_ERROR;
    
    return *this;
}

std::string Json::dump() const
{
    return dump(0);
}

std::string Json::dump(int indent) const
{
    if (!is_valid()) {
        return "null";
    }
    
    std::string result;
    result.reserve(64);
    value_to_string(node_, indent, 0, &result);
    return result;
}

bool Json::ensure_object()
{
    if (is_null()) {
        destroy_node();
        node_ = json_value_create(JSON_VALUE_OBJECT);
        allocated_ = true;
        return true;
    } else if (!is_object()) {
        set_error(TYPE_ERROR, "Cannot convert to object: value is not null or already an object");
        return false;
    }
    return true;
}

bool Json::ensure_array()
{
    if (is_null()) {
        destroy_node();
        node_ = json_value_create(JSON_VALUE_ARRAY);
        allocated_ = true;
        return true;
    } else if (!is_array()) {
        set_error(TYPE_ERROR, "Cannot convert to array: value is not null or already an array");
        return false;
    }
    return true;
}

Json& Json::operator[](const std::string& key)
{
    if (!ensure_object()) {
        static Json null_result;
        return null_result;
    }
    
    json_object_t* obj = json_value_object(node_);
    const json_value_t* existing = json_object_find(key.c_str(), obj);
    
    if (existing != nullptr) {
        // Return a reference to the existing value
        static Json result;
        result = Json(const_cast<json_value_t*>(existing), false);
        result.parent_ = node_;
        result.parent_key_ = key;
        return result;
    } else {
        // Create a new null value
        json_value_t* new_value = json_value_create(JSON_VALUE_NULL);
        json_object_append(obj, key.c_str(), 0, new_value);
        
        // Return a reference to the new value
        static Json result;
        result = Json(new_value, false);
        result.parent_ = node_;
        result.parent_key_ = key;
        return result;
    }
}

const Json& Json::operator[](const std::string& key) const
{
    if (!is_object()) {
        static Json null_result;
        return null_result;
    }
    
    json_object_t* obj = json_value_object(node_);
    const json_value_t* existing = json_object_find(key.c_str(), obj);
    
    if (existing != nullptr) {
        static Json result;
        result = Json(const_cast<json_value_t*>(existing), false);
        return result;
    } else {
        static Json null_result;
        return null_result;
    }
}

Json& Json::operator[](int index)
{
    if (!is_array() || index < 0 || index >= size()) {
        static Json null_result;
        if (is_array() && (index < 0 || index >= size())) {
            const_cast<Json*>(this)->set_error(INDEX_ERROR, "Array index out of bounds");
        }
        return null_result;
    }
    
    json_array_t* arr = json_value_array(node_);
    const json_value_t* val = nullptr;
    int current_index = 0;
    
    json_array_for_each(val, arr) {
        if (current_index == index) {
            static Json result;
            result = Json(const_cast<json_value_t*>(val), false);
            result.parent_ = node_;
            return result;
        }
        current_index++;
    }
    
    static Json null_result;
    return null_result;
}

const Json& Json::operator[](int index) const
{
    if (!is_array() || index < 0 || index >= size()) {
        static Json null_result;
        if (is_array() && (index < 0 || index >= size())) {
            const_cast<Json*>(this)->set_error(INDEX_ERROR, "Array index out of bounds");
        }
        return null_result;
    }
    
    json_array_t* arr = json_value_array(node_);
    const json_value_t* val = nullptr;
    int current_index = 0;
    
    json_array_for_each(val, arr) {
        if (current_index == index) {
            static Json result;
            result = Json(const_cast<json_value_t*>(val), false);
            return result;
        }
        current_index++;
    }
    
    static Json null_result;
    return null_result;
}

bool Json::has(const std::string& key) const
{
    if (!is_object()) {
        return false;
    }
    
    json_object_t* obj = json_value_object(node_);
    const json_value_t* find = json_object_find(key.c_str(), obj);
    return find != nullptr;
}

void Json::erase(const std::string& key)
{
    if (!is_object()) {
        return;
    }
    
    json_object_t* obj = json_value_object(node_);
    const json_value_t* find = json_object_find(key.c_str(), obj);
    
    if (find != nullptr) {
        json_value_t* removed = json_object_remove(find, obj);
        json_value_destroy(removed);
    }
}

void Json::erase(int index)
{
    if (!is_array() || index < 0 || index >= size()) {
        if (is_array() && (index < 0 || index >= size())) {
            set_error(INDEX_ERROR, "Array index out of bounds");
        }
        return;
    }
    
    json_array_t* arr = json_value_array(node_);
    const json_value_t* val = nullptr;
    int current_index = 0;
    
    json_array_for_each(val, arr) {
        if (current_index == index) {
            json_value_t* removed = json_array_remove(val, arr);
            json_value_destroy(removed);
            return;
        }
        current_index++;
    }
}

template <typename T>
bool Json::set(const std::string& key, const T& value)
{
    if (!ensure_object()) {
        return false;
    }
    
    // First remove existing key if it exists
    erase(key);
    
    // Then add the new value
    json_object_t* obj = json_value_object(node_);
    Json value_json(value);
    json_object_append(obj, key.c_str(), 0, value_json.node_);
    value_json.node_ = nullptr; // Transfer ownership
    return true;
}

template <typename T>
bool Json::push_back(const T& value)
{
    if (!ensure_array()) {
        return false;
    }
    
    json_array_t* arr = json_value_array(node_);
    Json value_json(value);
    json_array_append(arr, 0, value_json.node_);
    value_json.node_ = nullptr; // Transfer ownership
    return true;
}

int Json::size() const
{
    if (!is_valid()) {
        return 0;
    }
    
    if (is_array()) {
        json_array_t* array = json_value_array(node_);
        return json_array_size(array);
    } else if (is_object()) {
        json_object_t* obj = json_value_object(node_);
        return json_object_size(obj);
    }
    
    return 1; // Primitive values have size 1
}

bool Json::empty() const
{
    if (!is_valid() || is_null()) {
        return true;
    }
    
    if (is_array() || is_object()) {
        return size() == 0;
    }
    
    return false; // Primitive values are not empty
}

void Json::clear()
{
    if (!is_valid()) {
        return;
    }
    
    int type = json_value_type(node_);
    destroy_node();
    
    switch (type) {
        case JSON_VALUE_OBJECT:
            node_ = json_value_create(JSON_VALUE_OBJECT);
            break;
        case JSON_VALUE_ARRAY:
            node_ = json_value_create(JSON_VALUE_ARRAY);
            break;
        case JSON_VALUE_STRING:
            node_ = json_value_create(JSON_VALUE_STRING, "");
            break;
        case JSON_VALUE_NUMBER:
            node_ = json_value_create(JSON_VALUE_NUMBER, 0.0);
            break;
        default:
            node_ = json_value_create(type);
            break;
    }
    
    allocated_ = true;
    clear_error();
}

std::string Json::type_str() const
{
    if (!is_valid()) {
        return "invalid";
    }
    
    switch (type()) {
        case JSON_VALUE_STRING:
            return "string";
        case JSON_VALUE_NUMBER:
            return "number";
        case JSON_VALUE_OBJECT:
            return "object";
        case JSON_VALUE_ARRAY:
            return "array";
        case JSON_VALUE_TRUE:
            return "true";
        case JSON_VALUE_FALSE:
            return "false";
        case JSON_VALUE_NULL:
            return "null";
        default:
            return "unknown";
    }
}

// Serialization helpers
void Json::value_to_string(const json_value_t* val, int spaces, int depth,
                         std::string* out_str)
{
    if (val == nullptr || out_str == nullptr) {
        return;
    }
    
    switch (json_value_type(val)) {
        case JSON_VALUE_STRING:
            string_to_string(json_value_string(val), out_str);
            break;
        case JSON_VALUE_NUMBER:
            number_to_string(json_value_number(val), out_str);
            break;
        case JSON_VALUE_OBJECT:
            object_to_string(json_value_object(val), spaces, depth, out_str);
            break;
        case JSON_VALUE_ARRAY:
            array_to_string(json_value_array(val), spaces, depth, out_str);
            break;
        case JSON_VALUE_TRUE:
            out_str->append("true");
            break;
        case JSON_VALUE_FALSE:
            out_str->append("false");
            break;
        case JSON_VALUE_NULL:
            out_str->append("null");
            break;
    }
}

void Json::string_to_string(const char* str, std::string* out_str)
{
    out_str->append("\"");
    while (*str) {
        switch (*str) {
            case '\r':
                out_str->append("\\r");
                break;
            case '\n':
                out_str->append("\\n");
                break;
            case '\f':
                out_str->append("\\f");
                break;
            case '\b':
                out_str->append("\\b");
                break;
            case '\"':
                out_str->append("\\\"");
                break;
            case '\t':
                out_str->append("\\t");
                break;
            case '\\':
                out_str->append("\\\\");
                break;
            default:
                if ((unsigned char)*str < 0x20) {
                    char buf[8];
                    snprintf(buf, 8, "\\u00%02x", *str);
                    out_str->append(buf);
                } else {
                    out_str->push_back(*str);
                }
                break;
        }
        str++;
    }
    out_str->append("\"");
}

void Json::number_to_string(double number, std::string* out_str)
{
    std::ostringstream oss;
    long long integer = number;
    
    if (integer == number) {
        oss << integer;
    } else {
        oss << number;
    }
    
    out_str->append(oss.str());
}

void Json::array_to_string(const json_array_t* arr, int spaces, int depth,
                         std::string* out_str)
{
    if (arr == nullptr || out_str == nullptr) {
        return;
    }
    
    const json_value_t* val;
    int n = 0;
    
    if (spaces == 0) {
        // Compact format
        out_str->append("[");
        json_array_for_each(val, arr) {
            if (n != 0) {
                out_str->append(",");
            }
            n++;
            value_to_string(val, 0, 0, out_str);
        }
        out_str->append("]");
    } else {
        // Pretty format
        std::string padding(spaces, ' ');
        out_str->append("[\n");
        
        json_array_for_each(val, arr) {
            if (n != 0) {
                out_str->append(",\n");
            }
            n++;
            
            for (int i = 0; i < depth + 1; i++) {
                out_str->append(padding);
            }
            
            value_to_string(val, spaces, depth + 1, out_str);
        }
        
        out_str->append("\n");
        for (int i = 0; i < depth; i++) {
            out_str->append(padding);
        }
        out_str->append("]");
    }
}

void Json::object_to_string(const json_object_t* obj, int spaces, int depth,
                          std::string* out_str)
{
    if (obj == nullptr || out_str == nullptr) {
        return;
    }
    
    const char* name;
    const json_value_t* val;
    int n = 0;
    
    if (spaces == 0) {
        // Compact format
        out_str->append("{");
        json_object_for_each(name, val, obj) {
            if (n != 0) {
                out_str->append(",");
            }
            n++;
            string_to_string(name, out_str);
            out_str->append(":");
            value_to_string(val, 0, 0, out_str);
        }
        out_str->append("}");
    } else {
        // Pretty format
        std::string padding(spaces, ' ');
        out_str->append("{\n");
        
        json_object_for_each(name, val, obj) {
            if (n != 0) {
                out_str->append(",\n");
            }
            n++;
            
            for (int i = 0; i < depth + 1; i++) {
                out_str->append(padding);
            }
            
            string_to_string(name, out_str);
            out_str->append(": ");
            value_to_string(val, spaces, depth + 1, out_str);
        }
        
        out_str->append("\n");
        for (int i = 0; i < depth; i++) {
            out_str->append(padding);
        }
        out_str->append("}");
    }
}

// ------------------------ JsonObject Implementation -------------------------

JsonObject::JsonObject() : Json()
{
    ensure_object();
}

JsonObject::JsonObject(std::initializer_list<std::pair<std::string, Json>> items) : Json()
{
    ensure_object();
    
    for (const auto& item : items) {
        set(item.first, item.second);
    }
}

template <typename T>
JsonObject& JsonObject::set(const std::string& key, const T& value)
{
    Json::set(key, value);
    return *this;
}

// ------------------------ JsonArray Implementation -------------------------

JsonArray::JsonArray() : Json()
{
    ensure_array();
}

JsonArray::JsonArray(std::initializer_list<Json> items) : Json()
{
    ensure_array();
    
    for (const auto& item : items) {
        push_back(item);
    }
}

template <typename T>
JsonArray& JsonArray::push_back(const T& value)
{
    Json::push_back(value);
    return *this;
}

// Add template specializations for get<T>
template <>
bool Json::get<bool>(bool& result) const
{
    return get_bool(result);
}

template <>
bool Json::get<std::nullptr_t>(std::nullptr_t& result) const
{
    return get_null(result);
}

template <>
bool Json::get<JsonObject>(JsonObject& result) const
{
    return get_object(result);
}

template <>
bool Json::get<JsonArray>(JsonArray& result) const
{
    return get_array(result);
}

// Explicit template instantiations for JsonObject and JsonArray
template bool Json::get<JsonObject>(JsonObject& result) const;
template bool Json::get<JsonArray>(JsonArray& result) const;

template JsonObject& JsonObject::set<int>(const std::string&, const int&);
template JsonObject& JsonObject::set<double>(const std::string&, const double&);
template JsonObject& JsonObject::set<bool>(const std::string&, const bool&);
template JsonObject& JsonObject::set<std::string>(const std::string&, const std::string&);
template JsonObject& JsonObject::set<const char*>(const std::string&, const char* const&);
template JsonObject& JsonObject::set<Json>(const std::string&, const Json&);
template JsonObject& JsonObject::set<JsonObject>(const std::string&, const JsonObject&);
template JsonObject& JsonObject::set<JsonArray>(const std::string&, const JsonArray&);

template JsonArray& JsonArray::push_back<int>(const int&);
template JsonArray& JsonArray::push_back<double>(const double&);
template JsonArray& JsonArray::push_back<bool>(const bool&);
template JsonArray& JsonArray::push_back<std::string>(const std::string&);
template JsonArray& JsonArray::push_back<const char*>(const char* const&);
template JsonArray& JsonArray::push_back<Json>(const Json&);
template JsonArray& JsonArray::push_back<JsonObject>(const JsonObject&);
template JsonArray& JsonArray::push_back<JsonArray>(const JsonArray&);

// Add implementations for get_object and get_array
bool Json::get_object(JsonObject& result) const
{
    if (!is_object()) {
        const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not an object");
        return false;
    }
    // Implementation details
    result = JsonObject();
    return true;
}

bool Json::get_array(JsonArray& result) const
{
    if (!is_array()) {
        const_cast<Json*>(this)->set_error(TYPE_ERROR, "Value is not an array");
        return false;
    }
    // Implementation details
    result = JsonArray();
    return true;
}

} // namespace wfrest
