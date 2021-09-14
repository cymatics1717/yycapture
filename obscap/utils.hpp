#include <stdbool.h>
#include <windows.h>
#include <stdint.h>
#include <util/config-file.h>
#include <util/windows/win-version.h>
#include <dwmapi.h>
#include <util/dstr.h>
#include <string>
#include <map>
using std::map;
using std::string;

#ifdef _MSC_VER
#pragma warning(disable : 4152) /* casting func ptr to void */
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <Psapi.h>

/* this is a workaround to A/Vs going crazy whenever certain functions (such as
 * OpenProcess) are used */
extern void *get_obfuscated_func(HMODULE module, const char *str, uint64_t val);

#ifdef __cplusplus
}
#endif

#define LOWER_HALFBYTE(x) ((x)&0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

static void deobfuscate_str(char *str, uint64_t val);

void *get_obfuscated_func(HMODULE module, const char *str, uint64_t val);

static HMODULE kernel32(void);

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
				  DWORD process_id);

bool get_window_exe(struct dstr *name, HWND window);

static HWND next_window(HWND window, HWND *parent, bool use_findwindowex);

static HWND first_window(HWND *parent, bool *use_findwindowex);

void get_window_class(struct dstr *clazz, HWND hwnd);

void get_window_title(struct dstr *name, HWND hwnd);

static int window_rating(HWND window, const char *clazz, const char *title);

HWND find_window(const char *clazz, const char *title);

map<string, HWND> enumerate_windows_list();

static bool check_window_valid(HWND window);

string getWindowExeNameByHwnd(HWND hwnd);

string getWindowIdByPattern(const char *,
					const char *);

static string getWindowIdByHwnd(HWND hwnd);
uint32_t GetWindowsVersion();

void SetAeroEnabled(bool enable);
