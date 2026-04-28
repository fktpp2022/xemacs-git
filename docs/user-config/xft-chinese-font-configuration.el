;; ============================================================================
;; XEmacs 完整的 Xft 字体配置（含中文字体支持）
;; ============================================================================
;; 使用现代化的 Fontconfig 风格字体名称，完全兼容 Xft 后端
;; 格式："字体族-字号:属性=值:属性=值..."
;; 例如："DejaVu Sans Mono-18:bold:italic"
;;
;; 此配置添加了中文字体支持，使用 XEmacs 的 specifier tag 机制
;; 为二维字符集（如中文、日文、韩文等）设置特定字体
;; ============================================================================

;; ============================================================================
;; 字体配置参数 - 可以根据需要修改这些值
;; ============================================================================

;; 默认等宽字体族（用于代码编辑）
;; 推荐选项：
;; - "DejaVu Sans Mono" - 开源、跨平台、字符集完整（推荐）
;; - "Fira Code" - 现代编程字体，支持连字
;; - "Source Code Pro" - Adobe开源编程字体
;; - "Consolas" - Windows字体，需单独安装
;; - "Monaco" - macOS字体，需单独安装
(defvar my-monospace-family "DejaVu Sans Mono"
  "默认等宽字体族")

;; 比例字体族（用于文档、帮助等）
;; 推荐选项：
;; - "DejaVu Sans" - 开源、跨平台
;; - "Fira Sans" - 现代无衬线字体
;; - "Source Sans Pro" - Adobe开源字体
;; - "Arial" - Windows/macOS常用
;; - "Helvetica" - 经典无衬线字体
(defvar my-proportional-family "DejaVu Sans"
  "默认比例字体族")

;; 衬线字体族（可选，用于文档）
(defvar my-serif-family "DejaVu Serif"
  "默认衬线字体族")

;; 默认字号（磅，point）
(defvar my-default-size 18
  "默认字体大小（磅）")

;; 模式行字号（通常比默认稍小）
(defvar my-modeline-size 16
  "模式行字体大小（磅）")

;; 标题字号（比默认稍大）
(defvar my-heading-size 20
  "标题字体大小（磅）")

;; ============================================================================
;; 中文字体配置参数
;; ============================================================================
;; 可用的中文字体（根据系统安装情况选择）
;; 推荐的中文字体：
;; - "Noto Sans SC" - Google 开源字体，现代、美观、字符集完整（推荐）
;; - "Noto Serif SC" - Google 开源衬线字体
;; - "WenQuanYi Zen Hei" - 文泉驿正黑，开源中文字体
;; - "WenQuanYi Zen Hei Mono" - 文泉驿等宽正黑，适合编程
;; - "Microsoft YaHei" - 微软雅黑
;; - "SimHei" - 黑体
;; - "SimSun" - 宋体

(defvar my-chinese-sans-family "Noto Sans SC"
  "中文无衬线字体族（用于界面、文档等）")

(defvar my-chinese-serif-family "Noto Serif SC"
  "中文衬线字体族（用于文档等）")

(defvar my-chinese-monospace-family "WenQuanYi Zen Hei Mono"
  "中文等宽字体族（用于代码编辑）")

;; ============================================================================
;; 辅助函数：构建字体名称
;; ============================================================================

