#include "request_queue.h"

int RequestQueue::GetNoResultRequests() const {
    return std::count_if(requests_.begin(), requests_.end(), static_cast<bool (*)(const QueryResult & lhs)>([](const QueryResult& lhs) {
        return lhs.documents_.empty();
        }));
}

void RequestQueue::CheckingRequests() {
    ++actual_time_;
    if (actual_time_ > min_in_day_) {
        requests_.pop_front();
        --actual_time_;
    }
}