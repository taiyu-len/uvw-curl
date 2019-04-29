#ifndef UVW_CURL_CREATE_LOCK_HPP
#define UVW_CURL_CREATE_LOCK_HPP
#include <memory>
#include <type_traits>
namespace uvw_curl
{
namespace detail
{
template<typename T>
using init_t = decltype(std::declval<T>().init(), void());

template<class T, class = void>
struct has_init : std::false_type{};
template<class T>
struct has_init<T, init_t<T>> : std::true_type {};
} // namespace detail

/**
 * Requires classes that inherit from this to be constructed via create or
 * create_unique
 */
template<typename T>
struct CreateLock
{
protected:
	struct Key {};
public:
	template<typename... Args>
	static auto create(Args&&... args) -> std::shared_ptr<T>
	{
		auto ptr = std::make_shared<T>(Key{}, std::forward<Args>(args)...);
		return ptr && try_init(*ptr, detail::has_init<T>{})
			? ptr : nullptr;
	}

	template<typename... Args>
	static auto create_unique(Args&&... args) -> std::unique_ptr<T>
	{
		auto ptr = std::make_unique<T>(Key{}, std::forward<Args>(args)...);
		return ptr && try_init(*ptr, detail::has_init<T>{})
			? ptr : nullptr;
	}

	template<typename... Args>
	static auto create_raw(Args&&... args) -> T*
	{
		auto ptr = new T(Key{}, std::forward<Args>(args)...);
		return ptr && try_init(*ptr, detail::has_init<T>{})
			? ptr : nullptr;
	}

private:
	static bool try_init(T &x, std::true_type)  noexcept { return x.init(); }
	static bool try_init(T &x, std::false_type) noexcept { return true; }
};
} // namespace uvw_curl
#endif // UVW_CURL_CREATE_LOCK_HPP
