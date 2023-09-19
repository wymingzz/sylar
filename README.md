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

        Logger(日志类别)
            |
            |--------Fromatter(日志格式)
            |
        Appender(日志输出)

## 配置系统

Config --> Yaml

### yaml库的安装

使用[yaml-cpp](https://github.com/jbeder/yaml-cpp/releases/tag/0.8.0)(Yaml库)

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


## 协程库封装

## socket函数库

## http协议开发

## 分布式协议开发

## 推荐系统
