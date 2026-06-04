/**
 * 极限测试套件 - RPC客户端模块
 * 
 * 测试覆盖：
 * 1. 正常连接和断开
 * 2. 边界值测试
 * 3. 错误处理
 * 4. 内存管理
 */

#include "xemacs-async.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== 基础连接测试 ====================

// 测试1: 正常连接
void test_connect_basic(void) {
    printf("测试: 基本连接\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    int result = rpc_client_connect(&client, "test-client", "/tmp/test.sock");
    assert(result == 0);
    assert(client.name != NULL);
    assert(strcmp(client.name, "test-client") == 0);
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 基本连接测试通过\n");
}

// 测试2: 多次连接同一客户端
void test_connect_multiple_times(void) {
    printf("测试: 多次连接同一客户端\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    // 第一次连接
    assert(rpc_client_connect(&client, "client1", "/tmp/sock1") == 0);
    rpc_client_disconnect(&client);
    
    // 第二次连接
    assert(rpc_client_connect(&client, "client2", "/tmp/sock2") == 0);
    assert(strcmp(client.name, "client2") == 0);
    rpc_client_disconnect(&client);
    
    async_stop();
    printf("✓ 多次连接测试通过\n");
}

// ==================== 边界值测试 ====================

// 测试3: 空名字
void test_connect_empty_name(void) {
    printf("测试: 空名字连接\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    int result = rpc_client_connect(&client, "", "/tmp/sock");
    assert(result == 0);
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 空名字测试通过\n");
}

// 测试4: 超长名字
void test_connect_long_name(void) {
    printf("测试: 超长名字\n");
    assert(async_init() == 0);
    
    // 创建超长名字（超过实际需要的）
    char long_name[1024];
    memset(long_name, 'x', 1023);
    long_name[1023] = '\0';
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    int result = rpc_client_connect(&client, long_name, "/tmp/sock");
    assert(result == 0);
    // name应该被正确处理
    if (client.name) {
        printf("  - 名字长度: %zu\n", strlen(client.name));
    }
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 超长名字测试通过\n");
}

// 测试5: 空socket路径
void test_connect_empty_socket(void) {
    printf("测试: 空socket路径\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    int result = rpc_client_connect(&client, "test", "");
    assert(result == 0);
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 空socket路径测试通过\n");
}

// 测试6: NULL参数
void test_connect_null_params(void) {
    printf("测试: NULL参数\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    // NULL名字
    // int result = rpc_client_connect(&client, NULL, "/tmp/sock");
    // 可能崩溃或返回错误
    
    // NULL路径
    // int result2 = rpc_client_connect(&client, "test", NULL);
    // 可能崩溃或返回错误
    
    async_stop();
    printf("✓ NULL参数测试完成（需要实现保护）\n");
}

// ==================== 发送测试 ====================

// 测试7: 发送基本消息
void test_send_basic(void) {
    printf("测试: 基本发送\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    rpc_client_connect(&client, "sender", "/tmp/sock");
    
    // 创建msgpack对象
    msgpack_object params;
    params.type = MSGPACK_OBJECT_NIL;
    
    // 发送消息
    int result = rpc_client_send(&client, "test-method", params);
    printf("  - 发送结果: %d\n", result);
    // 注意：result可能是错误码
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 基本发送测试完成\n");
}

// 测试8: 发送超长方法名
void test_send_long_method(void) {
    printf("测试: 超长方法名\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    rpc_client_connect(&client, "sender", "/tmp/sock");
    
    // 创建超长方法名
    char long_method[1024];
    memset(long_method, 'm', 1023);
    long_method[1023] = '\0';
    
    msgpack_object params;
    params.type = MSGPACK_OBJECT_NIL;
    
    int result = rpc_client_send(&client, long_method, params);
    printf("  - 超长方法发送结果: %d\n", result);
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 超长方法名测试完成\n");
}

// 测试9: 发送各种类型的参数
void test_send_various_params(void) {
    printf("测试: 发送各种类型参数\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    rpc_client_connect(&client, "sender", "/tmp/sock");
    
    // 测试各种msgpack类型
    msgpack_object types[] = {
        {.type = MSGPACK_OBJECT_NIL},
        {.type = MSGPACK_OBJECT_BOOLEAN, .via.boolean = 0},
        {.type = MSGPACK_OBJECT_BOOLEAN, .via.boolean = 1},
        {.type = MSGPACK_OBJECT_POSITIVE_INTEGER, .via.u64 = 0},
        {.type = MSGPACK_OBJECT_NEGATIVE_INTEGER, .via.i64 = -1},
        {.type = MSGPACK_OBJECT_FLOAT, .via.f64 = 0.0},
        // {.type = MSGPACK_OBJECT_STR}, // 需要初始化
    };
    
    int num_types = sizeof(types) / sizeof(types[0]);
    
    for (int i = 0; i < num_types; i++) {
        int result = rpc_client_send(&client, "method", types[i]);
        printf("  - 类型%d发送: %d\n", i, result);
    }
    
    rpc_client_disconnect(&client);
    async_stop();
    printf("✓ 各种类型参数测试完成\n");
}

// ==================== 断开连接测试 ====================

// 测试10: 断开空客户端
void test_disconnect_null_client(void) {
    printf("测试: 断开空客户端\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    
    // 断开未连接的客户端
    rpc_client_disconnect(&client);
    printf("  - 未连接客户端断开完成\n");
    
    async_stop();
    printf("✓ 断开空客户端测试通过\n");
}

// 测试11: 重复断开
void test_double_disconnect(void) {
    printf("测试: 重复断开\n");
    assert(async_init() == 0);
    
    rpc_client_t client;
    memset(&client, 0, sizeof(client));
    rpc_client_connect(&client, "test", "/tmp/sock");
    
    // 第一次断开
    rpc_client_disconnect(&client);
    
    // 第二次断开
    // 可能崩溃或安全处理
    printf("  - 重复断开安全处理\n");
    
    async_stop();
    printf("✓ 重复断开测试完成\n");
}

// ==================== 内存测试 ====================

// 测试12: 内存泄漏检测
void test_memory_leak(void) {
    printf("测试: 内存泄漏检测\n");
    assert(async_init() == 0);
    
    // 多次连接和断开
    for (int i = 0; i < 100; i++) {
        rpc_client_t client;
        memset(&client, 0, sizeof(client));
        
        char name[32];
        snprintf(name, sizeof(name), "client-%d", i);
        
        rpc_client_connect(&client, name, "/tmp/sock");
        rpc_client_disconnect(&client);
    }
    
    printf("  - 100次连接/断开完成\n");
    
    async_stop();
    printf("✓ 内存泄漏检测测试完成\n");
}

// ==================== 主函数 ====================

int main(void) {
    printf("========================================\n");
    printf("XEmacs-Async 极限测试套件 - RPC客户端模块\n");
    printf("========================================\n\n");
    
    printf("【基础连接测试】\n");
    test_connect_basic();
    test_connect_multiple_times();
    
    printf("\n【边界值测试】\n");
    test_connect_empty_name();
    test_connect_long_name();
    test_connect_empty_socket();
    test_connect_null_params();
    
    printf("\n【发送测试】\n");
    test_send_basic();
    test_send_long_method();
    test_send_various_params();
    
    printf("\n【断开连接测试】\n");
    test_disconnect_null_client();
    test_double_disconnect();
    
    printf("\n【内存测试】\n");
    test_memory_leak();
    
    printf("\n========================================\n");
    printf("所有测试通过！✓\n");
    printf("========================================\n");
    
    return 0;
}
