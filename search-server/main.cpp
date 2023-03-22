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
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& raitings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        double count_tf = 1.0 / words.size();
        vector<int> ratings;
        int s_sum_raiting = ComputeAverageRating(raitings);

        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += count_tf;
        }

        document_data_.emplace(document_id, DataDocument{
                s_sum_raiting,
                status
            });
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

// -------- Начало модульных тестов поисковой системы ----------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T&, const string& t_str) {
    cerr << t_str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}


void TestSearchWithoutMinusWord() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("-cat"s);
    ASSERT_EQUAL(found_docs.size(), 0);
}

void TestMatchDocument() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::ACTUAL, { 5,6,7 });
    {
        const auto [matched, status] = server.MatchDocument("-cat", 2);
        ASSERT_EQUAL(matched.empty(), true);
    }
    {
        const auto [matched, status] = server.MatchDocument("cat", 2);
        ASSERT_EQUAL(matched[0], "cat");
    }
}

void TestRelevanceSort() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::ACTUAL, { 5,6,7 });
    {
        const auto found_docs = server.FindTopDocuments("cat walks street");
        for (int i = 1; i < static_cast<int>(found_docs.size()); i++) {
            ASSERT(found_docs[i - 1].relevance > found_docs[i].relevance);
        }
    }
}

void TestComputeAverageRating() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { -3,-4,-5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::ACTUAL, { -5,6,-7 });
    {
        const auto found_docs = server.FindTopDocuments("dog and cat");
        ASSERT_EQUAL(found_docs[0].rating, (1 + 2 + 3) / 3);
        ASSERT_EQUAL(found_docs[1].rating, (-3 - 4 - 5) / 3);
        ASSERT_EQUAL(found_docs[2].rating, (-5 + 6 - 7) / 3);
    }
}

void TestFindFilterDocumentsPredicate() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::ACTUAL, { 5,6,7 });
    {
        const auto found_docs = server.FindTopDocuments("cat", [](int document_id, DocumentStatus status,
            int rating) {return document_id % 2 == 0; });
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
}

void TestFindDocumentsBySetStatus() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::IRRELEVANT, { 3,4,5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::BANNED, { 5,6,7 });
    server.AddDocument(4, "dog walks on street", DocumentStatus::REMOVED, { 7,8, 9 });
    {
        const auto found_docs = server.FindTopDocuments("dog", DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs[0].id, 1);
    }
    {


        const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
    {

        const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs[0].id, 3);
    }
    {

        const auto found_docs = server.FindTopDocuments("dog", DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs[0].id, 4);
    }
}

void TestCorrectRelevance() {
    const double EPSILON = 1e-6;
    SearchServer server;
    server.AddDocument(0, "dog walks on street"s, DocumentStatus::ACTUAL, { 8, -3 }); //relevance = 0.274653
    server.AddDocument(1, "cat walks around"s, DocumentStatus::ACTUAL, { 7, 2, 7 }); //relevance = 0.135155
    server.AddDocument(2, "cat walks on street"s, DocumentStatus::ACTUAL, { 9 }); //relevance = 0.101366
    const auto found_docs = server.FindTopDocuments("dog and cat"s);

    ASSERT(abs(found_docs[0].relevance - 0.274653) < EPSILON);
    ASSERT(abs(found_docs[1].relevance - 0.135155) < EPSILON);
    ASSERT(abs(found_docs[2].relevance - 0.101366) < EPSILON);

}
void TestAddDocuments() {
    SearchServer server;
    server.AddDocument(0, "dog walks on street"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "cat walks around", DocumentStatus::ACTUAL, { 4, 5, 6 });
    server.AddDocument(2, "cat walks on street", DocumentStatus::ACTUAL, { 6, 7, 8 });
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
}
// -------- Конец модульных тестов поисковой системы ----------

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestSearchWithoutMinusWord);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestFindFilterDocumentsPredicate);
    RUN_TEST(TestFindDocumentsBySetStatus);
    RUN_TEST(TestCorrectRelevance);
    RUN_TEST(TestAddDocuments);
}
int main() {
    TestSearchServer();

    cout << "Search server testing finished"s << endl;
}