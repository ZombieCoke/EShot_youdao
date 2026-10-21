# EShot_youdao 接入有道截图翻译 设计规格

日期：2026-10-21
分支：`youdao-integration`
状态：待评审

## 1. 目标

在 EShot_youdao（Qt 6 / C++17 截图工具）中新增「有道在线截图翻译」能力：框选区域后一次请求
拿到 OCR 原文、译文与每行 bbox，支持双语结果列表、分别复制，以及把服务端渲染的译文图贴回
原选区位置。

现状是 Tesseract 本地 OCR 之后打开 Google 翻译网页，用户要跨应用操作，且拿不到原文与译文
的对应关系。

成功标准：

1. 选区到结果只发一次 HTTPS 请求，延迟在 1 秒量级（实测 159 ms 到 900 ms）。
2. 对话框内能看到每条区域的原文与译文，可分别复制、可整体复制。
3. 能一键把译文贴回原选区，位置与尺寸和选区一致，点一下或按 Esc 关闭。
4. 网络失败、密钥失效、无文字三种情况都有明确文案，且不破坏原有 Tesseract 离线 OCR。
5. 新增逻辑有离线单测，`ctest` 全绿。

## 2. 非目标

- 不做本地 bbox 自绘排版（中英字宽差异、折行、背景取样成本高，收益边际）。
- 不改 `project/youdao-dict-screenshot-re/` 目录，该目录只作只读参考。
- 不改动现有 Tesseract 识别流程与语言包下载逻辑。
- 不做批量图片翻译、不做历史记录、不做账号登录态接口（`luna-ai` 那套密钥不接）。

## 3. 接口契约（实现必须逐字对齐）

```
POST https://ocrtran.youdao.com/ocr/imgtranocr
User-Agent: YoudaoDictHelper/11.3.24.0
Content-Type: multipart/form-data
```

multipart 文本字段顺序：文本字段在前，文件字段在后。

| 字段 | 值 |
|---|---|
| clientele | `deskdict` |
| salt | `0.` 加 9 位零填充随机数字串 |
| sign | 见下方签名 |
| from | 源语言代码，`auto` 表示自动检测 |
| to | 目标语言代码 |
| isRender | `false` 取结构化文本，`true` 取服务端渲染译文图 |
| funDesc | `screenshot` |
| multipartFile | 图片二进制，文件名 `capture.png`，MIME 按实际编码取 `image/png` 或 `image/jpeg` |

签名：

```
b64  = Base64(上传的图片字节，标准字母表，保留 padding)
mid  = b64 前 10 个字符 + 十进制字符长度 + b64 后 10 个字符
sign = hex(MD5("deskdict" + mid + salt + secret))
secret = "VPaHE3kX_vl4BhgYiu2n"   （游客态默认值，见 7.2 可配置）
```

注意：签名必须基于**实际上传的那份字节**。如果上传前做了缩放或重编码，签名要在编码之后算。

响应：

```json
{
  "errorCode": "0",
  "resRegions": [
    {
      "boundingBox": "18,53,400,34",
      "verticesBoundingBox": "18,53,418,53,418,87,18,87",
      "lang": "en",
      "context": "Hello World, this is a test",
      "tranContent": "你好世界，这是个测试。"
    }
  ],
  "AIGC": { "Label": "1", "ContentPropagator": "..." }
}
```

`isRender=true` 时返回 `{"image": "<base64 JPEG>"}`，服务端已把译文排版回原图位置。

`boundingBox` 是 `x,y,w,h`，坐标系是**上传图片的像素坐标**，不是屏幕坐标、不是逻辑坐标。

## 4. 现状与插入点

| 位置 | 现状 | 本次改动 |
|---|---|---|
| `src/core/OcrEngine.cpp` | QProcess 调 tesseract，`textReady(QString)` | 不改 |
| `src/core/OcrLanguageSelector.cpp` | 语言标签为 tesseract 标签（`eng`、`chi_sim` 等） | 不改，新增映射层读取它 |
| `src/ui/OcrDialog.cpp` | 语言下拉 + 纯文本区 + 复制 + 打开 Google 翻译网页 + 重试 | 增加引擎下拉、双语结果区、贴回原位动作 |
| `src/capture/CaptureOverlay.cpp:4363` | `OcrDialog dlg(pix); dlg.exec();` | 传入选区屏幕矩形，接收贴回原位请求 |
| `src/capture/PinnedWindow.cpp` | 无边框置顶贴图窗口 | 复用，不改接口 |
| `src/core/TranslationManager.h` | 8 语言 i18n 表 | 追加新文案条目 |
| `CMakeLists.txt` | 已 `find_package(Qt6 ... Network)`，已有 `eshot_add_test` 模式 | 追加源文件与测试目标 |

