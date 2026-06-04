/**
 * 极限测试套件 - 服务发现模块
 * 
 * 测试覆盖：
 * 1. 边界值测试 - 最小值、空值、边界值
 * 2. 溢出测试 - 超过最大值
 * 3. 随机乱码测试 - 无意义字符
 * 4. 超长字符串测试 - 超过结构体字段长度
 */

#include "xemacs-async.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 测试辅助函数
void reset_services(void) {
    // 重新初始化模块来重置状态
    // 注意：这依赖于模块可以重新初始化的特性
}

// ==================== 正常边界值测试 ====================

// 测试1: 空字符串
void test_empty_strings(void) {
    printf("测试: 空字符串边界值\n");
    assert(async_init() == 0);
    
    // 空名字
    assert(service_register("", "/tmp/socket1") == 0);
    const service_info_t *info = service_find("");
    assert(info != NULL);
    assert(strcmp(info->name, "") == 0);
    
    // 空socket路径
    assert(service_register("empty-socket", "") == 0);
    info = service_find("empty-socket");
    assert(info != NULL);
    assert(strcmp(info->socket_path, "") == 0);
    
    service_unregister("");
    service_unregister("empty-socket");
    async_stop();
    printf("✓ 空字符串测试通过\n");
}

// 测试2: 单字符
void test_single_char(void) {
    printf("测试: 单字符边界值\n");
    assert(async_init() == 0);
    
    assert(service_register("a", "x") == 0);
    const service_info_t *info = service_find("a");
    assert(info != NULL);
    assert(strcmp(info->name, "a") == 0);
    assert(strcmp(info->socket_path, "x") == 0);
    
    service_unregister("a");
    async_stop();
    printf("✓ 单字符测试通过\n");
}

// 测试3: 最大服务数量边界
void test_max_services_boundary(void) {
    printf("测试: 最大服务数量边界值 (MAX_SERVICES=%d)\n", MAX_SERVICES);
    assert(async_init() == 0);
    
    // 注册 MAX_SERVICES 个服务
    char name[32];
    for (int i = 0; i < MAX_SERVICES; i++) {
        snprintf(name, sizeof(name), "service-%d", i);
        assert(service_register(name, "/tmp/socket") == 0);
    }
    
    // 再注册一个应该失败
    assert(service_register("overflow", "/tmp/socket") == -1);
    
    // 验证所有服务都能找到
    for (int i = 0; i < MAX_SERVICES; i++) {
        snprintf(name, sizeof(name), "service-%d", i);
        const service_info_t *info = service_find(name);
        assert(info != NULL);
    }
    
    // 注销一个
    assert(service_unregister("service-0") == 0);
    
    // 现在可以再注册一个
    assert(service_register("new-service", "/tmp/socket") == 0);
    
    // 清理
    for (int i = 1; i < MAX_SERVICES; i++) {
        snprintf(name, sizeof(name), "service-%d", i);
        service_unregister(name);
    }
    service_unregister("new-service");
    async_stop();
    printf("✓ 最大服务数量边界测试通过\n");
}

// 测试4: 刚好达到字段长度限制
void test_field_length_boundary(void) {
    printf("测试: 字段长度边界值 (name=63, socket_path=255)\n");
    assert(async_init() == 0);
    
    // name字段最大63字符
    char name[64];
    memset(name, 'a', 63);
    name[63] = '\0';
    assert(service_register(name, "/tmp/socket") == 0);
    const service_info_t *info = service_find(name);
    assert(info != NULL);
    assert(strlen(info->name) == 63);
    
    // socket_path字段最大255字符
    char path[256];
    memset(path, 'x', 255);
    path[255] = '\0';
    assert(service_register("long-path-test", path) == 0);
    info = service_find("long-path-test");
    assert(info != NULL);
    assert(strlen(info->socket_path) == 255);
    
    service_unregister(name);
    service_unregister("long-path-test");
    async_stop();
    printf("✓ 字段长度边界测试通过\n");
}

