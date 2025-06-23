#ifndef GET_SYS_LANG_HPP
#define GET_SYS_LANG_HPP

#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>

// 平台特定头文件
#if defined(_WIN32)
#include <windows.h> // Windows API
#elif defined(__APPLE__)
#include <CoreFoundation/CFLocale.h> // macOS本地化API
#elif defined(__linux__) || defined(__unix__) || defined(__ANDROID__)
#include <locale> // Linux/Unix环境变量支持
#endif

/**
 * @brief 获取系统当前语言设置（跨平台实现）
 * @return 返回标准化格式的语言代码（如 "zh-CN"），失败时返回默认值 "en-US"
 * @note 所有平台统一返回格式：语言代码小写-国家代码大写（如 "en-US"）
 */
inline std::string get_sys_lang() noexcept
{
    const std::string DEFAULT_LANGUAGE = "en-US"; // 默认返回值

// ==================== Windows平台实现 ====================
#if defined(_WIN32)
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH] = {0};

    // 获取系统语言设置（宽字符格式）
    if (GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH) == 0)
        return DEFAULT_LANGUAGE;

    // 将宽字符转换为UTF-8
    int size = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return DEFAULT_LANGUAGE;

    std::string result(size, '\0');
    int written = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), size, nullptr, nullptr);
    if (written <= 0)
        return DEFAULT_LANGUAGE;

    result.resize(written - 1); // 移除末尾的null终止符
    return result;              // Windows原生格式 直接返回

// ==================== macOS平台实现 ====================
#elif defined(__APPLE__)
    CFLocaleRef locale = CFLocaleCopyCurrent();
    if (!locale)
        return DEFAULT_LANGUAGE;

    CFStringRef localeID = CFLocaleGetIdentifier(locale);
    CFIndex bufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(localeID), kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(bufferSize);
    std::string result = DEFAULT_LANGUAGE;

    // 转换CFString到UTF-8 C字符串
    if (CFStringGetCString(localeID, buffer.data(), bufferSize, kCFStringEncodingUTF8))
    {
        result = buffer.data();
        // 将 "zh_Hans_CN" 转换为 "zh-CN"
        std::replace(result.begin(), result.end(), '_', '-');
        size_t first_dash = result.find('-');
        if (first_dash != std::string::npos)
        {
            size_t second_dash = result.find('-', first_dash + 1);
            if (second_dash != std::string::npos)
            {
                result.erase(second_dash); // 移除第三个部分（如 "Hans"）
            }
        }
    }

    CFRelease(locale);
    return result;

// ==================== Linux/Unix/Android平台实现 ====================
#elif defined(__linux__) || defined(__unix__) || defined(__ANDROID__)
                                                  // 按优先级检查环境变量：LC_ALL -> LC_MESSAGES -> LANG
    const char *lang = std::getenv("LC_ALL");
    if (!lang || lang[0] == '\0')
        lang = std::getenv("LC_MESSAGES");
    if (!lang || lang[0] == '\0')
        lang = std::getenv("LANG");
    if (!lang || lang[0] == '\0')
        return DEFAULT_LANGUAGE;

    std::string result = lang;

    // 处理特殊值
    if (result == "C" || result == "POSIX")
        return DEFAULT_LANGUAGE;

    // 转换格式：zh_CN -> zh-CN
    std::replace(result.begin(), result.end(), '_', '-');

    // 保证语言代码小写，国家代码大写
    auto dash_pos = result.find('-');
    if (dash_pos != std::string::npos && dash_pos + 1 < result.size())
    {
        std::transform(result.begin(), result.begin() + dash_pos, result.begin(), ::tolower);
        std::transform(result.begin() + dash_pos + 1, result.end(), result.begin() + dash_pos + 1, ::toupper);
    }

    // 移除编码后缀（如".UTF-8"）
    size_t dotPos = result.find('.');
    if (dotPos != std::string::npos)
        result = result.substr(0, dotPos);
    // 移除编码后缀（如"@"）
    size_t atPos = result.find('@');
    if (atPos != std::string::npos)
        result = result.substr(0, atPos);

    return result;

// ==================== 其他平台 ====================
#else
    return DEFAULT_LANGUAGE;
#endif
}

#endif // GET_SYS_LANG_HPP