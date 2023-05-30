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
#include <sstream>
#include <string>
#include <vector>

namespace wfrest
{

namespace detail
{

template <typename T>
struct is_string
{
    static constexpr bool value = false;
};

template <class T, class Traits, class Alloc>
struct is_string<std::basic_string<T, Traits, Alloc>>
{
    static constexpr bool value = true;
};

template <typename C>
struct is_char
    : std::integral_constant<bool, std::is_same<C, char>::value ||
                                       std::is_same<C, char16_t>::value ||
                                       std::is_same<C, char32_t>::value ||
                                       std::is_same<C, wchar_t>::value>
{
};

template <typename T>
struct is_number
    : std::integral_constant<bool, std::is_arithmetic<T>::value &&
                                       !std::is_same<T, bool>::value &&
                                       !detail::is_char<T>::value>
{
};

} // namespace detail

class Object_S;
class Array_S;

class Json
{
public:
    using Object = Object_S;
    using Array = Array_S;

public:
    static Json parse(const std::string& str);
    static Json parse(const std::ifstream& stream);
    static Json parse(FILE* fp);

    std::string dump() const;
    std::string dump(int spaces) const;

    // for correctness in clang
    Json operator[](const char* key);

    Json operator[](const char* key) const;

    Json operator[](const std::string& key);

    Json operator[](const std::string& key) const;

    Json operator[](int index);

    Json operator[](int index) const;

    template <typename T>
    void operator=(const T& val)
    {
        if (parent_ == nullptr)
            return;
        if (json_value_type(parent_) == JSON_VALUE_ARRAY)
        {
            update_arr(val);
        }
        else if (json_value_type(parent_) == JSON_VALUE_OBJECT)
        {
            push_back_obj(parent_key_, val);
        }
    }

    bool has(const std::string& key) const;

    template <typename T>
    typename std::enable_if<std::is_same<T, bool>::value, T>::type get() const
    {
        return json_value_type(node_) == JSON_VALUE_TRUE ? true : false;
    }

    template <typename T>
    typename std::enable_if<detail::is_number<T>::value, T>::type get() const
    {
        return static_cast<T>(json_value_number(node_));
    }

