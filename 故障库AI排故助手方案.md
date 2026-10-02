# 故障库 AI 排故助手：最小可行方案

> 本文档只包含通用方法和代码，不含任何单位数据。

## 目标

用单位已有的 Microsoft 365 Copilot（WorkIQ），把约 4 万行的故障库变成可以用自然语言提问的“排故助手”。

- 不买软件
- 不碰老数据库（SQL Server 2008）
- 不用外部 AI

## 总耗时

| 步骤 | 动手时间 |
|---|---|
| 1. 导出 Excel 并运行宏 | 约 15 分钟 |
| 2. 上传到 SharePoint | 约 10 分钟，之后需等待索引（几小时到 1 天） |
| 3. 新建代理 | 约 15 分钟 |
| 4. 测试 | 约 30 分钟 |

合计约 1～1.5 小时，可以分两天完成：第一天上传，第二天建代理并测试。

---

## 第 1 步：把 Excel 转成文本文件

**为什么要转：**Copilot 检索大表格不准，检索文档准。所以要把每一行转成一段带标签的文字。

**操作：**

1. 从数据库导出 Excel，确保第 1 行是表头。
2. （推荐）先按“系统”或“设备”列排序，这样同一个文件里的记录主题相近，检索更准。
3. 把文件保存到**本机文件夹**（如 D 盘或桌面）。不要直接在 OneDrive/SharePoint 里打开运行。
4. 按 `Alt + F11` 打开宏编辑器，选择“插入 → 模块”，粘贴下面的代码。
5. 回到表格，按 `Alt + F8`，运行 `ExportRecords`。
6. Excel 文件同目录下会生成 `kb_export` 文件夹。4 万行大约生成 130 多个文件。

```vba
Option Explicit

' ===== 可修改的设置 =====
Const ROWS_PER_FILE As Long = 300     ' 每个文件包含多少条记录
Const OUT_FORMAT As String = "txt"    ' "txt" 或 "docx"（docx 需要本机装有 Word）
Const FILE_PREFIX As String = "fault" ' 文件名前缀；导出设计库时改成 "design"
' ========================

Sub ExportRecords()
    Dim ws As Worksheet
    Dim data As Variant
    Dim lastRow As Long, lastCol As Long
    Dim r As Long, c As Long, startRow As Long, endRow As Long
    Dim fileIdx As Long
    Dim headers() As String
    Dim rec As String, buf As String, v As String
    Dim basePath As String, outDir As String, outPath As String
    Dim wd As Object

    Set ws = ActiveSheet
    basePath = ws.Parent.Path
    If basePath = "" Or LCase(Left(basePath, 4)) = "http" Then
        MsgBox "请先把 Excel 另存到本机文件夹（如 D 盘），再运行。"
        Exit Sub
    End If

    With ws.UsedRange
        lastRow = .Row + .Rows.Count - 1
        lastCol = .Column + .Columns.Count - 1
    End With
    If lastRow < 2 Then
        MsgBox "没有数据。"
        Exit Sub
    End If

    ' 一次性读入内存，速度快
    data = ws.Range(ws.Cells(1, 1), ws.Cells(lastRow, lastCol)).Value

    ReDim headers(1 To lastCol)
    For c = 1 To lastCol
        headers(c) = CellText(data(1, c))
    Next c

    outDir = basePath & "\kb_export"
    If Dir(outDir, vbDirectory) = "" Then MkDir outDir

    If OUT_FORMAT = "docx" Then
        Set wd = CreateObject("Word.Application")
        wd.Visible = False
    End If

    Application.ScreenUpdating = False
    For startRow = 2 To lastRow Step ROWS_PER_FILE
        endRow = startRow + ROWS_PER_FILE - 1
        If endRow > lastRow Then endRow = lastRow

        buf = ""
        For r = startRow To endRow
            rec = ""
            For c = 1 To lastCol
                v = CellText(data(r, c))
                ' 空单元格、无表头的列都跳过
                If Len(v) > 0 And Len(headers(c)) > 0 Then
                    rec = rec & "【" & headers(c) & "】" & v & vbCrLf
                End If
            Next c
            If Len(rec) > 0 Then
                buf = buf & "===== No." & (r - 1) & " =====" & vbCrLf & rec & vbCrLf
            End If
        Next r

        If Len(buf) > 0 Then
            fileIdx = fileIdx + 1
            outPath = outDir & "\" & FILE_PREFIX & "_" & Format(fileIdx, "000")
            If OUT_FORMAT = "docx" Then
                SaveDocx wd, outPath & ".docx", buf
            Else
                SaveUtf8 outPath & ".txt", buf
            End If
        End If

        Application.StatusBar = "Processing row " & endRow & " / " & lastRow
        DoEvents
    Next startRow

    If Not wd Is Nothing Then wd.Quit
    Application.StatusBar = False
    Application.ScreenUpdating = True
    MsgBox "完成：共生成 " & fileIdx & " 个文件。" & vbCrLf & outDir
End Sub

Private Function CellText(ByVal x As Variant) As String
    If IsError(x) Or IsEmpty(x) Then
        CellText = ""
    Else
        ' 单元格内换行替换为空格，避免记录被拆散
        CellText = Trim(Replace(Replace(CStr(x), vbCr, " "), vbLf, " "))
    End If
End Function

Private Sub SaveUtf8(ByVal path As String, ByVal text As String)
    Dim st As Object
    Set st = CreateObject("ADODB.Stream")
    st.Type = 2            ' 文本模式
    st.Charset = "utf-8"
    st.Open
    st.WriteText text
    st.SaveToFile path, 2  ' 覆盖已有文件
    st.Close
End Sub

Private Sub SaveDocx(wd As Object, ByVal path As String, ByVal text As String)
    Dim doc As Object
    Set doc = wd.Documents.Add
    doc.Content.Text = Replace(text, vbCrLf, vbCr)
    doc.SaveAs2 path, 16   ' 16 = .docx 格式
    doc.Close False
End Sub
```

