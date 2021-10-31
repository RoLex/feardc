// Make sure some of the clever SFINAE used by DC++ is working as expected on the current compiler.

#include "testbase.h"

#include <dcpp/Util.h>

using namespace dcpp;

struct Base {
	void fbase1() { }
	virtual void virt1() { }
	virtual void virt2() { }
	virtual void pure_virt() = 0;
};

struct Funcs : Base {
	void f1() { }
	void f2(int) { }
	int f3() { return 0; }
	void f4(const string&) { }
	void f5(string&) { }
	void virt1() { }
	void pure_virt() { }
};

template<typename T>
struct X {
	// these should be true.
	HAS_FUNC(F1, void, f1());
	HAS_FUNC(F2, void, f2(0));
	HAS_FUNC(F3, int, f3());
	HAS_FUNC(F4, void, f4(string()));
	HAS_FUNC(F5, void, f5(std::function<string&()>()()));
	HAS_FUNC(F6, void, fbase1());
	HAS_FUNC(F7, void, virt1());
	HAS_FUNC(F8, void, virt2());
	HAS_FUNC(F9, void, pure_virt());

	// these should be false.
	HAS_FUNC(FN1, void, lol());
	HAS_FUNC(FN2, bool, f1());
	HAS_FUNC(FN3, bool, f3());
	HAS_FUNC(FN4, int, virt1());

	bool test_F1() { return F1<T>::value; }
	bool test_F2() { return F2<T>::value; }
	bool test_F3() { return F3<T>::value; }
	bool test_F4() { return F4<T>::value; }
	bool test_F5() { return F5<T>::value; }
	bool test_F6() { return F6<T>::value; }
	bool test_F7() { return F7<T>::value; }
	bool test_F8() { return F8<T>::value; }
	bool test_F9() { return F9<T>::value; }

	bool test_FN1() { return FN1<T>::value; }
	bool test_FN2() { return FN2<T>::value; }
	bool test_FN3() { return FN3<T>::value; }
	bool test_FN4() { return FN4<T>::value; }

	bool i() {
		return i1();
	}

	template<typename U = T> typename std::enable_if<F1<U>::value, bool>::type i1() { return true; }
	template<typename U = T> typename std::enable_if<!F1<U>::value, bool>::type i1() { return false; }
};

TEST(testsfinae, test_has_func)
{
	X<Funcs> x;

	ASSERT_TRUE(x.test_F1());
	ASSERT_TRUE(x.test_F2());
	ASSERT_TRUE(x.test_F3());
	ASSERT_TRUE(x.test_F4());
	ASSERT_TRUE(x.test_F5());
	ASSERT_TRUE(x.test_F6());
	ASSERT_TRUE(x.test_F7());
	ASSERT_TRUE(x.test_F8());
	ASSERT_TRUE(x.test_F9());

	ASSERT_FALSE(x.test_FN1());
	ASSERT_FALSE(x.test_FN2());
	ASSERT_FALSE(x.test_FN3());
	ASSERT_FALSE(x.test_FN4());

	ASSERT_TRUE(x.i());
}
