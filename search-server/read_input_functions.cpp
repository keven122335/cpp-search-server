#include "read_input_functions.h"

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<int>ReadLineWithRating() {
    int rating_size;
    std::cin >> rating_size;
    std::vector<int> ratings(rating_size, 0);
    for (int& rating : ratings) {
        std::cin >> rating;
    }
    return ratings;
}

