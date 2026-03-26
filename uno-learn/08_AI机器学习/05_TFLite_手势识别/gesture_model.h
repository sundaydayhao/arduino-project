/*
 * gesture_model.h
 * 
 * TFLite 量化模型（INT8）- 手势识别
 * 
 * 模型信息：
 *   输入：119 × 6（119个时间步，6轴IMU数据：ax,ay,az,gx,gy,gz）
 *   输出：4类（上挥/下挥/左挥/右挥）
 *   大小：约 18KB
 * 
 * ⚠️ 注意：这是一个占位符模型，用于演示代码结构。
 *    实际使用时，请通过 Edge Impulse 或 TFLite 转换工具
 *    生成真实的量化模型，替换此文件中的数组内容。
 * 
 * 生成真实模型的方法：
 *   1. 使用 Edge Impulse 平台训练并导出 Arduino 库
 *   2. 或使用 Python 脚本：
 *      import tensorflow as tf
 *      converter = tf.lite.TFLiteConverter.from_saved_model(saved_model_dir)
 *      converter.optimizations = [tf.lite.Optimize.DEFAULT]
 *      tflite_model = converter.convert()
 *      # 转为 C 数组
 *      with open('gesture_model.h', 'w') as f:
 *          f.write('const unsigned char gesture_model[] = {')
 *          f.write(','.join([str(b) for b in tflite_model]))
 *          f.write('};')
 */

#ifndef GESTURE_MODEL_H
#define GESTURE_MODEL_H

// 模型数据（占位符 - 请替换为真实模型）
// 真实模型通过 Edge Impulse 或 TFLite 转换工具生成
alignas(8) const unsigned char gesture_model[] = {
  // TFLite FlatBuffer 格式的量化模型数据
  // 此处为示意性占位符，实际字节数约 18000+
  0x1C, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4C, 0x33,  // TFL3 magic
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00,
  // ... 实际模型数据由训练工具生成
  // 请使用 Edge Impulse 导出的真实模型替换此占位符
};

const int gesture_model_len = sizeof(gesture_model);

// 手势标签
const char* GESTURE_LABELS[] = {
  "上挥 (Swipe Up)",
  "下挥 (Swipe Down)",
  "左挥 (Swipe Left)",
  "右挥 (Swipe Right)"
};

const int NUM_GESTURES = 4;

#endif  // GESTURE_MODEL_H