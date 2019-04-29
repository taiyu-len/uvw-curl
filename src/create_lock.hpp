#ifndef UVW_CURL_CREATE_LOCK_HPP
#define UVW_CURL_CREATE_LOCK_HPP
namespace uvw_curl
{
/**
 * Prevents creation of a class that inherits from it from non-member/friend
 * functions
 */
template<typename T>
struct CreateLock
{
protected:
	struct Key {};
	CreateLock(Key) {};
};
}
#endif // UVW_CURL_CREATE_LOCK_HPP
