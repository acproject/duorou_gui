
SwiftUI 常用组件大全（Markdown 版）
1️⃣ 基础布局（Layout）
组件	作用	说明
VStack	垂直布局	子视图垂直排列
HStack	水平布局	子视图水平排列
ZStack	层叠布局	视图叠加
Spacer	弹性间距	自动撑开空间
Divider	分割线	横线或竖线
GeometryReader	读取父视图尺寸	自适应布局
Grid (iOS 16+)	网格布局	行列布局
LazyVStack	懒加载垂直栈	大列表性能更好
LazyHStack	懒加载水平栈	同上
LazyVGrid	懒加载网格	瀑布流风格
LazyHGrid	横向网格	横向网格
2️⃣ 文本与展示类
组件	作用	说明
Text	文本	显示字符串
Label	图标 + 文本	常用于菜单
Image	图片	本地或系统图标
AsyncImage	网络图片	iOS 15+
ProgressView	进度条	加载、进度
Divider	分割线	视觉分隔
ContentUnavailableView	空状态	iOS 17+
3️⃣ 输入控件（Form Controls）
组件	作用	说明
Button	按钮	点击触发
TextField	单行输入	文本输入
SecureField	密码输入	隐藏字符
TextEditor	多行文本	类似 textarea
Toggle	开关	On / Off
Slider	滑块	数值选择
Stepper	步进器	+/- 调整
Picker	选择器	下拉、滚轮
DatePicker	日期选择	日期、时间
ColorPicker	颜色选择	颜色面板
MultiDatePicker	多选日期	iOS 16+
4️⃣ 列表与集合视图
组件	作用	说明
List	列表	类似 UITableView
ForEach	数据驱动	用于循环
OutlineGroup	树结构	层级数据
Table	表格	macOS / iPadOS
ScrollView	滚动容器	自定义滚动
ScrollViewReader	滚动控制	程序控制滚动
5️⃣ 导航与容器
组件	作用	说明
NavigationStack	导航栈	新版导航
NavigationView	旧版导航	已逐渐弃用
NavigationLink	页面跳转	Push
TabView	底部标签栏	多页面
SplitView	分栏布局	iPad / mac
Group	逻辑分组	不影响布局
Section	分区	List / Form 用
6️⃣ 弹窗 & 覆盖层
组件	作用	说明
Alert	警告弹窗	简单提示
ConfirmationDialog	操作确认	底部弹出
Sheet	模态页	半屏
FullScreenCover	全屏模态	覆盖整个屏幕
Popover	气泡弹窗	iPad / mac
Overlay	覆盖视图	浮层
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
| TextField | TextField | 单行输入（支持 placeholder/value/focus 表现） |
| Checkbox | Toggle | SwiftUI 默认是开关样式；macOS 可配置为复选框样式 |
| Slider | Slider | 数值滑块 |

duorou_gui 未覆盖的 SwiftUI 常用项（待扩展）

Spacer、Image/AsyncImage、List/ScrollView、NavigationStack/TabView、Picker/Stepper、Alert/Sheet、动画相关等。