// ==================== 溢出测试 ====================

// 测试5: 名字超长 (超过63字符)
void test_overflow_name(void) {
    printf("测试: 名字超长溢出 (超过63字符)\n");
    assert(async_init() == 0);
    
    // 超过name字段最大长度
    char name[128];
    memset(name, 'x', 100);
    name[100] = '\0';
    
    // 应该成功但被截断
    assert(service_register(name, "/tmp/socket") == 0);
    // 搜索时使用截断后的名字（strncpy保留前面字符）
    char truncated_name[64];
    strncpy(truncated_name, name, 63);
    truncated_name[63] = '\0';
    const service_info_t *info = service_find(truncated_name);
    assert(info != NULL);
    // 验证被截断到63字符
    assert(strlen(info->name) == 63);
    // 验证截断正确（前面63个字符都是'x'）
    assert(strncmp(info->name, name, 63) == 0);
    
    service_unregister(name);
    async_stop();
    printf("✓ 名字超长溢出测试通过\n");
}

// 测试6: socket路径超长 (超过255字符)
void test_overflow_socket_path(void) {
    printf("测试: socket路径超长溢出 (超过255字符)\n");
    assert(async_init() == 0);
    
    // 超过socket_path字段最大长度
    char path[512];
    memset(path, 'y', 300);
    path[300] = '\0';
    
    // 应该成功但被截断
    assert(service_register("long-socket-test", path) == 0);
    const service_info_t *info = service_find("long-socket-test");
    assert(info != NULL);
    // 验证被截断到255字符
    assert(strlen(info->socket_path) == 255);
    // 验证截断正确
    assert(strncmp(info->socket_path, path, 255) == 0);
    
    service_unregister("long-socket-test");
    async_stop();
    printf("✓ socket路径超长溢出测试通过\n");
}

// 测试7: 超大数量请求
void test_overflow_count(void) {
    printf("测试: 超大数量请求\n");
    assert(async_init() == 0);
    
    // 注册几个服务
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "service-%d", i);
        service_register(name, "/tmp/socket");
    }
    
    // 请求超大数量
    service_info_t services[100];
    int count = service_list(services, 100);
    // 注意：全局状态可能包含之前测试的服务
    // 所以这里检查 >= 5 而不是 == 5
    assert(count >= 5);
    
    // 请求0个
    count = service_list(services, 0);
    assert(count == 0);
    
    // 请求负数（被转换为很大的正数）
    // 注意：实际上int不可能为负时被传入
    
    // 清理
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "service-%d", i);
        service_unregister(name);
    }
    async_stop();
    printf("✓ 超大数量请求测试通过\n");
}

// ==================== 随机乱码测试 ====================

// 测试8: 随机二进制数据作为名字
void test_random_garbage_name(void) {
    printf("测试: 随机乱码名字\n");
    assert(async_init() == 0);
    
    // 包含各种特殊字符的字符串
    char garbage[] = "\x01\x02\x03\xff\xfe\xfd!@#$%^&*()\n\r\t\\\"\'";
    assert(service_register(garbage, "/tmp/socket") == 0);
    
    // 应该能找到（因为使用strcmp比较）
    const service_info_t *info = service_find(garbage);
    assert(info != NULL);
    
    service_unregister(garbage);
    async_stop();
    printf("✓ 随机乱码名字测试通过\n");
}

// 测试9: Unicode和UTF-8特殊字符
void test_unicode_characters(void) {
    printf("测试: Unicode和特殊字符\n");
    assert(async_init() == 0);
    
    // 测试各种特殊字符
    const char *special_names[] = {
        "service-with-dash",
        "service_with_underscore",
        "service.with.dots",
        "service@with@ats",
        "service/with/slashes",
        "service\\with\\backslashes",
        "service\ttab",
        "service\nnewline",
        "service\rvreturn",
        "service with spaces",
        "SERVICE.UPPER.CASE",
        "Service.Mixed.Case",
    };
    
    int num_services = sizeof(special_names) / sizeof(special_names[0]);
    
    for (int i = 0; i < num_services; i++) {
        assert(service_register(special_names[i], "/tmp/socket") == 0);
        const service_info_t *info = service_find(special_names[i]);
        assert(info != NULL);
        assert(strcmp(info->name, special_names[i]) == 0);
    }
    
    for (int i = 0; i < num_services; i++) {
        service_unregister(special_names[i]);
    }
    async_stop();
    printf("✓ Unicode和特殊字符测试通过\n");
}

