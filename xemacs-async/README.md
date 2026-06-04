# XEmacs-Async

分布式模块化XEmacs的异步核心模块。

## 模块说明

xemacs-async 是分布式XEmacs架构中的异步核心模块，负责：
- 事件循环管理
- RPC通信
- 服务发现
- 文件监控

## 目录结构

```
xemacs-async/
├── src/                    # 源代码
│   ├── event_loop.c       # 事件循环实现
│   ├── rpc_server.c       # RPC服务器
│   ├── rpc_client.c       # RPC客户端
│   ├── file_watch.c       # 文件监控
│   └── service_disc.c     # 服务发现
├── include/               # 头文件
│   └── xemacs-async.h     # 主头文件
├── tests/                 # 测试套件
│   └── async/            # 单元测试
│       ├── test_event_loop.c
│       ├── test_service_discovery.c
│       ├── test_service_discovery_comprehensive.c
│       ├── test_event_loop_comprehensive.c
│       ├── test_file_watch_comprehensive.c
│       └── test_rpc_client_comprehensive.c
├── configure.ac           # Autoconf配置
└── Makefile.am           # Automake配置
```

## 依赖

- libuv (>= 1.0.0)
- msgpack-c (>= 2.0.0)
- GCC/Clang 编译器
- Autotools (autoconf, automake, libtool)

## 构建

```bash
# 生成configure脚本
autoreconf -ivf

# 配置
./configure --prefix=/tmp/xemacs-test

# 编译
make

# 安装（可选）
make install
```

## 测试

### 运行所有测试

```bash
make check
```

### 运行单个测试

```bash
./test_event_loop
./test_service_discovery
./test_service_discovery_comprehensive
./test_event_loop_comprehensive
./test_file_watch_comprehensive
./test_rpc_client_comprehensive
```

### 内存泄漏检测

```bash
valgrind --leak-check=full ./test_event_loop
```

## API 使用示例

### 初始化事件循环

```c
#include "xemacs-async.h"

int main() {
    if (async_init() != 0) {
        return -1;
    }
    
    // 你的代码...
    
    async_stop();
    return 0;
}
```

### 服务注册与发现

```c
// 注册服务
service_register("my-service", "/tmp/my-service.sock");

// 查找服务
const service_info_t *info = service_find("my-service");
if (info) {
    printf("Found: %s\n", info->socket_path);
}

// 列出所有服务
service_info_t services[100];
int count = service_list(services, 100);

// 注销服务
service_unregister("my-service");
```

### RPC客户端

```c
rpc_client_t client;
rpc_client_connect(&client, "my-client", "/tmp/server.sock");

msgpack_object params = {...};
rpc_client_send(&client, "method-name", params);

rpc_client_disconnect(&client);
```

### 文件监控

```c
void my_callback(const char *path, int events) {
    printf("File changed: %s\n", path);
}

int wd = file_watch_add("/path/to/watch", my_callback);
// ...

file_watch_remove(wd);
```

## 架构说明

### 事件驱动模型

模块使用 libuv 提供的事件循环，支持：
- 非阻塞I/O
- 定时器
- 信号处理

### RPC通信

使用 MessagePack 序列化协议，支持：
- 异步请求/响应
- 错误处理
- 超时检测

### 服务发现

基于内存的服务注册表，支持：
- 服务注册/注销
- 名称查找
- 服务列表

## 安全考虑

1. **内存管理**: 所有分配都有对应的释放
2. **空指针检查**: 所有外部输入都经过验证
3. **边界检查**: 字符串和数组操作有边界保护
4. **资源清理**: 使用 RAII 模式确保资源释放

## 测试覆盖

- 42个测试用例
- 100%核心函数覆盖
- 边界值测试
- 异常情况测试
- 内存泄漏检测

详见 [tests/TEST_DOCUMENTATION.md](tests/TEST_DOCUMENTATION.md)

## 贡献指南

1. 所有新功能必须包含测试
2. 使用 `make check` 确保测试通过
3. 运行 Valgrind 确保无内存泄漏
4. 更新本文档和测试文档

## 许可证

与XEmacs项目一致

## 联系

- 维护者: XEmacs开发团队
- 邮件: xemacs-dev@xemacs.org
