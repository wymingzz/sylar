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

使用[yaml-cpp](https://github.com/jbeder/yaml-cpp/releases/tag/0.8.0)(Yaml库)

进入yaml-cpp目录：

```
mkdir build
cd build
cmake ..
make install
```

安装好的`include`默认在`/usr/local/include`中

## 协程库封装

## socket函数库

## http协议开发

## 分布式协议开发

## 推荐系统
