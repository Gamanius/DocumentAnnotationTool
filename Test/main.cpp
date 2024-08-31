#include <iostream>

class Test {
private:
	int a;
public:
	Test(int val) : a(val) {}

	Test(Test& other) {
		std::cout << "Copy\n";
		a = other.a;
	}

	Test(Test&& other) noexcept {
		std::cout << "Move\n";
		swap(*this, other);
	}

	Test& operator=(Test other) {
		std::cout << "Assign\n";
		swap(*this, other);
		return *this;
	}

	friend void swap(Test& first, Test& second) {
		using std::swap;
		swap(first.a, second.a);
	}

	~Test() {
		std::cout << "Deleted " << a << "\n";
	}

};

Test getTest(int t) {
	Test a(3 - t);
	return a;
}

int main() {
	auto b = getTest(5);
}