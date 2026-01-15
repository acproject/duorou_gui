# UI编辑器
实时编辑 UI 样式（颜色、字体、间距等）
主题切换（Light/Dark/High-Contrast 等）
组件级样式覆盖
状态样式验证（hover / active / disabled）
热加载样式文件
可视化调试样式继承关系
“调试器 + Playground + Storybook + Theme Studio”

这个编辑器必须能验证以下系统是否正确：
暂时无法在飞书文档外展示此内容
Style 数据模型
样式解析器（TOML / JSON）
StyleManager（运行时）
依赖与继承系统
组件预览系统
UI 编辑面板
热加载系统
差异检测与刷新系统

主题管理
- 创建主题
- 删除主题
- 复制主题
- 切换主题
- 主题继承（Dark 继承 Light）

样式文件结构（TOML）
- 分段（table）命名规则：
  - `[Theme]`：主题元信息（name/base）
  - `[Global]`：全局默认属性
  - `[Button]`：组件级属性（Component）
  - `[Button.primary]`：组件变体属性（Variant）
  - `[Button.hover]`：组件状态属性（State）
  - `[Button.primary.hover]`：变体状态属性（Variant + State）

最小示例（TOML）
```
[Theme]
name = "Dark"
base = "Light"

[Global]
bg = 0xFF101010
color = 0xFFE0E0E0
font_size = 14

[Button]
bg = 0xFF202020
color = 0xFFEFEFEF
padding = 10
border = 0xFF3A3A3A
border_width = 1

[Button.primary]
bg = 0xFF2D6BFF
color = 0xFFFFFFFF

[Button.primary.hover]
bg = 0xFF3A7BFF

[Text]
color = 0xFFE0E0E0
font_size = 14

[TextField]
bg = 0xFF151515
border = 0xFF2A2A2A
border_width = 1
padding = 10

[TextField.focused]
border = 0xFF3A7BFF
```

样式文件结构（JSON）
- 根对象字段：
  - `theme`：`{ name, base }`
  - `global`：全局属性对象
  - `components`：组件名到样式定义的映射

最小示例（JSON）
```
{
  "theme": { "name": "Dark", "base": "Light" },
  "global": {
    "bg": "0xFF101010",
    "color": "0xFFE0E0E0",
    "font_size": 14
  },
  "components": {
    "Button": {
      "props": {
        "bg": "0xFF202020",
        "color": "0xFFEFEFEF",
        "padding": 10,
        "border": "0xFF3A3A3A",
        "border_width": 1
      },
      "variants": {
        "primary": {
          "props": { "bg": "0xFF2D6BFF", "color": "0xFFFFFFFF" },
          "states": { "hover": { "bg": "0xFF3A7BFF" } }
        }
      }
    },
    "Text": { "props": { "color": "0xFFE0E0E0", "font_size": 14 } },
    "TextField": {
      "props": { "bg": "0xFF151515", "border": "0xFF2A2A2A", "border_width": 1, "padding": 10 },
      "states": { "focused": { "border": "0xFF3A7BFF" } }
    }
  }
}
```
样式层级模型
需要支持以下层级：
Global
  └── Component (Button)
         └── Variant (primary)
                └── State (hover)
查找顺序：
State → Variant → Component → Global

统一的属性 schema：
暂时无法在飞书文档外展示此内容
暂时无法在飞书文档外展示此内容
暂时无法在飞书文档外展示此内容
暂时无法在飞书文档外展示此内容
暂时无法在飞书文档外展示此内容
暂时无法在飞书文档外展示此内容
Style Inspector（核心）
当用户点击一个组件：
- 显示最终计算结果
- 显示每个属性来自哪里
- 显示继承链
- 显示被 override 的值


Button.background
  → from Button.primary.hover
Button.padding
  → from Button
Button.fontSize
  → from Global

实时预览系统
- 所见即所得
- 鼠标 hover 触发状态
- 模拟 disabled / loading
- 响应窗口变化
热加载系统
- 监听样式文件变化
- 自动重新解析
- 计算 diff
- 局部刷新 UI
UI 布局设计
组件树
App
 ├─ Button
 ├─ Text
 ├─ Input
 ├─ Card
 └─ Modal

中间：实时预览
- 多布局
- 多状态
- 可缩放
- 可滚动
样式编辑面板
- 属性名
- 当前值
- 来源
- override 开关
