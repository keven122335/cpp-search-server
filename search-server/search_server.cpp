#include "search_server.h"

    void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& raitings) {
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        double count_tf = 1.0 / words.size();
        std::vector<int> ratings;
        int s_sum_raiting = ComputeAverageRating(raitings);
        if (document_id < 0)
            throw std::invalid_argument("Документ не был добавлен, так как его id отрицательный"s);

        if (document_data_.count(document_id))
            throw std::invalid_argument("Документ не был добавлен, так как его id совпадает с уже имеющимся"s);

        if (!IsValidWord(document))
            throw std::invalid_argument("Документ не был добавлен, так как содержит спецсимволы"s);

        for (const std::string& word : words) {

            word_to_document_freqs_[word][document_id] += count_tf;
        }
        document_data_.emplace(document_id, DataDocument{
                s_sum_raiting,
                status
            });

        documentId_index.push_back(document_id);
    }

    int SearchServer::GetDocumentCount() const {
        return static_cast<int> (document_data_.size());
    }

    std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
        const SearchServer::Query query_words = ParseQuery(raw_query);
        std::vector<std::string> words;

        for (const std::string& plus : query_words.plusWords) {
            if (word_to_document_freqs_.count(plus) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(plus).count(document_id)) {
                words.push_back(plus);
            }
        }

        for (const std::string& minus : query_words.minusWords) {
            if (word_to_document_freqs_.count(minus) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(minus).count(document_id)) {
                words.clear();
                return std::tuple(words, document_data_.at(document_id).status);
            }
        }

        std::sort(words.begin(), words.end(), static_cast<bool (*)(const std::string&, const std::string&)>([](const std::string& lhs, const std::string& rhs) {
            return lhs < rhs;
            }));
        return std::tuple(words, document_data_.at(document_id).status);
    }

    int SearchServer::GetDocumentId(int index) const {
        return documentId_index.at(index);
    }

    bool SearchServer::IsValidWord(const std::string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    bool SearchServer::correct(const std::string& word) const {
        for (int i = 0; i <= word.size() - 1; ++i) {
            if ((word[i] == '-' && word[i + 1] == '-') || (word[i] == '-' && word[i + 1] == ' ') || word[word.size() - 1] == '-') {
                return false;
            }

        }
        return true;
    }

    bool SearchServer::isStopWords(const std::string& word) const {
        return stop_words_.count(word);
    }

    std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;

        for (const std::string& word : SplitIntoWords(text)) {
            if (!isStopWords(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {

        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, isStopWords(text) };
    }

    SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        SearchServer::Query query;

        if (!correct(text)) {
            throw std::invalid_argument("Поисковый запрос имеет недопустимые символы"s);
        }
        if (!IsValidWord(text)) {
            throw std::invalid_argument("Ошибка в поисковом запросе"s);
        }

        for (const std::string& word : SplitIntoWords(text)) {
            const SearchServer::QueryWord query_word = ParseQueryWord(word);
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

    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        int rating_size = ratings.size();

        if (ratings.empty()) {
            return 0;
        }
        return static_cast<int> (std::accumulate(ratings.begin(), ratings.end(), 0) / rating_size);
    }

    double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        const int number_documents = GetDocumentCount();
        return log(number_documents * 1.0 / word_to_document_freqs_.at(word).size());
    }

    