# DicomViewer

一个轻量化的 CT DICOM Viewer，基于 `Qt Widgets + VTK + DCMTK` 实现。

项目目标不是临床级 workstation，而是一个可以完整演示 DICOM 导入、2D 浏览、MPR 和 VR 的轻量化项目。

## 当前状态

当前主链路已经打通：

```text
DICOM Folder
-> Series Scan / Selection
-> VolumeData
-> Stack View
-> MPR View
-> VR View
```

## 功能

### DICOM 导入

- 本地文件夹导入
- 按 `SeriesInstanceUID` 分组
- 多 series 时弹窗选择
- CT modality 过滤
- 基于 `ImagePositionPatient / ImageOrientationPatient` 的几何排序
- `spacingZ` 几何推导，无法推导时 fallback
- HU 转换：`RescaleSlope / RescaleIntercept`
- pixel padding 基础处理
- 导入日志和 warning 输出

### Stack View

- Axial slice 浏览
- 滚轮切片
- Pan / Zoom
- Window / Level
- Distance Measure
- Flip H / Flip V
- Invert
- Reset
- 方向标记 `L/R/A/P/S/I`
- overlay 信息显示
- 右侧 thumbnail 同步当前切片

### Thumbnail

- `QListView + QAbstractListModel + QStyledItemDelegate`
- 可见区域懒加载
- 后台线程生成 thumbnail
- 与主视图 slice 同步
- 与 Window/Level、Invert、Flip 显示参数同步
- display revision 防止旧异步任务覆盖新图

### MPR View

- Sagittal / Coronal / Axial 三视图
- voxel-space 正交 MPR
- 三方向 slice 构建
- Crosshair 点击同步
- Crosshair 拖动同步
- Crosshair 拖动节流到约 60 FPS
- 只更新 `sliceIndex` 实际变化的视图
- Flip 后 Crosshair 坐标修正
- Coronal / Sagittal 默认 `S` 在上、`I` 在下
- 三视图方向标记

### VR View

- VTK GPU volume rendering
- `vtkImageImport -> SetInputConnection(...)` pipeline
- CPU 侧共享同一份 `VolumeData::voxels`
- Bone / Soft Tissue / Skin 三个硬编码 preset
- VR 模式 toolbar 只显示 preset 和 Reset
- VTK trackball camera 交互
- 延迟刷新，避免隐藏 VR 页面在导入后立即上传和渲染 volume

## 技术栈

- `C++17`
- `Qt Widgets`
- `Qt Concurrent`
- `VTK`
- `DCMTK`
- `spdlog`
- `CMake + Ninja`
- 平台：`Linux`

## 构建

依赖需要能被 CMake 找到：

- Qt 5 或 Qt 6 Widgets / Concurrent
- VTK，包含 `GUISupportQt`、`RenderingOpenGL2`、`RenderingVolumeOpenGL2`
- DCMTK
- Ninja

## 项目结构

```text
src/
  app/          主窗口、主流程连接
  common/       工具函数、日志、样式初始化
  core/         DICOM 扫描、Volume 构建、Slice 图像生成
  services/     导入控制器、ViewerSession、导入结果模型
  ui/           页面、工具栏、控件、thumbnail、dialog
resources/      图标和 logo
docs/           项目说明、设计文档、阶段计划
```

## 设计重点

- `ViewerSession` 管理当前导入结果，Stack / MPR / VR 共享同一份 `VolumeData`
- Stack / MPR / VR 通过 `WorkSpaceWidget` 分页管理
- `SliceViewWidget` 负责 2D slice 的 VTK 显示和交互
- `MPRPage` 负责三视图状态、Crosshair、方向标记和坐标映射
- `VRPage` 负责 VTK volume rendering pipeline 和 preset
- Thumbnail 使用异步任务和 revision 管理，避免旧图覆盖新图

## 限制

- 当前主要面向常见未压缩 CT DICOM 数据
- 单次只加载一个 series
- MPR 是 voxel-space 正交 MPR，不是完整 patient-space reslice
- 不支持 oblique MPR、slab、MIP、MinIP
- VR 是 lightweight preview，不是临床级高性能 VR
- Thumbnail 在极端卡顿情况下仍可能需要进一步增强自恢复机制