(defun my-build-font-name (family size &optional style)
  "构建 Fontconfig 风格的字体名称
   FAMILY: 字体族名称
   SIZE: 字号（磅）
   STYLE: 样式，可选 'bold 'italic 'bold-italic"
  (let ((base-name (format "%s-%d" family size)))
    (cond
     ((eq style 'bold)
      (format "%s:bold" base-name))
     ((eq style 'italic)
      (format "%s:italic" base-name))
     ((eq style 'bold-italic)
      (format "%s:bold:italic" base-name))
     (t base-name))))

;; ============================================================================
;; 安全设置字体的辅助函数
;; ============================================================================

(defun my-set-face-font-safe (face font-spec &optional locale tag-set)
  "安全设置 face 的字体，如果 face 不存在则忽略
   FACE: face 符号
   FONT-SPEC: 字体规格字符串
   LOCALE: 可选，locale 参数
   TAG-SET: 可选，specifier tag 集合"
  (when (and (facep face) font-spec)
    (if tag-set
        (set-face-font face font-spec locale tag-set)
      (set-face-font face font-spec))))

;; ============================================================================
;; 辅助函数：检测并选择可用的中文字体
;; ============================================================================

(defun my-find-available-chinese-font (&optional preferred-list)
  "检测系统中可用的中文字体并返回第一个可用的字体
   PREFERRED-LIST: 优先尝试的字体列表，默认为标准列表"
  (when (featurep 'xft-fonts)
    (let ((available-fonts nil)
          (preferred (or preferred-list
                         '(;; 等宽字体（适合代码）
                           "WenQuanYi Zen Hei Mono"
                           "Noto Sans Mono CJK SC"
                           ;; 无衬线字体
                           "Noto Sans SC"
                           "Microsoft YaHei"
                           "SimHei"
                           "WenQuanYi Zen Hei"
                           "Droid Sans Fallback"
                           ;; 衬线字体
                           "Noto Serif SC"
                           "SimSun"))))
      
      ;; 尝试获取可用字体列表，添加错误处理
      (condition-case err
          (progn
            ;; 首先尝试使用 xft-font-families-for-device
            (if (fboundp 'xft-font-families-for-device)
                (setq available-fonts (xft-font-families-for-device))
              ;; 如果 xft-font-families-for-device 不可用，尝试 fc-find-available-font-families
              (if (fboundp 'fc-find-available-font-families)
                  (setq available-fonts (fc-find-available-font-families))
                ;; 如果都不可用，使用空列表
                (setq available-fonts nil))))
        (error
         (message "获取字体列表时出错: %s" err)
         (setq available-fonts nil)))
      
      ;; 如果成功获取了可用字体列表，检查首选字体
      (when available-fonts
        (catch 'found
          (dolist (font preferred)
            (when (member font available-fonts)
              (throw 'found font)))
          nil)))))

(defun my-get-chinese-font (preferred-family fallback)
  "获取中文字体，如果首选字体不可用则返回备用字体
   PREFERRED-FAMILY: 首选字体族
   FALLBACK: 备用字体族"
  (let ((detected (my-find-available-chinese-font (list preferred-family))))
    (if detected
        detected
      fallback)))

;; ============================================================================
;; 主要字体配置
;; ============================================================================

(when (featurep 'xft-fonts)
  ;; --------------------------------------------------------------------------
  ;; 1. 默认字体（等宽，用于代码编辑）
  ;; --------------------------------------------------------------------------
  ;; 这些是内置的核心 face，通常一定存在
  
  ;; 常规 - 这是最重要的设置
  (set-face-font 'default
                 (my-build-font-name my-monospace-family my-default-size))
  
  ;; 粗体 - 从default派生，但也可以显式设置
  (set-face-font 'bold
                 (my-build-font-name my-monospace-family my-default-size 'bold))
  
  ;; 斜体
  (set-face-font 'italic
                 (my-build-font-name my-monospace-family my-default-size 'italic))
  
  ;; 粗斜体
  (set-face-font 'bold-italic
                 (my-build-font-name my-monospace-family my-default-size 'bold-italic))
  
  ;; --------------------------------------------------------------------------
  ;; 2. 模式行
  ;; --------------------------------------------------------------------------
  
  ;; 模式行 - 使用比例字体，稍小的字号
  (my-set-face-font-safe 'modeline
                         (my-build-font-name my-proportional-family my-modeline-size))
  
  ;; --------------------------------------------------------------------------
  ;; 3. 其他可能存在的 face（安全设置）
  ;; --------------------------------------------------------------------------
  
  ;; 比例字体（用于文档、帮助等）- 可能不存在
  (my-set-face-font-safe 'variable-pitch
                         (my-build-font-name my-proportional-family my-default-size))
  
  ;; 模式行高亮（当前激活的buffer）- 可能不存在
  (my-set-face-font-safe 'modeline-buffer-id
                         (my-build-font-name my-proportional-family my-modeline-size 'bold))
  
  ;; 一级标题（LaTeX）
  (my-set-face-font-safe 'font-latex-sectioning-5-face
                         (my-build-font-name my-proportional-family my-heading-size 'bold))
  
  ;; 二级标题（LaTeX）
  (my-set-face-font-safe 'font-latex-sectioning-4-face
                         (my-build-font-name my-proportional-family (+ my-default-size 2) 'bold))
  
  ;; 注释 - 通常使用斜体
  (my-set-face-font-safe 'font-lock-comment-face
                         (my-build-font-name my-monospace-family my-default-size 'italic))
  
  ;; 关键字 - 通常使用粗体
  (my-set-face-font-safe 'font-lock-keyword-face
                         (my-build-font-name my-monospace-family my-default-size 'bold))
  
  ;; 字符串
  (my-set-face-font-safe 'font-lock-string-face
                         (my-build-font-name my-monospace-family my-default-size))
  
  ;; 函数名 - 可以使用粗体
  (my-set-face-font-safe 'font-lock-function-name-face
                         (my-build-font-name my-monospace-family my-default-size 'bold))
  
  ;; 类型名 - 可以使用粗体
  (my-set-face-font-safe 'font-lock-type-face
                         (my-build-font-name my-monospace-family my-default-size 'bold))
  
  ;; 高亮选中区域
  (my-set-face-font-safe 'region
                         (my-build-font-name my-monospace-family my-default-size))
  
  ;; isearch 高亮
  (my-set-face-font-safe 'isearch
                         (my-build-font-name my-monospace-family my-default-size 'bold))
  
  ;; minibuffer 提示
  (my-set-face-font-safe 'minibuffer-prompt
                         (my-build-font-name my-proportional-family my-default-size 'bold))
  
  ;; 工具提示
  (my-set-face-font-safe 'tooltip
                         (my-build-font-name my-proportional-family (- my-default-size 2)))
  
  )

;; ============================================================================
;; 中文字体配置（核心部分）
;; ============================================================================
;; 使用 XEmacs 的 specifier tag 机制为中文字符集设置字体
;;
;; 背景知识：
;; - XEmacs 使用 "specifier tags" 来匹配不同的字符集
;; - 已经预定义的 tags（在 faces.c 中）：
;;   * two-dimensional: 二维字符集（如中文、日文、韩文等）
;;   * one-dimensional: 一维字符集（如 ASCII、Latin 等）
;;   * initial: 字体匹配的初始阶段
;;   * final: 字体匹配的最终阶段
;; - 这些 tags 可以组合使用，例如：
;;   * (two-dimensional initial): 二维字符集的初始阶段
;;   * (two-dimensional final): 二维字符集的最终阶段
;;
;; 中文字符集包括：
;; - chinese-gb2312: 简体中文 GB2312
;; - chinese-big5-1, chinese-big5-2: 繁体中文 Big5
;; - chinese-cns11643-1 到 chinese-cns11643-7: CNS 标准
;;
;; 已经定义的中文字符集标签（在 mule/chinese.el 中）：
;; - chinese-gb/list: (chinese-gb2312 chinese-sisheng)
;; - chinese-big5/list: (chinese-big5-1 chinese-big5-2)
;; - chinese-cns/list: 所有 CNS 字符集
;; ============================================================================

(defun my-setup-chinese-fonts ()
  "为中文字符集配置字体
   使用 XEmacs 的 specifier tag 机制为二维字符集（如中文）设置字体"
  (interactive)
  (when (featurep 'xft-fonts)
    (let* (;; 首先尝试自动检测可用的中文字体
           ;; 如果无法检测，使用用户指定的默认字体
           (detected-sans (my-find-available-chinese-font
                           (list my-chinese-sans-family
                                 "Noto Sans SC"
                                 "Microsoft YaHei"
                                 "WenQuanYi Zen Hei")))
           (detected-mono (my-find-available-chinese-font
                           (list my-chinese-monospace-family
                                 "WenQuanYi Zen Hei Mono"
                                 "Noto Sans Mono CJK SC")))
           (detected-serif (my-find-available-chinese-font
                            (list my-chinese-serif-family
                                  "Noto Serif SC"
                                  "SimSun")))
           
           ;; 确定最终使用的字体
           ;; 如果检测失败，使用用户指定的默认值
           (chinese-sans (or detected-sans my-chinese-sans-family "Sans"))
           (chinese-mono (or detected-mono my-chinese-monospace-family chinese-sans))
           (chinese-serif (or detected-serif my-chinese-serif-family chinese-sans)))
      
      (message "配置中文字体: 无衬线=%s, 等宽=%s, 衬线=%s"
               chinese-sans chinese-mono chinese-serif)
      
      ;; ------------------------------------------------------------------------
      ;; 核心配置：为二维字符集设置字体
      ;; ------------------------------------------------------------------------
      ;; 使用 'final' 阶段的原因：
      ;; 1. initial 阶段：XEmacs 尝试精确匹配字体规格和字符集
      ;; 2. final 阶段：如果 initial 阶段失败，使用这里设置的字体
      ;;
      ;; 对于中文字体，我们通常希望：
      ;; - 英文使用 DejaVu Sans Mono 等宽字体
      ;; - 中文使用专门的中文字体
      ;;
      ;; 通过在 final 阶段为二维字符集设置中文字体，我们实现了：
      ;; - 英文（一维字符集）使用默认字体
      ;; - 中文（二维字符集）使用中文字体
      
      ;; 添加错误处理，确保即使字体设置失败也不会影响其他配置
      (condition-case err
          (progn
            ;; 为 default face 设置中文字体（用于代码编辑）
            ;; default face 是最重要的，用于普通文本和代码
            (set-face-font 'default
                           (my-build-font-name chinese-mono my-default-size)
                           'global
                           '(two-dimensional final))
            
            ;; 为 bold face 设置中文字体
            (set-face-font 'bold
                           (my-build-font-name chinese-mono my-default-size 'bold)
                           'global
                           '(two-dimensional final))
            
            ;; 为 italic face 设置中文字体
            (set-face-font 'italic
                           (my-build-font-name chinese-mono my-default-size 'italic)
                           'global
                           '(two-dimensional final))
            
            ;; 为 bold-italic face 设置中文字体
            (set-face-font 'bold-italic
                           (my-build-font-name chinese-mono my-default-size 'bold-italic)
                           'global
                           '(two-dimensional final))
            
            ;; ------------------------------------------------------------------------
            ;; 为其他 face 设置中文字体
            ;; ------------------------------------------------------------------------
            
            ;; 为 variable-pitch face 设置中文字体（用于文档、帮助等）
            (my-set-face-font-safe 'variable-pitch
                                   (my-build-font-name chinese-sans my-default-size)
                                   'global
                                   '(two-dimensional final))
            
            ;; 为 mode-line 设置中文字体
            (my-set-face-font-safe 'modeline
                                   (my-build-font-name chinese-sans my-modeline-size)
                                   'global
                                   '(two-dimensional final))
            
            ;; 为 mode-line-buffer-id 设置中文字体
            (my-set-face-font-safe 'modeline-buffer-id
                                   (my-build-font-name chinese-sans my-modeline-size 'bold)
                                   'global
                                   '(two-dimensional final))
            
            ;; 为 font-lock 相关 face 设置中文字体
            ;; 注意：这些 face 通常从 default 继承，但显式设置更安全
            
            (my-set-face-font-safe 'font-lock-comment-face
                                   (my-build-font-name chinese-mono my-default-size 'italic)
                                   'global
                                   '(two-dimensional final))
            
            (my-set-face-font-safe 'font-lock-keyword-face
                                   (my-build-font-name chinese-mono my-default-size 'bold)
                                   'global
                                   '(two-dimensional final))
            
            (my-set-face-font-safe 'font-lock-string-face
                                   (my-build-font-name chinese-mono my-default-size)
                                   'global
                                   '(two-dimensional final))
            
            (my-set-face-font-safe 'font-lock-function-name-face
                                   (my-build-font-name chinese-mono my-default-size 'bold)
                                   'global
                                   '(two-dimensional final))
            
            (my-set-face-font-safe 'font-lock-type-face
                                   (my-build-font-name chinese-mono my-default-size 'bold)
                                   'global
                                   '(two-dimensional final))
            
            ;; 为其他常用 face 设置中文字体
            (my-set-face-font-safe 'minibuffer-prompt
                                   (my-build-font-name chinese-sans my-default-size 'bold)
                                   'global
                                   '(two-dimensional final))
            
            (my-set-face-font-safe 'tooltip
                                   (my-build-font-name chinese-sans (- my-default-size 2))
                                   'global
                                   '(two-dimensional final))
            
            (message "中文字体配置成功"))
        (error
         (message "配置中文字体时出错: %s" err)
         ;; 即使出错也继续，确保 XEmacs 能够正常启动
         )))))

;; 立即执行中文字体配置
;; 添加错误处理，确保即使配置失败也不会影响 XEmacs 启动
(condition-case err
    (my-setup-chinese-fonts)
  (error
   (message "初始化中文字体配置时出错: %s" err)
   ;; 记录错误但继续启动
   ))

;; ============================================================================
;; 字体验证和调试函数
;; ============================================================================

(defun my-show-current-fonts ()
  "显示当前使用的字体信息，包括中文字体配置"
  (interactive)
  (with-output-to-temp-buffer "*Font Info*"
    (princ "当前字体配置:\n")
    (princ "================\n\n")
    
    (princ "英文字体配置:\n")
    (princ "----------------\n")
    
    ;; 只显示存在且有字体设置的 face
    (dolist (face '(default bold italic bold-italic variable-pitch modeline))
      (when (facep face)
        (let ((font (face-font-name face)))
          (princ (format "%-20s: %s\n" 
                         face 
                         (or font "(未设置)"))))))
    
    (princ "\n中文字体配置:\n")
    (princ "----------------\n")
    (princ "注意：中文字体通过 specifier tags 为二维字符集设置\n")
    (princ "使用 (two-dimensional final) tag 组合\n\n")
    
    ;; 显示检测到的可用中文字体
    (when (featurep 'xft-fonts)
      (let ((chinese-font (my-find-available-chinese-font)))
        (princ (format "检测到的中文字体: %s\n" 
                       (or chinese-font "(未找到)")))))
    
    (princ "\n系统中所有可用的 face:\n")
    (princ "========================\n")
    (dolist (face (sort (face-list) #'string<))
      (let ((font (and (facep face) (face-font-name face))))
        (when font
          (princ (format "%-30s: %s\n" face font)))))
    
    (princ "\n可用字体族:\n")
    (princ "==============\n")
    (when (featurep 'xft-fonts)
      (condition-case nil
          (let ((all-fonts (xft-font-families-for-device))
                (chinese-fonts '()))
            ;; 过滤出包含常见中文字体名称的字体
            (dolist (font all-fonts)
              (when (or (string-match "Noto.*SC" font)
                        (string-match "Noto.*CJK" font)
                        (string-match "WenQuanYi" font)
                        (string-match "Microsoft YaHei" font)
                        (string-match "SimHei" font)
                        (string-match "SimSun" font)
                        (string-match "KaiTi" font)
                        (string-match "FangSong" font)
                        (string-match "Droid Sans Fallback" font))
                (push font chinese-fonts)))
            (if chinese-fonts
                (progn
                  (princ "中文字体:\n")
                  (dolist (font (sort chinese-fonts #'string<))
                    (princ (format "  %s\n" font))))
              (princ "  (未检测到中文字体)\n"))
            
            ;; 询问是否显示所有字体
            (princ "\n是否显示所有可用字体？(y/n) ")
            ;; 这里只是示例，实际交互需要用户输入
            )
        (error (princ "  (无法获取字体族列表)"))))))

;; 绑定快捷键以便查看字体信息
;; 使用 XEmacs 兼容的向量形式
(global-set-key [f12] 'my-show-current-fonts)

;; ============================================================================
;; 字体快速切换函数（可选）
;; ============================================================================

(defvar my-font-size-list '(12 14 16 18 20 22 24)
  "可用的字号列表")

(defvar my-current-font-size-index 3
  "当前字号在列表中的索引（默认指向18）")

(defun my-cycle-font-size ()
  "循环切换字号，同时更新中文字体"
  (interactive)
  (setq my-current-font-size-index
        (mod (1+ my-current-font-size-index) (length my-font-size-list)))
  (let ((new-size (nth my-current-font-size-index my-font-size-list)))
    (setq my-default-size new-size)
    ;; 更新默认英文字体
    (set-face-font 'default
                   (my-build-font-name my-monospace-family new-size))
    ;; 重新配置中文字体以使用新的字号
    (my-setup-chinese-fonts)
    (message "字体大小已切换到: %d 磅" new-size)))

;; 绑定快捷键 - 使用 XEmacs 兼容的向量形式
;; C-+ = Control + Plus
(global-set-key [(control ?+)] 'my-cycle-font-size)
;; C-= = Control + Equal（作为 C-+ 的替代）
(global-set-key [(control ?=)] 'my-cycle-font-size)

;; ============================================================================
;; 字体族快速切换函数
;; ============================================================================

(defvar my-font-family-list
  '("DejaVu Sans Mono"
    "Fira Code"
    "Source Code Pro"
    "Consolas"
    "Monospace")
  "可用的等宽字体族列表")

(defvar my-current-family-index 0
  "当前字体族在列表中的索引")

(defun my-cycle-font-family ()
  "循环切换字体族，同时更新中文字体"
  (interactive)
  (setq my-current-family-index
        (mod (1+ my-current-family-index) (length my-font-family-list)))
  (let ((new-family (nth my-current-family-index my-font-family-list)))
    (setq my-monospace-family new-family)
    ;; 更新默认英文字体
    (set-face-font 'default
                   (my-build-font-name new-family my-default-size))
    ;; 重新配置中文字体
    (my-setup-chinese-fonts)
    (message "字体族已切换到: %s" new-family)))

;; 绑定快捷键 - 使用 XEmacs 兼容的向量形式
;; C-c f = Control-c 然后 f
(global-set-key [(control ?c) ?f] 'my-cycle-font-family)

;; ============================================================================
;; 配置说明
;; ============================================================================

;; 此配置的特点：
;; 1. 使用 Fontconfig 原生字体名称格式，Xft 后端直接支持
;;    格式："字体族-字号:属性=值:属性=值..."
;;    例如："DejaVu Sans Mono-18:bold:italic:antialias=true"
;;
;; 2. 安全设置：所有可选的 face 都使用 my-set-face-font-safe 函数，
;;    只有当 face 存在时才会设置字体
;;
;; 3. XEmacs 兼容的快捷键绑定：
;;    - 使用向量形式 [f12] 而不是 (kbd "F12")
;;    - 使用 [(control ?+)] 而不是 (kbd "C-+")
;;    - 使用 [(control ?c) ?f] 而不是 (kbd "C-c f")
;;
;; 4. 易于维护和修改，只需调整顶部的配置参数
;;
;; 5. 中文字体支持：
;;    - 使用 XEmacs 的 specifier tag 机制
;;    - 为二维字符集（中文、日文、韩文等）设置专门的字体
;;    - 自动检测系统中可用的中文字体
;;    - 英文和中文使用不同的字体（英文等宽，中文用专门中文字体）
;;
;; 6. 提供便捷的调试和切换功能：
;;    - F12: 查看当前字体信息和系统可用字体族
;;    - C-+ 或 C-=: 循环切换字号（同时更新中文字体）
;;    - C-c f: 循环切换字体族（同时更新中文字体）
;;
;; 使用方法：
;; 1. 根据需要修改顶部的配置参数（字体族、字号等）
;; 2. 重启 XEmacs 或使用 M-x eval-buffer 加载此文件
;; 3. 按 F12 查看当前字体信息
;;
;; 常见的 Fontconfig 属性：
;; - :bold          - 粗体
;; - :italic        - 斜体
;; - :oblique       - 倾斜（类似斜体但字形不同）
;; - :antialias=true/false  - 是否抗锯齿
;; - :hinting=true/false    - 是否启用hinting
;; - :rgba=rgb/vrgb/bgr/vbgr/m - 亚像素渲染顺序
;;
;; 常见字体族名称：
;; - 等宽："DejaVu Sans Mono", "Fira Code", "Source Code Pro", "Consolas", "Monaco"
;; - 无衬线："DejaVu Sans", "Fira Sans", "Source Sans Pro", "Arial", "Helvetica"
;; - 衬线："DejaVu Serif", "Times New Roman", "Georgia"
;;
;; 常见中文字体族名称：
;; - 等宽："WenQuanYi Zen Hei Mono", "Noto Sans Mono CJK SC"
;; - 无衬线："Noto Sans SC", "Microsoft YaHei", "SimHei", "WenQuanYi Zen Hei"
;; - 衬线："Noto Serif SC", "SimSun"
;;
;; 中文字体配置原理：
;; - XEmacs 使用 "specifier tags" 来匹配不同的字符集
;; - 一维字符集（如英文、数字）使用默认字体
;; - 二维字符集（如中文、日文、韩文）使用专门设置的字体
;; - 通过 (set-face-font face font-spec 'global '(two-dimensional final))
;;   为二维字符集设置字体
;; - 'final' 阶段意味着：如果初始字体匹配失败，使用这里设置的字体
