#pragma once

struct Document {
    Document();

    Document(int id_, double relevance_, int rating_);

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
