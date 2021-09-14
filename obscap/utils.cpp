#include "utils.hpp"

static void deobfuscate_str(char *str, uint64_t val)
{
	uint8_t *dec_val = (uint8_t *)&val;
	int i = 0;

	while (*str != 0) {
		int pos = i / 2;
		bool bottom = (i % 2) == 0;
		uint8_t *ch = (uint8_t *)str;
		uint8_t xor = bottom ? LOWER_HALFBYTE(dec_val[pos])
				     : UPPER_HALFBYTE(dec_val[pos]);

		*ch ^= xor;

		if (++i == sizeof(uint64_t) * 2)
			i = 0;

		str++;
	}
}

void *get_obfuscated_func(HMODULE module, const char *str, uint64_t val)
{
	char new_name[128];
	strcpy(new_name, str);
	deobfuscate_str(new_name, val);
	return GetProcAddress(module, new_name);
}

static HMODULE kernel32(void)
{
	static HMODULE kernel32_handle = NULL;
	if (!kernel32_handle)
		kernel32_handle = GetModuleHandleA("kernel32");
	return kernel32_handle;
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
				  DWORD process_id)
{
	typedef HANDLE(WINAPI * PFN_OpenProcess)(DWORD, BOOL, DWORD);
	static PFN_OpenProcess open_process_proc = NULL;
	if (!open_process_proc)
		open_process_proc = (PFN_OpenProcess)get_obfuscated_func(
			kernel32(), "B}caZyah`~q", 0x2D5BEBAF6DDULL);

	return open_process_proc(desired_access, inherit_handle, process_id);
}