    template <typename T>
    typename std::enable_if<detail::is_string<T>::value, T>::type get() const
    {
        return std::string(json_value_string(node_));
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Object>::value, T>::type
    get() const;

    template <typename T>
    typename std::enable_if<std::is_same<T, Array>::value, T>::type get() const;

    template <typename T>
    typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type
    get() const
    {
        return nullptr;
    }

    template <typename T>
    operator T()
    {
        return get<T>();
    }

public:
    int type() const
    {
        return json_value_type(node_);
    }

    std::string type_str() const;

    bool is_null() const
    {
        return type() == JSON_VALUE_NULL;
    }

    bool is_number() const
    {
        return type() == JSON_VALUE_NUMBER;
    }

    bool is_boolean() const
    {
        int type = this->type();
        return type == JSON_VALUE_TRUE || type == JSON_VALUE_FALSE;
    }

    bool is_object() const
    {
        return type() == JSON_VALUE_OBJECT;
    }

    bool is_array() const
    {
        return type() == JSON_VALUE_ARRAY;
    }

    bool is_string() const
    {
        return type() == JSON_VALUE_STRING;
    }

    bool is_valid() const
    {
        return node_ != nullptr;
    }

    int size() const;

    bool empty() const;

    void clear();

    Json copy() const;

    std::string key() const
    {
        return parent_key_;
    }

    const Json& value() const
    {
        return *this;
    }

public:
    // for object
    template <typename T, typename std::enable_if<detail::is_number<T>::value,
                                                  bool>::type = true>
    void push_back(const std::string& key, const T& val)
    {
        if (!can_obj_push_back())
        {
            return;
        }
        json_object_t* obj = json_value_object(node_);
        json_object_append(obj, key.c_str(), JSON_VALUE_NUMBER,
                           static_cast<double>(val));
    }

    template <typename T,
              typename std::enable_if<std::is_same<T, Object>::value ||
                                          std::is_same<T, Array>::value,
                                      bool>::type = true>
    void push_back(const std::string& key, const T& val);

    void push_back(const std::string& key, bool val);
    void push_back(const std::string& key, std::nullptr_t val);
    void push_back(const std::string& key, const std::string& val);
    void push_back(const std::string& key, const char* val);
    void push_back(const std::string& key, const Json& val);

private:
    // for object
    template <typename T>
    void push_back_obj(const std::string& key, const T& val)
    {
        if (!can_obj_push_back())
        {
            return;
        }
        if (is_placeholder())
        {
            placeholder_push_back(key, val);
        }
        else
        {
            normal_push_back(key, val);
        }
    }

    template <typename T, typename std::enable_if<detail::is_number<T>::value,
                                                  bool>::type = true>
    void placeholder_push_back(const std::string& key, const T& val)
    {
        json_object_t* obj = json_value_object(parent_);
        destroy_node(node_);
        node_ = json_object_append(obj, key.c_str(), JSON_VALUE_NUMBER,
                                   static_cast<double>(val));
    }

    template <typename T,
              typename std::enable_if<std::is_same<T, Object>::value ||
                                          std::is_same<T, Array>::value,
                                      bool>::type = true>
    void placeholder_push_back(const std::string& key, const T& val);

    void placeholder_push_back(const std::string& key, bool val);
    void placeholder_push_back(const std::string& key, std::nullptr_t val);
    void placeholder_push_back(const std::string& key, const std::string& val);
    void placeholder_push_back(const std::string& key, const char* val);
    void placeholder_push_back(const std::string& key, const Json& val);

    template <typename T, typename std::enable_if<detail::is_number<T>::value,
                                                  bool>::type = true>
    void normal_push_back(const std::string& key, const T& val)
    {
        json_object_t* obj = json_value_object(parent_);
        const json_value_t* find = json_object_find(key.c_str(), obj);
        if (find == nullptr)
        {
            json_object_append(obj, key.c_str(), JSON_VALUE_NUMBER,
                               static_cast<double>(val));
            return;
        }
        json_object_insert_before(find, obj, key.c_str(), JSON_VALUE_NUMBER,
                                  static_cast<double>(val));
        json_value_t* remove_val = json_object_remove(find, obj);
        json_value_destroy(remove_val);
    }

    template <typename T,
              typename std::enable_if<std::is_same<T, Object>::value ||
                                          std::is_same<T, Array>::value,
                                      bool>::type = true>
    void normal_push_back(const std::string& key, const T& val);

    void normal_push_back(const std::string& key, bool val);
    void normal_push_back(const std::string& key, std::nullptr_t val);
    void normal_push_back(const std::string& key, const std::string& val);
    void normal_push_back(const std::string& key, const char* val);
    void normal_push_back(const std::string& key, const Json& val);

public:
    void erase(const std::string& key);

    template <typename T, typename std::enable_if<detail::is_number<T>::value,
                                                  bool>::type = true>
    void push_back(T val)
    {
        if (!can_arr_push_back())
        {
            return;
        }
        json_array_t* arr = json_value_array(node_);
        json_array_append(arr, JSON_VALUE_NUMBER, static_cast<double>(val));
    }

    template <typename T,
              typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                          std::is_same<T, Json::Array>::value,
                                      bool>::type = true>
    void push_back(const T& val);

    void push_back(bool val);
    void push_back(const std::string& val);
    void push_back(const char* val);
    void push_back(std::nullptr_t val);
    void push_back(const Json& val);

    void erase(int index);

private:
    template <typename T, typename std::enable_if<detail::is_number<T>::value,
                                                  bool>::type = true>
    void update_arr(T val)
    {
        json_array_t* arr = json_value_array(parent_);
        json_array_insert_before(node_, arr, JSON_VALUE_NUMBER,
                                 static_cast<double>(val));
        json_value_t* remove_val = json_array_remove(node_, arr);
        json_value_destroy(remove_val);
    }

    template <typename T,
              typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                          std::is_same<T, Json::Array>::value,
                                      bool>::type = true>
    void update_arr(const T& val);

    void update_arr(bool val);
    void update_arr(const std::string& val);
    void update_arr(const char* val);
    void update_arr(std::nullptr_t val);
    void update_arr(const Json& val);

public:
    class IteratorBase
    {
    public:
        friend class Json;
        explicit IteratorBase(const json_value_t* val)
            : val_(val), json_(new Json)
        {
        }

        ~IteratorBase()
        {
            if (json_ != nullptr)
            {
                delete json_;
            }
        }

        IteratorBase(const IteratorBase& iter)
        {
            val_ = iter.val_;
            key_ = iter.key_;
            json_ = new Json(iter.json_->node_, iter.json_->parent_,
                             iter.json_->parent_key_);
        }

        IteratorBase& operator=(const IteratorBase& iter)
        {
            if (this == &iter)
            {
                return *this;
            }
            val_ = iter.val_;
            key_ = iter.key_;
            json_->reset(iter.json_->node_, iter.json_->parent_,
                         iter.json_->parent_key_);
            return *this;
        }

        IteratorBase(IteratorBase&& iter)
            : val_(iter.val_), key_(iter.key_), json_(iter.json_)
        {
            iter.val_ = nullptr;
            iter.key_ = nullptr;
            iter.json_ = nullptr;
        }

        IteratorBase& operator=(IteratorBase&& iter)
        {
            if (this == &iter)
            {
                return *this;
            }
            val_ = iter.val_;
            iter.val_ = nullptr;
            key_ = iter.key_;
            iter.key_ = nullptr;
            delete json_;
            json_ = iter.json_;
            iter.json_ = nullptr;
            return *this;
        }

        Json& operator*() const
        {
            if (key_ != nullptr)
            {
                json_->parent_key_ = std::string(key_);
            }
            else
            {
                json_->parent_key_ = "";
            }
            return *json_;
        }

        Json* operator->() const
        {
            if (key_ != nullptr)
            {
                json_->parent_key_ = std::string(key_);
            }
            else
            {
                json_->parent_key_ = "";
            }

            return json_;
        }

        bool operator==(const IteratorBase& other) const
        {
            return json_->node_ == other.json_->node_;
        }

        bool operator!=(IteratorBase const& other) const
        {
            return !(*this == other);
        }

    private:
        const json_value_t* val_ = nullptr;
        const char* key_ = nullptr;
        Json* json_; // cursor
    };

    class iterator : public IteratorBase
    {
    public:
        friend class Json;
        explicit iterator(const json_value_t* val) : IteratorBase(val)
        {
        }

        iterator& operator++()
        {
            if (is_end())
            {
                return *this;
            }
            forward();
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = (*this);
            if (!is_end())
            {
                ++(*this);
            }
            return old;
        }

    private:
        void set_begin()
        {
            json_->node_ = nullptr;
            key_ = nullptr;
            forward();
        }

        void set_end()
        {
            json_->node_ = nullptr;
            key_ = nullptr;
        }

        bool is_end()
        {
            return json_->node_ == nullptr && key_ == nullptr;
        }

        void forward()
        {
            if (json_value_type(val_) == JSON_VALUE_OBJECT)
            {
                json_object_t* obj = json_value_object(val_);
                key_ = json_object_next_name(key_, obj);
                json_->node_ = json_object_next_value(json_->node_, obj);
            }
            else if (json_value_type(val_) == JSON_VALUE_ARRAY)
            {
                json_array_t* arr = json_value_array(val_);
                json_->node_ = json_array_next_value(json_->node_, arr);
            }
        }
    };

    class reverse_iterator : public IteratorBase
    {
    public:
        friend class Json;
        explicit reverse_iterator(const json_value_t* val) : IteratorBase(val)
        {
        }

        reverse_iterator& operator++()
        {
            if (is_rend())
            {
                return *this;
            }
            backward();
            return *this;
        }

        reverse_iterator operator++(int)
        {
            reverse_iterator old = (*this);
            if (!is_rend())
            {
                ++(*this);
            }
            return old;
        }

    private:
        void set_rbegin()
        {
            json_->node_ = nullptr;
            key_ = nullptr;
            backward();
        }

        void set_rend()
        {
            json_->node_ = nullptr;
            key_ = nullptr;
        }

        bool is_rend()
        {
            return json_->node_ == nullptr && key_ == nullptr;
        }

        void backward()
        {
            if (json_value_type(val_) == JSON_VALUE_OBJECT)
            {
                json_object_t* obj = json_value_object(val_);
                key_ = json_object_prev_name(key_, obj);
                json_->node_ = json_object_prev_value(json_->node_, obj);
            }
            else if (json_value_type(val_) == JSON_VALUE_ARRAY)
            {
                json_array_t* arr = json_value_array(val_);
                json_->node_ = json_array_prev_value(json_->node_, arr);
            }
        }
    };

    iterator begin() const
    {
        iterator iter(node_);
        iter.set_begin();
        return iter;
    }

    iterator end() const
    {
        iterator iter(node_);
        iter.set_end();
        return iter;
    }

    reverse_iterator rbegin() const
    {
        reverse_iterator iter(node_);
        iter.set_rbegin();
        return iter;
    }

    reverse_iterator rend() const
    {
        reverse_iterator iter(node_);
        iter.set_rend();
        return iter;
    }

private:
    bool can_obj_push_back();

    bool can_arr_push_back();

    // can convert to any type, just like a placeholder
    bool is_placeholder() const
    {
        return is_null() && parent_ != nullptr;
    }

    bool is_incomplete() const
    {
        return node_ == nullptr;
    }

    void destroy_node(const json_value_t* node)
    {
        if (node != nullptr)
        {
            json_value_destroy(const_cast<json_value_t*>(node));
        }
    }

private:
    // for parse
    static void value_convert(const json_value_t* val, int spaces, int depth,
                              std::string* out_str);

    static void string_convert(const char* raw_str, std::string* out_str);

    static void number_convert(double number, std::string* out_str);

    static void array_convert(const json_array_t* arr, int spaces, int depth,
                              std::string* out_str);

    static void array_convert_not_format(const json_array_t* arr,
                                         std::string* out_str);

    static void object_convert(const json_object_t* obj, int spaces, int depth,
                               std::string* out_str);

    static void object_convert_not_format(const json_object_t* obj,
                                          std::string* out_str);

    friend inline std::ostream& operator<<(std::ostream& os, const Json& json)
    {
        return (os << json.dump());
    }

public:
    // Constructors for the various types of JSON value.
    Json();
    Json(const std::string& str, bool parse_flag);
    Json(const std::string& str);
    Json(const char* str);
    Json(std::nullptr_t null);
    Json(double val);
    Json(int val);
    Json(bool val);
    Json(const Array& val);
    Json(const Object& val);
    Json(Array&& val);
    Json(Object&& val);

    ~Json();

    Json(const Json& json) = delete;
    Json& operator=(const Json& json) = delete;
    Json(Json&& other);
    Json& operator=(Json&& other);

protected:
    // todo : need remove, default null type
    struct Empty
    {
    };
    Json(const json_value_t* node, const json_value_t* parent, bool allocated);
    // watcher
    Json(const json_value_t* parent, std::string&& key);
    Json(const json_value_t* parent, const std::string& key);
    Json(const json_value_t* parent);
    Json(const json_value_t* node, const json_value_t* parent);
    Json(const json_value_t* node, const json_value_t* parent,
         std::string&& key);
    Json(const json_value_t* node, const json_value_t* parent,
         const std::string& key);

    Json(const Empty&);

    bool is_root() const
    {
        return parent_ == nullptr;
    }
    void to_object();

    void to_array();

    void reset()
    {
        node_ = nullptr;
        parent_ = nullptr;
        parent_key_.clear();
    }

    void reset(const json_value_t* node, const json_value_t* parent,
               const std::string& parent_key)
    {
        node_ = node;
        parent_ = parent;
        parent_key_ = parent_key;
    }

    void reset(const json_value_t* node, const json_value_t* parent,
               std::string&& parent_key)
    {
        node_ = node;
        parent_ = parent;
        parent_key_ = std::move(parent_key);
    }

private:
    const json_value_t* node_ = nullptr;
    const json_value_t* parent_ = nullptr;
    std::string parent_key_;
    bool allocated_ = true;
};

class Object_S : public Json
{
public:
    Object_S()
    {
        to_object();
    }
    Object_S(const json_value_t* node, const json_value_t* parent)
        : Json(node, parent)
    {
    }
    using string_type = typename std::basic_string<char, std::char_traits<char>,
                                                   std::allocator<char>>;
    using pair_type = std::pair<string_type, Json>;

