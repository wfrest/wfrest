# WFRest JSON 包装器

这是一个为 WFRest 框架设计的现代 C++ JSON 数据操作包装器。它提供了简单、直观且类型安全的 JSON 数据操作接口。

## 特性

- **简洁的 API**：创建、修改和查询 JSON 数据的易用接口
- **类型安全**：强类型检查，提供有用的错误信息
- **现代 C++ 设计**：使用 C++11 特性实现更清晰的 API
- **内存安全**：基于 RAII 原则的自动内存管理
- **高性能**：高效实现，最小化开销
- **错误处理**：全面的错误报告机制

## 基本用法

### 创建 JSON 值

```cpp
// 创建不同类型的 JSON 值
Json null_value;                      // null
Json bool_value(true);                // true
Json number_value(42);                // 42
Json float_value(3.14);               // 3.14
Json string_value("hello");           // "hello"
Json object = Json::Object{           // Object
    {"name", "John"},
    {"age", 30},
    {"is_active", true}
};
Json array = Json::Array{1, 2, 3, "four", true};  // Array
```

### 解析 JSON

```cpp
// 从字符串解析
Json json = Json::parse(R"({"name": "John", "age": 30})");

// 从文件解析
std::ifstream file("data.json");
Json json = Json::parse(file);

// 检查解析错误
if (!json.is_valid()) {
    std::cerr << "解析错误: " << json.last_error() << std::endl;
}
```

### 访问 JSON 数据

```cpp
// 访问对象属性
Json json = Json::parse(R"({"name": "John", "age": 30, "address": {"city": "New York"}})");
std::string name = json["name"].get<std::string>();  // "John"
int age = json["age"].get<int>();                    // 30
std::string city = json["address"]["city"].get<std::string>();  // "New York"

// 访问数组元素
Json array = Json::parse(R"([1, 2, 3, "four", true])");
int first = array[0].get<int>();           // 1
std::string fourth = array[3].get<std::string>();  // "four"
bool fifth = array[4].get<bool>();         // true

// 检查键是否存在
if (json.has("name")) {
    // 使用 json["name"]
}

// 遍历对象
for (auto it = json.begin(); it != json.end(); ++it) {
    std::cout << "键: " << it->key() << ", 值: " << *it << std::endl;
}

// 遍历数组
for (auto it = array.begin(); it != array.end(); ++it) {
    std::cout << "值: " << *it << std::endl;
}
```

### 修改 JSON 数据

```cpp
// 修改对象属性
Json json = Json::Object{};
json["name"] = "John";
json["age"] = 30;
json["is_active"] = true;
json["tags"] = Json::Array{"developer", "c++"};

// 修改数组元素
Json array = Json::Array{1, 2, 3};
array[1] = "two";  // 替换元素
array.push_back(4);  // 添加元素
array.erase(0);  // 删除元素

// 清空 JSON 值
json.clear();  // 重置为空对象/数组或默认值
```

### 序列化 JSON

```cpp
// 转换为字符串（紧凑格式）
std::string str = json.dump();

// 转换为字符串（美化格式，2空格缩进）
std::string pretty = json.dump(2);

// 输出到流
std::cout << json << std::endl;
```

## 专用对象和数组类

对于更特定的操作，可以使用专用的 `JsonObject` 和 `JsonArray` 类：

```cpp
// 使用 JsonObject
JsonObject obj;
obj.set("name", "John")
   .set("age", 30)
   .set("is_active", true);

// 使用 JsonArray
JsonArray arr;
arr.push_back(1)
   .push_back("two")
   .push_back(true);
```

## 错误处理

JSON 包装器通过异常和错误消息提供错误处理：

```cpp
try {
    Json json = Json::parse("{invalid json}");
    int value = json["nonexistent"].get<int>();
} catch (const JsonException& e) {
    std::cerr << "错误: " << e.what() << std::endl;
}

// 或者检查错误消息
Json json = Json::parse("{invalid json}");
if (!json.is_valid()) {
    std::cerr << "解析错误: " << json.last_error() << std::endl;
}
```

## 相比之前版本的改进

这个 JSON 包装器相比之前的实现有以下几点改进：

1. **简化的 API**：减少了冗余方法，使接口更直观
2. **更好的错误处理**：添加了异常支持和错误消息
3. **类型安全**：改进了类型检查和转换
4. **内存管理**：简化了所有权模型
5. **现代 C++ 特性**：使用更多 C++11 特性
6. **专用类**：添加了 `JsonObject` 和 `JsonArray` 用于更特定的操作
7. **一致的行为**：修复了 API 中的不一致性
8. **文档**：添加了全面的文档

## 实现细节

JSON 包装器是在 Workflow JSON 解析器（`workflow/json_parser.h`）上的一个薄层封装。它提供了更方便和类型安全的接口，同时保持良好的性能。

主要类包括：

- `Json`：用于处理 JSON 数据的主类
- `JsonObject`：用于处理 JSON 对象的专用类
- `JsonArray`：用于处理 JSON 数组的专用类
- `JsonException`：用于 JSON 错误的异常类

## 性能考虑

- 包装器设计为高效，开销最小
- 使用移动语义避免不必要的复制
- 需要时提供对底层 JSON 数据的直接访问
- 对于大型 JSON 文档，考虑使用流式 API 或迭代器

## C++11 兼容性说明

本实现完全兼容 C++11 标准，不依赖任何 C++14 或 C++17 特性。主要的兼容性调整包括：

1. 使用 `typename std::enable_if<...>::type` 替代 `std::enable_if_t<...>`
2. 避免使用 `constexpr if` 结构
3. 不使用 `std::optional` 等 C++17 特性
4. 显式实例化模板以避免链接错误

如果您的项目使用 C++14 或 C++17，本库同样可以正常工作。 