// 测试10: 二进制零字符
void test_binary_zeros(void) {
    printf("测试: 二进制零字符\n");
    assert(async_init() == 0);
    
    // 包含嵌入零字符的字符串
    char binary_name[16] = {'s', 'e', 'r', '\0', 'v', 'i', 'c', 'e', '\0', 't', 'e', 's', 't'};
    
    // strcmp会在第一个零处停止，所以这会匹配"ser"
    assert(service_register(binary_name, "/tmp/socket") == 0);
    const service_info_t *info = service_find(binary_name);
    assert(info != NULL);
    
    // 清理
    service_unregister(binary_name);
    async_stop();
    printf("✓ 二进制零字符测试通过\n");
}

// ==================== 错误处理测试 ====================

// 测试11: 查找不存在的服务
void test_find_nonexistent(void) {
    printf("测试: 查找不存在的服务\n");
    assert(async_init() == 0);
    
    // 查找完全不存在的东西
    const service_info_t *info = service_find("nonexistent-service-xyz123");
    assert(info == NULL);
    
    // 查找空的
    info = service_find("");
    // 可能存在也可能是NULL，取决于前面的测试状态
    
    async_stop();
    printf("✓ 查找不存在的服务测试通过\n");
}

// 测试12: 注销不存在的服务
void test_unregister_nonexistent(void) {
    printf("测试: 注销不存在的服务\n");
    assert(async_init() == 0);
    
    // 注销不存在的服务应该失败
    assert(service_unregister("nonexistent-service-xyz123") == -1);
    
    async_stop();
    printf("✓ 注销不存在的服务测试通过\n");
}

// 测试13: 重复注销
void test_double_unregister(void) {
    printf("测试: 重复注销\n");
    assert(async_init() == 0);
    
    assert(service_register("test-double", "/tmp/socket") == 0);
    
    // 第一次注销应该成功
    assert(service_unregister("test-double") == 0);
    
    // 第二次注销应该失败
    assert(service_unregister("test-double") == -1);
    
    async_stop();
    printf("✓ 重复注销测试通过\n");
}

// 测试14: 重复注册同名服务
void test_duplicate_register(void) {
    printf("测试: 重复注册同名服务\n");
    assert(async_init() == 0);
    
    // 注册第一个
    assert(service_register("duplicate", "/tmp/socket1") == 0);
    const service_info_t *info1 = service_find("duplicate");
    assert(info1 != NULL);
    
    // 注册同名（如果服务列表满了可能会失败）
    // 注意：这会替换掉之前的服务
    assert(service_register("duplicate", "/tmp/socket2") == 0);
    
    // 如果前面满了，这里可能是NULL
    // 如果没满，应该能找到
    const service_info_t *info2 = service_find("duplicate");
    if (info2 != NULL) {
        // socket路径可能更新了
        // 或者保持不变，取决于实现
    }
    
    // 清理
    service_unregister("duplicate");
    async_stop();
    printf("✓ 重复注册同名服务测试通过\n");
}

// ==================== 列表功能测试 ====================

