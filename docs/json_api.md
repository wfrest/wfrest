## Read JSON from a file

To create a json object by reading a JSON file:

You can use c++ file stream:

```cpp
#include "Json.h"
#include <fstream>
using namespace wfrest;

std::ifstream f("example.json");
Json data = Json::parse(f);
```

Or you can use `FILE*`

```cpp
FILE* fp = fopen("example.json", "r");
Json data = Json::parse(fp);
fclose(fp);
```

## Creating json objects from string

Using (raw) string literals and json::parse

```cpp
Json data = Json::parse(R"(
{
  "pi": 3.141,
  "happy": true
}
)");
```

## Creating json objects by initializer list

```cpp
Json data = Json::Object{
    {"null", nullptr},
    {"integer", 1},
    {"float", 1.3},
    {"boolean", true},
    {"string", "something"},
    {"array", Json::Array{1, 2}},
    {"object",
     Json::Object{
         {"key", "value"},
         {"key2", "value2"},
     }},
};
```

## Create simple json value

```cpp
Json null1 = nullptr;

Json num1 = 1;

Json num2 = 1.0;

Json bool1 = true;

Json bool2 = false;

Json str1 = "string";

Json obj1 = Json::Object();

Json arr1 = Json::Array();
```

## Explict declare Json type

If you want to be explicit or express the json type is array or object, the functions Json::Array and Json::Object will help:

```cpp
Json empty_object_explicit = Json::Object();
Json empty_array_explicit = Json::Array();
```

## Parse and dump

```cpp
Json js = Json::parse(R"({"test1":false})");
```

```cpp
Json data;
data["key"]["chanchan"] = 1;

// default compact json string
{"key":{"chanchan":1}}

// using member function
std::string dump_str = data.dump();

// streams
std::ostringstream os;
os << data;
std::cout << os.str() << std::endl;

// For pretty stringification, there is the option to choose the identation size in number of spaces:
{
  "key": {
    "chanchan": 1
  }
}
std::string dump_str_pretty = data.dump(2);
```

## STL-like access

### Example JSON arrays

- Create an array using push_back

```cpp
Json data;
data.push_back(3.141);
data.push_back(true);
data.push_back("chanchan");
data.push_back(nullptr);
Json arr;
arr.push_back(42);
arr.push_back("answer");
data.push_back(arr);
```

- iterate the array

```cpp
for (Json::iterator it = data.begin(); it != data.end(); it++)
{
    std::cout << it->value() << std::endl;
}

for (auto it = data.begin(); it != data.end(); ++it)
{
    std::cout << *it << std::endl;
}

for (const auto& it : data)
{
    std::cout << it << std::endl;
}
```

-iterate the array reversely

```cpp
for (Json::reverse_iterator it = data.rbegin(); it != data.rend(); it++)
{
  std::cout << it->value() << std::endl;
}
```

- access by using operator[index]

```cpp
Json data;
data.push_back(1);        // 0
data.push_back(2.1);      // 1
data.push_back(nullptr);  // 2
data.push_back("string"); // 3
data.push_back(true);     // 4
data.push_back(false);    // 5

// You can user index to access the array element
int num_int = data[0].get<int>();
double num_double = data[1].get<double>();
std::nullptr_t null = data[2].get<std::nullptr_t>();
std::string str = data[3].get<std::string>();
bool bool_true = data[4].get<bool>();
bool bool_false = data[5].get<bool>();

// implicit conversion
int num_int = data[0];
double num_double = data[1];
std::nullptr_t null = data[2];
std::string str = data[3];
bool bool_true = data[4];
bool bool_false = data[5];

// Object
Json::Object obj;
obj["123"] = 12;
obj["123"]["1"] = "test";
data.push_back(obj); // 6
Json::Object obj1 = data[6].get<Json::Object>();
// implicit conversion
Json::Object obj2 = data[6];

// Array
Json::Array arr;
arr.push_back(1);
arr.push_back(nullptr);
data.push_back(arr);

Json::Array arr1 = data[7].get<Json::Array>();
// implicit conversion
Json::Array arr2 = data[7];
```

- Update element

```cpp
arr[1] = true;  // update method
```

- Erase element

```cpp
arr.erase(1);
```

## Example JSON objects

- use push_back

```cpp
Json data;

data.push_back("pi", 3.141);
data.push_back("happy", true);
data.push_back("name", "chanchan");
data.push_back("nothing", nullptr);
Json answer;
answer.push_back("everything", 42);
data.push_back("answer", answer);
```

- use operator[]

The JSON values can be constructed (comfortably) by using standard index operators:

Use operator[] to assign values to JSON objects:

```cpp
// create an empty structure (null)
Json data;
std::cout << "empty structure is " << data << std::endl;

// add a number that is stored as double (note the implicit conversion of j
// to an object)
data["pi"] = 3.141;

// add a Boolean that is stored as bool
data["happy"] = true;

// add a string that is stored as std::string
data["name"] = "chanchan";

// add another null object by passing nullptr
data["nothing"] = nullptr;

// add an object inside the object
data["answer"]["everything"] = 42;
```

- Update element

```cpp
data["key"] = 1;
data["key"] = 2.0;
```

- Erase element

```cpp
data.erase("key");
```

- iterate object elements

```cpp
for (Json::iterator it = data.begin(); it != data.end(); it++)
{
    std::cout << it->key() << " : " << it->value() << std::endl;
}
for (auto it = data.begin(); it != data.end(); it++)
{
    // equal to it->value()
    std::cout << *it << std::endl;
}
for (const auto& it : data)
{
    std::cout << it.key() << " : " << it.value() << std::endl;
}
```

- iterate object elements reversely

```cpp
for (Json::reverse_iterator it = data.rbegin(); it != data.rend(); it++)
{
  std::cout << it->key() << " : " << it->value() << std::endl;
}
```

