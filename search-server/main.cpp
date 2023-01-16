#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    void AddDocument(int document_id, const string& document) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        double count_tf = 1.0 / words.size();
        for (const string& word : words) {
            if (!stop_words_.count(word)) {
                word_to_document_freqs_[word][document_id] += count_tf;
            }
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query {
        set <string> plusWords;
        set <string> minusWords;
    };
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;

    bool isStopWords(const string& word) const {
        return stop_words_.count(word);
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (stop_words_.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (isStopWords(word)) {
                continue;
            }
            if (word[0] != '-') {
                query_words.plusWords.insert(word);
            }
            else {
                query_words.minusWords.insert(word.substr(1));
            }

        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        for (const string& plus : query_words.plusWords) {
            if (word_to_document_freqs_.count(plus)) {
                double count_idf = log(document_count_ * 1.0 / word_to_document_freqs_.at(plus).size());
                for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(plus)) {
                    document_to_relevance[document_id] += count_tf * count_idf;
                }

            }
        }
        for (const string& minus : query_words.minusWords) {
            if (word_to_document_freqs_.count(minus)) {
                for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(minus)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
        for (auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance });
        }
        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    const string stop_words_joined = ReadLine();
    const int document_count = ReadLineWithNumber();
    search_server.SetStopWords(stop_words_joined);
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}


int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (auto [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", relevance = "s << relevance << " }"s
            << endl;
    }
}