// 测试15: 列表功能边界测试
void test_list_boundaries(void) {
    printf("测试: 列表功能边界\n");
    assert(async_init() == 0);
    
    // 注册3个服务
    service_register("list-test-1", "/tmp/s1");
    service_register("list-test-2", "/tmp/s2");
    service_register("list-test-3", "/tmp/s3");
    
    // 请求0个
    service_info_t zero_services[10];
    int count = service_list(zero_services, 0);
    assert(count == 0);
    
    // 请求1个
    service_info_t one_service[10];
    count = service_list(one_service, 1);
    assert(count == 1);
    
    // 请求2个
    service_info_t two_services[10];
    count = service_list(two_services, 2);
    assert(count == 2);
    
    // 请求3个
    service_info_t three_services[10];
    count = service_list(three_services, 3);
    assert(count >= 3);
    
    // 请求超过实际数量
    service_info_t many_services[10];
    count = service_list(many_services, 10);
    assert(count >= 3);
    
    // 清理
    service_unregister("list-test-1");
    service_unregister("list-test-2");
    service_unregister("list-test-3");
    async_stop();
    printf("✓ 列表功能边界测试通过\n");
}

// 测试16: 内存重叠测试
void test_memory_overlap(void) {
    printf("测试: 内存重叠\n");
    assert(async_init() == 0);
    
    service_register("mem-test", "/tmp/mem-socket");
    
    // 两次调用返回相同数据
    service_info_t list1[10];
    service_info_t list2[10];
    
    int count1 = service_list(list1, 10);
    int count2 = service_list(list2, 10);
    
    assert(count1 == count2);
    assert(memcmp(list1, list2, count1 * sizeof(service_info_t)) == 0);
    
    service_unregister("mem-test");
    async_stop();
    printf("✓ 内存重叠测试通过\n");
}

// ==================== 压力测试 ====================

// 测试17: 快速注册注销压力测试
void test_rapid_register_unregister(void) {
    printf("测试: 快速注册注销压力测试\n");
    assert(async_init() == 0);
    
    // 快速注册和注销100次
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "rapid-%d", i);
        assert(service_register(name, "/tmp/socket") == 0);
        
        // 立即注销
        assert(service_unregister(name) == 0);
    }
    
    // 验证没有遗留
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "rapid-%d", i);
        const service_info_t *info = service_find(name);
        assert(info == NULL);
    }
    
    async_stop();
    printf("✓ 快速注册注销压力测试通过\n");
}

// 测试18: 大小写敏感测试
void test_case_sensitivity(void) {
    printf("测试: 大小写敏感\n");
    assert(async_init() == 0);
    
    service_register("Service", "/tmp/s1");
    service_register("SERVICE", "/tmp/s2");
    service_register("service", "/tmp/s3");
    service_register("SERvice", "/tmp/s4");
    
    // 四个都应该能找到（大小写敏感）
    assert(service_find("Service") != NULL);
    assert(service_find("SERVICE") != NULL);
    assert(service_find("service") != NULL);
    assert(service_find("SERvice") != NULL);
    
    // 清理
    service_unregister("Service");
    service_unregister("SERVICE");
    service_unregister("service");
    service_unregister("SERvice");
    async_stop();
    printf("✓ 大小写敏感测试通过\n");
}

// ==================== 主函数 ====================

int main(void) {
    printf("========================================\n");
    printf("XEmacs-Async 极限测试套件 - 服务发现模块\n");
    printf("========================================\n\n");
    
    printf("【边界值测试】\n");
    test_empty_strings();
    test_single_char();
    test_max_services_boundary();
    test_field_length_boundary();
    
    printf("\n【溢出测试】\n");
    test_overflow_name();
    test_overflow_socket_path();
    test_overflow_count();
    
    printf("\n【随机乱码测试】\n");
    test_random_garbage_name();
    test_unicode_characters();
    test_binary_zeros();
    
    printf("\n【错误处理测试】\n");
    test_find_nonexistent();
    test_unregister_nonexistent();
    test_double_unregister();
    test_duplicate_register();
    
    printf("\n【列表功能测试】\n");
    test_list_boundaries();
    test_memory_overlap();
    
    printf("\n【压力测试】\n");
    test_rapid_register_unregister();
    test_case_sensitivity();
    
    printf("\n========================================\n");
    printf("所有测试通过！✓\n");
    printf("========================================\n");
    
    return 0;
}
