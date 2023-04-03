#include <algorithm> 
#include <iostream> 
#include <set> 
#include <string> 
#include <utility> 
#include <vector> 
#include <map> 
#include <cmath> 
#include <numeric> 
#include <tuple>
#include <stdexcept>

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

vector<int>ReadLineWithRating() {
    int rating_size;
    cin >> rating_size;
    vector<int> ratings(rating_size, 0);
    for (int& rating : ratings) {
        cin >> rating;
    }
    return ratings;
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
    Document()
        :id(0), relevance(0), rating(0) {};

    Document(int id_, double relevance_, int rating_)
        :id(id_), relevance(relevance_), rating(rating_) {};

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {
        for (const string& stop_word : stop_words) {
            const vector<string> words = SplitIntoWords(stop_word);

            if (!all_of(words.cbegin(), words.cend(), IsValidWord)) {
                throw invalid_argument("Стоп слова имеет недопустимые символы"s);
            }
            for (const string& word : SplitIntoWords(stop_word)) {
                stop_words_.insert(word);
            }
        }
    }

    explicit SearchServer(const string& text)
        :SearchServer(SplitIntoWords(text)) {};


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& raitings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double count_tf = 1.0 / words.size();
        vector<int> ratings;
        int s_sum_raiting = ComputeAverageRating(raitings);
        if (document_id < 0)
            throw invalid_argument("Документ не был добавлен, так как его id отрицательный"s);

        if (document_data_.count(document_id))
            throw invalid_argument("Документ не был добавлен, так как его id совпадает с уже имеющимся"s);

        if (!IsValidWord(document))
            throw invalid_argument("Документ не был добавлен, так как содержит спецсимволы"s);

        for (const string& word : words) {

            word_to_document_freqs_[word][document_id] += count_tf;
        }
        document_data_.emplace(document_id, DataDocument{
                s_sum_raiting,
                status
            });

        documentId_index.push_back(document_id);
    }

    int GetDocumentCount() const {
        return static_cast<int> (document_data_.size());
    }

    template<typename Predicat>
    vector<Document> FindTopDocuments(const string& raw_query, Predicat predicat) const {

        const Query query_words = ParseQuery(raw_query);
        const double EPSILON = 1e-6;

        auto matched_documents = FindAllDocuments(query_words, predicat);
        sort(matched_documents.begin(), matched_documents.end(), [EPSILON](const Document& lhs, const Document& rhs) {
            return (lhs.relevance > rhs.relevance) || ((abs(lhs.relevance - rhs.relevance) < EPSILON) && (lhs.rating > rhs.rating));
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return  FindTopDocuments(raw_query, [status](int document_id, DocumentStatus lhs, int rating) { return status == lhs; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);

    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query_words = ParseQuery(raw_query);
        vector<string> words;

        for (const string& plus : query_words.plusWords) {
            if (word_to_document_freqs_.count(plus) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(plus).count(document_id)) {
                words.push_back(plus);
            }
        }

        for (const string& minus : query_words.minusWords) {
            if (word_to_document_freqs_.count(minus) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(minus).count(document_id)) {
                words.clear();
                return tuple(words, document_data_.at(document_id).status);
            }
        }

        sort(words.begin(), words.end(), [](const string& lhs, const string& rhs) {
            return lhs < rhs;
            });
        return tuple(words, document_data_.at(document_id).status);
    }

    int GetDocumentId(int index) const {
        return documentId_index.at(index);
    }

private:
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set <string> plusWords;
        set <string> minusWords;
    };

    struct DataDocument {
        int raiting;
        DocumentStatus status;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    map<int, DataDocument> document_data_;
    vector<int> documentId_index;

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    bool correct(const string& word) const {
        for (int i = 0; i <= word.size() - 1; ++i) {
            if ((word[i] == '-' && word[i + 1] == '-') || (word[i] == '-' && word[i + 1] == ' ') || word[word.size() - 1] == '-') {
                return false;
            }

        }
        return true;
    }

    bool isStopWords(const string& word) const {
        return stop_words_.count(word);
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;

        for (const string& word : SplitIntoWords(text)) {
            if (!isStopWords(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const {

        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, isStopWords(text) };
    }

    Query ParseQuery(const string& text) const {
        Query query;

        if (!correct(text)) {
            throw invalid_argument("Поисковый запрос имеет недопустимые символы"s);
        }
        if (!IsValidWord(text)) {
            throw invalid_argument("Ошибка в поисковом запросе"s);
        }

        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minusWords.insert(query_word.data);
                }
                else {
                    query.plusWords.insert(query_word.data);
                }
            }
        }
        return query;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_size = ratings.size();

        if (ratings.empty()) {
            return 0;
        }
        return static_cast<int> (accumulate(ratings.begin(), ratings.end(), 0) / rating_size);
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        const int number_documents = GetDocumentCount();
        return log(number_documents * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predicat>
    vector<Document> FindAllDocuments(const Query& query_words, Predicat predicat) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;

        for (const string& plus : query_words.plusWords) {
            if (word_to_document_freqs_.count(plus) == 0) {
                continue;
            }
            const double count_idf = ComputeWordInverseDocumentFreq(plus);
            for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(plus)) {
                document_to_relevance[document_id] += count_tf * count_idf;
            }
        }

        for (const string& minus : query_words.minusWords) {
            if (word_to_document_freqs_.count(minus) == 0) {
                continue;
            }
            for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(minus)) {
                document_to_relevance.erase(document_id);
            }
        }

        for (auto& [document_id, relevance] : document_to_relevance) {
            const auto& document_sourch_predicat = document_data_.at(document_id);
            if (predicat(document_id, document_sourch_predicat.status, document_sourch_predicat.raiting)) {
                matched_documents.push_back({ document_id, relevance, document_sourch_predicat.raiting });
            }
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    try {
        SearchServer search_server("и в на"s);
        // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
        // о неиспользуемом результате его вызова
        (void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(4, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        const auto documents = search_server.FindTopDocuments("--пушистый"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    catch (const invalid_argument& e) {
        cout << "Ошибка: "s << e.what() << endl;
    }
    catch (const out_of_range& e) {
        cout << "Ошибка: "s << e.what() << endl;
    }
}