bool get_window_exe(struct dstr *name, HWND window)
{
	wchar_t wname[MAX_PATH];
	struct dstr temp = {0};
	bool success = false;
	HANDLE process = NULL;
	char *slash;
	DWORD id;

	GetWindowThreadProcessId(window, &id);
	if (id == GetCurrentProcessId())
		return false;

	process = open_process(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
	if (!process)
		goto fail;

	if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
		goto fail;

	dstr_from_wcs(&temp, wname);
	slash = strrchr(temp.array, '\\');
	if (!slash)
		goto fail;

	dstr_copy(name, slash + 1);
	success = true;

fail:
	if (!success)
		dstr_copy(name, "unknown");

	dstr_free(&temp);
	CloseHandle(process);
	return true;
}

static HWND next_window(HWND window, HWND *parent, bool use_findwindowex)
{
	if (*parent) {
		window = *parent;
		*parent = NULL;
	}

	while (true) {
		if (use_findwindowex)
			window = FindWindowEx(GetDesktopWindow(), window, NULL,
					      NULL);
		else
			window = GetNextWindow(window, GW_HWNDNEXT);

		if (!window || check_window_valid(window))
			break;
	}

	return window;
}

static HWND first_window(HWND *parent, bool *use_findwindowex)
{
	HWND window = FindWindowEx(GetDesktopWindow(), NULL, NULL, NULL);

	if (!window) {
		*use_findwindowex = false;
		window = GetWindow(GetDesktopWindow(), GW_CHILD);
	} else {
		*use_findwindowex = true;
	}

	*parent = NULL;
	if (!check_window_valid(window)) {
		window = next_window(window, parent, *use_findwindowex);

		if (!window && *use_findwindowex) {
			*use_findwindowex = false;

			window = GetWindow(GetDesktopWindow(), GW_CHILD);
			if (!check_window_valid(window))
				window = next_window(window, parent,
						     *use_findwindowex);
		}
	}

	return window;
}

void get_window_class(struct dstr *clazz, HWND hwnd)
{
	wchar_t temp[256];

	temp[0] = 0;
	if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
		dstr_from_wcs(clazz, temp);
}

void get_window_title(struct dstr *name, HWND hwnd)
{

	int len = GetWindowTextLengthW(hwnd);
	if (!len)
		return;

	wchar_t *temp = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
	if (GetWindowTextW(hwnd, temp, len + 1))
		dstr_from_wcs(name, temp);

	free(temp);
}

static int window_rating(HWND window, const char *clazz, const char *title)
{
	struct dstr cur_class = {0};
	struct dstr cur_title = {0};
	struct dstr cur_exe = {0};
	int val = 0x7FFFFFFF;
	char *title_val = NULL;

	if (!get_window_exe(&cur_exe, window))
		return 0x7FFFFFFF;

	get_window_title(&cur_title, window);
	get_window_class(&cur_class, window);

	bool class_matches = dstr_cmpi(&cur_class, clazz) == 0;
	if ((&cur_title)->array) {
		title_val = (char *)dstr_find(&cur_title, title);
	}

	/* always match by name if class is generic */
	if (class_matches && title_val) {
		val = class_matches ? 1 : 0x7FFFFFFF;

	} else if (title_val) {
		val = 0;
	}

	dstr_free(&cur_class);
	dstr_free(&cur_title);
	dstr_free(&cur_exe);

	return val;
}

HWND find_window(const char *clazz, const char *title)
{
	HWND parent;
	bool use_findwindowex = false;

	HWND window = first_window(&parent, &use_findwindowex);
	HWND best_window = NULL;
	int best_rating = 0x7FFFFFFF;

	while (window) {
		int rating = window_rating(window, clazz, title);
		if (rating < best_rating) {
			best_rating = rating;
			best_window = window;
			if (rating == 0)
				break;
		}

		window = next_window(window, &parent, use_findwindowex);
	}

	return best_window;
}

map<string, HWND> enumerate_windows_list()
{
	HWND parent;
	bool use_findwindowex = false;
	map<string, HWND> data;
	HWND window = first_window(&parent, &use_findwindowex);
	while (window) {
		string temp = getWindowIdByHwnd(window);
		if (!temp.empty()) {
			data.insert(std::make_pair(temp, window));
		}
		
		window = next_window(window, &parent, use_findwindowex);
	}
	return data;
}

static bool check_window_valid(HWND window)
{
	DWORD styles, ex_styles;
	RECT rect;

	if (!IsWindowVisible(window) || IsIconic(window))
		return false;

	GetClientRect(window, &rect);
	styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		return false;
	if (styles & WS_CHILD)
		return false;
	if (rect.bottom == 0 || rect.right == 0)
		return false;

	return true;
}

string getWindowExeNameByHwnd(HWND hwnd)
{
	struct dstr exe = {0};
	string name = string();
	get_window_exe(&exe, hwnd);
	if (exe.array) {
		name.append(exe.array);
		return name;
	}

	name.append("default_name");
	dstr_free(&exe);
	return name;
}

string getWindowIdByPattern(const char *classPat,
					const char *namePat)
{
	HWND found = find_window(classPat, namePat);
	if (found) {
		return getWindowIdByHwnd(found);
	}
	blog(LOG_ERROR,
	     "Cannot find window with class pattern: %s, name pattern: %s\n",
	     classPat, namePat);

	return string();
}

static string getWindowIdByHwnd(HWND hwnd)
{
	string id = string();
	if (hwnd) {
		struct dstr name = {0};
		struct dstr clazz = {0};
		struct dstr exe = {0};

		get_window_title(&name, hwnd);
		get_window_class(&clazz, hwnd);
		get_window_exe(&exe, hwnd);
		if (name.array && clazz.array && exe.array) {
			id.append(name.array);
			id.append(":");
			id.append(clazz.array);
			id.append(":");
			id.append(exe.array);
		}
		dstr_free(&name);
		dstr_free(&clazz);
		dstr_free(&exe);
	}
	return id;
}

uint32_t GetWindowsVersion()
{
	static uint32_t ver = 0;

	if (ver == 0) {
		struct win_version_info ver_info;

		get_win_ver(&ver_info);
		ver = (ver_info.major << 8) | ver_info.minor;
	}

	return ver;
}

void SetAeroEnabled(bool enable)
{
	static HRESULT(WINAPI * func)(UINT) = nullptr;
	static bool failed = false;

	if (!func) {
		if (failed) {
			return;
		}

		HMODULE dwm = LoadLibraryW(L"dwmapi");
		if (!dwm) {
			failed = true;
			return;
		}

		func = reinterpret_cast<decltype(func)>(
			GetProcAddress(dwm, "DwmEnableComposition"));
		if (!func) {
			failed = true;
			return;
		}
	}

	func(enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);
}