## 5. 架构

新增两个单元，把「纯协议逻辑」与「网络 IO」分开，前者可离线单测。

### 5.1 `src/core/YoudaoProtocol.h/.cpp`（无 Qt 网络依赖，纯函数）

```cpp
namespace YoudaoProtocol {

struct Region {
    QRect  box;        // 上传图像素坐标
    QString lang;
    QString source;
    QString target;
};

struct Result {
    QVector<Region> regions;
    QImage rendered;   // isRender=true 时有效
    QString rawJson;
    qint64  elapsedMs = 0;
};

QString genSign(const QByteArray &uploadBytes, const QString &salt,
                const QString &secret);
QString randomSalt();
QString mapLanguage(const QString &eshotTag, bool asSource);
bool    parseResponse(const QByteArray &json, Result &out, QString &errorText);
QRect   scaledBox(const QRect &box, double factor);
QByteArray encodeUpload(const QImage &image, double &outScale, QString &outMime);

}
```

`parseResponse` 的错误语义：`errorCode` 缺失或为 `"0"` 视为成功；`10` 无有效文字；
`20` 翻译失败；`30` 签名或鉴权失败；`40` 语言参数不支持；`60` 图片为空或压缩失败；
其他值原样透出。`errorText` 带 code 与 `message`（若存在）。

### 5.2 `src/core/YoudaoTranslate.h/.cpp`（QObject，异步）

```cpp
class YoudaoTranslate : public QObject {
    Q_OBJECT
public:
    explicit YoudaoTranslate(QObject *parent = nullptr);
    void setSecret(const QString &secret);
    void translate(const QImage &image, const QString &from, const QString &to,
                   bool render);
    void cancel();
signals:
    void finished(const YoudaoProtocol::Result &result);
    void failed(const QString &reason, int errorCode);
};
```

实现要点：`QNetworkAccessManager` 成员，`QHttpMultiPart::FormDataType`，`setTransferTimeout(60000)`，
`setRawHeader("User-Agent", "YoudaoDictHelper/11.3.24.0")`；reply 的 `finished` 里先判 HTTP 状态，
再走 `parseResponse`；对象析构时 `abort()` 未完成 reply。

### 5.3 数据流

```
CaptureOverlay 选区
  → QPixmap
  → OcrDialog(pixmap, selectionGlobalRect)
      → 引擎 == 有道：
          encodeUpload()  缩放 / 编码，记录 scale
          genSign(上传字节, salt, secret)
          POST /ocr/imgtranocr
          parseResponse() → regions（box 乘 1/scale 回到原图像素）
          渲染双语卡片
          若 isRender：QImage rendered → emit pinRequested(image)
      → 引擎 == Tesseract：原路径不变
  → CaptureOverlay 收到 pinRequested → PinnedWindow 贴在 selectionGlobalRect
```

## 6. 关键实现细节

### 6.1 语言映射

EShot 语言标签到有道代码的映射，源语言额外支持 `auto`。

| EShot 标签 | 有道代码 | 说明 |
|---|---|---|
| auto | auto | 仅源语言 |
| eng | en | |
| chi_sim | zh-CHS | |
| chi_tra | zh-CHT | |
| jpn | ja | |
| kor | ko | |
| fra | fr | |
| deu | de | |
| spa | es | |
| por | pt | |
| rus | ru | |
| ita | it | |
| ara | ar | |
| tha | th | |
| vie | vi | |
| ind | id | |
| msa | ms | |
| tur | tr | |
| nld | nl | |
| pol | pl | |
| hin | hi | |
| ukr 及其他未列出标签 | en | 回退并提示该语言暂不支持 |

目标语言默认取 `TranslationManager::langCode()` 对应的有道代码，简体中文界面下为 `zh-CHS`。

### 6.2 上传前处理

