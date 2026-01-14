# SwiftUI 常用组件大全（Markdown 版）

## 1️⃣ 基础布局（Layout）

| 组件 | 作用 | 说明 |
|---|---|---|
| VStack | 垂直布局 | 子视图垂直排列 |
| HStack | 水平布局 | 子视图水平排列 |
| ZStack | 层叠布局 | 视图叠加 |
| Spacer | 弹性间距 | 自动撑开空间 |
| Divider | 分割线 | 横线或竖线 |
| GeometryReader | 读取父视图尺寸 | 自适应布局 |
| Grid (iOS 16+) | 网格布局 | 行列布局 |
| LazyVStack | 懒加载垂直栈 | 大列表性能更好 |
| LazyHStack | 懒加载水平栈 | 同上 |
| LazyVGrid | 懒加载网格 | 瀑布流风格 |
| LazyHGrid | 横向网格 | 横向网格 |

### duorou_gui 对照（Layout）

| SwiftUI | duorou_gui | 备注 |
|---|---|---|
| VStack | Column / VStack | VStack 是 Column 的别名 |
| HStack | Row / HStack | HStack 是 Row 的别名 |
| ZStack | Box / ZStack | ZStack 是 Box 的别名 |
| Spacer | Spacer | 在 Row/Column 中分配剩余空间 |
| Divider | Divider | 分割线 |
| GeometryReader | GeometryReader | 基于父可用尺寸生成子视图 |
| Grid (iOS 16+) | Grid | 网格布局 |
| LazyVStack | LazyVStack | 目前等同 VStack（未做虚拟化） |
| LazyHStack | LazyHStack | 目前等同 HStack（未做虚拟化） |
| LazyVGrid | LazyVGrid | 目前等同 Grid（未做虚拟化） |
| LazyHGrid | LazyHGrid | 目前等同 Grid（axis = horizontal，未做虚拟化） |

2️⃣ 文本与展示类

| 组件 | 作用 | 说明 |
|---|---|---|
| Text | 文本 | 显示字符串 |
| Label | 图标 + 文本 | 常用于菜单 |
| Image | 图片 | 本地或系统图标 |
| AsyncImage | 网络图片 | iOS 15+ |
| ProgressView | 进度条 | 加载、进度 |
| Divider | 分割线 | 视觉分隔 |
| ContentUnavailableView | 空状态 | iOS 17+ |

### duorou_gui 对照（文本与展示类）

| SwiftUI | duorou_gui | 备注 |
|---|---|---|
| Text | Text | 文本显示 |
| Label | Row + Image + Text | 暂无内置 Label，使用组合实现 |
| Image | Image | 同步纹理绘制（TextureHandle） |
| AsyncImage | （未内置） | 网络加载完成后转 TextureHandle 交给 Image |
| ProgressView | ProgressView | 线性进度条（value 0..1） |
| Divider | Divider | 分割线 |
| ContentUnavailableView | （未内置） | 使用 Column/Box + Text 组合实现 |
3️⃣ 输入控件（Form Controls）

| 组件 | 作用 | 说明 |
|---|---|---|
| Button | 按钮 | 点击触发 |
| TextField | 单行输入 | 文本输入 |
| SecureField | 密码输入 | 隐藏字符 |
| TextEditor | 多行文本 | 类似 textarea |
| Toggle | 开关 | On / Off |
| Slider | 滑块 | 数值选择 |
| Stepper | 步进器 | +/- 调整 |
| Picker | 选择器 | 下拉、滚轮 |
| DatePicker | 日期选择 | 日期、时间 |
| ColorPicker | 颜色选择 | 颜色面板 |
| MultiDatePicker | 多选日期 | iOS 16+ |

### duorou_gui 对照（输入控件）