    Object_S(std::initializer_list<pair_type> list)
    {
        std::for_each(list.begin(), list.end(),
                      [this](const pair_type& pair)
                      { this->push_back(pair.first, pair.second); });
    }
};

class Array_S : public Json
{
public:
    Array_S()
    {
        to_array();
    }

    Array_S(const json_value_t* node, const json_value_t* parent)
        : Json(node, parent)
    {
    }

    Array_S(std::initializer_list<Json> list)
    {
        std::for_each(list.begin(), list.end(),
                      [this](const Json& js) { this->push_back(js); });
    }
};

template <typename T>
typename std::enable_if<std::is_same<T, Json::Object>::value, T>::type
Json::get() const
{
    return Json::Object(node_, parent_);
}

template <typename T>
typename std::enable_if<std::is_same<T, Json::Array>::value, T>::type
Json::get() const
{
    return Json::Array(node_, parent_);
}

template <typename T,
          typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                      std::is_same<T, Json::Array>::value,
                                  bool>::type>
void Json::push_back(const std::string& key, const T& val)
{
    if (!can_obj_push_back())
    {
        return;
    }
    json_object_t* obj = json_value_object(node_);
    Json copy_json = val.copy();
    json_object_append(obj, key.c_str(), 0, copy_json.node_);
    copy_json.node_ = nullptr;
}

template <typename T,
          typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                      std::is_same<T, Json::Array>::value,
                                  bool>::type>
void Json::placeholder_push_back(const std::string& key, const T& val)
{
    json_object_t* obj = json_value_object(parent_);
    destroy_node(node_);
    Json copy_json = val.copy();
    node_ = json_object_append(obj, key.c_str(), 0, copy_json.node_);
    copy_json.node_ = nullptr;
}

template <typename T,
          typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                      std::is_same<T, Json::Array>::value,
                                  bool>::type>
void Json::normal_push_back(const std::string& key, const T& val)
{
    json_object_t* obj = json_value_object(parent_);
    const json_value_t* find = json_object_find(key.c_str(), obj);
    Json copy_json = val.copy();
    if (find == nullptr)
    {
        json_object_append(obj, key.c_str(), 0, copy_json.node_);
        copy_json.node_ = nullptr;
        return;
    }
    json_object_insert_before(find, obj, key.c_str(), 0, copy_json.node_);
    copy_json.node_ = nullptr;
    json_value_t* remove_val = json_object_remove(find, obj);
    json_value_destroy(remove_val);
}

template <typename T,
          typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                      ::std::is_same<T, Json::Array>::value,
                                  bool>::type>
void Json::push_back(const T& val)
{
    if (!can_arr_push_back())
    {
        return;
    }
    json_array_t* arr = json_value_array(node_);
    Json copy_json = val.copy();
    json_array_append(arr, 0, copy_json.node_);
    copy_json.node_ = nullptr;
}

template <typename T,
          typename std::enable_if<std::is_same<T, Json::Object>::value ||
                                      ::std::is_same<T, Json::Array>::value,
                                  bool>::type>
void Json::update_arr(const T& val)
{
    json_array_t* arr = json_value_array(parent_);
    Json copy_json = val.copy();
    json_array_insert_before(node_, arr, 0, copy_json.node_);
    copy_json.node_ = nullptr;
    json_value_t* remove_val = json_array_remove(node_, arr);
    json_value_destroy(remove_val);
}

} // namespace wfrest

#endif // WFREST_JSON_H_
