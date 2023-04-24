#include "document.h"

    Document::Document()
        :id(0), relevance(0), rating(0) {}

    Document::Document(int id_, double relevance_, int rating_)
        :id(id_), relevance(relevance_), rating(rating_) {}