- 最长边超过 2000 px 时等比缩小到 2000 px，记录 `scale = 上传宽 / 原图宽`。
- 缩小或原图无 alpha 通道时编码 JPEG（质量 85），否则编码 PNG。
- `scaledBox` 把 bbox 乘以 `1 / scale` 并夹紧到原图范围，供绘制与调试。
- 服务端实测可接受 4000×4000，2000 px 是延迟与清晰度的折中，不是硬限制。

### 6.3 界面

`OcrDialog` 顶部行增加「引擎」下拉：`本地 Tesseract` / `有道在线翻译`。选择写入
`QSettings` 的 `ocrEngine`，默认 `tesseract`。

有道模式下结果区从单 `QTextEdit` 换成分组列表，每条：

```
#1  en
原文  Hello World, this is a test        [复制原文]
译文  你好世界，这是个测试。              [复制译文]
```

底部按钮：`复制全部原文`、`复制全部译文`、`贴回原位`（仅 isRender 成功时可用）、`重新翻译`、
`关闭`。切目标语言后「重新翻译」复用上一次选区的 QPixmap，不重新截图。

`贴回原位` 通过信号交给 `CaptureOverlay`，由它把 `QImage` 交给 `PinnedWindow`，位置用选区
的全局矩形。PinnedWindow 已支持点击关闭与 Esc 关闭，不新增行为。

### 6.4 设置项与隐私

| 键 | 默认 | 说明 |
|---|---|---|
| `youdaoEnabled` | false | 在线翻译总开关 |
| `youdaoNoticeAccepted` | false | 是否已确认上传提示 |
| `youdaoSecret` | 空 | 留空时用内置游客密钥 |
| `ocrEngine` | `tesseract` | 上次使用的引擎 |
| `youdaoFromLang` / `youdaoToLang` | `auto` / 界面语言 | 语言选择 |

首次把引擎切到「有道在线翻译」且 `youdaoNoticeAccepted` 为 false 时，弹一次确认框，文案明确
写出：选区图片将上传至 `ocrtran.youdao.com`，该接口非官方公开接口。用户取消则回退到
Tesseract，不写设置。

## 7. 错误处理与降级

| 情况 | 处理 |
|---|---|
| HTTP 非 200 | 报「服务不可用（HTTP xxx）」，附响应体前 300 字节到日志 |
| errorCode 30 | 报「鉴权失败，内置密钥可能已失效」，提示在设置里填新密钥或改用本地 OCR |
| errorCode 10 | 报「未识别到文字」，不视为错误弹窗 |
| errorCode 20 / 40 / 60 | 分别给翻译失败 / 语言不支持 / 图片处理失败文案 |
| 超时 60 s 或断网 | 报网络错误，保留「重新翻译」可重试 |
| 服务端只回图片不回 regions | 结果区显示「服务端仅返回译文图」，贴回原位仍可用 |

任何失败都不自动切换引擎，只给提示，避免用户在不知情的情况下把图传到第三方。

## 8. 测试策略

离线单测（新增 `eshot_youdao_translate_tests`，按现有 `eshot_add_test` 注册）：

1. 签名固定向量：`tests/fixtures/hello.png`（从 `project/youdao-shot-translate/tests/fixtures/hello.png`
   原样复制，字节不能改动）的 b64 长度 5352，salt `0.123456789`，
   期望 `c2fe005b5ca5bad4b6654ce269a58d10`。该向量已在 Rust 实现中实测通过，作为
   `QCryptographicHash::Md5` 与拼接顺序的锚。
2. `randomSalt()` 形状：以 `0.` 开头、总长 11、全数字。
3. 语言映射：21 条映射逐条断言，未列出标签回退 `en`，`auto` 仅源语言可用。
4. `parseResponse`：成功样例、`errorCode` 为数字与字符串两种形态、缺失 `resRegions`、
   `isRender` 图片字段、错误码 10/20/30/40/60 各一条。
5. `scaledBox`：整数倍、非整数倍、越界夹紧、空 bbox 不崩。
6. `encodeUpload`：4000×3000 缩到 2000 宽且 scale 正确；小图不缩放。

构建与运行验证：

