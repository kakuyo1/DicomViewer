# UI 结构草图

## 1. 总体布局

主窗口采用典型 Qt 桌面软件结构，上方为菜单栏和工具栏，中间为主工作区，右侧为缩略图栏。

```text
+-------------------------------------------------------------------------------------------+ 
| MenuBar: File [Open Folder] [Exit]                                                         |
+--------------------------------------------------------------------------------------------+
| Main Toolbar: [Stack View] [MPR View] [VR View] | Mode-specific Toolbar                    |
+---------------------------------------------------------------+----------------------------+
|               |                                                         |                  |
| viewModeBar   |           WorkspaceWidget (QStackedWidget)              |  ThumbnailPanel  |
| |             |                                                         |                  |
| |-- stackBtn  |  StackPage / MPRPage / VRPage                           |  [thumb + index] |
| |-- MPRBtn    |                                                         |  [thumb + index] |
| |-- VRBtn     |                                                         |  [thumb + index] |
|               |                                                         |  ...             |
|               |                                                         |                  |
+---------------------------------------------------------------+----------------------------+
```

说明：

- `WorkspaceWidget` 负责切换三种工作模式页面
- 右侧缩略图栏默认服务于当前已加载的单个 series
- 患者与序列相关信息采用 overlay 方式绘制在图像区域周边，而不是额外占据固定面板

## 2. 导入流程草图

```text
File -> Open Folder
        |
        v
   扫描目录中的 DICOM 文件
        |
        v
按 SeriesInstanceUID 分组
        |
        +--> 仅 1 个可用 series ----> 直接加载
        |
        +--> 多个可用 series -------> 弹出 SeriesSelectionDialog -> 选择后加载
```

## 3. Stack View 草图

`Stack View` 采用单窗格 1x1 布局，强调基础 2D 浏览完整性。

```text
+-----------------------------------------------------------------------+
| Stack Toolbar: Pan | Zoom | W/L | Measure | FlipH | FlipV | Invert | Reset |
+-----------------------------------------------------------------------+
|                                                                       |
|                         SliceViewWidget                                |
|                                                                       |
|   LT: Series / Slice Info                            RT: WW / WL       |
|                                                                       |
|                         DICOM Slice Image                              |
|                                                                       |
|   LB: Orientation / Scale                          RB: Zoom / Tool     |
|                                                                       |
+-----------------------------------------------------------------------+
```

暂定的 overlay 信息：

- 左上：Series Description、Slice 编号
- 右上：WW / WL
- 左下：方向标识
- 右下：缩放比例、当前工具状态

## 4. MPR View 草图

`MPR View` 采用固定三窗格布局，左侧上下两个小窗，右侧一个大窗。当前激活窗格需要有明确高亮。

```text
+----------------------------------------------------------------------------------+
| MPR Toolbar: Pan | Zoom | W/L | Measure | Crosshair | Sync/Link | Reset         |
+---------------------------------------------+------------------------------------+
|                                             |                                    |
|  [Sagittal View]                            |                                    |
|  SliceViewWidget                            |                                    |
|  (small)                                    |                                    |
|  * active 时边框高亮                        |         [Axial View]               |
|                                             |         SliceViewWidget            |
+---------------------------------------------+         (large)                    |
|                                             |         * 常作为主展示窗格         |
|  [Coronal View]                             |                                    |
|  SliceViewWidget                            |                                    |
|  (small)                                    |                                    |
|  * active 时边框高亮                        |                                    |
|                                             |                                    |
+---------------------------------------------+------------------------------------+
```

交互约束：

- 工具栏操作始终作用于当前 `active view`
- 三个窗格共享同一份 `VolumeData`
- 十字线表示同一个空间位置在三视图中的投影
- 当 `Sync/Link` 开启时，切片联动和十字线联动同时生效

## 5. VR View 草图

`VR View` 采用单窗格全区域显示，强调体渲染演示效果。

```text
+----------------------------------------------------------------------------+
| VR Toolbar: Rotate | Pan | Zoom | Preset(Bone/Soft Tissue/Skin) | Reset   |
+----------------------------------------------------------------------------+
|                                                                            |
|                           VolumeViewWidget                                 |
|                                                                            |
|                           Volume Rendering                                  |
|                                                                            |
|         LT: Series Info                        RB: Current Preset           |
|                                                                            |
+----------------------------------------------------------------------------+
```

说明：

- VR 模式下可保留右侧缩略图栏，也可以在实现时考虑默认弱化或折叠
- 第一版重点是“可演示”，不追求复杂调参面板

## 6. 工具栏分层设计

工具栏分为两层：

```text
[全局视图模式切换]
Stack View | MPR View | VR View

[模式相关工具栏]
Stack: Pan | Zoom | W/L | Measure | FlipH | FlipV | Invert | Reset
MPR:   Pan | Zoom | W/L | Measure | Crosshair | Sync/Link | Reset
VR:    Rotate | Pan | Zoom | Preset | Reset Camera
```

这样设计的目的：

- 避免在错误模式下暴露不适用工具
- 让用户明确区分“切换工作模式”和“当前模式下的操作工具”
- 降低交互歧义，增强桌面应用的工程感

## 7. 缩略图栏草图

右侧缩略图栏使用 Qt 的 Model/View 结构实现。

```text
+----------------------+
| Thumbnail Sidebar    |
+----------------------+
| [thumb] Slice 001    |
| [thumb] Slice 002    |
| [thumb] Slice 003    |
| [thumb] Slice 004    |
| ...                  |
+----------------------+
```

交互约束：

- 初始只加载可见区域和邻近范围缩略图
- 滚动过程中后台继续补齐
- 点击任意缩略图后，主视图跳转到对应切片
- 当前切片对应 item 需要有选中态或高亮态

## 8. 关键交互状态

为减少使用歧义，UI 中应明确体现以下状态：

- 当前 `View Mode`：Stack / MPR / VR
- 当前 `Tool`：Pan / Zoom / W/L / Measure / Rotate 等
- MPR 当前 `Active View`
- 当前切片编号
- 当前窗宽/窗位
- 当前缩放比例

## 9. 第一版 UI 设计原则

第一版 UI 追求以下原则：

- 清晰，不堆砌不必要面板
- 接近医学影像软件常见交互习惯
- 便于 Qt 实现，不引入过重的复杂布局系统
- 优先保证演示流畅度，而不是追求过度华丽的界面
