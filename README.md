# C++ 服务器框架(sylar)

## 开发环境
Ubuntu 22.04.3

gcc 11.4.0

cmake

## 项目结构
bin -- 二进制

build -- 中间文件路径

cmake -- cmake函数文件夹

CMakeLists.txt -- cmake的定义文件

lib -- 库的输出路径

sylar -- 源代码路径

tests -- 测试代码路径

## 日志系统
1. Log4j(日志格式)

```shell
# 当前已实现的日志输出格式
%m - 日志内容
%p - 日志等级
%r - 程序启动开始到现在的毫秒数
%c - 日志名称
%t - 线程id
%n - 换行
%d - 日期和时间
%f - 文件名
%l - 行号
%F - 协程id
```


        Logger(日志类别)
            |
            |--------Fromatter(日志格式)
            |
        Appender(日志输出)

## 配置系统

Config --> Yaml

### yaml库的安装

使用[yaml-cpp](https://github.com/jbeder/yaml-cpp/releases)(Yaml库)

进入yaml-cpp目录：

```shell
mkdir build
cd build
cmake ..
make install
```

安装好的`include`默认在`/usr/local/include`中

### yaml-cpp的使用

加载yaml文件

```cpp
YAML::Node root = YAML::LoadFile(your_yaml_file_path);
```

yaml文件内容的遍历(yaml数据的三种类型)

```cpp
// map结构的yaml，使用node.IsMap()判断
for(auto it = node.begin(); it != node.end(); ++ it) {
    // map节点中的结构
    // it->first std::string
    // it->second YAML::Node
}

// sequence结构的yaml，使用node.IsSequence()判断
for (size_t i = 0; i < node.size(); ++ i) {

}

// 简单类型的yaml，使用node.IsScalar()判断
// 可直接使用node.Scalar()输出
```

### 配置系统的约定

**配置系统的原则：约定优于配置**

由于之前使用`boost`库中的转换只能在普通类型之间

若想支持复杂类型(`std::vector`、`std::set`等容器)与`std::string`之间的转换，需要进行容器特化

```cpp
// T FromStr()(const std::string) 将std::string转换成特定类型
// std::string ToStr()(const T) 将T类型转换成std::string
template<T, FromStr, ToStr>
class ConfigVar;


// 通用转换模式，支持容器片特化，目前支持vector<T>, list<T>,
// set<T>, unordered_set<T>,
// map<std::string, T>, unordered_map<std::string, T>
template<F, T>
class LexicalCast;
```

#### 自定义类型

若想在配置系统中使用自定义类型，需要实现`sylar::LexicalCast`的片特化设置，实现后就可以支持`Config`解析自定义类型，并且自定义类型可以和常规`stl`容器一起使用

下面是一个模板示例：

```cpp
namespace sylar
{
    template <>
    class LexicalCast<std::string, your_class>
    {
    public:
        your_class operator()(const std::string &v){}
    };

    template <>
    class LexicalCast<your_class, std::string>
    {
    public:
        std::string operator()(const your_class &v){}
    };
}
```

## 协程库封装

## socket函数库

## http协议开发

## 分布式协议开发

## 推荐系统
