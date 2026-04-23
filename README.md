# DicomViewer

一个轻量化的 DICOM Viewer。

- 本地导入 CT DICOM
- 选 series
- 构建统一 `VolumeData`
- 在 2D 视图里稳定地浏览、测量、调窗宽窗位
- 把缩略图、异步任务、UI 交互这些工程细节也做得说得过去

目前它更像一个**完成度比较高的 2D Viewer**，MPR / VR 还在后续推进中。

## 现在有啥

- `Open Folder` 导入本地 DICOM 目录
- 按 `SeriesInstanceUID` 分组，多序列时弹窗选择
- Stack View：
  - 滚轮翻页
  - Pan / Zoom
  - Window / Level
  - Distance Measure
  - Flip H / Flip V / Invert / Reset
  - overlay 信息显示
- 右侧缩略图栏：
  - Model/View 结构
  - 懒加载
  - 后台线程生成
  - 与主视图切片、显示参数同步
- 一版深色工作站风格 UI
- `spdlog` 日志

## 现在还没有

- MPR View
- VR View
- 临床级兼容性和完整性

所以如果你现在打开它，主角还是 Stack View。

## 技术栈

- `C++17`
- `Qt`
- `VTK`
- `DCMTK`
- `spdlog`
- 平台：`Linux`

## 构建

项目当前用 `CMake + Ninja`。

```bash
cmake -S . -B build/debug -G Ninja
cmake --build build/debug
```

运行：

```bash
./build/debug/DicomViewer
```

## 项目结构

```text
src/
  app/          主窗口与应用入口
  services/     导入流程、会话状态
  core/         DICOM 扫描、Volume 构建、图像生成
  ui/           页面、控件、缩略图、对话框
  common/       工具函数、全局初始化
docs/           设计说明和范围文档
```

## 说明

- 当前默认样式文件是 `src/theme/style.qss`
- 当前主要面向常见未压缩 CT DICOM 数据
- 单次只加载一个 series

## 文档

可以先看这些：

- `docs/项目范围说明.md`
- `docs/功能清单和优先级.md`
- `docs/核心架构设计.md`
- `docs/UI结构草图.md`

## 最后

这个项目还没到“收尾”阶段，但已经过了“只能截图不能演示”的阶段。

现在它是：

- 一个能稳定演示主流程的 DICOM Viewer
- 一个以 2D 为主、正在继续往 MPR / VR 推进的工程项目
