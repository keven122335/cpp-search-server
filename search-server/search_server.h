#pragma once
#include <iostream> 
#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <algorithm> 
#include <set> 
#include <map> 
#include<numeric>
#include <cmath>


#include "document.h"
#include "string_processing.h"

using std::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {
        for (const std::string& stop_word : stop_words) {
            const std::vector<std::string> words = SplitIntoWords(stop_word);

            if (!all_of(words.cbegin(), words.cend(), IsValidWord)) {
                throw std::invalid_argument("Стоп слова имеет недопустимые символы"s);
            }
            for (const std::string& word : words) {
                stop_words_.insert(word);
            }
        }
    }

    explicit SearchServer(const std::string& text)
        :SearchServer(SplitIntoWords(text)) {};


    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& raitings);

    int GetDocumentCount() const;

    template<typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return  FindTopDocuments(raw_query, [status](int document_id, DocumentStatus lhs, int rating) { return status == lhs; });
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    int GetDocumentId(int index) const;

private:
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set <std::string> plusWords;
        std::set <std::string> minusWords;
    };

    struct DataDocument {
        int raiting;
        DocumentStatus status;
    };

    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::set<std::string> stop_words_;
    std::map<int, DataDocument> document_data_;
    std::vector<int> documentId_index;

    static bool IsValidWord(const std::string& word);

    bool IsCorrectWord(const std::string& word) const;

    bool isStopWords(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query_words, Predicate predicate) const;
};

template<typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Predicate predicate) const {

    const Query query_words = ParseQuery(raw_query);
    const double EPSILON = 1e-6;

    auto matched_documents = FindAllDocuments(query_words, predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
        [EPSILON](const Document& lhs, const Document& rhs) {
            return (lhs.relevance > rhs.relevance) ||
            ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && (lhs.rating > rhs.rating));
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query_words, Predicate predicate) const {
    std::vector<Document> matched_documents;
    std::map<int, double> document_to_relevance;

    for (const std::string& plus : query_words.plusWords) {
        if (word_to_document_freqs_.count(plus) == 0) {
            continue;
        }
        const double count_idf = ComputeWordInverseDocumentFreq(plus);
        for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(plus)) {
            document_to_relevance[document_id] += count_tf * count_idf;
        }
    }

    for (const std::string& minus : query_words.minusWords) {
        if (word_to_document_freqs_.count(minus) == 0) {
            continue;
        }
        for (const auto& [document_id, count_tf] : word_to_document_freqs_.at(minus)) {
            document_to_relevance.erase(document_id);
        }
    }

    for (auto& [document_id, relevance] : document_to_relevance) {
        const auto& document_sourch_predicat = document_data_.at(document_id);
        if (predicate(document_id, document_sourch_predicat.status, document_sourch_predicat.raiting)) {
            matched_documents.push_back({ document_id, relevance, document_sourch_predicat.raiting });
        }
    }
    return matched_documents;
}