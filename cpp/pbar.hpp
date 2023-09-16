#pragma once
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#pragma warning(suppress : 5105)
#include <Windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#include <io.h>
#else
#define DWORD unsigned long
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace pbar {
namespace utils {
#ifdef _WIN32
/// <summary>マルチバイト文字（UTF8 or SJIS）からUTF16に変換する</summary>
/// <param name="enc_src">変換元の文字コードを指定する。UTF8: CP_UTF8, SJIS:
/// CP_THREAD_ACP</param>
inline std::wstring to_utf16(UINT enc_src, const std::string& src) {
	// 変換先の文字列長を求めておいてから変換する (pre-flighting)
	//  length_utf16にはヌル文字分も入る
	int length_utf16 = MultiByteToWideChar(enc_src, 0, src.c_str(), -1, NULL, 0);
	if (length_utf16 <= 0) {
		return L"";
	}
	std::wstring str_utf16(length_utf16, 0);
	MultiByteToWideChar(enc_src, 0, src.c_str(), -1, &str_utf16[0], length_utf16);
	return str_utf16.erase(static_cast<size_t>(length_utf16 - 1), 1);  // ヌル文字削除
}
#endif
// 0との比較をするため浮動小数点型には対応不可
template <typename T, std::enable_if_t<std::is_integral_v<T>, std::nullptr_t> = nullptr>
std::uint64_t get_digit(const T num) {
	// 0の場合、forループで演算すると戻り値が0となるのでここで1を返す
	if (0 == num) return 1;
	std::uint64_t digit = 0;
	for (T i = num; i != 0; i /= 10, digit++)
		;
	return digit;
}
}  // namespace utils

namespace detail {

struct u8cout_ : private std::streambuf, public std::ostream {
	u8cout_() : std::ostream(this) {}
	void flush() {
#ifdef _WIN32
		auto str_utf16 = utils::to_utf16(CP_UTF8, oss.str());
		::WriteConsoleW(::GetStdHandle(STD_OUTPUT_HANDLE), str_utf16.data(),
						static_cast<int>(str_utf16.size()), nullptr, nullptr);
		oss.str("");
		oss.clear();
#else
		std::cout.flush();
#endif
	}

   private:
	int overflow(int c) override {
#ifdef _WIN32
		oss.put(static_cast<char>(c));
		if (c == '\n') {
			flush();
		}
#else
		std::cout.put(static_cast<char>(c));
#endif
		return 0;
	}
	std::ostringstream oss;
};
}  // namespace detail

namespace term {
constexpr auto clear_line = "\x1b[2K";
constexpr auto show_cursor = "\x1b[?25h";
constexpr auto hide_cursor = "\x1b[?25l";

constexpr auto reset = "\x1b[0m";

constexpr auto black = "\x1b[30m";
constexpr auto red = "\x1b[31m";
constexpr auto green = "\x1b[32m";
constexpr auto yellow = "\x1b[33m";
constexpr auto blue = "\x1b[34m";
constexpr auto magenta = "\x1b[35m";
constexpr auto cyan = "\x1b[36m";
constexpr auto white = "\x1b[37m";
constexpr auto reset_fg = "\x1b[39m";

constexpr auto bright_red = "\x1b[91m";
constexpr auto bright_green = "\x1b[92m";
constexpr auto bright_yellow = "\x1b[93m";
constexpr auto bright_blue = "\x1b[94m";
constexpr auto bright_magenta = "\x1b[95m";
constexpr auto bright_cyan = "\x1b[96m";
constexpr auto bright_white = "\x1b[97m";

inline bool equal_stdout_term() {
#ifdef _WIN32
	if (_isatty(_fileno(stdout))) {
#else
	if (isatty(fileno(stdout))) {
#endif
		return true;
	}
	return false;
}

inline bool equal_stderr_term() {
#ifdef _WIN32
	if (_isatty(_fileno(stderr))) {
#else
	if (isatty(fileno(stderr))) {
#endif
		return true;
	}
	return false;
}

inline std::string up(short dist) {
	std::ostringstream oss;
	if (dist < 0) {
		throw std::runtime_error("dist must be non-negative");
	}
	oss << "\x1b[" << dist << "A";
	return oss.str();
}

inline DWORD enable_escape_sequence() {
#ifdef _WIN32
	if (!equal_stdout_term()) {
		return 0;
	}
	auto hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode_orig_;
	if (hOutput == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("GetStdHandle failed.");
	}
	if (!GetConsoleMode(hOutput, &dwMode_orig_)) {
		throw std::runtime_error("GetConsoleMode failed.");
	}
	if (!SetConsoleMode(hOutput, dwMode_orig_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING |
									 DISABLE_NEWLINE_AUTO_RETURN)) {
		throw std::runtime_error("SetConsoleMode failed. cannot set virtual terminal flags.");
	}
	return dwMode_orig_;
#else
	return 0;
#endif
}

#ifdef _WIN32
inline void reset_term_setting(DWORD dwMode_orig_) {
	if (!equal_stdout_term()) {
		return;
	}

	auto hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOutput == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("GetStdHandle failed. cannot reset console mode.");
	}
	if (!SetConsoleMode(hOutput, dwMode_orig_)) {
		throw std::runtime_error("SetConsoleMode failed. cannot reset console mode.");
	}
#else
inline void reset_term_setting(DWORD) {
	return;
#endif
}

inline std::optional<int> get_console_width() {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	bool ret = ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	if (ret) {
		return csbi.dwSize.X;
	}
	return std::nullopt;
#else
	struct winsize w;
	if (!equal_stdout_term()) {
		return std::nullopt;
	}
	if (ioctl(fileno(stdout), TIOCGWINSZ, &w)) {
		return std::nullopt;
	}
	return w.ws_col;
#endif
}
}  // namespace term

class pbar {
   public:
	pbar(std::uint64_t total, const std::string& desc = "")
		: pbar(total, static_cast<std::uint64_t>(term::get_console_width().value_or(1) - 1),
			   desc){};

	pbar(std::uint64_t total, std::uint64_t ncols, const std::string& desc = "")
		: total_(total), ncols_(ncols), desc_(desc) {
		digit_ = utils::get_digit(total);
		if (!enable_stack_) {
			dwMode_orig_ = term::enable_escape_sequence();
			if (term::equal_stdout_term()) u8cout_ << term::hide_cursor;
		}
		if (total_ == 0) throw std::runtime_error("total_ must be greater than zero");
	}

	~pbar() {
		if (enable_stack_) {
			return;
		}
		if (term::equal_stdout_term()) u8cout_ << term::show_cursor;
		try {
			term::reset_term_setting(dwMode_orig_);
		} catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	void tick(std::uint64_t delta = 1) {
		using namespace std::chrono;
		if (term::equal_stdout_term() == 0) {
			return;
		}

		if (!progress_.has_value() && enable_stack_) {
			u8cout_ << std::endl;
		}

		if (!progress_.has_value()) {
			progress_ = 0;
			ncols_ = std::min(static_cast<std::uint64_t>(term::get_console_width().value_or(1) - 1),
							  ncols_);
		}
		std::uint64_t prog = progress_.value();
		prog += delta;
		prog = std::min(prog, total_);
		progress_ = prog;

		if (recalc_cycle_ && (prog % recalc_cycle_.value()) == 0) {
			ncols_ = std::min(static_cast<std::uint64_t>(term::get_console_width().value_or(1) - 1),
							  ncols_);
		}

		nanoseconds dt = 0s;
		seconds remaining = 0s;
		double vel = 0;

		if (enable_time_measurement_) {
			if (!epoch_) {
				epoch_ = steady_clock::now();
			} else {
				dt = steady_clock::now() - *epoch_;
			}
			if (dt.count() > 0) {
				vel = static_cast<double>(prog) / (dt.count() * 1e-9);
				remaining = seconds(static_cast<long long>(std::round((total_ - prog) / (vel))));
			}
		}
		std::int64_t width_non_brackets_base = desc_.size() + 2 * digit_ + 8;
		std::int64_t width_non_brackets_time = 0;
		if (enable_time_measurement_) {
			width_non_brackets_time += utils::get_digit(static_cast<std::int64_t>(vel)) + 23;
			if (auto dt_h = duration_cast<hours>(dt).count(); dt_h > 0) {
				width_non_brackets_time += 1 + utils::get_digit(dt_h);
			}
			if (auto remain_h = duration_cast<hours>(remaining).count(); remain_h > 0) {
				width_non_brackets_time += 1 + utils::get_digit(remain_h);
			}
		}
		std::uint64_t width_non_brackets = width_non_brackets_base + width_non_brackets_time;
		std::uint64_t width_brackets;
		if (ncols_ > width_non_brackets) {
			width_brackets = ncols_ - width_non_brackets;
		} else {
			disable_time_measurement();
			width_brackets = 10;
			ncols_ = width_brackets + width_non_brackets_base;
		}

		double prog_rate = static_cast<double>(prog) / total_;
		std::uint64_t num_brackets =
			static_cast<std::uint64_t>(std::round(prog_rate * width_brackets));

		auto prev = u8cout_.fill(' ');

		u8cout_ << term::clear_line << '\r';
		if (!desc_.empty()) {
			u8cout_ << desc_ << ":";
		}
		u8cout_ << std::setw(3) << static_cast<int>(std::round(prog_rate * 100)) << "%"
				<< opening_bracket_char_;
		for (decltype(num_brackets) _ = 0; _ < num_brackets; _++) {
			u8cout_ << done_char_;
		}
		for (decltype(num_brackets) _ = 0; _ < width_brackets - num_brackets; _++) {
			u8cout_ << todo_char_;
		}
		u8cout_ << closing_bracket_char_ << " " << std::setw(digit_) << prog << "/" << total_;
		if (enable_time_measurement_) {
			u8cout_ << " [" << std::setfill('0');
			if (auto dt_h = duration_cast<hours>(dt).count(); dt_h > 0) {
				u8cout_ << dt_h << ':';
			}
			u8cout_ << std::setw(2) << duration_cast<minutes>(dt).count() % 60 << ':'
					<< std::setw(2) << duration_cast<seconds>(dt).count() % 60 << '<';
			if (auto remain_h = duration_cast<hours>(remaining).count(); remain_h > 0) {
				u8cout_ << remain_h % 60 << ':';
			}
			u8cout_ << std::setw(2) << duration_cast<minutes>(remaining).count() % 60 << ':'
					<< std::setw(2) << remaining.count() % 60 << ", " << std::setw(0) << std::fixed
					<< std::setprecision(2) << vel << "it/s]";
		}
		if (progress_ == total_) {
			if (!leave_) {
				u8cout_ << term::clear_line << '\r';
			} else {
				u8cout_ << "\r" << std::endl;
			}
			if (enable_stack_ && !interrupted_) {
				u8cout_ << term::up(1);
			}
			reset();
		}
		u8cout_ << std::setfill(prev);
		u8cout_.flush();
	}

	// we assume desc_ consists of ascii characters
	void set_description(const std::string& desc) { desc_ = desc; }
	void set_description(std::string&& desc) { desc_ = std::move(desc); }
#if __cplusplus > 201703L  // for C++20
	void set_description(const std::u8string& desc) {
		desc_ = reinterpret_cast<const char*>(desc.data());
	}
	void set_description(std::u8string&& desc) {
		desc_ = reinterpret_cast<const char*>(std::move(desc.data()));
	}
#endif
	void enable_stack() {
		enable_stack_ = true;
		leave_ = false;
	}
	void enable_leave() { leave_ = true; }
	void disable_leave() { leave_ = false; }
	void disable_time_measurement() { enable_time_measurement_ = false; }
	void enable_time_measurement() { enable_time_measurement_ = true; }
	void enable_recalc_console_width(std::uint64_t cycle) {
		if (cycle == 0) {
			throw std::invalid_argument("cycle must be greater than zero");
		}
		recalc_cycle_ = cycle;
	}
	void disable_recalc_console_width() { recalc_cycle_ = std::nullopt; }

	void reset() {
		progress_ = std::nullopt;
		epoch_ = std::nullopt;
		interrupted_ = false;
	}

	void init() { tick(0); }

	template <typename T>
	std::ostream& operator<<(T&& obj) {
		if (term::equal_stdout_term()) {
			u8cout_ << term::clear_line << '\r';
			u8cout_ << std::forward<T>(obj);
			interrupted_ = true;
			return u8cout_;
		} else {
			std::cout << std::forward<T>(obj);
			return std::cout;
		}
	}

	template <class T>
	void warn(T&& msg) {
		static_assert(std::is_constructible_v<std::string, T>,
					  "std::string(T) must be constructible");
		if (term::equal_stderr_term() && term::equal_stdout_term()) {
			std::cerr << term::clear_line << '\r';
			interrupted_ = true;
		}
		std::cerr << std::forward<T>(msg);
	}
	pbar& operator+=(std::uint64_t delta) {
		tick(delta);
		return *this;
	}
	pbar& operator++(void) {
		tick(1);
		return *this;
	}
	pbar& operator++(int) {
		tick(1);
		return *this;
	}

	pbar& operator=(const pbar& other) {
		total_ = other.total_;
		digit_ = other.digit_;
		recalc_cycle_ = other.recalc_cycle_;
		epoch_ = other.epoch_;
		enable_stack_ = other.enable_stack_;
		leave_ = other.leave_;
		enable_time_measurement_ = other.enable_time_measurement_;
		interrupted_ = other.interrupted_;
		return *this;
	}
	pbar& operator=(pbar&& other) noexcept {
		digit_ = std::move(other.digit_);
		total_ = std::move(other.total_);
		recalc_cycle_ = std::move(other.recalc_cycle_);
		epoch_ = std::move(other.epoch_);
		enable_stack_ = std::move(other.enable_stack_);
		leave_ = std::move(other.leave_);
		enable_time_measurement_ = std::move(other.enable_time_measurement_);
		interrupted_ = std::move(other.interrupted_);
		return *this;
	}

   private:
	std::uint64_t total_ = 0;
	std::uint64_t ncols_ = 80;
	std::optional<std::uint64_t> progress_ = std::nullopt;
	// the following member variables with "char_" suffix must consist of one character
#if __cplusplus > 201703L  // for C++20
	inline static const std::string done_char_ = reinterpret_cast<const char*>(u8"█");
#else
	inline constexpr static auto done_char_ = u8"█";
#endif
	inline constexpr static auto todo_char_ = " ";
	inline constexpr static auto opening_bracket_char_ = "|";
	inline constexpr static auto closing_bracket_char_ = "|";
	std::string desc_ = "";
	std::uint64_t digit_;
	std::optional<std::uint64_t> recalc_cycle_ = std::nullopt;
	std::optional<std::chrono::steady_clock::time_point> epoch_ = std::nullopt;
	bool enable_stack_ = false;
	bool leave_ = true;
	bool enable_time_measurement_ = true;
	bool interrupted_ = false;
	detail::u8cout_ u8cout_;
	DWORD dwMode_orig_ = 0;
};

class spinner {
   public:
	spinner(std::string text, std::chrono::milliseconds interval = interval_default)
		: interval_(interval), text_(text), dwMode_orig_(0) {}
	~spinner() {
		if (!thr_renderer_) return;
		stop();
		if (term::equal_stdout_term()) {
			u8cout_ << term::show_cursor;
			u8cout_.flush();
		}
		try {
			term::reset_term_setting(dwMode_orig_);
		} catch (std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	void start() {
		if (thr_renderer_) {
			throw std::runtime_error("spinner is already working");
		}
		active_ = true;
		dwMode_orig_ = term::enable_escape_sequence();
		if (term::equal_stdout_term()) u8cout_ << term::hide_cursor;
		thr_renderer_ = std::thread([&]() {
			size_t c = 0;
			if (!term::equal_stdout_term()) return;
			while (true) {
				{
					std::lock_guard lock(mtx_active_);
					if (!active_) return;
				}
				{
					std::lock_guard lock(mtx_output_);
					u8cout_ << '\r';
#if !defined(_WIN32) && __cplusplus > 201703L  // for C++20
					std::u8string spinner_char = spinner_chars_[c];
					u8cout_ << reinterpret_cast<const char*>(spinner_char.data());
#else
					u8cout_ << spinner_chars_[c];
#endif
					u8cout_ << ' ' << text_;
					u8cout_.flush();
				}
				c = (c + 1) % spinner_chars_.size();
				std::this_thread::sleep_for(interval_);
			}
		});
	}

	void ok() {
		constexpr auto icon = u8"✔";
		constexpr auto msg = "SUCCESS";
		constexpr auto color = term::bright_green;
#if __cplusplus > 201703L  // for C++20
		print_result(reinterpret_cast<const char*>(icon), msg, color);
#else
		print_result(icon, msg, color);
#endif
	}

	void err() {
		constexpr auto icon = u8"✖";
		constexpr auto msg = "FAILURE";
		constexpr auto color = term::bright_red;
#if __cplusplus > 201703L  // for C++20
		print_result(reinterpret_cast<const char*>(icon), msg, color);
#else
		print_result(icon, msg, color);
#endif
	}

	template <typename T>
	std::ostream& operator<<(T&& obj) {
		if (term::equal_stdout_term()) {
			std::lock_guard lock(mtx_output_);
			u8cout_ << term::clear_line << '\r';
			u8cout_ << std::forward<T>(obj);
			return u8cout_;
		} else {
			std::cout << std::forward<T>(obj);
			return std::cout;
		}
	}

	template <class T>
	void warn(T&& msg) {
		static_assert(std::is_constructible_v<std::string, T>,
					  "std::string(T) must be constructible");
		std::lock_guard lock(mtx_output_);
		if (term::equal_stderr_term() && term::equal_stdout_term()) {
			std::cerr << term::clear_line << '\r';
		}
		std::cerr << std::forward<T>(msg);
	}

	spinner& operator=(const spinner& other) {
		if (thr_renderer_ || other.thr_renderer_) {
			throw std::runtime_error("spinner is working");
		}
		interval_ = other.interval_;
		text_ = other.text_;
		dwMode_orig_ = other.dwMode_orig_;
		active_ = other.active_;
		return *this;
	}

	spinner& operator=(spinner&& other) noexcept {
		other.stop();
		interval_ = std::move(other.interval_);
		text_ = std::move(other.text_);
		dwMode_orig_ = std::move(other.dwMode_orig_);
		active_ = std::move(other.active_);
		return *this;
	}

   private:
	bool stop() {
		if (!thr_renderer_) {
			return false;
		}
		{
			std::lock_guard lock(mtx_active_);
			active_ = false;
		}
		thr_renderer_->join();
		thr_renderer_ = std::nullopt;
		return true;
	}

	void print_result(const std::string& icon, const std::string& msg, const std::string& color) {
		if (!stop()) {
			return;
		}
		std::ostringstream oss;
		oss << icon << ' ' << text_ << " [" << msg << "]" << std::endl;

		if (term::equal_stdout_term()) {
			u8cout_ << color;
			u8cout_ << '\r' << oss.str();
			u8cout_ << term::reset;
			u8cout_ << term::show_cursor;
			u8cout_.flush();
		} else {
			std::cout << oss.str();
		}
		term::reset_term_setting(dwMode_orig_);
	}

#ifdef _WIN32
	inline static const std::array<std::string, 4> spinner_chars_ = {{"|", "/", "-", "\\"}};
	constexpr static std::chrono::milliseconds interval_default = std::chrono::milliseconds(130);
#else
#if __cplusplus > 201703L  // for C++20
	inline static const std::array<std::u8string, 10> spinner_chars_ = {
#else
	inline static const std::array<std::string, 10> spinner_chars_ = {
#endif
		{u8"⠋", u8"⠙", u8"⠹", u8"⠸", u8"⠼", u8"⠴", u8"⠦", u8"⠧", u8"⠇", u8"⠏"}
	};
	constexpr static std::chrono::milliseconds interval_default = std::chrono::milliseconds(80);
#endif
	std::chrono::milliseconds interval_;
	std::string text_;
	bool active_ = false;
	std::optional<std::thread> thr_renderer_ = std::nullopt;
	std::mutex mtx_output_;
	std::mutex mtx_active_;
	detail::u8cout_ u8cout_;
	DWORD dwMode_orig_ = 0;
};

}  // namespace pbar