**生成的文件内容示例（虚构数据）：**

```
===== No.1 =====
【故障编号】EX-0001
【设备】示例设备A
【故障现象】示例：运行中压力波动
【原因】示例：密封件老化
【处理措施】示例：更换密封件
```

**说明：**

- 宏会自动读取表头，不需要改代码就能适配任何列名。
- 默认输出 `txt`。如果后面发现代理检索 txt 效果不好，把 `OUT_FORMAT` 改成 `"docx"` 重新运行即可。
- 文件夹和文件名使用英文，是为了避免不同系统编码导致乱码。
- 如果单位禁用宏，可以问 IT 能否临时启用，或者把这段代码发给单位 Copilot，请它给出替代方法。

---

## 第 2 步：上传到 SharePoint

1. 在团队的 SharePoint 文档库新建一个文件夹，例如“故障知识库”。
2. 在里面建子文件夹：
   - `故障记录`：放第 1 步生成的文件
   - `设计记录`：设计库 Excel 用同一个宏导出（把 `FILE_PREFIX` 改成 `"design"`）
   - `设计文档`：Word 原文件直接放进去，不需要转换
3. **权限：**要用这个助手的同事，必须对这个文件夹有读取权限。
4. **等待索引：**上传后需要几小时到 1 天才能被检索到。刚上传就测试搜不到是正常的。

---

## 第 3 步：新建代理

1. 打开 Copilot 聊天，在左侧找到“代理 / Agents” → “新建代理”。
   - 如果找不到，问 IT 开通权限。
   - 有 Microsoft 365 Copilot 许可证时，建内部代理不额外收费。
2. 填写名称（如“排故助手”）和一句话描述。
3. 在“指令 / Instructions”栏粘贴下面的模板。
4. 在“知识 / Knowledge”里选择 SharePoint，粘贴“故障知识库”文件夹的链接。
5. **关闭“网络搜索”**，只让它从单位自己的资料里回答。
6. 如果可以选模型，选 Opus 或列表中最新的推理模型。
7. 测试满意后点“创建 / 发布”，再分享给同事。

**指令模板：**

```
你是本单位的设备排故助手，只根据知识库中的历史故障记录和设计资料回答。

用户描述故障现象后：
1. 检索最相似的 3–5 条历史故障，列出故障编号、设备、现象、原因、处理措施。
2. 基于这些案例，总结可能原因，并按可能性从高到低给出排查顺序。
3. 如果设计资料中有相关零件或设计要求，一并列出。
4. 每条结论都要标注依据的故障编号或文件名。

如果知识库里没有相似记录，直接说明“未找到相似案例”，不要凭常识编造。
用户描述不清时，先追问设备型号、发生工况、报警信息。
用中文回答，简洁分条。
```

---

## 第 4 步：测试清单

挑 10 条你熟悉的故障，用**口语化**的说法去问（不要照抄原文），逐条记录：

- [ ] 找到原记录了吗？
- [ ] 故障编号对不对？
- [ ] 有没有编造知识库里不存在的内容？

**10 条里命中 7 条以上**就可以推广给同事；低于这个数，按下面的方法调优。

---

## 效果不好时怎么调

| 问题 | 调整方法 |
|---|---|
| 找不准 | 把 `ROWS_PER_FILE` 减小到 100；或先按设备排序再导出 |
| 乱编 | 在指令中加强“只用知识库内容，不确定就说不知道” |
| 术语对不上 | 在指令末尾加同义词表，例如“漏液 = 泄漏 = 渗漏” |
| 想做统计 | 不要问这个代理。把原 Excel 交给 Copilot 的 Analyst 代理，或用 Excel 里的 Copilot |

---

## 可以直接问单位 Copilot 的话

- “这是我的 Excel 表头：……。请检查这段 VBA 宏是否需要修改。”
- “请根据我们的设备类型和常用术语，帮我优化下面这段排故助手指令：……”
- “宏运行报错：……，请告诉我原因和修改方法。”

---

## 后续（可选）

- **故障库与设计库关联：**两边导出时都保留零件号或图号列，代理就能同时检索两边，回答“这个零件的设计要求和历史故障”。
- **数据更新：**每季度重新导出一次，覆盖 SharePoint 里的旧文件即可。