| SwiftUI | duorou_gui | 状态 | 备注 |
|---|---|---|---|
| Button | Button | ✅ | 通过事件回调触发业务逻辑 |
| TextField | TextField | ✅ | 支持 placeholder/value/focus/caret 表现 |
| SecureField | TextField（secure = true） | ✅ | SecureField 为 TextField 的 secure 模式 |
| TextEditor | TextEditor | ✅ | 编辑逻辑由事件实现（demo 中已示例） |
| Toggle | Checkbox / Toggle | ✅ | Toggle 是 Checkbox 的别名（样式偏复选框） |
| Slider | Slider | ✅ | value/min/max |
| Stepper | Stepper | ✅ | 点击左右区域，由业务决定加减 |
| Picker | （未内置） | ⏳ | 可用 Button/ScrollView/List/Overlay 组合实现 |
| DatePicker | （未内置） | ⏳ | 可用 TextField + 业务侧弹窗/Overlay 组合实现 |
| ColorPicker | （未内置） | ⏳ | 可用 Slider/Box 组合实现，或接入原生取色器 |
| MultiDatePicker | （未内置） | ⏳ | 同 DatePicker，需业务侧维护多选状态 |
4️⃣ 列表与集合视图

| 组件 | 作用 | 说明 |
|---|---|---|
| List | 列表 | 类似 UITableView |
| ForEach | 数据驱动 | 用于循环 |
| OutlineGroup | 树结构 | 层级数据 |
| Table | 表格 | macOS / iPadOS |
| ScrollView | 滚动容器 | 自定义滚动 |
| ScrollViewReader | 滚动控制 | 程序控制滚动 |

### duorou_gui 对照（列表与集合视图）

| SwiftUI | duorou_gui | 状态 | 备注 |
|---|---|---|---|
| List | List | ✅ | 当前为 ScrollView + Column 的组合默认样式 |
| ForEach | children(fn)/循环 add | ✅ | 通过构建阶段循环添加 children（demo 已示例） |
| OutlineGroup | （未内置） | ⏳ | 可用递归生成 Column/List 组合实现 |
| Table | （未内置） | ⏳ | 可用 Grid + ScrollView 组合实现 |
| ScrollView | ScrollView | ✅ | 支持滚轮与拖拽滚动、滚动条指示器 |
| ScrollViewReader | （未内置） | ⏳ | 可通过维护 ScrollView 的 scroll_y/scroll_offsets 实现程序滚动 |
5️⃣ 导航与容器

| 组件 | 作用 | 说明 |
|---|---|---|
| NavigationStack | 导航栈 | iOS 16+，基于路径的导航模型 |
| NavigationView | 旧版导航 | iOS 16 起逐渐弃用（推荐 NavigationStack） |
| NavigationLink | 页面跳转 | 在导航容器内 push（也可 value 驱动） |
| TabView | 底部标签栏 | 多页面切换（tab bar / page 风格） |
| NavigationSplitView | 分栏布局 | iPadOS/macOS 常用（iOS 16+） |
| Group | 逻辑分组 | 不影响布局 |
| Section | 分区 | List / Form 用 |

### duorou_gui 对照（导航与容器）

| SwiftUI | duorou_gui | 状态 | 备注 |
|---|---|---|---|
| NavigationStack | （未内置） | ⏳ | 可用单 window + 状态驱动切换根视图模拟 |
| NavigationView | （未内置） | ⏳ | 同上 |
| NavigationLink | （未内置） | ⏳ | 可用 Button 改写状态实现页面切换 |
| TabView | （未内置） | ⏳ | 可用 Row + Button + 条件渲染组合实现 |
| NavigationSplitView | （未内置） | ⏳ | 可用 Row + Box/Column 组合实现 |
| Group | Group | ✅ | 逻辑分组（重建阶段扁平化，不影响布局） |
| Section | Section | ✅ | 标题+内容的组合样式（Box + Column） |

示例：运行 `duorou_gpu_demo --nav-container`（或 `--nav`）查看导航与容器组合演示。
6️⃣ 弹窗 & 覆盖层

| 组件 | 作用 | 说明 |
|---|---|---|
| Alert | 警告弹窗 | 简单提示 |
| ConfirmationDialog | 操作确认 | 底部弹出 |
| Sheet | 模态页 | 半屏 |
| FullScreenCover | 全屏模态 | 覆盖整个屏幕 |
| Popover | 气泡弹窗 | iPad / mac |
| Overlay | 覆盖视图 | 浮层 |

### duorou_gui 对照（弹窗 & 覆盖层）

