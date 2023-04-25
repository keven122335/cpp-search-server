#pragma once
#include <deque>

#include "search_server.h"

using std::string_literals::operator""s;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        :search_server_(search_server) {};

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus lhs, int rating) { return status == lhs; });
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        int result_count = 0;
        bool document_empty = false;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int actual_time_ = 0;
    const SearchServer& search_server_;

    void CheckingRequests();
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    CheckingRequests();
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    QueryResult result;
    result.result_count = static_cast <int>(top_documents.size());
    result.document_empty = top_documents.empty();
    requests_.push_back(result);
    return top_documents;
}
