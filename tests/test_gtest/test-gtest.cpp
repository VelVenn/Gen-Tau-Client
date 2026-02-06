#include <gtest/gtest.h>

// 示例 1: 简单的逻辑测试
TEST(SimpleTest, MathCheck) {
    EXPECT_EQ(1 + 1, 2);
}

// 示例 2: 模拟你关心的 GStreamer 失败逻辑
TEST(VidRenderTest, MockInitFailure) {
    // 这里可以利用我们之前讨论的 setenv 技巧
    // setenv("GST_PLUGIN_FEATURE_RANK", "qml6glsink:NONE", 1);
    
    // 模拟检查
    bool is_clean = true; 
    EXPECT_TRUE(is_clean);
}