| SwiftUI | duorou_gui | 状态 | 备注 |
|---|---|---|---|
| Overlay | Overlay | ✅ | 叠加容器（子节点共享同一布局区域） |
| Sheet | Sheet | ✅ | 由 Overlay + Column/Spacer + Box 组合实现 |
| FullScreenCover | FullScreenCover | ✅ | 由 Overlay + 全屏 Box 组合实现 |
| Alert | AlertDialog | ✅ | 由 Overlay + Box + Button 组合实现 |

示例：运行 `duorou_gpu_demo`，在 “Modal / Overlay demo” 区域点击 `Open Sheet` / `Open Alert` / `Open FullScreenCover`。
7️⃣ 交互与手势
组件	作用	说明
onTapGesture	点击	手势
onLongPressGesture	长按	
DragGesture	拖拽	
MagnificationGesture	缩放	
RotationGesture	旋转	
gesture()	组合手势	
8️⃣ 动画 & 视觉效果
组件	作用	说明
withAnimation	动画包裹	
.animation()	绑定动画	
matchedGeometryEffect	视图过渡	
Transition	转场动画	
TimelineView	时间驱动	
Canvas	自定义绘制	
9️⃣ 系统集成类
组件	作用	说明
Map	地图	MapKit
VideoPlayer	视频播放	AVKit
PhotosPicker	相册	
ShareLink	分享	
Link	外部链接	
🔟 状态管理 & 架构相关
组件	作用	说明
Form	表单	自动布局
FocusState	焦点管理	
@State	本地状态	
@Binding	双向绑定	
@ObservedObject	观察对象	
@StateObject	状态对象	
@Environment	环境变量	
@EnvironmentObject	全局共享	

---

duorou_gui 组件对照（当前实现）

duorou_gui 目前的组件类型由 `ViewNode.type` 决定（例如 `"Text"` / `"Button"`），下面是与 SwiftUI 的常用对照关系：

| duorou_gui | SwiftUI 对应 | 备注 |
|---|---|---|
| Column | VStack | 垂直排列子视图 |
| Row | HStack | 水平排列子视图 |
| Box | ZStack | 子视图叠加（当前实现更接近简单叠放） |
| Spacer | Spacer | 弹性间距（在 Row/Column 中分配剩余空间） |
| Divider | Divider | 分割线 |
| Text | Text | 文本显示 |
| Button | Button | 按钮（依赖事件回调触发行为） |
| TextField | TextField | 单行输入（支持 placeholder/value/focus/caret 表现） |
| SecureField | SecureField | 使用 TextField 的 secure 模式显示 |
| TextEditor | TextEditor | 多行显示（支持 caret；编辑逻辑由事件实现） |
| Checkbox | Toggle | SwiftUI 默认是开关样式；macOS 可配置为复选框样式 |
| Slider | Slider | 数值滑块 |
| Stepper | Stepper | 加减器（点击左右区域由业务处理加减） |
| GeometryReader | GeometryReader | 读取父布局尺寸，基于可用尺寸生成子视图 |
| Grid | Grid / LazyVGrid / LazyHGrid | 网格布局（支持 axis/columns/rows/spacing_x/spacing_y） |
| ProgressView | ProgressView | 线性进度条（value 0..1） |
| Image | Image | 同步纹理绘制（TextureHandle） |
| ScrollView | ScrollView | 垂直滚动（drag scroll + clip + scroll_y） |
| List | List | ScrollView + Column 的组合默认样式 |
| Group | Group | 逻辑分组（重建阶段扁平化，不影响布局） |
| Section | Section | 标题+内容的组合样式（Box + Column） |
| Overlay | Overlay | 叠加容器（子节点共享同一布局区域） |
| Sheet | Sheet | 组合组件（Overlay + Column/Spacer + Box） |
| FullScreenCover | FullScreenCover | 组合组件（Overlay + 全屏 Box） |
| Alert | AlertDialog | 组合组件（Overlay + 居中布局 + Box） |

duorou_gui 未覆盖的 SwiftUI 常用项（待扩展）

AsyncImage、NavigationStack/TabView、Picker、动画相关等。