- `cmake -S . -B build -G Ninja && cmake --build build` 通过。
- `ctest --test-dir build --output-on-failure` 全绿（含原有测试）。
- `xvfb-run -a ./build/EShot` 启动 5 秒不 panic。
- 真实请求冒烟：用同一张 `hello.png` 走新模块与 `project/youdao-shot-translate` 的
  `shotrans --file` 对照，region 数与文本一致。
- 失败路径：断网、故意错密钥、纯色空白图，各跑一次确认文案正确。

## 9. 分阶段计划

| 阶段 | 内容 | 完成标志 |
|---|---|---|
| P0 环境 | 装 cmake / ninja / qt6-base-dev / libsecret-1-dev / xvfb，编基线 | 基线 ctest 全绿 |
| P1 协议层 | `YoudaoProtocol` 全部纯函数 + 单测 | 新增测试全绿，签名向量匹配 |
| P2 网络层 | `YoudaoTranslate` 异步请求 + 冒烟脚本 | 真实请求 200，解析出 regions |
| P3 对话框 | 引擎下拉、双语卡片、复制、错误文案、8 语言 i18n | 手动走通两种引擎 |
| P4 贴回原位 | isRender + PinnedWindow + 选区全局矩形 | 贴图位置尺寸与选区一致 |
| P5 收尾 | README 段落、THIRD_PARTY_NOTICES 说明、提交拆分 | 分支可评审 |

## 10. 风险

1. 游客密钥是有道客户端里的常量，服务端或客户端更新可能轮换。缓解：密钥可配置、errorCode 30
   有专门文案、Tesseract 始终可用。
2. 无公开配额与 SLA。缓解：串行请求、失败不自动重试、不做批量。
3. 隐私语义变化。缓解：默认关闭、首次开启明确告知、失败不静默改引擎。
4. 公开分发风险。缓解：README 与 THIRD_PARTY_NOTICES 注明非官方接口、可能随时失效。
5. 上游同步成本。缓解：新逻辑放独立文件，`OcrDialog` 改动集中在构造与结果渲染两处，
   `CaptureOverlay` 只加一个信号连接。

## 11. 约束

- 源码与界面文案用中文注释，但界面字符串走 `TranslationManager` 的 8 语言表。
- 不使用分节符号作为分隔符。
- 不改 `project/youdao-dict-screenshot-re/`。
- 不新增第三方依赖，只用 Qt 6 已有模块。

## 12. 实现偏差记录（2026-10-21）

实施过程中相对本规格的调整，均为实测后修正：

1. **网络层新增 `setEndpoint()`**。原规格没有，单测里那个"网络失败"用例直接打真实接口，
   断网才通过、联网就挂。改成可替换端点后指向 `http://127.0.0.1:1/`，失败路径变成确定性，
   单测耗时 5.1 秒降到 0.09 秒，也不再产生真实请求。
2. **`mapLanguage()` 同时接受 ISO 代码**。界面语言 `TranslationManager::langCode()` 给的是
   `ja` / `de` / `zh` 这类 ISO 代码，而原映射表只有 tesseract 标签（`jpn` / `deu` / `chi_sim`），
   直接查表会把日德法西俄土全部回退成 `en`。现在先认 ISO 与 `zh-CHS` / `zh-CHT`，再走标签表。
3. **`Result` 增加 `uploadScale`**。`isRender` 返回的图是上传尺寸，大选区缩到 2000 px 后
   贴回原位会比选区小一圈（实测 3040 宽的选区，渲染图只有 2000 宽）。现在 bbox 在对话框内
   乘 `1/uploadScale` 换算回原图像素，贴图前再把渲染图缩放到选区尺寸。
4. **`scaledBox()` 有了真实调用方**（第 3 条的换算），不再是被测试单独调用的死代码。

验证记录：

| 项 | 结果 |
|---|---|
| `cmake --build build` | 通过，无 error |
| `xvfb-run -a ctest` | 63/63 通过（基线 62 + 新增 1） |
| 签名固定向量 | 通过，与逆向报告一致 |
| 真实接口（420x140 英文图） | HTTP 200，232 ms，1 个区域，渲染图 420x140 |
| 真实接口（3040x1200 放大图） | HTTP 200，572 ms，3 个区域，uploadScale 0.658，bbox 换算回原图正确 |
| `timeout 8 xvfb-run ./build/bin/EShot` | 存活到超时（退出码 124），无崩溃输出 |

