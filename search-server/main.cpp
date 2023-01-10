// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
using namespace std;

int main() {
	int j = 0;
	for (int i = 0; i <= 1000; ++i) {
		string str_i = to_string(i);
		for (char ch : str_i) {
			if (ch == '3') {
				j++;
			}
		}
	}
	cout << j << endl;
}

// Закомитьте изменения и отправьте их в свой репозиторий.
